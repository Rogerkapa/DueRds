/*
  RDS ENCODER ARDUINO DUE modded from Jonas work
  Thanks to Vincent to help coding CT group
  #define RDS_COUNTRY_PT    0x08E4  /* Portugal */
*/

#include "genR.h"
#include "RTClib.h"

extern RTC_PCF8563 rtc;

void RDSClass::begin(uint32_t period) {
  SineTable_UP(2, 254);  //255Max  155 RDS LEVEL OUTPUT

  // SOFT INTERRUPT
  NVIC_ClearPendingIRQ(SOFT_IRQn);
  NVIC_SetPriority(SOFT_IRQn, 1);
  NVIC_EnableIRQ(SOFT_IRQn);

  // Configure DAC
  pmc_enable_periph_clk (DACC_INTERFACE_ID) ; // start clocking DAC
  dacc_reset(DACC);
  dacc_set_transfer_mode(DACC, 0);
  dacc_set_power_save(DACC, 0, 1);      // sleep = 0, fastwkup = 1
  dacc_set_analog_control(DACC, DACC_ACR_IBCTLCH0(0x02) | DACC_ACR_IBCTLCH1(0x02) | DACC_ACR_IBCTLDACCORE(0x01));
  dacc_set_trigger(DACC, 1);
  dacc_set_channel_selection(DACC, 1);
  dacc_enable_channel(DACC, 1);
  //dacc_set_channel_selection(DACC, 0);
  //dacc_enable_channel(DACC, 0);

  NVIC_DisableIRQ(DACC_IRQn);
  NVIC_ClearPendingIRQ(DACC_IRQn);
  NVIC_SetPriority(DACC_IRQn, 0);
  NVIC_EnableIRQ(DACC_IRQn);
  dacc_enable_interrupt(DACC, DACC_IER_ENDTX);

  DACC->DACC_TPR  =  (uint32_t)SignalTable_57khz;      // DMA buffer
  DACC->DACC_TCR  =  480;  //480;
  DACC->DACC_TNPR =  (uint32_t)SignalTable_57khz;      // next DMA buffer
  DACC->DACC_TNCR =  30;   //30;
  DACC->DACC_PTCR =  0x00000100;  //TXTEN - 8, RXTEN - 1.

  // Configure Timer Counter to trigger DAC
  // --------------------------------------
  pmc_enable_periph_clk(TC_INTERFACE_ID + 0 * 3 + 0);

  t->TC_CCR = TC_CCR_CLKDIS;
  t->TC_IDR = 0xFFFFFFFF;
  t->TC_SR;
  t->TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK1 |
              TC_CMR_WAVE |
              TC_CMR_WAVSEL_UP_RC |
              TC_CMR_EEVT_XC0 |
              TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_CLEAR |
              TC_CMR_BCPB_CLEAR | TC_CMR_BCPC_CLEAR;

  t->TC_RC = TCdivider_2;
  t->TC_RA = TCdivider_2 / 2;
  t->TC_CMR = (t->TC_CMR & 0xFFF0FFFF) | TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_SET;
  t->TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;

  // Configure pins
  PIO_Configure(g_APinDescription[DAC0].pPort,
                g_APinDescription[DAC0].ulPinType,
                g_APinDescription[DAC0].ulPin,
                g_APinDescription[DAC0].ulPinConfiguration);
  PIO_Configure(g_APinDescription[DAC1].pPort,
                g_APinDescription[DAC1].ulPinType,
                g_APinDescription[DAC1].ulPin,
                g_APinDescription[DAC1].ulPinConfiguration);

  // Enable interrupt controller for DAC
  dacc_disable_interrupt(DACC, 0xFFFFFFFF);
  NVIC_DisableIRQ(DACC_IRQn);
  NVIC_ClearPendingIRQ(DACC_IRQn);
  NVIC_SetPriority(DACC_IRQn, 0);
  NVIC_EnableIRQ(DACC_IRQn);
  dacc_enable_interrupt(DACC, DACC_IER_ENDTX);
}

void RDSClass::end() {
  TC_Stop(TC0, 1);
  NVIC_DisableIRQ(DACC_IRQn);
}

bool RDSClass::PILOT_sync_state(void) const {
  return sync_OK;
}

void RDSClass::PS_Set(char* pps) {
    for (int i = 0; i < 8; i++) PS_NAME[i] = *(pps + i);
}

/*---SCROLL PS---*/
void RDSClass::scroll_ps(bool enable) {
  static int point_pos = 0, cicle_count = 0;

  if (enable) {
    if (cicle_count > 1) {
      cicle_count = 0;
      if (*PS_Pointer) {
        PS_Pointer = DINAMIC_PS + point_pos++;
      }
      else {
        PS_Pointer = DINAMIC_PS;
        point_pos = 0;
      }
    }
    cicle_count++;
  }
  else {
    PS_Pointer = PS_NAME;
    point_pos = 0;
  }
}


