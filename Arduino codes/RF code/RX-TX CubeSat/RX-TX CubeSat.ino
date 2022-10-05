#include <VirtualWire.h>
// Receiver.
bool flag =1;
void setup()
{
 Serial.begin(9600);
  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);
  
  pinMode(2, OUTPUT); //when HIGH =>> RX
  pinMode(4, OUTPUT); //when LOW =>>  RX
  
 Serial.println("setup");
 vw_setup(3000); // Bits per sec
 vw_rx_start();  // Start the receiver PLL running
}
void loop()
{
  
  if(flag==1){

    //RX
    digitalWrite(8, LOW);
    digitalWrite(4, HIGH);
    digitalWrite(2, LOW);
  }
  else if(flag==0){
    //TX
    digitalWrite(8, HIGH);
    digitalWrite(2, HIGH);
    digitalWrite(4, LOW);
  }
  
 uint8_t buffer[VW_MAX_MESSAGE_LEN];
 uint8_t buffer_length = VW_MAX_MESSAGE_LEN;
 if (vw_get_message(buffer, &buffer_length)) // Non-blocking
 {
int i;
// Message with a good checksum received, dump HEX
Serial.print("Data Received: ");
for (i = 0; i < buffer_length; i++)
{
 Serial.print(buffer[i], HEX);
 Serial.print(" ");
}
Serial.println("");

 }
 delay(150);
 flag =!flag;
 
}


