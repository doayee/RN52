/*  
  
  Meta Data from RN52 with LCD - an example for RN52 library
 
  This example is free; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This example is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details. 

  Written by Thomas Cousins, Thomas McQueen and Jacob Rawson for 
  https://doayee.co.uk

  See https://doayee.co.uk/bal/library for more details.
  
 */
 
/*************************************************************************************
 * Includes
 ************************************************************************************/
#include <RN52.h>
#include "LiquidCrystal.h"

/*************************************************************************************
 * Definitions
 ************************************************************************************/
#define NUMBERMACENTRIES 3          //number of MAC addresses in the table under Globals
#define MODE_RATE 10000             //rate at which the lower display toggles in ms
#define UPDATE_RATE 1000            //rate at which the sketch checks for a track change/new device and pulls data
#define REFRESH_RATE 200            //rate at which the screen updates (for scrolling)
#define SCROLL_WAIT 2000            //rate at which a string stays still before it starts scrolling
#define _SS_MAX_RX_BUFF 512
#define LOCATION "  Set Location"    //location to display on splash screen while disconnected

/*************************************************************************************
 * Types
 ************************************************************************************/
typedef struct scroll_t {
  bool padded;
  String raw;
  int index;
} scroll_t;

/*************************************************************************************
 * Function Prototypes
 ************************************************************************************/
String getStringToDisplay(scroll_t *s);

/*************************************************************************************
 * Object Definitions
 ************************************************************************************/
LiquidCrystal lcd(7,6,5,4,3,2);
RN52 rn52(10,11);  //set RX to pin 10 and TX to pin 11 on Arduino (other way round on RN52)

/*************************************************************************************
 * Globals
 ************************************************************************************/

/* fill in a 12 digit MAC alongside a friendly name */
String macTable[NUMBERMACENTRIES][2] = {
  {"58E28F699B0F","Tom's iPhone"},
  {"C0EEFB5079F0","Thom's OnePlus2"},
  {"60E3AC0E2E15","Jacob's Phone"}
};

/* global state indicators */
bool deviceConnected = false;
bool newConnection = false;
String bluetoothName="";

/* determines which information is displayed on the second line of the display */
int mode = 0;

/* counters to cue events */
unsigned long modeMillisCompare = 0;
unsigned long updateMillisCompare= 0;
unsigned long refreshMillisCompare = 0;
unsigned long scrollwaitMillisCompare = 0;
unsigned long titleScrollwaitMillisCompare = 0;

/* !!! */
/* TODO: make acquiring this info quicker */

/* holds all the data which will be displayed */
scroll_t title;
scroll_t album;
scroll_t artist;
scroll_t trackCount;
scroll_t connectedDevice;

/*******************************************************************************
 * Functions                                                              
 ******************************************************************************/

/*******************************************************************************
 * Name:   void setup()                                                       
 * Inputs: None                                                                 
 * Return: None                                       
 * Notes:  Runs once                              
 ******************************************************************************/
void setup() {
  rn52.begin(38400);   //begin communication at a baud rate of 38400
  Serial.begin(9600);  //begin Serial communication with computer at a baud rate of 9600

  lcd.begin(16,2);     //begin the LCD interface
  lcd.clear();         

  /* wait for the RN52 to turn on and decide whether it wants to reconnect */
  /* these delays exist purely so the RN52 firmware doesnt freak out, we wish they didnt need to be there too */
  lcd.setCursor(0,0);
  lcd.print(make_str("BAL by Doayee"));
  lcd.setCursor(0,1);
  lcd.print(make_str("Booting"));
  delay(1000);

  lcd.setCursor(0,1);
  lcd.print(make_str("Booting."));
  delay(1000);
  
  lcd.setCursor(0,1);
  lcd.print(make_str("Booting.."));
  delay(1000);

  lcd.setCursor(0,1);
  lcd.print(make_str("Booting..."));
  
  bluetoothName = rn52.name();
  delay(1000);       
  
  
  /* either establish that no device connected or pull some data */
  UpdateData();
  
}
  
/*******************************************************************************
 * Name:   void loop()                                                       
 * Inputs: None                                                                 
 * Return: None                                       
 * Notes:  Runs repeatedly.                               
 ******************************************************************************/
