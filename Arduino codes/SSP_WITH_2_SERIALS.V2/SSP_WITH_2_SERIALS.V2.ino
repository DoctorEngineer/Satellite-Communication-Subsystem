#include <SoftwareSerial.h>
#define POLY          0x8408
#define COMM_ID       0x02
#define OBC_ID        0x04
#define PING          0x00
#define INIT          0x01
#define ACK           0x02
#define NACK          0x03
#define FEND          0xC0
#define FESC          0xDB
#define TFEND         0xDC
#define TFESC         0xDD
#define GET_TELM      0x04
#define SET_OBC_TIME  0x05
#define GROUND        0x15
#define BUFFER_SIZE   15
#define TempSensor    A3  //read Temp Analog pin

typedef struct buff_gen_t 
{
  byte source;
  byte destination;
  byte type;
  byte data[4];
} buff_gen_t;


buff_gen_t frame_content;

int buffer_index = 0;
int end_frame;
int state = 0;
int recieveFrame_length = 0;
int data_length = 0;
int data_exist = 0;
float TempRead = 0.0;
float voltage = 0.0;

byte ssp_buffer[BUFFER_SIZE];
byte recieve_buffer[BUFFER_SIZE];

void frame_processing(buff_gen_t* buff);
void Buffer_Gen(buff_gen_t* buff);
uint16_t CRC16K(unsigned char *data,int length);
void transmit1(void);
void transmit3(void);
void recieveFunc(void);
float TempTelm(void);

/******************************************************************************
 * FUNCTION NAME : Setup
 * INPUT : void
 * RETURN : void
 ******************************************************************************/
void setup() 
{
  Serial1.begin(9600);
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
  switch (state)
   {
    case 0:
   delay(100);
     recieveFrame_length = Serial1.available();    //ground serial
      if (recieveFrame_length >= 7 && recieveFrame_length <= 15)
      {
        recieveFunc();
      }
    break;

    case 1:
      frame_processing(&frame_content);
    break;

    case 2:
      Buffer_Gen(&frame_content);
      transmit1();
    break;
  }
}

/******************************************************************************
 * FUNCTION NAME : transmit1
 * INPUT : void
 * RETURN : void
 ******************************************************************************/
void transmit1(void) 
{
  Serial1.write(ssp_buffer,end_frame);
  state = 0;
}

/******************************************************************************
 * FUNCTION NAME : transmit3
 * INPUT : void
 * RETURN : void
 ******************************************************************************/
void transmit3(void) 
{
  Serial3.write(ssp_buffer,end_frame);
  //state = 0;
}

/******************************************************************************
 * FUNCTION NAME : recieveFunc
 * INPUT : integer variable contain the length of recieved frame
 * RETURN : void
 ******************************************************************************/
void recieveFunc(void) 
{
   Serial1.readBytes(recieve_buffer,recieveFrame_length);
   state = 1;
}

/******************************************************************************
 * FUNCTION NAME : TempTelm [Temperature Telemetry ]
 * INPUT : void
 * RETURN : TempRead
 ******************************************************************************/