void RDSClass::SendingGroups(void) {
  static int group_i = 0;
  static bool d_ps = 0;

  if (!new_group_end) {
    if (1) {

      if (DINAMIC_PS_ON_OFF && d_ps) sendGroup(Group_0A);
      else {
        sendGroup(GROUPS[group_i]);
        d_ps = !d_ps;
        group_i++;
      }
      new_group_end = true;

      if (group_i > 5) group_i = 0;   //4 changed to 5 to send Group 4A with CT=hour, minutes
    }
    else group_i = 0;
  }
}
/*---SENDING LOOP END---
  =======================================================================================================================================
  ---SEND GROUP---*/
int RDSClass::sendGroup(Group_type group) {
  
  setBlock(PI_CODE, Block_A);

  switch (group)
  {
    case Group_0A:

      setBlock((((word)group) << 11) | (((word)TP) << 10) | (((word)PTY) << 5) | (((word)TA) << 4) | (((word)M_S) << 3) | (((word)DI[di_ps_i / 2]) << 2) | (word)(di_ps_i / 2), Block_B);
      setBlock(ALT_FREQ, Block_C);
      setBlock((((word) * (PS_Pointer + di_ps_i++)) << 8) | *(PS_Pointer + di_ps_i++), Block_D);
      if (di_ps_i > 7) {
        di_ps_i = 0;
        scroll_ps(DINAMIC_PS_ON_OFF);
      }
      break;

    case Group_0B:

      setBlock((((word)group) << 11) | (((word)TP) << 10) | (PTY << 5) | (((word)TA) << 4) | (((word)M_S) << 3) | (((word)DI[di_ps_i / 2]) << 2) | (di_ps_i / 2), Block_B);
      setBlock(PI_CODE, Block_C_prime);
      setBlock((((word) * (PS_Pointer + di_ps_i++)) << 8) | *(PS_Pointer + di_ps_i++), Block_D);
      if (di_ps_i > 7) {
        di_ps_i = 0;
        scroll_ps(DINAMIC_PS_ON_OFF);
      }
      break;

    case Group_1A:

      break;

    case Group_1B:

      setBlock(PI_CODE, Block_C_prime);
      break;

    case Group_2A:
      setBlock((((word)group) << 11) | (((word)TP) << 10) | ((word)PTY << 5) | (((word)RT_A_B_FLAG) << 4) | (rt_i / 4), Block_B);
      setBlock((((word)RT_MESSAGE[rt_i++]) << 8) | RT_MESSAGE[rt_i++], Block_C);
      setBlock((((word)RT_MESSAGE[rt_i++]) << 8) | RT_MESSAGE[rt_i++], Block_D);
      if (rt_i > 63) rt_i = 0;
      break;

    case Group_2B:

      if (rt_i > 31) rt_i = 0;
      setBlock((((word)group) << 11) | (((word)TP) << 10) | ((word)PTY << 5) | (((word)RT_A_B_FLAG) << 4) | (rt_i / 4), Block_B);
      setBlock(PI_CODE, Block_C_prime);
      setBlock((((word)RT_MESSAGE[rt_i++]) << 8) | RT_MESSAGE[rt_i++], Block_D);
      if (rt_i > 31) rt_i = 0;
      break;

    case Group_3A:

      setBlock((((word)group) << 11) | (((word)TP) << 10) | ((word)PTY << 5) | ((word)Group_11A), Block_B);
      setBlock((rtPlus_rfu << 13) | (((word)rtPlus_CB_flag) << 12) | (rtPlus_SCB_flag << 8), Block_C);
      setBlock(0100101111010111, Block_D);
      break;

    case Group_3B:

      setBlock(PI_CODE, Block_C_prime);
      break;

    case Group_4A:
    {
// Vincent code                         //
// --------------------------------------
//int month   = 5;
//int year    = 2023;
//int day     = 21;
//int hours   = 16;
//int minute = 21;

//int offs = month < 1 ? 1 : 0;

//int MonthYearDay = 14956 + day + (int)((year - 1) * 365.25) + (int)((month + 2 + offs*12) * 30.6001);

//word blockB = 0x4400 | (MonthYearDay>>15);
//word blockC = (MonthYearDay<<1) | (hours>>4);
//word blockD = (hours & 0x0F)<<12 | minute<<6;

//setBlock(blockB, Block_B);
//setBlock(blockC, Block_C);
//setBlock(blockD, Block_D);
// --------------------------------------
  
    DateTime now = rtc.now();
    
    int day = now.day();
    int month = now.month();
    int year = now.year();
    int hour = now.hour();
    int minute = now.minute();

//    Serial.print(now.hour(), DEC);    //Debug if i2c RTC is OK
//    Serial.print(':');
//    Serial.println(now.minute(), DEC);
    
    int l = month <= 1 ? 1 : 0;
    int mjd = 14956 + day +(int)((year - l) * 365.25) + (int)((month + 2 + l*12) * 30.6001);

    setBlock((((word)group) << 11)| 0x4400 | (mjd>>15), Block_B);
    setBlock((word) (mjd<<1) | (hour>>4), Block_C);
    setBlock((word) ((hour & 0xF)<<12) | minute<<6, Block_D);
    }
      break;
    
    case Group_4B:
      setBlock(PI_CODE, Block_C_prime);
      
      break;

    case Group_5A:

      break;

    case Group_5B:

      setBlock(PI_CODE, Block_C_prime);
      break;

    case Group_6A:

      break;

    case Group_6B:

      setBlock(PI_CODE, Block_C_prime);
      break;

    case Group_7A:

      break;

    case Group_7B:

      setBlock(PI_CODE, Block_C_prime);
      break;

    case Group_8A:

      break;

    case Group_8B:
      setBlock(PI_CODE, Block_C_prime);
      break;

    case Group_9A:

      break;

    case Group_9B:
      setBlock(PI_CODE, Block_C_prime);
      break;

    case Group_10A:

      setBlock((((word)group) << 11) | (((word)TP) << 10) | (((word)PTY) << 5) | (((word)PTYN_FLAG) << 4) | (word)(ptyn_i / 4), Block_B);
      setBlock((((word) * (PTYN_NAME + ptyn_i++)) << 8) | *(PTYN_NAME + ptyn_i++), Block_C);
      setBlock((((word) * (PTYN_NAME + ptyn_i++)) << 8) | *(PTYN_NAME + ptyn_i++), Block_D);
      if (ptyn_i > 7) {
        ptyn_i = 0;
      }
      break;

    case Group_10B:
      setBlock(PI_CODE, Block_C_prime);
      break;

    case Group_11A:

      break;

    case Group_11B:
      setBlock(PI_CODE, Block_C_prime);
      break;

    case Group_12A:

      break;

    case Group_12B:
      setBlock(PI_CODE, Block_C_prime);
      break;

    case Group_13A:

      break;

    case Group_13B:
      setBlock(PI_CODE, Block_C_prime);
      break;

    case Group_14A:

      break;

    case Group_14B:
      setBlock(PI_CODE, Block_C_prime);
      break;

    case Group_15A:

      setBlock((((word)group) << 11) | (((word)TP) << 10) | (((word)PTY) << 5) | (((word)TA) << 4) | (word)(di_ps_i / 4), Block_B);
      setBlock((((word) * (PS_Pointer + di_ps_i++)) << 8) | *(PS_Pointer + di_ps_i++), Block_C);
      setBlock((((word) * (PS_Pointer + di_ps_i++)) << 8) | *(PS_Pointer + di_ps_i++), Block_D);
      if (di_ps_i > 7) {
        di_ps_i = 0;
      }

      break;

    case Group_15B:

      setBlock((((word)group) << 11) | (((word)TP) << 10) | ((word)PTY << 5) | (((word)RT_A_B_FLAG) << 4) | (rt_i / 4), Block_B);
      setBlock(PI_CODE, Block_C_prime);
      setBlock((((word)group) << 11) | (((word)TP) << 10) | ((word)PTY << 5) | (((word)RT_A_B_FLAG) << 4) | (rt_i / 4), Block_D);
      if (di_ps_i > 7) {
        di_ps_i = 0;
      }
      break;
  }
}

