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

#ifndef RN52_h
#define RN52_h

#include <inttypes.h>
#include <Stream.h>

/******************************************************************************
* Definitions
******************************************************************************/

#define _SS_MAX_RX_BUFF 64 // RX buffer size
#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

class RN52 : public Stream
{
private:
  // per object data
  uint8_t _receivePin;
  uint8_t _receiveBitMask;
  volatile uint8_t *_receivePortRegister;
  uint8_t _transmitBitMask;
  volatile uint8_t *_transmitPortRegister;
  volatile uint8_t *_pcint_maskreg;
  uint8_t _pcint_maskvalue;

  // Expressed as 4-cycle delays (must never be 0!)
  uint16_t _rx_delay_centering;
  uint16_t _rx_delay_intrabit;
  uint16_t _rx_delay_stopbit;
  uint16_t _tx_delay;

  uint16_t _buffer_overflow:1;
  uint16_t _inverse_logic:1;

  // static data
  static char _receive_buffer[_SS_MAX_RX_BUFF];
  static volatile uint8_t _receive_buffer_tail;
  static volatile uint8_t _receive_buffer_head;
  static volatile bool _trackChanged;
  static RN52 *active_object;
  static short IOMask;
  static short IOProtect;
  static short IOStateMask;
  static short IOStateProtect;
  static short IO;
  static short IOState;

  // private methods
  void recv() __attribute__((__always_inline__));
  uint8_t rx_pin_read();
  void tx_pin_write(uint8_t pin_state) __attribute__((__always_inline__));
  void setTX(uint8_t transmitPin);
  void setRX(uint8_t receivePin);
  void setRxIntMsk(bool enable) __attribute__((__always_inline__));

  // Return num - sub, or 1 if the result would be < 1
  static uint16_t subtract_cap(uint16_t num, uint16_t sub);

  // private static method for timing
  static inline void tunedDelay(uint16_t delay);



public:

  // public methods
  RN52(uint8_t receivePin, uint8_t transmitPin, bool inverse_logic = false);
  ~RN52();
  void begin(long speed);
  bool listen();
  void end();
  bool isListening() { return this == active_object; }
  bool stopListening();
  bool overflow() { bool ret = _buffer_overflow; if (ret) _buffer_overflow = false; return ret; }
  int peek();

  virtual size_t write(uint8_t byte);
  virtual int read();
  virtual int available();
  virtual void flush();
  operator bool() { return true; }

  using Print::write;

  static inline void handle_interrupt() __attribute__((__always_inline__));
  bool GPIOPinMode(int pin, bool state);
  void GPIODigitalWrite(int pin, bool state);
  bool GPIODigitalRead(int pin);
  void setDiscoverability(bool discoverable);
  void toggleEcho();
  void name(String nom, bool normalized);
  String name(void);
  void factoryReset();
  int idlePowerDownTime(void);
  void idlePowerDownTime(int timer);
  void reboot();
  void call(String number);
  void endCall();
  void playPause();
  void nextTrack();
  void prevTrack();
  String getMetaData();
  String trackTitle();
  String album();
  String artist();
  String genre();
  int trackNumber();
  int trackCount();
  String getConnectionData();
  String connectedMAC();
  short getExtFeatures();
  //not yet documented
  short getEventReg();
  bool trackChanged();
  bool isConnected();
  //end
  void setExtFeatures(bool state, int bit);
  void setExtFeatures(short settings);
  bool AVRCPButtons();
  void AVRCPButtons(bool state);
  bool powerUpReconnect();
  void powerUpReconnect(bool state);
  bool startUpDiscoverable();
  void startUpDiscoverable(bool state);
  bool rebootOnDisconnect();
  void rebootOnDisconnect(bool state);
  bool volumeToneMute();
  void volumeToneMute(bool state);
  bool systemTonesDisabled();
  void systemTonesDisabled(bool state);
  bool powerDownAfterPairingTimeout();
  void powerDownAfterPairingTimeout(bool state);
  bool resetAfterPowerDown();
  void resetAfterPowerDown(bool state);
  bool reconnectAfterPanic();
  void reconnectAfterPanic(bool state);
  //Not in guide
  bool trackChangeEvent();
  void trackChangeEvent(bool state);
  //</Not in guide>
  bool tonesAtFixedVolume();
  void tonesAtFixedVolume(bool state);
  bool autoAcceptPasskey();
  void autoAcceptPasskey(bool state);
  int volumeOnStartup();
  void volumeOnStartup(int vol);
  void volumeUp();
  void volumeDown();
  //Following not in the guide
  short getAudioRouting();
  int sampleWidth();
  void sampleWidth(int width);
  int sampleRate();
  void sampleRate(int rate);
  int A2DPRoute();
  void A2DPRoute(int route);
};

// Arduino 0012 workaround
#undef int
#undef char
#undef long
#undef byte
#undef float
#undef abs
#undef round

#endif
