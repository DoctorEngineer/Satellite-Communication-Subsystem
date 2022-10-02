#include <SoftwareSerial.h>

//common
#define POLY 0x8408

//for AX_protocol
#define E 0X45
#define U 0X55
#define T 0X54
#define S 0X53
#define A 0X41
#define T 0X54
#define G 0X47
#define C 0X43
#define zero 0x0000
#define StartEndFrame 0x7E
#define ControlId 0xBB
#define completeSspData 0xAA
//#define       A          0xA
#define AxLength 34
//for SSP_protocol
#define COMM_ID 0x02
#define OBC_ID 0x04
#define PING 0x00
//#define PING 0x49
#define INIT 0x01
#define ACK 0x02
//#define ACK 0x49
#define NACK 0x03
#define FEND 0xC0
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD
#define GET_TELM 0x04
#define SET_OBC_TIME 0x05
//#define SET_OBC_TIME 0x49
#define GROUND 0x15
#define SSP_BUFFER_SIZE 15
#define TempSensor A3  //read Temp Analog pin

//for AX_protocol
int condition = 0;
void Ax_recieveFunc(void);
void AX_recieveframe_processing();
void ssp_protocol();
void Ax_Buffer_Gen();
void Ax_transmitFunc();
uint16_t CRC16K(unsigned char*, int);
void AX_Test_recieve() ;
byte AX_transmit_buffer[AxLength];
byte AX_recieve_buffer[AxLength];
int Ax_recieveFrame_length = 34;


// for SSP_protocol
typedef struct SSP_buff_gen_t {
  byte source;
  byte destination;
  byte type;
  byte data[4];
} SSP_buff_gen_t;


SSP_buff_gen_t SSP_frame_content;
int SSP_transmit_func_buffer_index = 0;
int SSP_end_frame;
int state = 0;
int ssp_length = 0;
int SSP_recieveFrame_length = 0;
int SSP_data_length = 0;
int SSP_data_exist = 0;
float TempRead = 0.0;
float voltage = 0.0;

byte SSP_transmit_func_buffer[SSP_BUFFER_SIZE];
byte SSP_recieve_buffer[SSP_BUFFER_SIZE];


void SSP_frame_processing(SSP_buff_gen_t* buff);
void SSP_Buffer_Gen(SSP_buff_gen_t* buff);
uint16_t CRC16K(unsigned char* data, int length) ;
void SSP_transmit_func(void);
void SSP_transmit2_func(void);
void SSP_recieve_func(void);
float TempTelm();
void fill_ssp();
void SSP_OBC_recieve();



/******************************************************************************
 * FUNCTION NAME : Setup
 * INPUT : void
 * RETURN : void
 ******************************************************************************/
void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial3.begin(9600);
  pinMode(TempSensor, INPUT);
}

/******************************************************************************
 * FUNCTION NAME : LOOP
 * INPUT : void
 * RETURN : void
 ******************************************************************************/
void loop() 
{
  switch (condition)
    {
    case 0:
      Ax_recieveFrame_length = Serial.available();
      if (Ax_recieveFrame_length == 34) 
      {
         Ax_recieveFunc();
      }
      break;

      case 1:
      AX_recieveframe_processing();
      break;

      case 2:
      Ax_Buffer_Gen();
      Ax_transmitFunc();
      break;
    }
}

/******************************************************************************
 * FUNCTION NAME : Ax_recieveFunc
 * INPUT : void
 * RETURN : void
 ******************************************************************************/
void Ax_recieveFunc(void) 
{
  condition = 1;
  Serial.readBytes(AX_recieve_buffer, AxLength);
}
/******************************************************************************
 * FUNCTION NAME :  Ax_transmitFunc
 * INPUT : void
 * RETURN : void
 ******************************************************************************/
void Ax_transmitFunc() 
{
  condition = 0;
  Serial.write(AX_transmit_buffer, AxLength);

}
/******************************************************************************
 * FUNCTION NAME : Buffer_Gen
 * INPUT : pointer to struct that hold the frame contents
 * RETURN : void
 ******************************************************************************/