float TempTelm()
 {
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
void Buffer_Gen(buff_gen_t* buff) 
  {
    int flag = 0;
    int flag_noData = 0;
    ssp_buffer[0] = FEND;  // start flag
    ssp_buffer[1] = buff->destination;
    ssp_buffer[2] = buff->source;
    ssp_buffer[3] = buff->type;

    /* check the value of data */
    if (data_exist == 1)
    {
      for (int i = 4; i < data_length; i++)
      {
        if (buff->data[i - 4] == FEND)
        {
            if (flag == 1)
            {
              ssp_buffer[i + 1] = FESC;
              ssp_buffer[i + 2] = TFEND;
              flag++;
            } 
              else if (flag == 2) 
              {
              ssp_buffer[i + 2] = FESC;
              ssp_buffer[i + 3] = TFEND;
              flag++;
            } 
              else if (flag == 3) 
              {
                ssp_buffer[i + 3] = FESC;
                ssp_buffer[i + 4] = TFEND;
                flag++;
              }
            else
              {
                ssp_buffer[i] = FESC;
                ssp_buffer[i + 1] = TFEND;
                flag++;
              }
          }
        else if (buff->data[i - 4] == FESC)
            {
              if (flag == 1)
              {
                ssp_buffer[i + 1] = FESC;
                ssp_buffer[i + 2] = TFESC;
                flag++;
              } 
              else if (flag == 2) 
              {
                ssp_buffer[i + 2] = FESC;
                ssp_buffer[i + 3] = TFESC;
                flag++;
              } 
              else if (flag == 3) 
              {
                ssp_buffer[i + 3] = FESC;
                ssp_buffer[i + 4] = TFESC;
                flag++;
              } 
              else 
              {
                ssp_buffer[i] = FESC;
                ssp_buffer[i + 1] = TFESC;
                flag++;
              }
          } 
          else
          {
              if (flag == 1)
              {
                ssp_buffer[i + 1] = buff->data[i - 4];
              } 
              else if (flag == 2) 
              {
                ssp_buffer[i + 2] = buff->data[i - 4];
              } 
              else if (flag == 3) 
              {
                ssp_buffer[i + 3] = buff->data[i - 4];
              } 
              else 
              {
                ssp_buffer[i] = buff->data[i - 4];
              }
            }
        }
    }
  // create crc 
    uint16_t crc = CRC16K(&ssp_buffer[1], (flag + data_length-1));
  //Serial.println((flag + data_length-1));
    ssp_buffer[flag + data_length+1] = (byte)((crc >> 8) & 0xff);
    ssp_buffer[flag + (data_length)] = (byte)(crc & 0xff);
    //end flag 
    ssp_buffer[flag + (data_length + 2)] = FEND;
    /* determine the end of frame */
    end_frame = (flag + (data_length + 3));
  }
  
/******************************************************************************
 * FUNCTION NAME : CRC16K
 * INPUT : pointer to unsigned 8 bit char & length of bytes in thr frame
 * RETURN : 2 bytes crc 
 ******************************************************************************/
uint16_t CRC16K(unsigned char *data,int length) 
{
    unsigned short crc = 0xffff;
unsigned char *bufp = data;
int len;
int i;
for (len = length; len > 0; len--) {
unsigned char ch = *bufp++;
for (i = 8; i > 0; i--) {
crc = (crc >> 1) ^ ( ((ch ^ crc) & 0x01) ? POLY : 0 );
ch >>= 1;
}
}
return crc;
}

/******************************************************************************
 * FUNCTION NAME : frame_processing
 * INPUT : pointer to struct that hold the frame contents
 * RETURN : void
 ******************************************************************************/
void frame_processing(buff_gen_t* buff)
{
  int flag1 = 0;
  int flag2 = 0;
  /* check crc */
  uint16_t crc = CRC16K(&recieve_buffer[1], (recieveFrame_length - 4));
  if (recieve_buffer[(recieveFrame_length - 2)] == (byte)((crc >> 8) & 0xff)
      && recieve_buffer[(recieveFrame_length - 3)] == (byte)(crc & 0xff))
   {
    /* check if data exist */
    // process TYPE and data
    //IF there is data
    if (recieveFrame_length > 7) 
    {
      switch (recieve_buffer[3])
     {
      case SET_OBC_TIME:
          // first send to OBC
            buff->type = SET_OBC_TIME;
            buff->source = COMM_ID;
            buff->destination = OBC_ID; 
          data_exist = 1;
          data_length = (recieveFrame_length - 7);
          for (int i = 4; i < (data_length + 4); i++) 
          {
            if (recieve_buffer[i] == FESC && recieve_buffer[i + 1] == TFEND)
             {
              buff->data[i - 4] = FEND;
              flag1++;
            }
             else if (recieve_buffer[i] == FESC && recieve_buffer[i + 1] == TFESC)
              {
              buff->data[i - 4] = FESC;
              flag1++;
            } 
            else 
            {
              if(recieve_buffer[i] == TFEND || recieve_buffer[i] == TFESC)
              {
                //Do Nothing                            // this condition need specific case
              }
              else
              {
                if(flag1>0)
                {
                buff->data[i-5]=recieve_buffer[i];
                }
                else
                {
                  buff->data[i-4]=recieve_buffer[i];
                }
              }
            }
          }
          data_length = (data_length + 4) - flag1;
          Buffer_Gen(&frame_content);
          transmit3();  // OBC Connected to Serial2
          
          //second send the frame to Ground 
          buff->type = SET_OBC_TIME;
          buff->source = COMM_ID;
          buff->destination = GROUND;
           data_exist = 1;
          data_length = (recieveFrame_length - 7);
          for (int i = 4; i < (data_length + 4); i++) 
          {
            if (recieve_buffer[i] == FESC && recieve_buffer[i + 1] == TFEND)
             {
              buff->data[i - 4] = FEND;
              flag2++;
            }
             else if (recieve_buffer[i] == FESC && recieve_buffer[i + 1] == TFESC)
              {
              buff->data[i - 4] = FESC;
              flag2++;
            } 
            else {
              if(recieve_buffer[i] == TFEND || recieve_buffer[i] == TFESC)
              {
                //Do Nothing                            // this condition need specific case
              }
              else
              {
                if(flag2>0)
                {
                buff->data[i-5]=recieve_buffer[i];
                }
                else
                {
                  buff->data[i-4]=recieve_buffer[i];
                }
              }
            }
          }

          data_length = (data_length + 4)-flag2;
          state = 2; //Mean that Send on Serial1,Connected to ground 
      break;
     }
    }
    else  //if there is no data
    {
      switch (recieve_buffer[3])
      {
        case PING:
             switch(recieve_buffer[1])
             {
              case COMM_ID:
                   buff->type = ACK;
                   buff->source = COMM_ID;
                   buff->destination = GROUND;
                   data_exist = 0;
                   data_length = 4;
                   state = 2;
                   break;
              case OBC_ID:
                   buff->type = PING;
                   buff->source = GROUND;
                   buff->destination = OBC_ID;
                   data_exist = 0;
                   data_length = 4;
                   Buffer_Gen(&frame_content);
                   transmit3();  // OBC Connected to Serial2
                   while(!(Serial3.available()==7));              //OBC serial
                   Serial3.readBytes(recieve_buffer,7);              
                   buff->type =recieve_buffer[3] ;
                   buff->source =recieve_buffer[2];
                   buff->destination = recieve_buffer[1];
                   data_exist = 0;
                   data_length = 4;
                   state =2;
                   break; 
             }
            
        break;
        case GET_TELM:
            buff->type = ACK;
            buff->source = COMM_ID;
            buff->destination = recieve_buffer[2];
            float TempRead = TempTelm();  // call the telemetry fun
            data_exist = 1;
            data_length = 5;
            buff->data[0] = 0x55;//TempRead;
            state = 2;
        break;
      }        
    }
  }
 }
