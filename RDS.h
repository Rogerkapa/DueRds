/*
  RogRDS ENCODER V2.0 CT
*/

#ifndef RDS_INCLUDED
#define RDS_INCLUDED

#include <Arduino.h>
#include "pins_arduino.h"  

/******* SOFT INTERRUPT ---> *******/
#define SOFT_IRQn TC1_IRQn
#define SOFT_Handler TC1_Handler
/******* <--- SOFT INTERRUPT *******/

/***TIME CT***/
#define RDS_GROUP_LENGTH 4
#define RDS_BITS_PER_GROUP (RDS_GROUP_LENGTH * (RDS_BLOCK_SIZE+RDS_POLY_DEG))
#define RDS_POLY 0x1B9
#define RDS_POLY_DEG 10
#define RDS_MSB_BIT 0x8000
#define RDS_BLOCK_SIZE 16
/***TIME CT***/


class RDSClass
{
  public:

    //    RDSClass();
    void begin(uint32_t period);
    void end();

    typedef enum {Block_A = 0x0FC,
                  Block_B = 0x198,
                  Block_C = 0x168,
                  Block_C_prime = 350,
                  Block_D = 0x1B4,
                  Block_E = 0
                 } offset_type;


    typedef enum {Group_0A, Group_0B,
                  Group_1A, Group_1B,
                  Group_2A, Group_2B,
                  Group_3A, Group_3B,
                  Group_4A, Group_4B,
                  Group_5A, Group_5B,
                  Group_6A, Group_6B,
                  Group_7A, Group_7B,
                  Group_8A, Group_8B,
                  Group_9A, Group_9B,
                  Group_10A, Group_10B,
                  Group_11A, Group_11B,
                  Group_12A, Group_12B,
                  Group_13A, Group_13B,
                  Group_14A, Group_14B,
                  Group_15A, Group_15B
                 } Group_type ;


    typedef enum {PTY_No_programme_type_or_undefined,
                  PTY_News,
                  PTY_Current_affairs,
                  PTY_Information,
                  PTY_Sport,
                  PTY_Education,
                  PTY_Drama,
                  PTY_Culture,
                  PTY_Science,
                  PTY_Varied,
                  PTY_Pop_music,
                  PTY_Rock_music,
                  PTY_Easy_listening,
                  PTY_Light_classical,
                  PTY_Serious_classical,
                  PTY_Other_music,
                  PTY_Weather,
                  PTY_Finance,
                  PTY_Childrens_programmes,
                  PTY_Social_affairs,
                  PTY_Religion,
                  PTY_Phone_in,
                  PTY_Travel,
                  PTY_Leisure,
                  PTY_Jazz_music,
                  PTY_Country_music,
                  PTY_National_music,
                  PTY_Oldies_music,
                  PTY_Folk_music,
                  PTY_Documentary,
                  PTY_Alarm_test,
                  PTY_Alarm
                 } pty_type;

    typedef enum {PTY_RBDS_No_program_type_or_undefined
    , PTY_RBDS_News
    , PTY_RBDS_Information
    , PTY_RBDS_Sports
    , PTY_RBDS_Talk
    , PTY_RBDS_Rock
    , PTY_RBDS_Classic_rock
    , PTY_RBDS_Adult_hits
    , PTY_RBDS_Soft_rock
    , PTY_RBDS_Top_40
    , PTY_RBDS_Country
    , PTY_RBDS_Oldies
    , PTY_RBDS_Soft
    , PTY_RBDS_Nostalgia
    , PTY_RBDS_Jazz
    , PTY_RBDS_Classical
    , PTY_RBDS_Rhythm_and_blues
    , PTY_RBDS_Soft_rhythm_and_blues
    , PTY_RBDS_Language
    , PTY_RBDS_Religious_music
    , PTY_RBDS_Religious_talk
    , PTY_RBDS_Personality
    , PTY_RBDS_Public
    , PTY_RBDS_College
    , PTY_RBDS_Spanish_Talk
    , PTY_RBDS_Spanish_Music
    , PTY_RBDS_Hip_Hop
    , PTY_RBDS_Unassigned_1
    , PTY_RBDS_Unassigned_2
    , PTY_RBDS_Weather
    , PTY_RBDS_Emergency_test
    , PTY_RBDS_Emergency
                 } pty_rbds_type;

    void SineTable_UP(int new_rds_phase, int new_rds_level);

    bool PS_mode(bool mode)  {
      DINAMIC_PS_ON_OFF = mode;
      return mode;
    };

    bool TP = false;
    bool TA = false;
    bool M_S = true;
    /*---Decoder ID---*/
    bool Stereo;
    bool Artificial_Head;

    void PS_Set(char* pps);

    void PTYN_Set(char* pptyn) {
      for (int i = 0; i < 8; i++) {
        if (*pptyn != 0 ) PTYN_NAME[i] = *pptyn + i;
        else PTYN_NAME[i] = (' ');
      }
      PTYN_FLAG = !PTYN_FLAG;
    }

