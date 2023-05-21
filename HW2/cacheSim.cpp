/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "cmath"
#include <list>


using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

#define WRITE 0
#define READ 1
#define MISS false
#define HIT true
#define VALID 1
#define INVALID 0

using namespace std;

class Block
{
public:
	//********* members *********//
	 
	unsigned blockSize;
	unsigned tag;
	unsigned startingAddress;
	bool dirtyBit;
	//bool validBit=false;
	unsigned lruLastTouchTime;
	bool isEmpty;

	//*********methods*********//
	explicit Block(unsigned blockSize_t);
	bool isHit(unsigned address);
	void makeDirty();
	void setStartingAddress(unsigned address);




};
Block::Block(unsigned blockSize_t): blockSize(blockSize_t){
	this->lruLastTouchTime=0;
	this->startingAddress=-1;
	this->dirtyBit=false;
	this->isEmpty=true;
};

bool Block::isHit(unsigned address)
{
	
	printf("address is %d\n",address );
	printf("startingAddress is %d\n",startingAddress );
	unsigned blockByteSize=log2((int)blockSize);
	printf("blockByteSize is %d\n",blockByteSize );
	if(address>=startingAddress && address<startingAddress+blockByteSize)
	{
		
		return true;
	}
	else
		return false;	
}

void Block::makeDirty()
{
	this->dirtyBit=true;
}

void Block::setStartingAddress(unsigned address) /// need to check if it works
{
	this->startingAddress=address;
}



class Layer
{
public:
	//********* members *********//
	unsigned layerSize;
	unsigned numOfWays;
	unsigned layerBlockSize;
	unsigned cyclesPerOperation;
	bool isWriteAllocate;
	bool isContain; // true if the layer contain the other layer
	// bool isWriteAllocate; // true if the layer is write allocate

	unsigned numOfSets; // sets = num of line / ways.
	unsigned numOfLines; // num of line = size of layer / block size.
	unsigned missCounter=0;
	unsigned hitCounter=0;
	unsigned timeStamp=1;

	
	vector< unsigned > LRU = vector< unsigned >(numOfSets); // LRU[2] wiil hold the index of the LRU in set 2.
	vector< vector< Block > > myBlocks = vector< vector< Block > >(numOfWays); // myBlocks[2][20] will hold the block in way 2, set 20.

	//*********methods*********//
	Layer(unsigned layerSize_t, unsigned numOfWays_t, unsigned layerBlockSize_t, unsigned cyclesPerOperation_t, bool isContain_t,bool isWriteAllocate_t);
	void addressToTagAndSet(unsigned address, unsigned* tag, unsigned* set);
	void updateLRU(unsigned int set);
	bool findBlockInLayer(unsigned address,unsigned* way);
	bool findEmptySpotInLayer(unsigned address,unsigned* freeWay);
	bool writeBlock(unsigned address);
	bool readBlock(unsigned address);


};



Layer::Layer (unsigned layerSize_t, unsigned numOfWays_t, unsigned layerBlockSize_t, unsigned cyclesPerOperation_t,bool isContain_t, bool isWriteAllocate_t): layerSize(layerSize_t), numOfWays(numOfWays_t),
																						 layerBlockSize(layerBlockSize_t), cyclesPerOperation(cyclesPerOperation_t), isContain(isContain_t), isWriteAllocate(isWriteAllocate_t)
{
 this->numOfLines = layerSize_t / layerBlockSize_t;
 this->numOfSets = this->numOfLines / this->numOfWays;

}
void Layer::addressToTagAndSet(unsigned address, unsigned* tag, unsigned* set)
{
	int numOfBytesInBlock=log2((int)layerBlockSize);
	address=address>>numOfBytesInBlock;
	*set=address%(int)numOfSets;
	*tag=address>>(int)log2((int)numOfSets);

}

void Layer :: updateLRU(unsigned set){
	unsigned oldestUsed=0;
	for (int i = 0; i < numOfWays; i++)
	{
		if((timeStamp-myBlocks[i][set].lruLastTouchTime)>(timeStamp-myBlocks[oldestUsed][set].lruLastTouchTime))
		{
			oldestUsed=i;
		}	
	}
	LRU[set]=oldestUsed;
	timeStamp++;
	
}

bool Layer::findBlockInLayer(unsigned address,unsigned* way)
{
	unsigned tag=0;
	unsigned set=0;

	addressToTagAndSet(address,&tag,&set);
	for (int curr_way = 0; curr_way < numOfWays; curr_way++)
	{
		printf("start to findBlockInLayer\n");
		printf("startingAddress is %d\n",myBlocks[curr_way][set].lruLastTouchTime );
		if((myBlocks[curr_way][set].startingAddress!=-1) && (myBlocks[curr_way][set].isHit(address)) )
		{
			*way=curr_way;
			return true;
		}
		 
	}
	printf("exit to findBlockInLayer\n");
	return false;
}

bool Layer::findEmptySpotInLayer(unsigned int set,unsigned* freeWay)
{

	for (int way = 0; way < numOfWays; way++)
	{
		if(myBlocks[way][set].isEmpty)
		{
			*freeWay=way;
			return true;
		}
		 
	}
	return false;
}	

