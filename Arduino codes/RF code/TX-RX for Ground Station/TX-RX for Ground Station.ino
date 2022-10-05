
#include <VirtualWire.h>
// Transmitter
bool flag =1;
void setup()
{
  Serial.begin(9600);
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);
 
  pinMode(2, OUTPUT); //when HIGH =>> TX
  pinMode(4, OUTPUT); //when LOW =>>  TX
 
 vw_setup(2000); // Bits per sec
}
void loop()
{
  
  if(flag==1){
//TX    
    digitalWrite(8, HIGH);
    digitalWrite(2, LOW);
    digitalWrite(4, HIGH);
  }
  else if(flag==0){
    //RX
    digitalWrite(8, LOW);
    digitalWrite(4, LOW);
    digitalWrite(2, HIGH);
  }
  
 const char *sent_data = "hello hello";
 vw_send((uint8_t *)sent_data, strlen(sent_data));
 Serial.print("Data Sent: ");
 Serial.println(sent_data);
 delay(150);
 flag =!flag;
}