    void RT_Set(char* prt) {
      int i;
      for (i = 0 ; (i < 64) && (*(prt + i) != '\0') ; i++) RT_MESSAGE[i] = *(prt + i);
      for ( ; i < 64; i++) RT_MESSAGE[i] = (' ');
      RT_A_B_FLAG = !RT_A_B_FLAG;
    }

    bool PILOT_sync_state(void) const;

    void DACC_ISR_ISR(void);

    Group_type GROUPS[10] = {Group_0A, Group_2A, Group_10A, Group_2A, Group_10A};

    void SendingGroups(void);

 /*---TIME CT ---*/
 void ct(int16_t year, uint8_t mon, uint8_t mday, uint8_t hour, uint8_t min, int16_t gmtoff);
 uint16_t value_pi = 0xCAFE; // program ID
 inline void msgbyte(uint16_t a, uint8_t b)
    {
      //volatile uint8_t *fmrds_msg_data = (volatile uint8_t *) 0xFFFFFC04;
      //volatile uint16_t *fmrds_msg_addr = (volatile uint16_t *) 0xFFFFFC06;
      volatile uint32_t *fmrds_msg_data_addr = (volatile uint32_t *) 0xFFFFFC04;
      *fmrds_msg_data_addr = (a << 16) | b;
      //*fmrds_msg_addr = a;
      //*fmrds_msg_data = b;
    }
 
 /*---TIME CT ---*/   
 
  private:

    const int NORMAL_PHASE_0 = 0;
    const int INVERT_PHASE_0 = 240;            //240
    const int NORMAL_PHASE_1 = 720;            //720
    const int INVERT_PHASE_1 = 1200;            //1200

    TcChannel * t = &(TC0->TC_CHANNEL)[0];

    uint16_t SignalTable_57khz[1681];
    uint32_t PHASE_VAR;

    word PI_CODE = 0x9000;            //(BLOCK_A) , (Block_C_prime)

    pty_type PTY = PTY_Pop_music;
    char PTYN_NAME[9] = ("DANCE M");

    /*---Decoder identification and Dynamic PTY indicator---*/
    bool DI[4] {true, true, false, true};  //[0] Static PTY - Dinamic PTY, [1] Not compressed - Compressed,
    //[2] Not Artificial Head - Artificial Head, [3] mono - stereo

    word ALT_FREQ = (0xE0 << 8) | 0xE0;
    char PS_NAME[9] = ("NJOYLIFE");  //(" N-JOY  ");
    char DINAMIC_PS[65] = ("N-JOY RADIO -BEST MUSIC IN SETUBAL");
    bool DINAMIC_PS_ON_OFF = false;
    //char RT_MESSAGE[65] = ("N-JOY THE BEST MUSIC IN THE CITY-100.0 FM ST SETUBAL HD SOUND   ");    
    //char RT_MESSAGE[65] = ("FIQUE EM CASA-RESPEITE A AUTORIDADE-USE LUVAS E MASCARAS NA RUA ");//char RT_MESSAGE[65] = ("OUVE A MELHOR MUSICA AQUI E NAO ESQUECAS DE USAR MASCARA -N-JOY ");
    //char RT_MESSAGE[65] = ("OUVE A MELHOR MUSICA AQUI E NAO ESQUECAS DE USAR MASCARA -N-JOY ");
      char RT_MESSAGE[65] = ("PEACE AND LOVE FOR UKRAINE -FUCK PUTIN- SLAVA UKRAINI - Njoylife"); 
   
    bool RT_A_B_FLAG = false;                                            
    bool PTYN_FLAG = true;
    char* PS_Pointer = PS_NAME;
    const int TCdivider_1 = 73;
    const int TCdivider_2 = 74;
    bool sync_OK;

    int sendGroup(Group_type);
    void scroll_ps(bool);
    word crc(word);
    void setBlock(word, offset_type);
    word RDS_Data_Block_word[2][4];                                               //Ezzel a bittel váltunk az aktuálisan olvasott és az írandó tömbelem közt.
    volatile bool new_group_end = false;
    bool MEM_select = false;
    int di_ps_i = 0, rt_i = 0, ptyn_i = 0;

    word rtPlus_rfu = 0;
    bool rtPlus_CB_flag = 0;
    word rtPlus_SCB_flag = 0;
    word rtPlus_Temp_num = 0;

    /* time stuff CT*/
    void binary_buf_crc(uint8_t *buffer, uint16_t *blocks);
    void binary_ct_group(uint8_t *buffer);
    void send_ct();    
    uint8_t tm_hour, tm_min;
    uint8_t tm_mon, tm_mday;
    int16_t tm_year; // 
    int16_t tm_gmtoff; //   
};

inline int digitalReadDirect(int pin) {
  return !!(g_APinDescription[pin].pPort -> PIO_PDSR & g_APinDescription[pin].ulPin);
}

extern RDSClass RDS;

#endif
