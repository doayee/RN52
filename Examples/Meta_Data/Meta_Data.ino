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

  Written by Thomas Cousins for http://doayee.co.uk

  See http://doayee.co.uk/bal/library for more details.
  
 */

#include <RN52.h>

RN52 rn52(10,11);  //set RX to pin 10 and TX to pin 11 on Arduino (other way round on RN52)

void setup() {
  rn52.begin(38400);   //begin communication at a baud rate of 38400
  Serial.begin(9600);  //begin Serial communication with computer at a baud rate of 9600
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
      Serial.print("Track title: ");
      Serial.println(rn52.trackTitle());    //print track title from RN52
      Serial.print("Album: ");
      Serial.println(rn52.album());         //print album from RN52
      Serial.print("Artist: ");
      Serial.println(rn52.artist());        //print artist from RN52
      Serial.println("Full meta data: ");
      Serial.println("BEGIN");
      Serial.print(rn52.getMetaData());     //print Meta Data from RN52
      Serial.println("END");
      Serial.println();
      Serial.println();
      delay(1000);
    }
  }
}
