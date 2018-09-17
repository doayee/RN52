/*
	RN52.h
	Adapted from the SoftwareSerial library
	With added functionality to interface with the RN52.
	SoftwareSerial library by ladyada, Mikal Hart, Paul Stoffregen, Garrett Mace, and Brett Hagman.
	RN52 changed by Thomas Cousins and Thomas McQueen for https://doayee.co.uk

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// When set, _DEBUG co-opts pins 11 and 13 for debugging with an
// oscilloscope or logic analyzer.  Beware: it also slightly modifies
// the bit times, so don't rely on it too much at high baud rates
#define _DEBUG 0
#define _DEBUG_PIN1 11
#define _DEBUG_PIN2 13
//
// Includes
//
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <Arduino.h>
#include <RN52.h>
#include <util/delay_basic.h>

//
// Statics
//
RN52 *RN52::active_object = 0;
char RN52::_receive_buffer[_SS_MAX_RX_BUFF];
volatile uint8_t RN52::_receive_buffer_tail = 0;
volatile uint8_t RN52::_receive_buffer_head = 0;
short RN52::IOMask = 0x0004;
short RN52::IOProtect = 0x3C64;
short RN52::IOStateMask = 0x0094;
short RN52::IOStateProtect = 0x3CF4;
short RN52::IO;
short RN52::IOState;
volatile bool  RN52::_trackChanged = false;

//
// Debugging
//
// This function generates a brief pulse
// for debugging or measuring on an oscilloscope.
inline void DebugPulse(uint8_t pin, uint8_t count)
{
#if _DEBUG
  volatile uint8_t *pport = portOutputRegister(digitalPinToPort(pin));

  uint8_t val = *pport;
  while (count--)
  {
    *pport = val | digitalPinToBitMask(pin);
    *pport = val;
  }
#endif
}

//
// Private methods
//

/* static */
inline void RN52::tunedDelay(uint16_t delay) {
  _delay_loop_2(delay);
}

// This function sets the current object as the "listening"
// one and returns true if it replaces another
bool RN52::listen()
{
  if (!_rx_delay_stopbit)
    return false;

  if (active_object != this)
  {
    if (active_object)
      active_object->stopListening();

    _buffer_overflow = false;
    _receive_buffer_head = _receive_buffer_tail = 0;
    active_object = this;

    setRxIntMsk(true);
    return true;
  }

  return false;
}

// Stop listening. Returns true if we were actually listening.
bool RN52::stopListening()
{
  if (active_object == this)
  {
    setRxIntMsk(false);
    active_object = NULL;
    return true;
  }
  return false;
}

