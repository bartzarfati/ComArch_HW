/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"

// define all the stats 
#define SNT 0
#define WNT 1
#define WT 2
#define ST 3

//define results
#define TAKEN true
#define NOT_TAKEN false

//define the states
#define GHGT 0
#define GHLT 1
#define LHGT 2
#define LHLT 3

//define btb columns
#define TAG 0
#define TARGET 1
#define HISTORY 2

//define shared status
#define NOT_SHARED 0
#define SHARED_LSB 1
#define SHARED_MID 2

int mode=GHGT;
unsigned  BpBtbSize=0;
unsigned  BpHistorySize=0;
unsigned  BpTagSize=0;
unsigned  BpFsmState=0;
int BpShared=NOT_SHARED;
unsigned BpTotalSize;

unsigned GlobalHistory=0;
unsigned FsmIndex=0;

int BranchNum=0;
int FlushNum=0;
int SizeOfBtb=0;
void get_tag_and_index(unsigned pc,unsigned btb_size, unsigned tag_size,unsigned* tag,unsigned * index){
	pc=pc/4;
	*index=pc%btb_size;
	pc=pc/ btb_size;
	*tag=pc%((unsigned)pow(2,tag_size));
}
void SetHistory(bool branchRes, unsigned limit, unsigned *history){
	unsigned a =0;
	if(branchRes)a=1;
	(*history)=(*history)*2+a;
	(*history)=(*history)%limit;
}

void Taken(unsigned* fsm_state){
	if(*fsm_state<ST){
		(*fsm_state)++;
	}
}
void NotTaken(unsigned* fsm_state){
	if(*fsm_state>SNT){
		(*fsm_state)--;
	}
}
bool GetPrediction(unsigned curr_state){
	if(curr_state==WNT	||	curr_state==SNT)return NOT_TAKEN;
	return TAKEN;
}





/*******************************************************************************************/
//for Global History Global Table
/*******************************************************************************************/
unsigned* FsmArryGHGT;
unsigned** BtbGHGT;
int initGHGT(unsigned btbsize,unsigned historysize,unsigned tagSize,unsigned fsmState,int Shared);
int initGHGT(unsigned btbsize,unsigned historysize,unsigned tagSize,unsigned fsmState,int Shared){
	unsigned arrySize=(unsigned)pow(2,historysize);
	FsmArryGHGT=malloc((unsigned)arrySize*sizeof(unsigned));
	if(FsmArryGHGT==NULL)return -1;
	for(int i=0;i<arrySize;i++){
		FsmArryGHGT[i]=fsmState;
	}
	BtbGHGT=(unsigned**)malloc(btbsize*sizeof(unsigned*));
	if(BtbGHGT==NULL)return -1;
	for(int i=0;i<BpBtbSize;i++){
		BtbGHGT[i]=malloc(2*sizeof(unsigned));
		if(BtbGHGT[i]==NULL){
			for(unsigned j=0;j<i;j++){
				free(BtbGHGT[j]);
			}
			free(BtbGHGT);
			free(FsmArryGHGT);
			return -1;
		}
		BtbGHGT[i][TAG]=UINT32_MAX;
		BtbGHGT[i][TARGET]=UINT32_MAX;
		
	}

	return 0;
}


