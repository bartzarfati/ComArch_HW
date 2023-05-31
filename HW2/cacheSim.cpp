/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <list>
#include <algorithm>
#include <cstdio>

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
#define INF 1000000000

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
	int lruLastTouchTime;
	bool isEmpty;
	int validBit;
	//*********methods*********//
	Block() = default ;
	Block(unsigned blockSize_t):
    		blockSize(blockSize_t), tag(0), startingAddress(0), dirtyBit(false), lruLastTouchTime(INF), isEmpty(true),validBit(INVALID){}

	bool isHit(unsigned address);
	void makeDirty();
	void setStartingAddress(unsigned address);
	bool operator == (const Block& block2)
	{
		if (this->tag == block2.tag)
		{
			return true;
		}
		return false;
	}
	~Block()=default;

};

Block& getBlock(vector<Block> set, unsigned way) 
{

	for (size_t i = 0; i < set.size(); i++) {
		if (i == way) {
        		return set[i];
    		}
	}
	Block block=Block(set[0].blockSize);
	return block;
}

bool Block::isHit(unsigned address)
{
	unsigned blockByteSize=log2((int)this->blockSize);
	if(address>=this->startingAddress && address<this->startingAddress+this->blockSize)
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

	
	vector< int > LRU ; // LRU[2] will hold the index of the LRU in set 2.
	vector< Block>**  mySet ;//= (vector< vector< Block > >)numOfWays; // mySet[2][20] will hold the block in way 2, set 20.

	//*********methods*********//
	Layer(unsigned layerSize_t, unsigned numOfWays_t, unsigned layerBlockSize_t, unsigned cyclesPerOperation_t, bool isContain_t,bool isWriteAllocate_t);
	void addressToTagAndSet(unsigned address, unsigned* tag, unsigned* set);
	void updateLRU(unsigned int set);
	bool findBlockInLayer(unsigned address,unsigned* way);
	bool findEmptySpotInLayer(unsigned address,unsigned* freeWay);
	bool writeBlock(unsigned address,unsigned* removedAddress,bool* isRemovedDirty);
	bool readBlock(unsigned address,unsigned* removedAddress,bool* isRemovedDirty);
	void removeBlock(unsigned address);

	~Layer();


};



Layer::Layer (unsigned layerSize_t, unsigned numOfWays_t, unsigned layerBlockSize_t, unsigned cyclesPerOperation_t,bool isContain_t, bool isWriteAllocate_t): layerSize(layerSize_t), numOfWays(numOfWays_t),
																						 layerBlockSize(layerBlockSize_t), cyclesPerOperation(cyclesPerOperation_t), isContain(isContain_t), isWriteAllocate(isWriteAllocate_t)
{	
 this->numOfLines = layerSize_t / layerBlockSize_t;
 this->numOfSets = this->numOfLines / this->numOfWays;
 mySet=(vector< Block>**)malloc(numOfSets*sizeof(vector<Block>*));
 for (unsigned int i = 0; i < numOfSets; i++)
 {
	mySet[i]=new vector<Block>; 
 }

 for (unsigned int i = 0; i <numOfSets; i++)
 {
 	for (unsigned int j = 0; j <numOfWays; j++)
 	{
 		Block* block=new Block(layerBlockSize);
 		mySet[i]->push_back(*block);
 	}
 }

 LRU.resize(numOfSets);
 for (unsigned int i = 0; i < numOfSets; i++)
 {
 	LRU[i]=-1;
 }

}

void Layer::removeBlock(unsigned address)
{
	unsigned tag;
	unsigned set;
	addressToTagAndSet(address,&tag,&set);
	auto setVec=mySet[set];
	int currenyWay=0;
	for (auto block : *setVec)
	{
		if((block.tag==tag) && (block.validBit==VALID))
		{
			
			setVec->erase(setVec->begin()+currenyWay);
			block =Block(layerBlockSize);
			block.lruLastTouchTime=timeStamp;
			setVec->insert(setVec->begin()+currenyWay,block);
		}
		currenyWay++;
	}

	
}

Layer::~Layer()
{
	for (unsigned int i = 0; i < numOfSets; i++)
	{
		delete mySet[i];
	}
	free (mySet);
}

void Layer::addressToTagAndSet(unsigned address, unsigned* tag, unsigned* set)
{

	address=address/(layerBlockSize);
	*set=address%(numOfSets);
	address=address/(numOfSets);
	*tag=address;

}

