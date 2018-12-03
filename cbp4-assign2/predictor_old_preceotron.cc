#include "predictor.h"

/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////

#define TWOBIT_TABLE_LENGTH 4096

static int twobit_history_table[TWOBIT_TABLE_LENGTH];

void InitPredictor_2bitsat() {
    int i;
    for (i = 0; i< TWOBIT_TABLE_LENGTH; i++)
    {
      twobit_history_table[i] = 3;
     }
}

bool GetPrediction_2bitsat(UINT32 PC) {
  // mask to get the bits 2 to 14 of PC
  UINT32 twobit_mask = 0x00003FFC;
  int twobit_index = (int) ((PC & twobit_mask)>>2); 
  int predictValue = twobit_history_table [twobit_index];

  //printf (" PC: %d, index: %d, value: %d \n",PC, twobit_index, predictValue);
  //case for not taken
  if (predictValue == 0 || predictValue ==1)
    return NOT_TAKEN;
  //case for taken
  else 
    return TAKEN;

}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

  // mask to get the bits 2 to 14 of PC
  UINT32 twobit_mask = 0x00003FFC;
  int twobit_index = (int) ((PC & twobit_mask)>>2); 
  int predictValue = twobit_history_table [twobit_index];
  if (resolveDir){ //taken
	if (predictValue < 3)
		twobit_history_table [twobit_index]++;	
  } else if (!resolveDir){ //not taken
	if (predictValue > 0)
		twobit_history_table [twobit_index]--;

  }
}



/////////////////////////////////////////////////////////////
// 2level
/////////////////////////////////////////////////////////////

#define TWOLEVEL_BHT_LENGTH 512
#define TWOLEVEL_PHT_LENGTH 64
#define TWOLEVEL_PHT_TABLE_NUMBER 8



static UINT32 twolevel_BHT[TWOLEVEL_BHT_LENGTH];
static UINT32 twolevel_PHT[TWOLEVEL_PHT_TABLE_NUMBER][TWOLEVEL_PHT_LENGTH];


void InitPredictor_2level() {
    int i;
    for (i = 0; i< TWOLEVEL_BHT_LENGTH; i++)
    {
      twolevel_BHT[i] = 0x00000000;
     }
   int j;
    for (i = 0; i< TWOLEVEL_PHT_TABLE_NUMBER; i++)
    {
      for (j = 0; j< TWOLEVEL_PHT_LENGTH; j++)
      {
        twolevel_PHT[i][j] =1;
      } 
    } 

}

bool GetPrediction_2level(UINT32 PC) {
  // get PHT table number
  UINT32 pht_table_mask = 0x00000007;
  int pht_table_number = (int)(PC & pht_table_mask);
  // get the index for BHT
  UINT32 bht_mask = 0x00000ff8;
  int bht_index = (int)((PC & bht_mask)>>3);
  // get which PHT index
  int pht_index = (int) twolevel_BHT[bht_index];
  //printf("PHT: %d, pht_table_number : %d, bht_index: %d, \n", pht_index,pht_table_number, bht_index);
  //get the prediction value
  int predictValue = twolevel_PHT[pht_table_number][pht_index];
  if (predictValue == 0 || predictValue ==1)
   {
   // printf("can return NOT_ TAKEN\n");
    return NOT_TAKEN;
   }
  //case for taken
  else 
    {
   // printf("can return TAKEN\n");
    return TAKEN;
    }
  
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

  // get PHT table number
  UINT32 pht_table_mask = 0x00000007;
  int pht_table_number = (int)(PC & pht_table_mask);
  // get the index for BHT
  UINT32 bht_mask = 0x00000ff8;
  int bht_index = (int)((PC & bht_mask)>>3);
  // get which PHT index
  int pht_index = (int) twolevel_BHT[bht_index];
  int predictValue = twolevel_PHT[pht_table_number][pht_index];
  if (resolveDir){ //taken
        // update the history pattern
        twolevel_BHT[bht_index] = ((twolevel_BHT[bht_index]<<1) | (0x00000001)) & 0x0000003f;
	if (predictValue < 3)
		twolevel_PHT[pht_table_number][pht_index]++;	
  } else if (!resolveDir){ //not taken
        twolevel_BHT[bht_index] = ((twolevel_BHT[bht_index]<<1) | (0x00000000)) & 0x0000003f;
	if (predictValue > 0)
		twolevel_PHT[pht_table_number][pht_index]--;

  }

}

/////////////////////////////////////////////////////////////
// openend
/////////////////////////////////////////////////////////////

// choose to implement preceptron predictor

#define HISTORY_LENGTH 64
#define WEIGHT_ENTRY 1024 //take 10 bits from PC

static int weight_table[WEIGHT_ENTRY][HISTORY_LENGTH];
static int global_history[HISTORY_LENGTH];

// the threshold taken from a research paper
int threshold = 1.93*HISTORY_LENGTH + 14;
int prediction = 0;

int GetIndexFromPC (UINT32 PC){
  UINT32 index_mask = 0x000003ff;
  int index = (int)(PC & index_mask);
  return index;
}


void InitPredictor_openend() {
  int i;
  int j;
  //initialize weight table
  for (i = 0; i < WEIGHT_ENTRY; i++){
    for (j = 0; j < HISTORY_LENGTH; j++){
      weight_table[i][j] = 0;
    }
  }
  //initialize global history table
  for (i=0; i < HISTORY_LENGTH; i++){
    global_history[i] = 0;
  }
}

bool GetPrediction_openend(UINT32 PC) {
  int index = GetIndexFromPC (PC);
  int i;
  prediction = 0;
  for (i = 0; i < HISTORY_LENGTH; i++){
    int w = weight_table[index][i];
    prediction += w * global_history[i];
  }
  // the branch is predicted to be taken if >0
  if (prediction >0)
    return TAKEN;
  else 
    return NOT_TAKEN;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  int index = GetIndexFromPC(PC);
  // get absolute value of prediction
  if (prediction < 0)
    prediction = prediction * (-1);
  // handle the scenario where the preDir is not equal to resolveDir, decrease weight correspondingly
  if(resolveDir != predDir || prediction <= threshold){
    int i;
    for (int i = 0; i< HISTORY_LENGTH; i++){
      bool history = (global_history[i]==1) ? TAKEN:NOT_TAKEN;
      if ((history == resolveDir) && (weight_table[index][i] < threshold)){
        weight_table[index][i] += 1;
      }
      else if ((history != resolveDir) && (weight_table[index][i] > threshold * (-1))){
        weight_table[index][i] -= 1;
      }
    }
    
    //shift the local history

    for (i = 1;i< HISTORY_LENGTH; i++)
      global_history[i-1] = global_history[i];
    if (resolveDir == TAKEN)
      global_history[HISTORY_LENGTH-1] = 1;
    else 
      global_history[HISTORY_LENGTH-1] = -1;
    
  }
}