void Ax_Buffer_Gen()
{
  AX_transmit_buffer[0] = StartEndFrame;
  AX_transmit_buffer[1] = E;
  AX_transmit_buffer[2] = U;
  AX_transmit_buffer[3] = T;
  AX_transmit_buffer[4] = S;
  AX_transmit_buffer[5] = A;
  AX_transmit_buffer[6] = T;
  AX_transmit_buffer[7] = zero;
  AX_transmit_buffer[8] = E;
  AX_transmit_buffer[9] = U;
  AX_transmit_buffer[10] = T;
  AX_transmit_buffer[11] = G;
  AX_transmit_buffer[12] = C;
  AX_transmit_buffer[13] = S;
  AX_transmit_buffer[14] = zero;
  AX_transmit_buffer[15] = ControlId;
 uint16_t crc = CRC16K(&AX_transmit_buffer[1], 30);
  AX_transmit_buffer[31] = (byte)(crc & 0xff);
  AX_transmit_buffer[32] = (byte)((crc >> 8) & 0xff);
  AX_transmit_buffer[33] = StartEndFrame;

}
/******************************************************************************
 * FUNCTION NAME : checksumCalculator 
 * USE : creating crc
 * INPUT : pointer to unsigned 8 bit integer & length of bytes in thr frame
 * RETURN : 2 bytes crc 
 ******************************************************************************/

uint16_t CRC16K(unsigned char* data, int length) 
{
  unsigned short crc = 0xffff;
  unsigned char *bufp = data;
  int len;
  int i;
  for (len = length; len > 0; len--) {
    unsigned char ch = *bufp++;
    for (i = 8; i > 0; i--) {
      crc = (crc >> 1) ^ (((ch ^ crc) & 0x01) ? POLY : 0);
      ch >>= 1;
    }
  }
  return crc;
}


/******************************************************************************
 * FUNCTION NAME :AX frame_processing
 * INPUT : pointer to struct that hold the frame contents
 * RETURN : void
 ******************************************************************************/
void AX_recieveframe_processing()
{
  condition = 2;
  uint16_t crc = CRC16K(&AX_recieve_buffer[1], (Ax_recieveFrame_length - 4));

  if (AX_recieve_buffer[(Ax_recieveFrame_length - 2)] == (byte)((crc >> 8) & 0xff)  
     && AX_recieve_buffer[(Ax_recieveFrame_length - 3)] == (byte)(crc & 0xff))

  {     
    //determine the actual length of SSP
 
  for (int i = 16; i < 31; i++) 
  {
    if (completeSspData == AX_recieve_buffer[i]) 
    {
      ssp_length = (i-1) - 15;  // to determine the actual end index of SSP
      break;            
    } 
  }
    ssp_protocol();
   }
}

/******************************************************************************
 * FUNCTION NAME : ssp_protocol
 * INPUT : void
 * RETURN : void
 ******************************************************************************/
void ssp_protocol()
{
    SSP_recieveFrame_length = ssp_length;

  if (SSP_recieveFrame_length >= 7 &&SSP_recieveFrame_length <=15 ) 
  {

    SSP_recieve_func();

    SSP_frame_processing(&SSP_frame_content);

  }
  else
  {
  // do no thing
  }

}

/******************************************************************************
 * FUNCTION NAME : SSP_recieve_func
 * INPUT : void
 * RETURN : void
 ******************************************************************************/
void SSP_recieve_func(void)
{
  int j = 0;
  for (int i = 16; i <= (SSP_recieveFrame_length+15); i++) 
  {
    SSP_recieve_buffer[j] = AX_recieve_buffer[i];
    j++;
  }
}
//////////////////////////////////////////////
 void SSP_OBC_recieve()
 {
   for(;;)
   {
   SSP_recieveFrame_length = Serial3.available();    //OBC serial
    if (SSP_recieveFrame_length >= 7 && SSP_recieveFrame_length <= 15)
    {
      break;
    }
   }
   Serial3.readBytes(SSP_recieve_buffer, SSP_recieveFrame_length);
 }

//////////////////////////////////////////////

/******************************************************************************
 * FUNCTION NAME : SSP_frame_processing
 * INPUT : integer variable contain the length of recieved frame
 * RETURN : void
 ******************************************************************************/