void Layer :: updateLRU(unsigned set)
{
	unsigned oldestUsed=0;
	auto setVec=(vector<Block>)*mySet[set];

	if((LRU[set]!=-1)){
		auto lruBlock=getBlock(setVec,LRU[set]);
		int i=0;
		for (auto block : setVec)
		{
			if((block.validBit==VALID)&&(timeStamp-(block.lruLastTouchTime))>=((int)(timeStamp-(lruBlock.lruLastTouchTime))))
			{

				oldestUsed=i;
				lruBlock=block;
			}	
			i++;
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
	auto setVec=(vector<Block>)*mySet[set];
	int curr_way=0;
	for (auto block : setVec)
	{
		
		if((block.validBit==VALID) && (block.tag==tag) )
		{
			*way=curr_way;
			return true;
		}
		curr_way++;
		 
	}
	return false;
}

bool Layer::findEmptySpotInLayer(unsigned int set,unsigned* freeWay)
{
	auto setVec=(vector<Block>)*mySet[set];
	int way=0;
	for (auto block : setVec)
	{

		if((block.isEmpty))
		{
			*freeWay=way;
			return true;
		}
		way++;
		 
	}
	return false;
}	

bool Layer :: writeBlock(unsigned address,unsigned* removedAddress,bool* isRemovedDirty){
	unsigned way=0;
	unsigned tag=0;
	unsigned set=0;	
	addressToTagAndSet(address,&tag,&set);
	auto setVec=mySet[set];
	if (findBlockInLayer(address,&way))
	{
		Block block=getBlock(*setVec,way);
		setVec->erase(find(setVec->begin(),setVec->end(),block));
		block.tag=tag;
		block.makeDirty(); 
		block.lruLastTouchTime=timeStamp;
		setVec->insert(setVec->begin()+way,block);
		updateLRU(set);
		return HIT;
	}
	else {
		if (isWriteAllocate)
		{
		
			unsigned freeWay=0;
			if(findEmptySpotInLayer(set,&freeWay)){
				Block block=getBlock(*setVec,freeWay);
				setVec->erase(setVec->begin()+freeWay);
				block.setStartingAddress(address);
				block.tag=tag;
				block.isEmpty=false;
				block.validBit=VALID;
				block.makeDirty();
				block.lruLastTouchTime=timeStamp;
				setVec->insert(setVec->begin()+freeWay,block);
				updateLRU(set);
				return MISS;
			}
		
			else
			{
				printf("delete block from the LRU\n" );
				way=LRU[set];
				Block block=getBlock(*setVec,way);
				*removedAddress=block.startingAddress;
				*isRemovedDirty=block.dirtyBit;
				setVec->erase(find(setVec->begin(),setVec->end(),block));
				block.setStartingAddress(address);
				block.tag=tag;
				block.lruLastTouchTime=timeStamp;
				block.isEmpty=false;
				block.validBit=VALID;
				block.makeDirty();
				setVec->insert(setVec->begin()+way,block);
				updateLRU(set);
				return MISS;
			}
		}

		else
		{
			return MISS;
		}
		
		
	}
}
 
bool Layer :: readBlock(unsigned address,unsigned* removedAddress,bool* isRemovedDirty){
	unsigned way=0;
	unsigned tag=0;
	unsigned set=0;	
	addressToTagAndSet(address,&tag,&set);
	auto setVec=mySet[set];
	if (findBlockInLayer(address,&way))
	{
		Block block=getBlock(*setVec,way);
		setVec->erase(find(setVec->begin(),setVec->end(),block));
		block.tag=tag;
		block.lruLastTouchTime=timeStamp;
		setVec->insert(setVec->begin()+way,block);
		updateLRU(set);
		return HIT;
	}
	else{
		unsigned freeWay=0;
		if (findEmptySpotInLayer(set,&freeWay))
		{
			Block block=getBlock(*setVec,freeWay);
			setVec->erase(setVec->begin()+freeWay);
			block.setStartingAddress(address);
			block.tag=tag;
			block.isEmpty=false;
			block.validBit=VALID;
			block.lruLastTouchTime=timeStamp;
			setVec->insert(setVec->begin()+freeWay,block);
			updateLRU(set);			
			return MISS;
		}
		else{
			printf("delete block from the LRU\n" );
			way=LRU[set];
			Block block=getBlock(*setVec,way);
			*removedAddress=block.startingAddress;
			*isRemovedDirty=block.dirtyBit;
			setVec->erase(find(setVec->begin(),setVec->end(),block));
			block.setStartingAddress(address);
			block.tag=tag;
			block.lruLastTouchTime=timeStamp;
			block.validBit=VALID;
			setVec->insert(setVec->begin()+way,block);
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
	unsigned int totalNumberOfOperations;
	Layer L1;
	Layer L2;
	Memory(unsigned int memoryCyclePerOperation_t, unsigned int layerBlockSize1 , unsigned int layerSize1, unsigned int numOfWays1,unsigned cyclePerOperation1, bool isContain1,
													unsigned int layerBlockSize2 , unsigned int layerSize2, unsigned int numOfWays2, unsigned cyclePerOperation2, bool isContain2, bool isWriteAllocate): 
													L1(Layer(layerSize1,numOfWays1,layerBlockSize1,cyclePerOperation1,isContain1,isWriteAllocate)), L2(Layer(layerSize2,numOfWays2,layerBlockSize2,cyclePerOperation2,isContain2,isWriteAllocate))
													{
														this->memoryCyclePerOperation=memoryCyclePerOperation_t;
														this->totalTime=0;
														this->totalAccesses=0;
														this->totalNumberOfOperations=0;
													}
	bool read(unsigned address);
	bool write(unsigned address);
	void readNextLine(char operation,unsigned address);												

};
bool Memory :: read(unsigned address){
	totalAccesses++;
	totalTime+=L1.cyclesPerOperation;
	unsigned removedAddress1;
	bool isRemovedDirty1;
	unsigned removedAddress2;
	unsigned temp=removedAddress2;
	bool isRemovedDirty2;
	unsigned tag=0;
	unsigned set=0;

	if(L1.readBlock(address,&removedAddress1,&isRemovedDirty1)==MISS)
	{
		L1.missCounter++;
		totalAccesses++;
		totalTime+=L2.cyclesPerOperation;
		if (isRemovedDirty1)
		{
			L2.readBlock(removedAddress1,&removedAddress2,&isRemovedDirty2);
		}
		
		if(L2.readBlock(address,&removedAddress2,&isRemovedDirty2)==MISS)
		{
			L2.missCounter++;
			totalAccesses++;
			totalTime+=memoryCyclePerOperation;
			if(removedAddress2!=temp)
			{
				L1.removeBlock(removedAddress2);
				L1.addressToTagAndSet(removedAddress2,&tag,&set);
				L1.updateLRU(set);
				L1.timeStamp--;
			}
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
	unsigned removedAddress1;
	bool isRemovedDirty1;
	unsigned removedAddress2;
	unsigned temp=removedAddress2;
	bool isRemovedDirty2;
	unsigned tag=0;
	unsigned set=0;
	if(L1.writeBlock(address,&removedAddress1,&isRemovedDirty1)==MISS)
	{
		L1.missCounter++;
		totalAccesses++;
		totalTime+=L2.cyclesPerOperation;

		if (isRemovedDirty1)
		{
			L2.writeBlock(removedAddress1,&removedAddress2,&isRemovedDirty2);
		}

		if(L2.writeBlock(address,&removedAddress2,&isRemovedDirty2)==MISS)
		{
			L2.missCounter++;
			totalAccesses++;
			totalTime+=memoryCyclePerOperation;
			if(removedAddress2!=temp)
			{
				L1.removeBlock(removedAddress2);
				L1.addressToTagAndSet(removedAddress2,&tag,&set);
				L1.updateLRU(set);	
				L1.timeStamp--;
			}
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

void Memory :: readNextLine(char operation,unsigned address){
	totalNumberOfOperations++;
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

		

		string cutAddress = address.substr(2); // Removing the "0x" part of the address


		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);


		
		mainMem.readNextLine(operation,num);
	}
	
	int L1access=mainMem.L1.hitCounter+mainMem.L1.missCounter;
	int L2access=mainMem.L2.hitCounter+mainMem.L2.missCounter;
	double mainMemoryAccessTime=(mainMem.totalAccesses-(L1access+L2access))*mainMem.memoryCyclePerOperation;
	double L1AccessTime=(double)((L1access)*(mainMem.L1.cyclesPerOperation));
	double L2AccessTime=(double)((L2access)*(mainMem.L2.cyclesPerOperation));
	double L1MissRate=(double)mainMem.L1.missCounter/(double)(L1access);
	double L2MissRate=(double)mainMem.L2.missCounter/(double)(L2access);
	double avgAccTime=(double)(mainMemoryAccessTime+L1AccessTime+L2AccessTime)/(double)(mainMem.totalNumberOfOperations);
	

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);


	return 0;
}
