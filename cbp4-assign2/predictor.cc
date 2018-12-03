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
printf ("finish initializing\n");

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
 // printf("Values: PC: %04x Resolved: %d Predicted: %d\n",PC,resolveDir, predDir);
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


#define FOUR_BIT_STAGE_LENGTH 16 //4bits
#define EIGHT_BIT_STAGE_LENGTH 256 //8bits
#define SIXTEEN_BIT_STAGE_LENGTH 32768 //15bits
#define BIMODAL_TABLE_LENGTH 4096

static int bimodal_history_table[BIMODAL_TABLE_LENGTH];

static UINT32 GHR;
static int FOUR_BIT_TABLE[FOUR_BIT_STAGE_LENGTH];
static int EIGHT_BIT_TABLE[EIGHT_BIT_STAGE_LENGTH];
static int SIXTEEN_BIT_TABLE[SIXTEEN_BIT_STAGE_LENGTH];


void InitPredictor_openend() {
  int i;
  for (i = 0; i < BIMODAL_TABLE_LENGTH; i++){
    twobit_history_table[i] = 3;
  }
  for (i = 0; i < SIXTEEN_BIT_STAGE_LENGTH; i++){
    if (i < FOUR_BIT_STAGE_LENGTH)
      FOUR_BIT_TABLE[i] = -1;
    if (i < EIGHT_BIT_STAGE_LENGTH)
      EIGHT_BIT_TABLE[i] = -1;

    SIXTEEN_BIT_TABLE[i] = -1;
  }
  GHR = 0x11111111;
}

bool GetPrediction_openend(UINT32 PC) {
  //First Check Bimodal
  UINT32 twobit_mask = 0x00003FFC;
  int twobit_index = (int) ((PC & twobit_mask)>>2); 
  int predictValue = bimodal_history_table [twobit_index];
  
  bool answer;
  //case for not taken
  if (predictValue == 0 || predictValue ==1)
    answer = NOT_TAKEN;
  //case for taken
  else 
    answer = TAKEN;


  int answer4 = -1;
  //Check 4 Bit Table
  UINT32 fourbit_mask = 0x0000000F;
  int fourbit_index = (int) (PC & fourbit_mask) ^ (GHR & fourbit_mask);
  predictValue = FOUR_BIT_TABLE[fourbit_index];
  if (predictValue == 0 || predictValue ==1)
    answer4 = NOT_TAKEN;
  //case for taken
  else if (predictValue == 2 || predictValue == 3)
    answer4 = TAKEN;


  //Check 8 Bit Table
  int answer8 = -1;
  UINT32 eightbit_mask = 0x000000FF;
  int eightbit_index = (int) (PC & eightbit_mask) ^ (GHR & eightbit_mask);
  predictValue = EIGHT_BIT_TABLE[eightbit_index];
  if (predictValue == 0 || predictValue ==1)
    answer8 = NOT_TAKEN;
  //case for taken
  else if (predictValue == 2 || predictValue == 3)
    answer8 = TAKEN;

  //Check 16 Bit Table
  int answer16 = -1;
  UINT32 sixteenbit_mask = 0x00007FFF;
  int sixteenbit_index = (int) (PC & sixteenbit_mask) ^ (GHR & sixteenbit_mask);
  predictValue = SIXTEEN_BIT_TABLE[sixteenbit_index];
  if (predictValue >= 0 && predictValue <= 3)
    answer16 = NOT_TAKEN;
  //case for taken
  else if (predictValue >= 4 && predictValue <= 7)
    answer16 = TAKEN;

  if (answer16 != -1)
    return (bool) answer16;
  else if (answer8 != -1)
    return (bool) answer8;
  else if (answer4 != -1)
    return (bool) answer4;
  else
    return answer;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {

  //Update Bimodal
  UINT32 twobit_mask = 0x00003FFC;
  int twobit_index = (int) ((PC & twobit_mask)>>2); 
  int predictValue = bimodal_history_table [twobit_index];
  if (resolveDir){ //taken
    if (predictValue < 3)
      bimodal_history_table[twobit_index]++;  
  } else if (!resolveDir){ //not taken
    if (predictValue > 0)
      bimodal_history_table[twobit_index]--;

  }


  //Update 4 Bit
   UINT32 fourbit_mask = 0x0000000F;
  int fourbit_index = (int) (PC & fourbit_mask) ^ (GHR & fourbit_mask);
  predictValue = FOUR_BIT_TABLE[fourbit_index];
  if (resolveDir){
    if (predictValue == -1){
      FOUR_BIT_TABLE[fourbit_index] = 1;
    }
    if (predictValue < 3)
      FOUR_BIT_TABLE[fourbit_index]++;

  } else if (!resolveDir){
    if (predictValue == -1){
      FOUR_BIT_TABLE[fourbit_index] = 1;
    }
    if (predictValue > 0)
      FOUR_BIT_TABLE[fourbit_index]--;

  }


  //Update 8 Bit
  UINT32 eightbit_mask = 0x000000FF;
  int eightbit_index = (int) (PC & eightbit_mask) ^ (GHR & eightbit_mask);
  predictValue = EIGHT_BIT_TABLE[eightbit_index];
  if (resolveDir){
    if (predictValue == -1){
      EIGHT_BIT_TABLE[eightbit_index] = 1;
    }
    if (predictValue < 3)
      EIGHT_BIT_TABLE[eightbit_index]++;

  } else if (!resolveDir){
    if (predictValue == -1){
      EIGHT_BIT_TABLE[eightbit_index] = 1;
    }
    if (predictValue > 0)
      EIGHT_BIT_TABLE[eightbit_index]--;

  }


  //Update 16 Bit
  UINT32 sixteenbit_mask = 0x00007FFF;
  int sixteenbit_index = (int) (PC & sixteenbit_mask) ^ (GHR & sixteenbit_mask);
  predictValue = SIXTEEN_BIT_TABLE[sixteenbit_index];
  if (resolveDir){
    if (predictValue == -1){
      SIXTEEN_BIT_TABLE[sixteenbit_index] = 1;
    }
    if (predictValue < 7)
      SIXTEEN_BIT_TABLE[sixteenbit_index]++;

  } else if (!resolveDir){
    if (predictValue == -1){
      SIXTEEN_BIT_TABLE[sixteenbit_index] = 1;
    }
    if (predictValue > 0)
      SIXTEEN_BIT_TABLE[sixteenbit_index]--;
	

  }
  


  //Update Global History Register
  if (resolveDir)
    GHR = ((GHR << 1) | 0x00000001);
  else
    GHR = (GHR << 1);


}