void SSP_frame_processing(SSP_buff_gen_t* buff)
{
  int flag1 = 0;
  int flag2 = 0;
  // check crc 
  uint16_t crc = CRC16K(&SSP_recieve_buffer[1], (SSP_recieveFrame_length - 4)); //make it 3 if error

  if (SSP_recieve_buffer[(SSP_recieveFrame_length - 2)] == (byte)((crc >> 8) & 0xff)
      && SSP_recieve_buffer[(SSP_recieveFrame_length - 3)] == (byte)(crc & 0xff))   
  { 
    if(SSP_recieve_buffer[1]== OBC_ID)
{
  // transmit to OBC_ID

    SSP_transmit3_func();
    SSP_OBC_recieve();
    SSP_end_frame = SSP_recieveFrame_length;
    SSP_transmit_func();

}
else if(SSP_recieve_buffer[1]== COMM_ID)
{
      if (SSP_recieveFrame_length > 7) 
    {
      switch (SSP_recieve_buffer[3])
     {
      case SET_OBC_TIME:
          buff->type = ACK;
          buff->source = COMM_ID;
          buff->destination = GROUND;
           SSP_data_exist = 1;
          SSP_data_length = (SSP_recieveFrame_length - 7);
          for (int i = 4; i < (SSP_data_length + 4); i++) 
          {
            if (SSP_recieve_buffer[i] == FESC && SSP_recieve_buffer[i + 1] == TFEND)
             {
              buff->data[i - 4] = FEND;
              flag2++;
            }
             else if (SSP_recieve_buffer[i] == FESC && SSP_recieve_buffer[i + 1] == TFESC)
              {
              buff->data[i - 4] = FESC;
              flag2++;
            } 
            else {
              if(SSP_recieve_buffer[i] == TFEND || SSP_recieve_buffer[i] == TFESC)
              {
                //Do Nothing                            // this condition need specific case
              }
              else
              {
                if(flag2>0)
                {
                buff->data[i-5]=SSP_recieve_buffer[i];
                }
                else
                {
                  buff->data[i-4]=SSP_recieve_buffer[i];
                }
              }
            }
          }

          SSP_data_length = (SSP_data_length + 4)-flag2;
          state = 2; //Mean that Send on Serial1,Connected to ground 
              SSP_Buffer_Gen(&SSP_frame_content);
              SSP_transmit_func(); 
      break;
     }
    }
    else  //if there is no data
    {
      switch (SSP_recieve_buffer[3])
      {
        case PING:
            buff->type = ACK;
            buff->source = COMM_ID;
            buff->destination = SSP_recieve_buffer[2];
            SSP_data_exist = 0;
            SSP_data_length = 4;
            state = 2;
            SSP_Buffer_Gen(&SSP_frame_content);
            SSP_transmit_func(); 
        break;
        case GET_TELM:
            buff->type = ACK;
            buff->source = COMM_ID;
            buff->destination = SSP_recieve_buffer[2];
            float TempRead = TempTelm();  // call the telemetry fun
            SSP_data_exist = 1;
            SSP_data_length = 5;
            buff->data[0] = 0x55;//TempRead;
            state = 2;
            SSP_Buffer_Gen(&SSP_frame_content);
            SSP_transmit_func(); 
        break;
      }        
    }
}
  }
 }


/******************************************************************************
 * FUNCTION NAME : TempTelm [Temperature Telemetry ]
 * INPUT : void
 * RETURN : TempRead
 ******************************************************************************/
float TempTelm() {
  float TempRead = 0.0;
  float voltage = 0.0;
  // for type Set telemetry
  TempRead = analogRead(TempSensor);
  // converting that reading to voltage, for 3.3v arduino use 3.3
  voltage = TempRead * 5.0;
  voltage /= 1024.0;

  //converting from 10 mv per degree with 500 mV offset
  //to degrees ((voltage - 500mV) times 100)

  TempRead = (voltage - 0.5) * 100;
  return TempRead;
}

/******************************************************************************
 * FUNCTION NAME : Buffer_Gen
 * INPUT : pointer to struct that hold the frame contents
 * RETURN : void
 ******************************************************************************/
