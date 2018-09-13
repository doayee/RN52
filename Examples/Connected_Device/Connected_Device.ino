
/*  
  
  Meta Data from RN52 - example for RN52 library
 
  This example is free; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This example is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details. 

  Written by Thomas McQueen for http://doayee.co.uk

  See http://doayee.co.uk/bal/library for more details.
  
 */

#include <RN52.h>
#define NUMBERMACENTRIES 3

String macTable[NUMBERMACENTRIES][2] = {
  {"58E28F699B0F","Tom's iPhone"},
  {"C0EEFB5079F0","Thom's OnePlus2"},
  {"60E3AC0E2E15","Jacob's Phone"}

};


RN52 rn52(10,11);  //set RX to pin 10 and TX to pin 11 on Arduino (other way round on RN52)

void setup() {
  rn52.begin(38400);   //begin communication at a baud rate of 38400
  Serial.begin(9600);  //begin Serial communication with computer at a baud rate of 9600
}

String MACtoFriendly(String _mac){
  
  if(_mac.equals("NotConnected")){
    return "Not Connected";
  }

  for(int i=0; i<NUMBERMACENTRIES; i++){
    //Serial.println(macTable[i][0]);
     if(_mac.equals(macTable[i][0])){
      return macTable[i][1];  
     }
  }
  return "Unknown Device... SAD";
 
}


void loop() {
  Serial.println("RN52 MUST BE PAIRED TO A DEVICE FOR THIS SKETCH TO OUTPUT ANYTHING USEFUL");
  Serial.println("-------------------Once connected send a 'y' character-------------------");
  Serial.println();
  Serial.println();
  while(Serial.available() == 0);           //wait while there is no data from computer
  if(Serial.read() == 'y')
  {
    while(1) {
      Serial.print("Connected MAC: ");
      Serial.println(rn52.connectedMAC());
      Serial.print("Connected device: ");
      Serial.println(MACtoFriendly(rn52.connectedMAC()));
      delay(5000);
    }
  }
}
