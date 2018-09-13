/*  
  
  Renaming the RN52 - example for RN52 library
 
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

RN52 rn52(10,11); //set RX to pin 10 and TX to pin 11 on Arduino (other way round on RN52)

void setup() {
  rn52.begin(38400);   //begin communication at a baud rate of 38400
  Serial.begin(9600);  //begin Serial communication with computer at a baud rate of 9600
}

void loop() {
  Serial.print("Current name: ");
  Serial.println(rn52.name());      //print the RN52 current name to computer
  Serial.println("Rename? (y/n)");
  while(Serial.available() == 0);   //wait while there is no data from computer
  if(Serial.read() == 'y') {
    Serial.print("Enter new name: ");
    String newName;
    while(Serial.available() == 0); //wait while there is no data from computer
    while(Serial.available() != 0)  //while there is data to be read
    {
      char a = Serial.read();       //read in character from computer to char 'a'
      newName += a;                 //append 'a' to String newName
      delay(50);                    //delay to allow more data from computer (if this were not here it might terminate the name early
    }
    rn52.name(newName, 0);          //rename RN52 to newName, not normalized (does not have last 4 digits of MAC address appended)
    Serial.println();
    Serial.println("Restarting rn52...");
    rn52.reboot();                  //reboot RN52
    Serial.println();
    Serial.print("New name: ");
    Serial.println(rn52.name());    //print the new name of the RN52 to computer
    Serial.println();
    while(1);                       //do nothing
  }
  else while(1);
}