void RDSClass::setBlock(word message, offset_type offset)
{
  int bite = 0;
  static int bytecount = 0;
  static int BytePointer = 0;
  switch (offset)
  {
    case Block_A:
      BytePointer = 0;
      break;
    case Block_B:
      BytePointer = 1;
      break;
    case Block_C:
      BytePointer = 2;
      break;
    case Block_C_prime:
      BytePointer = 2;
      break;
    case Block_D:
      BytePointer = 3;
      break;
  }
  RDS_Data_Block_word[MEM_select][BytePointer] = (message << 10) | (crc(message) ^ (word)offset);
  
}

/*  ---CRC---*/
word RDSClass::crc(word message)
{
  word crc = 0;
  int message_bit;
  int crc_bit;
  for (int i = 0; i < 16; i++)          //we are finished when all bits of the message are looked at
  {
    message_bit = (message >> 15) & 1;  //bit of message we are working on. 15=block length-1
    crc_bit = (crc >> 9) & 1;           //bit of crc we are working on. 9=poly order-1

    crc <<= 1;           //shift to next crc bit (have to do this here before Gate A does its thing)
    message <<= 1;       //shift to next message bit

    if (crc_bit ^ message_bit)crc = crc ^ 0x1B9;  //add to the crc the poly mod2 if crc_bit + block_bit = 1 mod2

  }
  return (crc & 0x03FF);       //ditch things above 10bits
  
}