void SSP_Buffer_Gen(SSP_buff_gen_t* buff) 
  {
    int flag = 0;
    int flag_noData = 0;
    SSP_transmit_func_buffer[0] = FEND;  // start flag
    SSP_transmit_func_buffer[1] = buff->destination;
    SSP_transmit_func_buffer[2] = buff->source;
    SSP_transmit_func_buffer[3] = buff->type;

    /* check the value of data */
    if (SSP_data_exist == 1)
    {
      for (int i = 4; i < SSP_data_length; i++)
      {
        if (buff->data[i - 4] == FEND)
        {
            if (flag == 1)
            {
              SSP_transmit_func_buffer[i + 1] = FESC;
              SSP_transmit_func_buffer[i + 2] = TFEND;
              flag++;
            } 
              else if (flag == 2) 
              {
              SSP_transmit_func_buffer[i + 2] = FESC;
              SSP_transmit_func_buffer[i + 3] = TFEND;
              flag++;
            } 
              else if (flag == 3) 
              {
                SSP_transmit_func_buffer[i + 3] = FESC;
                SSP_transmit_func_buffer[i + 4] = TFEND;
                flag++;
              }
            else
              {
                SSP_transmit_func_buffer[i] = FESC;
                SSP_transmit_func_buffer[i + 1] = TFEND;
                flag++;
              }
          }
        else if (buff->data[i - 4] == FESC)
            {
              if (flag == 1)
              {
                SSP_transmit_func_buffer[i + 1] = FESC;
                SSP_transmit_func_buffer[i + 2] = TFESC;
                flag++;
              } 
              else if (flag == 2) 
              {
                SSP_transmit_func_buffer[i + 2] = FESC;
                SSP_transmit_func_buffer[i + 3] = TFESC;
                flag++;
              } 
              else if (flag == 3) 
              {
                SSP_transmit_func_buffer[i + 3] = FESC;
                SSP_transmit_func_buffer[i + 4] = TFESC;
                flag++;
              } 
              else 
              {
                SSP_transmit_func_buffer[i] = FESC;
                SSP_transmit_func_buffer[i + 1] = TFESC;
                flag++;
              }
          } 
          else
          {
              if (flag == 1)
              {
                SSP_transmit_func_buffer[i + 1] = buff->data[i - 4];
              } 
              else if (flag == 2) 
              {
                SSP_transmit_func_buffer[i + 2] = buff->data[i - 4];
              } 
              else if (flag == 3) 
              {
                SSP_transmit_func_buffer[i + 3] = buff->data[i - 4];
              } 
              else 
              {
                SSP_transmit_func_buffer[i] = buff->data[i - 4];
              }
            }
        }
    }
  // create crc 
    uint16_t crc = CRC16K(&SSP_transmit_func_buffer[1], (flag + SSP_data_length-1));
    SSP_transmit_func_buffer[flag + SSP_data_length+1] = (byte)((crc >> 8) & 0xff);
    SSP_transmit_func_buffer[flag + (SSP_data_length)] = (byte)(crc & 0xff);
    SSP_transmit_func_buffer[flag + (SSP_data_length + 2)] = FEND;  //end flag 
    SSP_end_frame = (flag + (SSP_data_length + 3));   // determine the end of frame 
  }

/************************************************************************************
 * FUNCTION NAME : fill_ssp
 *USE : to make sure that ssp frame will be completed 15 bytes by fill it with 0xAA 
 * INPUT : void
 * RETURN : void
 ************************************************************************************/
void fill_ssp() 
{
  for (int i = SSP_end_frame; i <= SSP_BUFFER_SIZE; i++) // to add the A in ssp
  {
    SSP_transmit_func_buffer[i] = completeSspData;
  }
}
/******************************************************************************
 * FUNCTION NAME : SSP_transmit_func
 * INPUT : void
 * RETURN : void
 ******************************************************************************/
void SSP_transmit_func(void) 
{
  fill_ssp();
  int j = 0;
  for (int i = 16; i <= 30; i++)
   {
    AX_transmit_buffer[i] = SSP_transmit_func_buffer[j];
      j++;
    }      
}
/******************************************************************************
 * FUNCTION NAME : transmit2
 * INPUT : void
 * RETURN : void
 ******************************************************************************/
void SSP_transmit3_func(void) 
{
  Serial3.write(SSP_recieve_buffer,ssp_length);
}