bool Layer :: writeBlock(unsigned address){
	unsigned way=0;
	unsigned tag=0;
	unsigned set=0;	
	addressToTagAndSet(address,&tag,&set);
	if (findBlockInLayer(address,&way))
	{
		myBlocks[way][set].dirtyBit=true;
		myBlocks[way][set].lruLastTouchTime=timeStamp;
		updateLRU(set);
		return HIT;
	}
	else {
		
		unsigned freeWay=0;
		if(findEmptySpotInLayer(set,&freeWay)){
			myBlocks[freeWay][set].setStartingAddress(address);
			myBlocks[freeWay][set].isEmpty=false;
			myBlocks[freeWay][set].lruLastTouchTime=timeStamp;
			updateLRU(set);
			return MISS;
		}
		else{
			if(isWriteAllocate){
			way=LRU[set];
			myBlocks[way][set].setStartingAddress(address);
			myBlocks[way][set].lruLastTouchTime=timeStamp;
			myBlocks[way][set].dirtyBit=true;
			myBlocks[way][set].isEmpty=false;
			updateLRU(set);
			return MISS;
			}
			else{
				return MISS;
			}
		}
		
	}
}
 
bool Layer :: readBlock(unsigned address){
	unsigned way=0;
	unsigned tag=0;
	unsigned set=0;	
	addressToTagAndSet(address,&tag,&set);
	printf("tag: %d set: %d \n",tag,set );
	printf("before findBlockInLayer\n");
	if (findBlockInLayer(address,&way))
	{
		
		myBlocks[way][set].lruLastTouchTime=timeStamp;
		updateLRU(set);
		return HIT;
	}
	else{
		unsigned freeWay=0;
		printf("before findEmptySpotInLayer\n");
		if (findEmptySpotInLayer(set,&freeWay))
		{
			printf("after findEmptySpotInLayer\n");
			myBlocks[freeWay][set].setStartingAddress(address);
			myBlocks[freeWay][set].isEmpty=false;
			myBlocks[freeWay][set].lruLastTouchTime=timeStamp;
			updateLRU(set);
			return MISS;
		}
		else{
			way=LRU[set];
			myBlocks[way][set].setStartingAddress(address);
			myBlocks[way][set].lruLastTouchTime=timeStamp;
			updateLRU(set);
			return MISS;
		}
	}
}





class Memory{
public:
	unsigned int memoryCyclePerOperation;
	unsigned int totalAccesses;
	unsigned int totalTime;
	Layer L1;
	Layer L2;
	Memory(unsigned int memoryCyclePerOperation_t, unsigned int layerBlockSize1 , unsigned int layerSize1, unsigned int numOfWays1,unsigned cyclePerOperation1, bool isContain1,
													unsigned int layerBlockSize2 , unsigned int layerSize2, unsigned int numOfWays2, unsigned cyclePerOperation2, bool isContain2, bool isWriteAllocate): 
													L1(Layer(layerSize1,numOfWays1,layerBlockSize1,cyclePerOperation1,isContain1,isWriteAllocate)), L2(Layer(layerSize2,numOfWays2,layerBlockSize2,cyclePerOperation2,isContain2,isWriteAllocate))
													{
														this->memoryCyclePerOperation=memoryCyclePerOperation_t;
														this->totalTime=0;
														this->totalAccesses=0;
													}
	bool read(unsigned address);
	bool write(unsigned address);
	void readNextLine(char operation,unsigned address);												

};
bool Memory :: read(unsigned address){
	totalAccesses++;
	totalTime+=L1.cyclesPerOperation;

	if(L1.readBlock(address)==MISS)
	{
		printf("L1 miss\n");
		L1.missCounter++;
		totalAccesses++;
		totalTime+=L2.cyclesPerOperation;
		if(L2.readBlock(address)==MISS)
		{
			printf("L2 miss\n");
			L2.missCounter++;
			totalAccesses++;
			totalTime+=memoryCyclePerOperation;
			return MISS;
		}
		else{
			L2.hitCounter++;
			return HIT;
		}
	}
	else{
		L1.hitCounter++;
		return HIT;
	}

}

bool Memory :: write(unsigned address)
{
	totalAccesses++;
	totalTime+=L1.cyclesPerOperation;
	if(L1.writeBlock(address)==MISS)
	{
		L1.missCounter++;
		totalAccesses++;
		totalTime+=L2.cyclesPerOperation;
		if(L2.writeBlock(address)==MISS)
		{
			L2.missCounter++;
			totalAccesses++;
			totalTime+=memoryCyclePerOperation;
			//maybe we need to add writr allocate here
		}
		else{
			L2.hitCounter++;
			return HIT;
		}
	}
	else{
		L1.hitCounter++;
		return HIT;
	}
}
void Memory :: readNextLine(char operation,unsigned address){
	if (operation=='r')
	{
		
		read(address);

	}
	else if	(operation=='w'){
		write(address);
	}
}






int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	} 

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}
	
	Memory mainMem = Memory(MemCyc,pow(2,BSize),pow(2,L1Size),pow(2,L1Assoc),L1Cyc,false,pow(2,BSize),pow(2,L2Size),pow(2,L2Assoc),L2Cyc,true,WrAlloc);
	
											
	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

	
		mainMem.readNextLine(operation,num);

	}

	double L1MissRate=mainMem.L1.missCounter/(mainMem.L1.timeStamp-1);
	double L2MissRate=mainMem.L2.missCounter/(mainMem.L2.timeStamp-1);
	double avgAccTime=mainMem.totalTime/mainMem.totalAccesses;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);



	return 0;
}