void loop() 
{    
  /* If it is time to update the data */
  if (millis()-updateMillisCompare > UPDATE_RATE)
  {
    UpdateData();
    updateMillisCompare=millis();
  }

  /* If UpdateData() said we are not connected and it is time to refresh the screen */
  if (!deviceConnected && (millis()-refreshMillisCompare > REFRESH_RATE))
  {
    updateScreenDisconnected();
    refreshMillisCompare = millis();
  }
  
  /* If we are connected and it is time to refresh the screen */
  else if (deviceConnected && millis()-refreshMillisCompare > REFRESH_RATE)
  {
    updateScreen();
    refreshMillisCompare = millis();
  }
  
  /* If it is time to change the mode */
  if (millis()-modeMillisCompare > MODE_RATE)
  {
    /* Wrap the mode */
    if (++mode > 3) mode=0;
    
    /* Reset all the indexes for the scrolling */
    album.index = 0;
    artist.index = 0;
    trackCount.index = 0;
    connectedDevice.index = 0;
    
    modeMillisCompare=millis();
    scrollwaitMillisCompare=millis();
  }
}

/*******************************************************************************
 * Name:   void updateScreen(void)                                                       
 * Inputs: None                                                                 
 * Return: None                                             
 * Notes:  Updates the screen with info from the global scroll_t types.                            
 ******************************************************************************/
void updateScreen() {
  
  /* Get the title to display */
  String titleToPrint = getStringToDisplay(&title);
 
  /* Display it */
  lcd.setCursor(0,0);
  lcd.print(make_str(titleToPrint));
  
  /* Move the cursor */
  lcd.setCursor(0,1);
  
  /* Print the necessary info */
  switch(mode)
  {
    case 0:
      lcd.print(make_str(getStringToDisplay(&artist)));
      break;
    case 1: 
      lcd.print(make_str(getStringToDisplay(&album)));
      break;
    case 2: 
      lcd.print(make_str(getStringToDisplay(&trackCount)));
      break;
    case 3:
      lcd.print(make_str(getStringToDisplay(&connectedDevice)));
    default:
      break;
  }
}

/*******************************************************************************
 * Name:   void updateScreenDisconnected(void)                                                       
 * Inputs: None                                                                 
 * Return: None                                             
 * Notes:  Updates the screen with the disconnected display.                            
 ******************************************************************************/
void updateScreenDisconnected() {

  lcd.setCursor(0,0);
 
  switch(mode)
  {
    case 0:
      lcd.print(make_str("   Welcome to"));
      lcd.setCursor(0,1);
      lcd.print(make_str(LOCATION));
      break;
    case 1: 
      lcd.print(make_str("   Connect to"));
      lcd.setCursor(0,1);
      lcd.print(make_str(bluetoothName));
      break;
    case 2: 
      lcd.print(make_str("   Welcome to"));
      lcd.setCursor(0,1);
      lcd.print(make_str(LOCATION));
      break;
    case 3:
      lcd.print(make_str("   Connect to"));
      lcd.setCursor(0,1);
      lcd.print(make_str(bluetoothName));
    default:
      break;
  }
}


/*******************************************************************************
 * Name:   String make_str(String str)                                                       
 * Inputs: String str - the raw string to format                                                                 
 * Return: String - formatted to fit on the 16 character screen.                                             
 * Notes:  Pads the input string to 16 chars if less than 16, and cuts to 16.            
 ******************************************************************************/
String make_str(String str)
{ 
  /* If the string is greater than 16 chars */
  if (str.length() >= 16)
  {
    /* Cut it to 16 */
    str.remove(16);
    
    return str;
  }
  
  String toAppend = "";

  /* Build a string of spaces to pad the input string to 16 chars */
  for(int i = 16; i > str.length(); i--) {
      toAppend += " ";  
  }

  str += toAppend;
  
  return str;
}

/*******************************************************************************
 * Name:   String MACtoFriendly(String _mac)                                                     
 * Inputs: String _mac - the raw MAC to convert                                                                 
 * Return: String - The friendly name converted from the MAC                                          
 * Notes:  Looks up the friendly name for an input MAC address, and returns it            
 ******************************************************************************/
String MACtoFriendly(String _mac)
{
  /* Self explanatory */
  if(_mac.equals("NotConnected")) return "Not Connected";

  else if(_mac.equals("Invalid MAC Address")) return _mac;

  /* Look through the database and return the name paired with it */
  for(int i=0; i<NUMBERMACENTRIES; i++)
     if(_mac.equals(macTable[i][0]))
      return macTable[i][1];  
  
  /* Couldn't find it */
  return "Unknown Device... SAD";
 
}

/*******************************************************************************
 * Name:   String getStringToDisplay(scroll_t *s)                                                   
 * Inputs: scroll_t *s - pointer to the scroll_t with the data needed                                                                 
 * Return: String - The string to display on the screen                                          
 * Notes:  Handles the scrolling of the strings to be displayed. Every time 
 *         this function is called it gives back the "next" scrolled string.  
 ******************************************************************************/