//
// The receive routine called by the interrupt handler
//
void RN52::recv()
{

#if GCC_VERSION < 40302
// Work-around for avr-gcc 4.3.0 OSX version bug
// Preserve the registers that the compiler misses
// (courtesy of Arduino forum user *etracer*)
  asm volatile(
    "push r18 \n\t"
    "push r19 \n\t"
    "push r20 \n\t"
    "push r21 \n\t"
    "push r22 \n\t"
    "push r23 \n\t"
    "push r26 \n\t"
    "push r27 \n\t"
    ::);
#endif

  uint8_t d = 0;

  // If RX line is high, then we don't see any start bit
  // so interrupt is probably not for us
  if (_inverse_logic ? rx_pin_read() : !rx_pin_read())
  {
    // Disable further interrupts during reception, this prevents
    // triggering another interrupt directly after we return, which can
    // cause problems at higher baudrates.
    setRxIntMsk(false);

    // Wait approximately 1/2 of a bit width to "center" the sample
    tunedDelay(_rx_delay_centering);
    DebugPulse(_DEBUG_PIN2, 1);

    // Read each of the 8 bits
    for (uint8_t i=8; i > 0; --i)
    {
      tunedDelay(_rx_delay_intrabit);
      d >>= 1;
      DebugPulse(_DEBUG_PIN2, 1);
      if (rx_pin_read())
        d |= 0x80;
    }

    if (_inverse_logic)
      d = ~d;

    // if buffer full, set the overflow flag and return
    uint8_t next = (_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF;
    if (next != _receive_buffer_head)
    {
      // save new data in buffer: tail points to where byte goes
      _receive_buffer[_receive_buffer_tail] = d; // save new byte
      _receive_buffer_tail = next;
    }
    else
    {
      DebugPulse(_DEBUG_PIN1, 1);
      _buffer_overflow = true;
    }

    // skip the stop bit
    tunedDelay(_rx_delay_stopbit);
    DebugPulse(_DEBUG_PIN1, 1);

    // Re-enable interrupts when we're sure to be inside the stop bit
    setRxIntMsk(true);

  }

#if GCC_VERSION < 40302
// Work-around for avr-gcc 4.3.0 OSX version bug
// Restore the registers that the compiler misses
  asm volatile(
    "pop r27 \n\t"
    "pop r26 \n\t"
    "pop r23 \n\t"
    "pop r22 \n\t"
    "pop r21 \n\t"
    "pop r20 \n\t"
    "pop r19 \n\t"
    "pop r18 \n\t"
    ::);
#endif
}

uint8_t RN52::rx_pin_read()
{
  return *_receivePortRegister & _receiveBitMask;
}

//
// Interrupt handling
//

/* static */
inline void RN52::handle_interrupt()
{
  if (active_object)
  {
    active_object->recv();
  }
}

#if defined(PCINT0_vect)
ISR(PCINT0_vect)
{
  RN52::handle_interrupt();
}
#endif

#if defined(PCINT1_vect)
ISR(PCINT1_vect, ISR_ALIASOF(PCINT0_vect));
#endif

#if defined(PCINT2_vect)
ISR(PCINT2_vect, ISR_ALIASOF(PCINT0_vect));
#endif

#if defined(PCINT3_vect)
ISR(PCINT3_vect, ISR_ALIASOF(PCINT0_vect));
#endif

//
// Constructor
//
RN52::RN52(uint8_t receivePin, uint8_t transmitPin, bool inverse_logic /* = false */) :
  _rx_delay_centering(0),
  _rx_delay_intrabit(0),
  _rx_delay_stopbit(0),
  _tx_delay(0),
  _buffer_overflow(false),
  _inverse_logic(inverse_logic)
{
  setTX(transmitPin);
  setRX(receivePin);
}

//
// Destructor
//
RN52::~RN52()
{
  end();
}

void RN52::setTX(uint8_t tx)
{
  // First write, then set output. If we do this the other way around,
  // the pin would be output low for a short while before switching to
  // output hihg. Now, it is input with pullup for a short while, which
  // is fine. With inverse logic, either order is fine.
  digitalWrite(tx, _inverse_logic ? LOW : HIGH);
  pinMode(tx, OUTPUT);
  _transmitBitMask = digitalPinToBitMask(tx);
  uint8_t port = digitalPinToPort(tx);
  _transmitPortRegister = portOutputRegister(port);
}

void RN52::setRX(uint8_t rx)
{
  pinMode(rx, INPUT);
  if (!_inverse_logic)
    digitalWrite(rx, HIGH);  // pullup for normal logic!
  _receivePin = rx;
  _receiveBitMask = digitalPinToBitMask(rx);
  uint8_t port = digitalPinToPort(rx);
  _receivePortRegister = portInputRegister(port);
}

uint16_t RN52::subtract_cap(uint16_t num, uint16_t sub) {
  if (num > sub)
    return num - sub;
  else
    return 1;
}

//
// Public methods
//

void RN52::begin(long speed)
{
  _rx_delay_centering = _rx_delay_intrabit = _rx_delay_stopbit = _tx_delay = 0;

  // Precalculate the various delays, in number of 4-cycle delays
  uint16_t bit_delay = (F_CPU / speed) / 4;

  // 12 (gcc 4.8.2) or 13 (gcc 4.3.2) cycles from start bit to first bit,
  // 15 (gcc 4.8.2) or 16 (gcc 4.3.2) cycles between bits,
  // 12 (gcc 4.8.2) or 14 (gcc 4.3.2) cycles from last bit to stop bit
  // These are all close enough to just use 15 cycles, since the inter-bit
  // timings are the most critical (deviations stack 8 times)
  _tx_delay = subtract_cap(bit_delay, 15 / 4);

  // Only setup rx when we have a valid PCINT for this pin
  if (digitalPinToPCICR(_receivePin)) {
    #if GCC_VERSION > 40800
    // Timings counted from gcc 4.8.2 output. This works up to 115200 on
    // 16Mhz and 57600 on 8Mhz.
    //
    // When the start bit occurs, there are 3 or 4 cycles before the
    // interrupt flag is set, 4 cycles before the PC is set to the right
    // interrupt vector address and the old PC is pushed on the stack,
    // and then 75 cycles of instructions (including the RJMP in the
    // ISR vector table) until the first delay. After the delay, there
    // are 17 more cycles until the pin value is read (excluding the
    // delay in the loop).
    // We want to have a total delay of 1.5 bit time. Inside the loop,
    // we already wait for 1 bit time - 23 cycles, so here we wait for
    // 0.5 bit time - (71 + 18 - 22) cycles.
    _rx_delay_centering = subtract_cap(bit_delay / 2, (4 + 4 + 75 + 17 - 23) / 4);

    // There are 23 cycles in each loop iteration (excluding the delay)
    _rx_delay_intrabit = subtract_cap(bit_delay, 23 / 4);

    // There are 37 cycles from the last bit read to the start of
    // stopbit delay and 11 cycles from the delay until the interrupt
    // mask is enabled again (which _must_ happen during the stopbit).
    // This delay aims at 3/4 of a bit time, meaning the end of the
    // delay will be at 1/4th of the stopbit. This allows some extra
    // time for ISR cleanup, which makes 115200 baud at 16Mhz work more
    // reliably
    _rx_delay_stopbit = subtract_cap(bit_delay * 3 / 4, (37 + 11) / 4);
    #else // Timings counted from gcc 4.3.2 output
    // Note that this code is a _lot_ slower, mostly due to bad register
    // allocation choices of gcc. This works up to 57600 on 16Mhz and
    // 38400 on 8Mhz.
    _rx_delay_centering = subtract_cap(bit_delay / 2, (4 + 4 + 97 + 29 - 11) / 4);
    _rx_delay_intrabit = subtract_cap(bit_delay, 11 / 4);
    _rx_delay_stopbit = subtract_cap(bit_delay * 3 / 4, (44 + 17) / 4);
    #endif


    // Enable the PCINT for the entire port here, but never disable it
    // (others might also need it, so we disable the interrupt by using
    // the per-pin PCMSK register).
    *digitalPinToPCICR(_receivePin) |= _BV(digitalPinToPCICRbit(_receivePin));
    // Precalculate the pcint mask register and value, so setRxIntMask
    // can be used inside the ISR without costing too much time.
    _pcint_maskreg = digitalPinToPCMSK(_receivePin);
    _pcint_maskvalue = _BV(digitalPinToPCMSKbit(_receivePin));

    tunedDelay(_tx_delay); // if we were low this establishes the end

  }

#if _DEBUG
  pinMode(_DEBUG_PIN1, OUTPUT);
  pinMode(_DEBUG_PIN2, OUTPUT);
#endif

  listen();
}

void RN52::setRxIntMsk(bool enable)
{
    if (enable)
      *_pcint_maskreg |= _pcint_maskvalue;
    else
      *_pcint_maskreg &= ~_pcint_maskvalue;
}

void RN52::end()
{
  stopListening();
}


// Read data from buffer
int RN52::read()
{
  if (!isListening())
    return -1;

  // Empty buffer?
  if (_receive_buffer_head == _receive_buffer_tail)
    return -1;

  // Read from "head"
  uint8_t d = _receive_buffer[_receive_buffer_head]; // grab next byte
  _receive_buffer_head = (_receive_buffer_head + 1) % _SS_MAX_RX_BUFF;
  return d;
}

int RN52::available()
{
  if (!isListening())
    return 0;

  return (_receive_buffer_tail + _SS_MAX_RX_BUFF - _receive_buffer_head) % _SS_MAX_RX_BUFF;
}

size_t RN52::write(uint8_t b)
{
  if (_tx_delay == 0) {
    setWriteError();
    return 0;
  }

  // By declaring these as local variables, the compiler will put them
  // in registers _before_ disabling interrupts and entering the
  // critical timing sections below, which makes it a lot easier to
  // verify the cycle timings
  volatile uint8_t *reg = _transmitPortRegister;
  uint8_t reg_mask = _transmitBitMask;
  uint8_t inv_mask = ~_transmitBitMask;
  uint8_t oldSREG = SREG;
  bool inv = _inverse_logic;
  uint16_t delay = _tx_delay;

  if (inv)
    b = ~b;

  cli();  // turn off interrupts for a clean txmit

  // Write the start bit
  if (inv)
    *reg |= reg_mask;
  else
    *reg &= inv_mask;

  tunedDelay(delay);

  // Write each of the 8 bits
  for (uint8_t i = 8; i > 0; --i)
  {
    if (b & 1) // choose bit
      *reg |= reg_mask; // send 1
    else
      *reg &= inv_mask; // send 0

    tunedDelay(delay);
    b >>= 1;
  }

  // restore pin to natural state
  if (inv)
    *reg &= inv_mask;
  else
    *reg |= reg_mask;

  SREG = oldSREG; // turn interrupts back on
  tunedDelay(_tx_delay);

  return 1;
}

void RN52::flush()
{
  if (!isListening())
    return;

  uint8_t oldSREG = SREG;
  cli();
  _receive_buffer_head = _receive_buffer_tail = 0;
  SREG = oldSREG;
}

int RN52::peek()
{
  if (!isListening())
    return -1;

  // Empty buffer
  if (_receive_buffer_head == _receive_buffer_tail)
    return -1;

  // Read from "head"
  return _receive_buffer[_receive_buffer_head];
}

//For use with the GPIO on the rn52, sets inputs and outputs
bool RN52::GPIOPinMode(int pin, bool state)
{
  if (state) IO = IO | (1 << pin);
  else {
    short mask = 65535 ^ (1 << pin);
    IO = IO & mask;
  }
  short toWrite = (IO | IOMask) & IOProtect;
  print("I@,");
  if (toWrite < 4096) print("0");
  if (toWrite < 256) print("0");
  if (toWrite < 16) print("0");
  println(toWrite, HEX);
  while (available() == 0);
  char c = read();
  if (c == 'A') {
    delay(100);
    flush();
    return 1;
  }
  else {
    delay(100);
    flush();
    return 0;
  }
  delay(50);
}

//Writes outputs high or low, if inputs enables/disables internal pullup
void RN52::GPIODigitalWrite(int pin, bool state)
{
  if (state) IOState = IOState | (1 << pin);
  else {
    short mask = 65535 ^ (1 << pin);
    IOState = IOState & mask;
  }
  short toWrite = (IOState | IOStateMask) & IOStateProtect;
  print("I&,");
  if (toWrite < 4096) print("0");
  if (toWrite < 256) print("0");
  if (toWrite < 16) print("0");
  println(toWrite, HEX);
  delay(50);
}

//reads back the current state of the GPIO
bool RN52::GPIODigitalRead(int pin)
{
  while (available() > 0)
  {
    char c = read();
  }
  println("I&");
  delay(100);
  short valueIn = 0;
  for (int i = 0; i < 4; i++)
  {
    while (available() == 0);
    char c = read();
    if (c >= '0' && c <= '9')
    {
      valueIn *= 16;
      valueIn += (c - '0');
    }
    else if (c >= 'A' && c <= 'F')
    {
      valueIn *= 16;
      valueIn += (c - 'A') + 10;
    }
  }
  return (valueIn & (1 << pin)) >> pin;
}

void RN52::setDiscoverability(bool discoverable)
{
  print("@,");
  println(discoverable);
  delay(50);
}

void RN52::toggleEcho()
{
  println("+");
  delay(50);
}

void RN52::name(String nom, bool normalized)
{
  print("S");
  if (normalized) print("-,");
  else print("N,");
  println(nom);
  delay(50);
}

String RN52::name(void)
{
  while (available() > 0)
  {
    char c = read();
  }
  println("GN");
  String nom;
  char c;
  while (available() == 0);
 do
  {
    c = read();
    nom = nom + c;
  }
  while (c != '\r');
  int len=nom.length();
  nom.remove(len - 1);
  return nom;
}

void RN52::factoryReset()
{
  println("SF,1");
  delay(50);
}

int RN52::idlePowerDownTime(void)
{
  while (available() > 0)
  {
    char c = read();
  }
  int timer = 0;
  println("G^");
  while (available() == 0);
  delay(50);
  while (available() > 2)
  {
    char c = read();
    timer *= 10;
    timer += (c - '0');
    delay(50);
  }
  return timer;
}

void RN52::idlePowerDownTime(int timer)
{
  print("S^,");
  println(timer);
  delay(50);
}

void RN52::reboot()
{
  println("R,1");
  delay(2000);
}

void RN52::call(String number)
{
  print("A,");
  println(number);
  delay(50);
}

void RN52::endCall()
{
  println("E");
  delay(50);
}

void RN52::playPause()
{
  println("AP");
  delay(50);
}

void RN52::nextTrack()
{
  println("AT+");
  delay(50);
}

void RN52::prevTrack()
{
  println("AT-");
  delay(50);
}

//Credit to Greg Shuttleworth for assistance on this function
String RN52::getMetaData()
{
  while (available() > 0)
  {
    char c = read();
  }
  println("AD");
  while (available() == 0);
  String metaData;
  int i = 8;
  long count;
  while (i != 0)
  {
    if (available() > 0)
    {
      char c = read();
      count = millis();
      metaData += c;
      if (c == '\n') i--;
    }
    if ((millis() - count) > 500) i--;
  }
  return metaData;
}

String RN52::trackTitle()
{
  String metaData = getMetaData();
  int n = metaData.indexOf("Title=") + 6;
  if (n != -1) {
    metaData.remove(0, n);
    n = metaData.indexOf('\r');
    metaData.remove(n);
  }
  else metaData = "";
  return metaData;
}

String RN52::album()
{
  String metaData = getMetaData();
  int n = metaData.indexOf("Album=") + 6;
  if (n != -1) {
    metaData.remove(0, n);
    n = metaData.indexOf('\r');
    metaData.remove(n);
  }
  else metaData = "";
  return metaData;
}

String RN52::artist()
{
  String metaData = getMetaData();
  int n = metaData.indexOf("Artist=") + 7;
  if (n != -1) {
    metaData.remove(0, n);
    n = metaData.indexOf('\r');
    metaData.remove(n);
  }
  else metaData = "";
  return metaData;
}

String RN52::genre()
{
  String metaData = getMetaData();
  int n = metaData.indexOf("Genre=") + 6;
  if (n != -1) {
    metaData.remove(0, n);
    n = metaData.indexOf('\r');
    metaData.remove(n);
  }
  else metaData = "";
  return metaData;
}

int RN52::trackNumber()
{
  int trackNumber = 0;
  int attempts = 0;

  String metaData = "";

  while(trackNumber==0 && attempts < 5){
      metaData = getMetaData();
      int n = metaData.indexOf("TrackNumber=") + 12;
      if (n != -1) {
        metaData.remove(0, n);
        n = metaData.indexOf('\r');
        metaData.remove(n);
        trackNumber = metaData.toInt();
      }
    else trackNumber = 0;
    attempts++;
  }

  return trackNumber;
}

int RN52::trackCount()
{
  int trackCount = 0;
  int attempts = 0;

  String metaData = "";

  while(trackCount==0 && attempts < 5){
    metaData = getMetaData();
    int n = metaData.indexOf("TrackCount=") + 11;
    if (n != -1) {
      metaData.remove(0, n);
      n = metaData.indexOf('\r');
      metaData.remove(n);
      trackCount = metaData.toInt();
    }
    else trackCount = 0;
    attempts++;
  }
  return trackCount;
}

String RN52::getConnectionData()
{
  while (available() > 0)
  {
    char c = read();
  }
  println("D");
  while (available() == 0);
  String connectionData;
  int i = 13;
  long count;
  while (i != 0)
  {
    if (available() > 0)
    {
      char c = read();
      count = millis();
      connectionData += c;
      if (c == '\n') i--;
    }
    if ((millis() - count) > 500) i--;
  }
  return connectionData;
}

String RN52::connectedMAC()
{
  String connectionData="";
  int attempts=0;

  while(connectionData.length()!=12 && attempts < 5){

    connectionData = getConnectionData();

    int n = connectionData.indexOf("BTAC=") + 5;
    if (n != -1) {
      connectionData.remove(0, n);
      n = connectionData.indexOf('\r');
      connectionData.remove(n);
    }
    attempts++;
  }

  if(connectionData.length()!= 12){
    connectionData = "Invalid MAC Address";
  }
  return connectionData;
}

short RN52::getExtFeatures()
{
  while (available() > 0)
  {
    char c = read();
  }
  short valueIn = 0;
  char c;
  while (c != '\r')
  {
    while (available() == 0) {
      println("G%");
      delay(50);
    }
    c = read();
    if (c >= '0' && c <= '9')
    {
      valueIn *= 16;
      valueIn += (c - '0');
    }
    else if (c >= 'A' && c <= 'F')
    {
      valueIn *= 16;
      valueIn += (c - 'A') + 10;
    }
    else if ((c == '?') || (c == '!'))
    {
      while (available() > 0)
      {
        char d = read();
      }
    }
  }
  return valueIn;
}

/* <EXPERIMENTAL Q Command Stuff> */

short RN52::getEventReg()
{
  while (available() > 0)
    char c = read();

  short valueIn = 0;
  char c;

  while (c != '\r')
  {
    while (available() == 0) {
      println("Q");
      delay(50);
    }

    c = read();

    if (c >= '0' && c <= '9')
    {
      valueIn *= 16;
      valueIn += (c - '0');
    }
    else if (c >= 'A' && c <= 'F')
    {
      valueIn *= 16;
      valueIn += (c - 'A') + 10;
    }

    else if ((c == '?') || (c == '!'))
    {
      while (available() > 0)
      {
        char d = read();
      }
    }
  }

  /* Record the track change internally */
  if(!_trackChanged && (valueIn & (1 << 13)))
	  _trackChanged = true;

  return valueIn;
}

bool RN52::trackChanged(void)
{
	/* If the track has changed since this function was last called
	   and getEventReg has been called since */
	if(_trackChanged)
	{
		_trackChanged = false;
		return true;
	}

	bool change = (getEventReg() & (1 << 13));
	_trackChanged = false;
	return change;
}

bool RN52::isConnected(void)
{
	return (getEventReg() & 0x0F00);
}

/* </EXPERIMENTAL Q Command Stuff> */

void RN52::setExtFeatures(bool state, int bit)
{
  short toWrite;
  if (state) toWrite = getExtFeatures() | (1 << bit);
  else toWrite = getExtFeatures() & (65535 ^ (1 << bit));
  print("S%,");
  if (toWrite < 4096) print("0");
  if (toWrite < 256)  print("0");
  if (toWrite < 16)   print("0");
  println(toWrite, HEX);
  delay(100);
}

void RN52::setExtFeatures(short settings)
{
  short toWrite = settings;
  print("S%,");
  if (toWrite < 4096) print("0");
  if (toWrite < 256)  print("0");
  if (toWrite < 16)   print("0");
  println(toWrite, HEX);
  delay(100);
}

bool RN52::AVRCPButtons()
{
  return (getExtFeatures() & 1);
}

void RN52::AVRCPButtons(bool state)
{
  setExtFeatures(state, 0);
}

bool RN52::powerUpReconnect()
{
  return (getExtFeatures() & (1 << 1));
}

void RN52::powerUpReconnect(bool state)
{
  setExtFeatures(state, 1);
}

bool RN52::startUpDiscoverable()
{
  return (getExtFeatures() & (1 << 2));
}

void RN52::startUpDiscoverable(bool state)
{
  setExtFeatures(state, 2);
}

bool RN52::rebootOnDisconnect()
{
  return (getExtFeatures() & (1 << 4));
}

void RN52::rebootOnDisconnect(bool state)
{
  setExtFeatures(state, 4);
}

bool RN52::volumeToneMute()
{
  return (getExtFeatures() & (1 << 5));
}

void RN52::volumeToneMute(bool state)
{
  setExtFeatures(state, 5);
}

bool RN52::systemTonesDisabled()
{
  return (getExtFeatures() & (1 << 7));
}

void RN52::systemTonesDisabled(bool state)
{
  setExtFeatures(state, 7);
}

bool RN52::powerDownAfterPairingTimeout()
{
  return (getExtFeatures() & (1 << 8));
}

void RN52::powerDownAfterPairingTimeout(bool state)
{
  setExtFeatures(state, 8);
}

bool RN52::resetAfterPowerDown()
{
  return (getExtFeatures() & (1 << 9));
}

void RN52::resetAfterPowerDown(bool state)
{
  setExtFeatures(state, 9);
}

bool RN52::reconnectAfterPanic()
{
  return (getExtFeatures() & (1 << 10));
}

void RN52::reconnectAfterPanic(bool state)
{
  setExtFeatures(state, 10);
}

bool RN52::trackChangeEvent()
{
  return (getExtFeatures() & (1 << 12));
}

void RN52::trackChangeEvent(bool state)
{
  setExtFeatures(state, 12);
}

bool RN52::tonesAtFixedVolume()
{
  return (getExtFeatures() & (1 << 13));
}

void RN52::tonesAtFixedVolume(bool state)
{
  setExtFeatures(state, 13);
}

bool RN52::autoAcceptPasskey()
{
  return (getExtFeatures() & (1 << 14));
}

void RN52::autoAcceptPasskey(bool state)
{
  setExtFeatures(state, 14);
}

int RN52::volumeOnStartup(void)
{
  while (available() > 0)
  {
    char c = read();
  }
  println("GS");
  delay(100);
  int vol = 0;
  char c;
  while (c != '\r')
  {
    while (available() == 0);
    c = read();
    if (c >= '0' && c <= '9')
    {
      vol *= 16;
      vol += (c - '0');
    }
    else if (c >= 'A' && c <= 'F')
    {
      vol *= 16;
      vol += (c - 'A') + 10;
    }
  }
  return vol;
}

void RN52::volumeOnStartup(int vol)
{
  print("SS,");
  print("0");
  println(vol, HEX);
  delay(50);
}

void RN52::volumeUp(void)
{
  println("AV+");
  delay(50);
}

void RN52::volumeDown(void)
{
  println("AV-");
  delay(50);
}

short RN52::getAudioRouting()
{
  while (available() > 0)
  {
    char c = read();
  }
  println("G|");
  delay(100);
  short routing = 0;
  char c;
  while (c != '\r')
  {
    while (available() == 0);
    c = read();
    if (c >= '0' && c <= '9')
    {
      routing *= 16;
      routing += (c - '0');
    }
    else if (c >= 'A' && c <= 'F')
    {
      routing *= 16;
      routing += (c - 'A') + 10;
    }
  }
  return routing;
}

int RN52::sampleWidth()
{
  int width = (getAudioRouting() & 0x00F0) >> 4;
  return width;
}

void RN52::sampleWidth(int width)
{
  short mask = getAudioRouting() & 0xFF0F;
  short toWrite = mask | (width << 4);
  print("S|,");
  if (toWrite < 4096) print("0");
  if (toWrite < 256) print("0");
  if (toWrite < 16) print("0");
  println(toWrite, HEX);
  delay(50);
}

int RN52::sampleRate()
{
  int rate = getAudioRouting() & 0x000F;
  return rate;
}

void RN52::sampleRate(int rate)
{
  short mask = getAudioRouting() & 0xFFF0;
  short toWrite = mask | rate;
  print("S|,");
  if (toWrite < 4096) print("0");
  if (toWrite < 256) print("0");
  if (toWrite < 16) print("0");
  println(toWrite, HEX);
  delay(50);
}

int RN52::A2DPRoute()
{
  int route = (getAudioRouting() & 0x0F00) >> 8;
  return route;
}

void RN52::A2DPRoute(int route)
{
  short mask = getAudioRouting() & 0x00FF;
  short toWrite = mask | (route << 8);
  print("S|,");
  if (toWrite < 4096) print("0");
  if (toWrite < 256) print("0");
  if (toWrite < 16) print("0");
  println(toWrite, HEX);
  delay(50);
}