bool predictGHGT(uint32_t pc,uint32_t* dst);
bool predictGHGT(uint32_t pc,uint32_t* dst){
	unsigned tag=0,index=0,tempPC=pc;

	get_tag_and_index(pc,BpBtbSize,BpTagSize,&tag,&index);
	if(BpShared==NOT_SHARED){
		FsmIndex=GlobalHistory;
	}
	else if (BpShared==SHARED_LSB){
		tempPC=tempPC/4;
		FsmIndex=tempPC%((unsigned)pow(2,BpHistorySize));
		FsmIndex=FsmIndex^GlobalHistory;
	}
	else if (BpShared==SHARED_MID){
		tempPC=(unsigned)(tempPC/(pow(2,16)));
		FsmIndex=tempPC%((unsigned)pow(2,BpHistorySize));
		FsmIndex=FsmIndex^GlobalHistory;
	}
	if(BtbGHGT[index][TAG]==tag){
		bool predict=GetPrediction(FsmArryGHGT[FsmIndex]);
		if(predict==NOT_TAKEN){
			(*dst)=pc+4;
		}
		else{
			(*dst)=BtbGHGT[index][TARGET];
		}
		return predict;

	}
	BtbGHGT[index][TAG]=tag;
	*dst=pc+4;
	return NOT_TAKEN;
}
void updateGHGT(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
void updateGHGT(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	unsigned tag,index;
	get_tag_and_index(pc,BpBtbSize,BpTagSize,&tag,&index);
	BtbGHGT[index][TARGET]=targetPc;
	if(taken){
		Taken(&FsmArryGHGT[FsmIndex]);
	}
	else{
		NotTaken(&FsmArryGHGT[FsmIndex]);
	}

	SetHistory(taken,(unsigned)pow(2,BpHistorySize),&GlobalHistory);
}
/*******************************************************************************************/
//end og GHGT
/*******************************************************************************************/










/*******************************************************************************************/
//for Global History Local Table
/*******************************************************************************************/
unsigned** FsmArryGHLT;
unsigned** BtbGHLT;

/// @brief init function for GHGL
/// @param btbsize 
/// @param historysize 
/// @param tagSize 
/// @param fsmState 
/// @param Shared 
/// @return -1 for error, 0 for success
int initGHLT(unsigned btbsize,unsigned historysize,unsigned tagSize,unsigned fsmState,int Shared);
int initGHLT(unsigned btbsize,unsigned historysize,unsigned tagSize,unsigned fsmState,int Shared){
	unsigned arrySize=(unsigned)pow(2,historysize);
	FsmArryGHLT=malloc(BpBtbSize*sizeof(unsigned*));
	if(FsmArryGHLT==NULL)return -1;
	for(int i=0;i<BpBtbSize;i++){
		FsmArryGHLT[i]=malloc(arrySize*sizeof(unsigned));
		if(FsmArryGHLT[i]==NULL){
			for(unsigned j=0;j<i;j++){
				free(FsmArryGHLT[j]);
			}
			free(FsmArryGHLT);
			return -1;
		}
		for(int j=0;j<arrySize;j++){
			FsmArryGHLT[i][j]=BpFsmState;
		}
	}

	BtbGHLT=(unsigned**)malloc((unsigned)btbsize*sizeof(unsigned*));
	if(BtbGHLT==NULL){
		for(unsigned j=0;j<BpBtbSize;j++){
			free(FsmArryGHLT[j]);
		}
		free(FsmArryGHLT);
		return -1;
	}

	for(int i=0;i<BpBtbSize;i++){
		BtbGHLT[i]=malloc(2*sizeof(unsigned));
		if(BtbGHLT[i]==NULL){
			for(unsigned j=0;j<BpBtbSize;j++){
				free(FsmArryGHLT[j]);
			}
			free(FsmArryGHLT);
			for(unsigned j=0;j<i;j++){
				free(BtbGHLT[j]);
			}
			free(BtbGHLT);
			return -1;
		}
		BtbGHLT[i][TAG]=UINT32_MAX;
		BtbGHLT[i][TARGET]=UINT32_MAX;
		
	}

	return 0;
}


/// @brief predict function for GHGL
/// @param pc 
/// @param dst 
/// @return the prediction
bool predictGHLT(uint32_t pc ,uint32_t *dst);
bool predictGHLT(uint32_t pc ,uint32_t *dst){
	unsigned BtbIndex=0,tag=0;

	unsigned FsmNum=(unsigned)pow(2,BpHistorySize);

	get_tag_and_index(pc,BpBtbSize,BpTagSize,&tag,&BtbIndex);

	if(BtbGHLT[BtbIndex][TAG]!=tag){
		BtbGHLT[BtbIndex][TAG]=tag;
		*dst=pc+4;
		for (int i = 0; i < FsmNum; i++)
		{
			FsmArryGHLT[BtbIndex][i]=BpFsmState;
		}
		return NOT_TAKEN;
	}

	else{
		bool predict=GetPrediction(FsmArryGHLT[BtbIndex][GlobalHistory]);
		if(predict==NOT_TAKEN){
			*dst=pc+4;
		}
		else{
			*dst=BtbGHLT[BtbIndex][TARGET];
		}
		return predict;
	}

}

/// @brief the update function for GHGL
/// @param pc 
/// @param targetPc 
/// @param taken 
/// @param pred_dst 
void updateGHLT(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
void updateGHLT(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	unsigned BtbIndex,tag;

	get_tag_and_index(pc,BpBtbSize,BpTagSize,&tag,&BtbIndex);
	unsigned FsmNum=(unsigned)pow(2,BpHistorySize);
	BtbGHLT[BtbIndex][TARGET]=targetPc;
	if(taken){
		Taken(&FsmArryGHLT[BtbIndex][GlobalHistory]);
	}
	else{
		NotTaken(&FsmArryGHLT[BtbIndex][GlobalHistory]);
	}
	SetHistory(taken,FsmNum,&GlobalHistory);
}
/*******************************************************************************************/
//end of GHLT
/*******************************************************************************************/









/*******************************************************************************************/
//for local history global table
/*******************************************************************************************/
unsigned* FsmArryLHGT;
unsigned** BtbLHGT;
/// @brief imit function for LHGT
/// @param btbSize 
/// @param historySize 
/// @param tagSize 
/// @param fsmState 
/// @param Shared 
/// @return -1 for error, 0 for success
int initLHGT(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,int Shared);
int initLHGT(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,int Shared){
	unsigned FsmNum=(unsigned)pow(2,historySize);
	FsmArryLHGT=malloc((unsigned)FsmNum*sizeof(unsigned));
	if(FsmArryLHGT==NULL)return -1;

	for(int i=0;i<FsmNum;i++){
		FsmArryLHGT[i]=BpFsmState;
	}

	BtbLHGT=(unsigned**)malloc((unsigned)btbSize*sizeof(unsigned*));
	if(BtbLHGT==NULL){
		free(FsmArryLHGT);
		return -1;
	}

	for(int i=0;i<BpBtbSize;i++){
		BtbLHGT[i]=malloc(3*sizeof(unsigned));
		if(BtbLHGT[i]==NULL){
			for(int j=0;j<i;j++){
				free(BtbLHGT[j]);
			}
			free(FsmArryLHGT);
			return -1;
		}
		BtbLHGT[i][HISTORY]=(unsigned)(0);
		BtbLHGT[i][TAG]=UINT32_MAX;
		BtbLHGT[i][TARGET]=UINT32_MAX;
		
	}

	return 0;
}

/// @brief the predict function for LHGT
/// @param pc 
/// @param dst 
/// @return the prediction or NOT_TAKEN
bool predictLHGT(uint32_t pc ,uint32_t *dst);
bool predictLHGT(uint32_t pc ,uint32_t *dst){
	unsigned BtbIndex=0,tag=0,history=0,tempPc=pc;

	get_tag_and_index(pc,BpBtbSize,BpTagSize,&tag,&BtbIndex);

	if(BtbLHGT[BtbIndex][TAG]==tag){
		history=BtbLHGT[BtbIndex][HISTORY];
	}
	if(BpShared==NOT_SHARED){
		FsmIndex=history;
	}
	else if(BpShared==SHARED_LSB){
		tempPc=tempPc/4;
		FsmIndex=tempPc%((unsigned)pow(2,BpHistorySize));
		FsmIndex=FsmIndex^history;
	}
	else if(BpShared==SHARED_MID){
		tempPc=(unsigned)(tempPc/pow(2,16));
		FsmIndex=tempPc%((unsigned)pow(2,BpHistorySize));
		FsmIndex=FsmIndex^history;
	}
	if(BtbLHGT[BtbIndex][TAG]==tag){
		bool prediction = GetPrediction(FsmArryLHGT[FsmIndex]);
		if(prediction==NOT_TAKEN){
			*dst=pc+4;
		}
		else{
			*dst=BtbLHGT[BtbIndex][TARGET];
		}
		return prediction;
	}
	BtbLHGT[BtbIndex][TAG]=tag;
	BtbLHGT[BtbIndex][HISTORY]=0;
	*dst=pc+4;
	return NOT_TAKEN;
}

/// @brief the update function for LHGT
/// @param pc 
/// @param targetPc 
/// @param taken 
/// @param pred_dst 
void updateLHGT(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
void updateLHGT(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	unsigned tag,BtbIndex;
	get_tag_and_index(pc,BpBtbSize,BpTagSize,&tag,&BtbIndex);
	unsigned FsmNum=(unsigned)pow(2,BpHistorySize);
	BtbLHGT[BtbIndex][TARGET]=targetPc;
	if(taken){
		Taken(&FsmArryLHGT[FsmIndex]);
	}
	else{
		NotTaken(&FsmArryLHGT[FsmIndex]);
	}
	SetHistory(taken,FsmNum,&BtbLHGT[BtbIndex][HISTORY]);
}
/*******************************************************************************************/
//end of LHGT
/*******************************************************************************************/













/*******************************************************************************************/
//for local history local table
/*******************************************************************************************/

unsigned** BtbLHLT;
unsigned** FsmArryLHLT;
/// @brief the init function for LHLT
/// @param btbSize 
/// @param historySize 
/// @param tagSize 
/// @param fsmState 
/// @param Shared 
/// @return -1 for error, 0 for success
int initLHLT(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,int Shared);
int initLHLT(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,int Shared){
	unsigned FsmNum=(unsigned)pow(2,historySize);
	FsmArryLHLT=malloc((unsigned)btbSize*sizeof(unsigned*));
	if(FsmArryLHLT==NULL)return -1;
	for (int i = 0; i < BpBtbSize; i++)
	{
		FsmArryLHLT[i]=malloc((unsigned)FsmNum*sizeof(unsigned));
		if(FsmArryLHLT[i]==NULL){
			for(int j=0;j<i;j++){
				free(FsmArryLHLT[j]);
			}
			free(FsmArryLHLT);
			return -1;
		}
		for(int j=0;j<FsmNum;j++){
			FsmArryLHLT[i][j]=BpFsmState;
		}
	}
	BtbLHLT=(unsigned**)malloc((unsigned)btbSize*sizeof(unsigned*));
	if(BtbLHLT==NULL){
		for(int i=0;i<BpBtbSize;i++){
			free(FsmArryLHLT[i]);
		}
		free(FsmArryLHLT);
		return -1;
	}
	for (int i = 0; i < BpBtbSize; i++)
	{
		BtbLHLT[i]=malloc(3*sizeof(unsigned));
		if(BtbLHLT[i]==NULL){
			for(int j=0;j<i;j++){
				free(BtbLHLT[j]);
			}
			for(int j=0;j<BpBtbSize;j++){
				free(FsmArryLHLT[j]);
			}
			free(FsmArryLHLT);
			free(BtbLHLT);
			return -1;
		}
		BtbLHLT[i][HISTORY]=(unsigned)(0);
		BtbLHLT[i][TAG]=UINT32_MAX;
		BtbLHLT[i][TARGET]=UINT32_MAX;	
	}
	return 0;
}

/// @brief the predict function for LHLT
/// @param pc 
/// @param dst 
/// @return NOT_TAKEN for not taken, TAKEN for taken 
bool predictLHLT(uint32_t pc ,uint32_t *dst);
bool predictLHLT(uint32_t pc ,uint32_t *dst){

	unsigned BtbIndex=0,tag=0;
	unsigned FsmNum=(unsigned)pow(2,BpHistorySize);

	get_tag_and_index(pc,BpBtbSize,BpTagSize,&tag,&BtbIndex);

	if(BtbLHLT[BtbIndex][TAG]!=tag){
		BtbLHLT[BtbIndex][TAG]=tag;
		BtbLHLT[BtbIndex][HISTORY]=0;
		*dst=pc+4;
		for (int i = 0; i < FsmNum; i++)
		{
			FsmArryLHLT[BtbIndex][i]=BpFsmState;
		}
		return NOT_TAKEN;
	}
	else{
		unsigned history=BtbLHLT[BtbIndex][HISTORY];
		bool prediction = GetPrediction(FsmArryLHLT[BtbIndex][history]);
		if(prediction==NOT_TAKEN){
			*dst=pc+4;
		}
		else{
			*dst=BtbLHLT[BtbIndex][TARGET];
		}
		return prediction;
	}

}

/// @brief the update function for LHLT
/// @param pc 
/// @param targetPc 
/// @param taken 
/// @param pred_dst 
void updateLHLT(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
void updateLHLT(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	unsigned tag,BtbIndex;
	get_tag_and_index(pc,BpBtbSize,BpTagSize,&tag,&BtbIndex);
	unsigned FsmNum=(unsigned)pow(2,BpHistorySize);
	BtbLHLT[BtbIndex][TARGET]=targetPc;
	if(taken){
		Taken(&FsmArryLHLT[BtbIndex][BtbLHLT[BtbIndex][HISTORY]]);
	}
	else{
		NotTaken(&FsmArryLHLT[BtbIndex][BtbLHLT[BtbIndex][HISTORY]]);
	}
	SetHistory(taken,FsmNum,&BtbLHLT[BtbIndex][HISTORY]);
}
/*******************************************************************************************/
//end of LHLT
/*******************************************************************************************/












int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	BpBtbSize=btbSize;
	BpHistorySize=historySize;
	BpTagSize=tagSize;
	BpFsmState=fsmState;
	BpShared=Shared;

	if(isGlobalHist){
		if(isGlobalTable){
			mode=GHGT;
			BpTotalSize=btbSize*(tagSize+31)+historySize+2*pow(2,historySize);
			return initGHGT(btbSize,historySize,tagSize,fsmState,Shared);
		}
		else{
			mode=GHLT;
			BpTotalSize=btbSize*(tagSize+31)+btbSize*((2*(pow(2,historySize))))+historySize;
			return initGHLT(btbSize,historySize,tagSize,fsmState,Shared);
		}
		
	}
	else{
		if(isGlobalTable){
			mode=LHGT;
			BpTotalSize=btbSize*(tagSize+31)+btbSize*historySize+2*pow(2,historySize);
			return initLHGT(btbSize,historySize,tagSize,fsmState,Shared);
		}
		else{
			mode=LHLT;
			BpTotalSize=btbSize*(tagSize+31)+btbSize*(historySize+2*pow(2,historySize));
			return initLHLT(btbSize,historySize,tagSize,fsmState,Shared);
		}
	}
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	if(mode==GHGT){
		return predictGHGT(pc,dst);
	}
	else if(mode==GHLT){
		return predictGHLT(pc,dst);
	}
	else if(mode==LHGT){
		return predictLHGT(pc,dst);
	}
	else if(mode==LHLT){
		return predictLHLT(pc,dst);
	}
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	if(mode==GHGT){
		updateGHGT(pc,targetPc,taken,pred_dst);
	}
	else if(mode==GHLT){
		updateGHLT(pc,targetPc,taken,pred_dst);
	}
	else if(mode==LHGT){
		updateLHGT(pc,targetPc,taken,pred_dst);
	}
	else if(mode==LHLT){
		updateLHLT(pc,targetPc,taken,pred_dst);
	}
	if((taken&&(targetPc!=pred_dst))	||	(!taken && pred_dst!=pc+4) ){
		FlushNum++;	
	}
	BranchNum++;
}

void BP_GetStats(SIM_stats *curStats){
	curStats->flush_num=FlushNum;
	curStats->br_num=BranchNum;
	curStats->size=BpTotalSize;

	if(mode==GHGT){
		free(FsmArryGHGT);
		for (int i = 0; i < BpBtbSize; i++)
		{
			free(BtbGHGT[i]);
		}
		free(BtbGHGT);
	}
	else if(mode==GHLT){
		
		for (unsigned i = 0; i < BpBtbSize; i++)
		{
			free(BtbGHLT[i]);
		}
		free(BtbGHLT);
		for (int j = 0; j < BpBtbSize; j++)
		{
			free(FsmArryGHLT[j]);
		}
		free(FsmArryGHLT);
		
	}
	else if(mode==LHGT){
		free(FsmArryLHGT);
		for (unsigned i = 0; i < BpBtbSize; i++)
		{
			free(BtbLHGT[i]);
		}
		free(BtbLHGT);
	}
	else if(mode==LHLT){
		for (int j = 0; j < BpBtbSize; j++)
		{
			free(FsmArryLHLT[j]);
		}
		
		free(FsmArryLHLT);
		for (int i = 0; i < BpBtbSize; i++)
		{
			free(BtbLHLT[i]);
		}
		free(BtbLHLT);
	}
}