String getStringToDisplay(scroll_t *s)
{
  if(s == NULL) return "ERROR IN STRING";
  
  /* if the string does not require scrolling */
  if(s->raw.length() <= 16) return s->raw;

  /* if it is not yet time to scroll, return unmodified */
  if(millis()-titleScrollwaitMillisCompare < SCROLL_WAIT && s->index==0 && (s == &title)) return s->raw;
  else if(millis()-scrollwaitMillisCompare < SCROLL_WAIT && s->index==0) return s->raw;
  
  /* if we made it this far and we're looking at the title object, reset its wait timer */
  if(s == &title) titleScrollwaitMillisCompare = millis();
  else scrollwaitMillisCompare = millis();

  /* If the string needs padding */
  if(!s->padded)
  {
    String pad = "     ";
    s->raw += pad;
    s->padded = true;
  }
    
  /* get the index of the scroll, and update the index for next iteration */
  int localIndex = s->index++ % s->raw.length();
  
  /*if the local index is zero, check that the real index isn't 1 (ie. first scroll) and reset the index so we do a scroll wait */
  if(localIndex == 0 && s->index != 1) s->index = 0;
  
  /* Create local copy of string to return */
  String toReturn = s->raw;

  /* Remove up to the index value */
  toReturn.remove(0, localIndex);

  /* Add 16 to find the end of the snip */
  localIndex+=16;

  /* If there is still text after the snip */
  if(s->raw.length() > localIndex)
  	toReturn.remove(localIndex);
  else
  {
    /* Fetches data from the start of the string and adds it to the end */
    int toFetch = localIndex - s->raw.length();
    String toAppend = s->raw;
    toAppend.remove(toFetch);
    toReturn += toAppend;
  }

  return toReturn;

}

/*******************************************************************************
 * Name:   String getStringToDisplay(scroll_t *s)                                                   
 * Inputs: scroll_t *s - pointer to the scroll_t with the data needed                                                                 
 * Return: String - The string to display on the screen                                          
 * Notes:  Handles the scrolling of the strings to be displayed. Every time 
 *         this function is called it gives back the "next" scrolled string.  
 ******************************************************************************/
void UpdateData(void)
{
  /* if theres nothing connected, update the global state and dont request any info*/
  if(!rn52.isConnected()) {
    deviceConnected = false; 
    return;
  }

  /*else there is something connected, but the global state says there isnt, then something just connnected*/
  else if(deviceConnected == false){
    deviceConnected = true;
    newConnection = true;

    /* connecting splash screen */
    /* these delays exist purely so the RN52 firmware doesnt freak out, we wish they didnt need to be there too */
    lcd.setCursor(0,0);
    lcd.print(make_str("Device Found!"));
    lcd.setCursor(0,1);
    lcd.print(make_str("Connecting"));
    delay(1000);
    
    lcd.setCursor(0,1);
    lcd.print(make_str("Connecting."));
    delay(1000);
    
    lcd.setCursor(0,1);
    lcd.print(make_str("Connecting.."));
    delay(1000);
    
    lcd.setCursor(0,1);
    lcd.print(make_str("Connecting..."));
    delay(1000);
  }

  /* if the track hasnt changed and the device hasnt just connected dont request any information */
  if(!rn52.trackChanged() && !newConnection) return;

  /* if you made it this far, update the track data */

  delay(1000); //wait at a track change to ensure new data in the rn52 makes sense

  String titleStr = rn52.trackTitle();
  String albumStr = rn52.album();
  String artistStr = rn52.artist();
  int trackCountLocal = rn52.trackCount();
  int trackNumLocal = rn52.trackNumber();
  
  /* Populate the structure */
  trackCount.index = 0;
  trackCount.raw = String(trackNumLocal)+" of "+String(trackCountLocal);
  trackCount.padded = false;
  
  /* Populate the structure */
  title.index = 0;
  title.raw = titleStr;
  title.padded = false;
  
  /* Populate the structure */
  album.index = 0;
  album.raw = albumStr;
  album.padded = false;
  
  /* Populate the structure */
  artist.index = 0;
  artist.raw = artistStr;
  artist.padded = false;
  
  /* Populate the structure */
  /* only performed once for a new connection */
  if(newConnection){
  connectedDevice.index = 0;
  connectedDevice.raw = MACtoFriendly(rn52.connectedMAC());
  connectedDevice.padded = false;
  }
  
  newConnection = false; //avoid pull first-run data every loop
}


