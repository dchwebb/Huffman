#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <bitset>
#include <map>

std::string s = "The vast majority of C++ users think that the using-directive is injecting names into the scope where it\'s declared. In the example above, that would be the scope of the function. In reality, the names are injected into the nearest common ancestor of the target namespace. ";
//std::string s = "abbcccddddeeeeeffffff";

// Vector for storing and sorting frequency of each char in string
struct freqItem {
   char item;
   uint32_t freq;
   int32_t treeIndex = -1;
   bool inTree = false;
};
std::vector<freqItem> freqVec;

// Vector for storing the huffman tree
struct huffItem {
   char leaf;
   uint32_t score;
   int32_t parent;
   bool isOne;
   bool isParent;
   uint8_t code = 0;
   uint8_t codeLength = 0;
};

class huff {
public:
   std::vector<huffItem> tree;
   int32_t nodeIndex = 0;	   // keep count of nodes added to tree

   void addLeaves(freqItem& f1, freqItem& f2) {
      // Add parent node to huffman tree
	  uint32_t sum = static_cast<uint32_t>(f1.freq + f2.freq);
	  tree.push_back({ 0, sum, -1, true, true });
	  int32_t parentindex = tree.size() - 1;

	  // Add two leaves to tree
	  if (f1.treeIndex > -1) {		// already in tree so a parent node
		 tree[f1.treeIndex].parent = parentindex;
		 tree[f1.treeIndex].isOne = true;
	  }
	  else {
		 tree.push_back({ f1.item, f1.freq, parentindex, true });
	  }

	  if (f2.treeIndex > -1) {
		 tree[f2.treeIndex].parent = parentindex;
		 tree[f2.treeIndex].isOne = false;
	  }
	  else {
		 tree.push_back({ f2.item, f2.freq, parentindex, false });
	  }

	  // Add parent node to freqVector queue
	  freqVec.push_back({ 0, sum, parentindex });
   }

};

// Item to store final code for quick lookup
struct codeItem {
   uint32_t codeVal;
   uint32_t codeSize;
};
std::map<char, codeItem> codeLookup;

struct sortFreq {
   inline bool operator() (const freqItem& f1, const freqItem& f2) {
	  return (f1.freq < f2.freq);
   }
};

bool lowestFreq(freqItem f1, freqItem f2) {
   return !f1.inTree && (f2.inTree || f1.freq < f2.freq);
}

int main() {
   std::cout << "Message:\n" << s << "\n\n";

   huff huffTree;

   for (auto c : s) {
	  auto it = std::find_if(freqVec.begin(), freqVec.end(), [&](const freqItem& f) {return c == f.item; });
	  if (it == freqVec.end()) {
		 freqVec.push_back({ c, 1 });
		 codeLookup.insert({ c, {} });
	  }
	  else {
		 it->freq++;
	  }
   }
   std::sort(freqVec.begin(), freqVec.end(), sortFreq());
   
   // Print frequencies:
   for (const auto& f : freqVec) {
	  std::cout << f.item << ": " << f.freq << std::endl;
   }

   uint32_t charCount = freqVec.size();
 
   // Locate leaves not already assigned a parent with lowest score and add to tree
   while (std::count_if(freqVec.begin(), freqVec.end(), [&](const freqItem& f) {return !f.inTree; }) > 1) {
	  auto minFreq1 = std::min_element(freqVec.begin(), freqVec.end(), lowestFreq);
	  minFreq1->inTree = true;
	  auto minFreq2 = std::min_element(freqVec.begin(), freqVec.end(), lowestFreq);
	  minFreq2->inTree = true;
	  huffTree.addLeaves(*minFreq1, *minFreq2);
   }

   // Generate codes - these need a number and a length as they can start with multiple zeros
   for (auto it = huffTree.tree.begin(); it < huffTree.tree.end(); ++it) {
	  huffItem h = *it;
	  int depth = 0;
	  while (h.parent != -1) {
		 //it->code += (h.isOne ? 1 : 0) << depth++;
		 it->code = (it->code << 1) + (h.isOne ? 1 : 0);
		 depth++;
		 h = huffTree.tree[h.parent];
	  }
	  it->codeLength = depth;
	  
	  // print code
	  if (!it->isParent) {
		 std::cout << it->leaf << " " << std::bitset<32>(it->code).to_string().substr(32 - depth, 32) << std::endl;
	  }
   }

   // Update map of the chars for quick lookup and calculation of final code size
   uint32_t codedSize = 0;
   uint8_t minCodeSize = 255;
   uint8_t maxCodeSize = 0;
   for (auto& cl : codeLookup) {
	  // Get code from huffman tree
	  auto hi = std::find_if(huffTree.tree.begin(), huffTree.tree.end(), [&](const huffItem& h) {return cl.first == h.leaf && !h.isParent; });
	  cl.second.codeSize = hi->codeLength;
	  cl.second.codeVal = hi->code;

	  minCodeSize = std::min(minCodeSize, hi->codeLength);
	  maxCodeSize = std::max(maxCodeSize, hi->codeLength);

	  // Get frequency to calculate size of final code
	  auto fi = std::find_if(freqVec.begin(), freqVec.end(), [&](const freqItem& f) {return cl.first == f.item; });
	  codedSize += fi->freq * hi->codeLength;
   }

   // Create a dynamically allocated array
   uint32_t codedBytes = static_cast<uint32_t>(std::ceil(static_cast<float>(codedSize) / 8));
   std::unique_ptr<uint8_t[]> codedMsg(new uint8_t[codedBytes]{});
   //uint8_t* codedMsg = new uint8_t[std::ceil(static_cast<float>(codedSize) / 8)]{};
   
   // Encode the message
   uint32_t posMsg = 0;
   for (auto c : s) {
	  // get nearest byte and cast to a 32 bit pointer (assuming no code will be longer than 32 - 7 bits
	  uint32_t* pos = reinterpret_cast<uint32_t*>(&codedMsg[posMsg / 8]);
	  *pos |= codeLookup[c].codeVal << (posMsg % 8);	 // shift and bitwise 'and' the code value to the 32 bit value
	  
	  posMsg += codeLookup[c].codeSize;
   }

   // print the code
   for (int i = codedBytes - 1; i > -1; --i) {
	  std::cout << std::bitset<8>(codedMsg[i]) << " ";
   }

   // Decode the message: get the potential code represented by an increasing number of bits and check if a valid code
   std::string decodedMsg;
   uint32_t decodePos = 0;

   for (uint32_t c = 0; c < s.size(); ++c) {
	  for (int b = maxCodeSize; b >= minCodeSize; --b) {
		 // check if code is too long for remaining bits
		 while (decodePos + b > codedSize) {
			--b;
		 }

		 // get nearest byte and cast to a 32 bit pointer (assuming no code will be longer than 32 - 7 bits
		 uint32_t* pos = reinterpret_cast<uint32_t*>(&codedMsg[decodePos / 8]);
		 uint8_t potCode = ((*pos) >> (decodePos % 8)) & ((2 << (b - 1)) - 1);

		 // Check if potential code is in map
		 auto mi = std::find_if(codeLookup.begin(), codeLookup.end(), [&](const auto& m) {return m.second.codeSize == b && m.second.codeVal == potCode; });
		 if (mi != codeLookup.end()) {
			// add item to decoded message
			decodedMsg += mi->first;
			decodePos += b;
			b = 0;
		 }
	  }
   }
   std::cout << "\n\nDecoded:\n" << decodedMsg << "\n\nOriginal message: " << s.size() << ". Coded message: " << codedBytes;

 }