/*---SIGNAL---*/
void RDSClass::SineTable_UP(int new_rds_phase, int new_rds_level) {
  const double pi = 3.141592653589793238462643383279502884197169399375105820974944;
  int iii = NORMAL_PHASE_0;
  int sample_57k = 10;  
  static int old_rds_phase = 0; 
  static int old_rds_level = 0;

  if ((old_rds_phase != new_rds_phase) && (old_rds_level != new_rds_level)) {
    for (int i = 0; i < 72; i++) {    
      for (int ii = 0; ii < sample_57k; ii++) {
        SignalTable_57khz[iii++] = 2047 + sin((pi / 24) * i) * (sin((pi / (sample_57k / 2)) * ii) * (8 * new_rds_level));
      }
    }

    iii = NORMAL_PHASE_1;
    for (int i = 0; i < 96; i++) {     
      for (int ii = 0; ii < sample_57k; ii++) {
        SignalTable_57khz[iii++] = 2047 + ((sin((pi / 48) * i) * 2.5) + sin((pi / 16) * i)) * (sin((pi / (sample_57k / 2)) * ii) * (3 * new_rds_level));
      }
    }
  }
  old_rds_phase = new_rds_phase;
  old_rds_level = new_rds_level;

}

RDSClass RDS;

/*---ISR---*/
void SOFT_Handler(void) {
  RDS.SendingGroups();
}

/*---ISR---*/
void DACC_Handler(void) {
  if ((dacc_get_interrupt_status(DACC) & DACC_ISR_ENDTX) == DACC_ISR_ENDTX) {
    RDS.DACC_ISR_ISR();
  }
}

void RDSClass::DACC_ISR_ISR(void)
{
  static bool DataBit = false, PhaseShistBit = false;
  static int BlockNumber = 0, bitcounter = 0, sync_c_1 = 0, sync_c_2 = 0;
  static word RDS_Data_Block_bit_mask = (1 << 25);

  //---PILOT SYNC--->>>---
  if (digitalReadDirect(A1)) {
    t->TC_RC = 73;
    sync_c_1--;
  }
  else {
    t->TC_RC = 74;
    sync_c_1++;
  }
  if (sync_c_2 & (1 << 10)) {     //10
    if ((sync_c_1 > 340) && (sync_c_1 < 380))    // 340-344  360-397-  355 392 INT19KHZ _  RVR EXT19KHZ READ = 373-377 OMNIA=373 375
      sync_OK = true;
        
    else sync_OK = false;
    //Serial.println(sync_c_1);   //=341 343 With internal 19KHZ print READ sync_c_1 and copy value to adjust pll
    sync_c_1 = 0;
    sync_c_2 = 0;   
  }
  else {
    sync_c_2++;
  }
  //---<<<---PILOT SYNC---

  bitcounter += 30;
  if (bitcounter >= 480) {      //480
    bitcounter = 0;

    if (RDS_Data_Block_word[MEM_select][BlockNumber] & RDS_Data_Block_bit_mask) {
      PhaseShistBit = !PhaseShistBit;
      DataBit = true;                 
    }
    else DataBit = false;             
    if (PhaseShistBit) {
      if (DataBit) PHASE_VAR = NORMAL_PHASE_1;
      else PHASE_VAR = INVERT_PHASE_0;
    }
    else {
      if (DataBit) PHASE_VAR = INVERT_PHASE_1;
      else PHASE_VAR = NORMAL_PHASE_0;
    }


    //---------------------
    RDS_Data_Block_bit_mask >>= 1;
    if (!RDS_Data_Block_bit_mask) {
      RDS_Data_Block_bit_mask = (1 << 25);

      BlockNumber++;
      if (BlockNumber > 3) {

        NVIC_SetPendingIRQ(SOFT_IRQn);

        BlockNumber = 0;
        if (new_group_end) {
          new_group_end = false;
          MEM_select = !MEM_select;
        }
      }
    }
  }
  DACC->DACC_TNPR = (uint32_t)SignalTable_57khz + (PHASE_VAR + bitcounter) * 2;
  DACC->DACC_TNCR = 30;
}
