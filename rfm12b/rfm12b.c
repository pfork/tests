// RF12 command codes
#define RF_RECV_CONTROL 0x94A0
#define RF_RECEIVER_ON 0x82DD
#define RF_XMITTER_ON 0x823D
#define RF_IDLE_MODE 0x820D
#define RF_SLEEP_MODE 0x8205
#define RF_WAKEUP_MODE 0x8207
#define RF_TXREG_WRITE 0xB800
#define RF_RX_FIFO_READ 0xB000
#define RF_WAKEUP_TIMER 0xE000
// RF12 status bits
#define RF_LBD_BIT 0x0400
#define RF_RSSI_BIT 0x0100
// bits in the node id configuration byte
#define NODE_BAND 0xC0 // frequency band
#define NODE_ID 0x1F // id of this node, as A..Z or 1..31
// transceiver states, these determine what to do with each interrupt
enum {
  TXCRC1, TXCRC2, TXTAIL, TXDONE, TXIDLE,
  TXRECV,
  TXPRE1, TXPRE2, TXPRE3, TXSYN1, TXSYN2,
};
static uint8_t nodeid; // address of this node
static uint8_t group; // network group
static uint16_t frequency; // Frequency within selected band
static volatile uint8_t rxfill; // number of data bytes in rf12_buf
static volatile int8_t rxstate; // current transceiver state
#define RETRIES 8 // stop retrying after 8 times
#define RETRY_MS 1000 // resend packet every second until ack'ed
static uint8_t ezInterval; // number of seconds between transmits
static uint8_t ezSendBuf[RF12_MAXDATA]; // data to send
static char ezSendLen; // number of bytes to send
static uint8_t ezPending; // remaining number of retries
static long ezNextSend[2]; // when was last retry [0] or data [1] sent
volatile uint16_t rf12_crc; // running crc value
volatile uint8_t rf12_buf[RF_MAX]; // recv/xmit buf, including hdr & crc bytes
static uint8_t rf12_fixed_pkt_len; // fixed packet length reception

static void rf12_xfer (uint16_t cmd) {
  SPI_read(cmd >> 8) << 8;
  SPI_read(cmd);
}
static void rf12_interrupt () {
  // a transfer of 2x 16 bits @ 2 MHz over SPI takes 2x 8 us inside this ISR
  // correction: now takes 2 + 8 µs, since sending can be done at 8 MHz
  rf12_xfer(0x0000);
  if (rxstate == TXRECV) {
    uint8_t in = rf12_xferSlow(RF_RX_FIFO_READ);
    if (rxfill == 0 && group != 0)
      rf12_buf[rxfill++] = group;
    rf12_buf[rxfill++] = in;
    rf12_crc = _crc16_update(rf12_crc, in);
    if (rxfill >= rf12_len + 5 || rxfill >= RF_MAX)
      rf12_xfer(RF_IDLE_MODE);
  } else {
    uint8_t out;
    if (rxstate < 0) {
      uint8_t pos = 3 + rf12_len + rxstate++;
      out = rf12_buf[pos];
      rf12_crc = _crc16_update(rf12_crc, out);
    } else
      switch (rxstate++) {
      case TXSYN1: out = 0x2D; break;
      case TXSYN2: out = group; rxstate = - (2 + rf12_len); break;
      case TXCRC1: out = rf12_crc; break;
      case TXCRC2: out = rf12_crc >> 8; break;
      case TXDONE: rf12_xfer(RF_IDLE_MODE); // fall through
      default: out = 0xAA;
      }
    rf12_xfer(RF_TXREG_WRITE + out);
  }
}

static void rf12_recvStart () {
  if (rf12_fixed_pkt_len) {
    rf12_len = rf12_fixed_pkt_len;
    rf12_grp = rf12_hdr = 0;
    rxfill = 3;
  } else
    rxfill = rf12_len = 0;
  rf12_crc = ~0;
#if RF12_VERSION >= 2
  if (group != 0)
    rf12_crc = _crc16_update(~0, group);
#endif
  rxstate = TXRECV;
  rf12_xfer(RF_RECEIVER_ON);
}

/// @details
/// The timing of this function is relatively coarse, because SPI transfers are
/// used to enable / disable the transmitter. This will add some jitter to the
/// signal, probably in the order of 10 µsec.
///
/// If the result is true, then a packet has been received and is available for
/// processing. The following global variables will be set:
///
/// * volatile byte rf12_hdr -
/// Contains the header byte of the received packet - with flag bits and
/// node ID of either the sender or the receiver.
/// * volatile byte rf12_len -
/// The number of data bytes in the packet. A value in the range 0 .. 66.
/// * volatile byte rf12_data -
/// A pointer to the received data.
/// * volatile byte rf12_crc -
/// CRC of the received packet, zero indicates correct reception. If != 0
/// then rf12_hdr, rf12_len, and rf12_data should not be relied upon.
///
/// To send an acknowledgement, call rf12_sendStart() - but only right after
/// rf12_recvDone() returns true. This is commonly done using these macros:
///
/// if(RF12_WANTS_ACK){
/// rf12_sendStart(RF12_ACK_REPLY,0,0);
/// }
/// @see http://jeelabs.org/2010/12/11/rf12-acknowledgements/
uint8_t rf12_recvDone () {
  if (rxstate == TXRECV && (rxfill >= rf12_len + 5 || rxfill >= RF_MAX)) {
    rxstate = TXIDLE;
    if (rf12_len > RF12_MAXDATA)
      rf12_crc = 1; // force bad crc if packet length is invalid
    if (!(rf12_hdr & RF12_HDR_DST) || (nodeid & NODE_ID) == 31 ||
        (rf12_hdr & RF12_HDR_MASK) == (nodeid & NODE_ID)) {
      return 1; // it's a broadcast packet or it's addressed to this node
    }
  }
  if (rxstate == TXIDLE)
    rf12_recvStart();
  return 0;
}
/// @details
/// Call this when you have some data to send. If it returns true, then you can
/// use rf12_sendStart() to start the transmission. Else you need to wait and
/// retry this call at a later moment.
///
/// Don't call this function if you have nothing to send, because rf12_canSend()
/// will stop reception when it returns true. IOW, calling the function
/// indicates your intention to send something, and once it returns true, you
/// should follow through and call rf12_sendStart() to actually initiate a send.
/// See [this weblog post](http://jeelabs.org/2010/05/20/a-subtle-rf12-detail/).
///
/// Note that even if you only want to send out packets, you still have to call
/// rf12_recvDone() periodically, because it keeps the RFM12B logic going. If
/// you don't, rf12_canSend() will never return true.
uint8_t rf12_canSend () {
  // need interrupts off to avoid a race (and enable the RFM12B, thx Jorg!)
  // see http://openenergymonitor.org/emon/node/1051?page=3
  if (rxstate == TXRECV && rxfill == 0 &&
      (rf12_control(0x0000) & RF_RSSI_BIT) == 0) {
    rf12_control(RF_IDLE_MODE); // stop receiver
    rxstate = TXIDLE;
    return 1;
  }
  return 0;
}
void rf12_sendStart (uint8_t hdr) {
  rf12_hdr = hdr & RF12_HDR_DST ? hdr :
    (hdr & ~RF12_HDR_MASK) + (nodeid & NODE_ID);
  rf12_crc = ~0;
#if RF12_VERSION >= 2
  rf12_crc = _crc16_update(rf12_crc, group);
#endif
  rxstate = TXPRE1;
  rf12_xfer(RF_XMITTER_ON); // bytes will be fed via interrupts
}
/// @details
/// Switch to transmission mode and send a packet.
/// This can be either a request or a reply.
///
/// Notes
/// -----
///
/// The rf12_sendStart() function may only be called in two specific situations:
///
/// * right after rf12_recvDone() returns true - used for sending replies /
/// acknowledgements
/// * right after rf12_canSend() returns true - used to send requests out
///
/// Because transmissions may only be started when there is no other reception
/// or transmission taking place.
///
/// The short form, i.e. "rf12_sendStart(hdr)" is for a special buffer-less
/// transmit mode, as described in this
/// [weblog post](http://jeelabs.org/2010/09/15/more-rf12-driver-notes/).
///
/// The call with 4 arguments, i.e. "rf12_sendStart(hdr, data, length, sync)" is
/// deprecated, as described in that same weblog post. The recommended idiom is
/// now to call it with 3 arguments, followed by a call to rf12_sendWait().
/// @param hdr The header contains information about the destination of the
/// packet to send, and flags such as whether this should be
/// acknowledged - or if it actually is an acknowledgement.
/// @param ptr Pointer to the data to send as packet.
/// @param len Number of data bytes to send. Must be in the range 0 .. 65.
void rf12_sendStart (uint8_t hdr, const void* ptr, uint8_t len) {
  rf12_len = len;
  memcpy((void*) rf12_data, ptr, len);
  rf12_sendStart(hdr);
}
/// @details
/// Wait until transmission is possible, then start it as soon as possible.
/// @note This uses a (brief) busy loop and will discard any incoming packets.
/// @param hdr The header contains information about the destination of the
/// packet to send, and flags such as whether this should be
/// acknowledged - or if it actually is an acknowledgement.
/// @param ptr Pointer to the data to send as packet.
/// @param len Number of data bytes to send. Must be in the range 0 .. 65.
void rf12_sendNow (uint8_t hdr, const void* ptr, uint8_t len) {
  while (!rf12_canSend())
    rf12_recvDone(); // keep the driver state machine going, ignore incoming
  rf12_sendStart(hdr, ptr, len);
}
/// @details
/// Wait for completion of the preceding rf12_sendStart() call, using the
/// specified low-power mode.
/// @note rf12_sendWait() should only be called right after rf12_sendStart().
/// @param mode Power-down mode during wait: 0 = NORMAL, 1 = IDLE, 2 = STANDBY,
/// 3 = PWR_DOWN. Values 2 and 3 can cause the millisecond time to
/// lose a few interrupts. Value 3 can only be used if the ATmega
/// fuses have been set for fast startup, i.e. 258 CK - the default
/// Arduino fuse settings are not suitable for full power down.
void rf12_sendWait (uint8_t mode) {
  // wait for packet to actually finish sending
  // go into low power mode, as interrupts are going to come in very soon
  while (rxstate != TXIDLE)
    if (mode) {
      // power down mode is only possible if the fuses are set to start
      // up in 258 clock cycles, i.e. approx 4 us - else must use standby!
      // modes 2 and higher may lose a few clock timer ticks
      set_sleep_mode(mode == 3 ? SLEEP_MODE_PWR_DOWN :
#ifdef SLEEP_MODE_STANDBY
                     mode == 2 ? SLEEP_MODE_STANDBY :
#endif
                     SLEEP_MODE_IDLE);
      sleep_mode();
    }
}
/// @details
/// Call this once with the node ID (0-31), frequency band (0-3), and
/// optional group (0-255 for RFM12B, only 212 allowed for RFM12).
/// @param id The ID of this wireless node. ID's should be unique within the
/// netGroup in which this node is operating. The ID range is 0 to 31,
/// but only 1..30 are available for normal use. You can pass a single
/// capital letter as node ID, with 'A' .. 'Z' corresponding to the
/// node ID's 1..26, but this convention is now discouraged. ID 0 is
/// reserved for OOK use, node ID 31 is special because it will pick
/// up packets for any node (in the same netGroup).
/// @param band This determines in which frequency range the wireless module
/// will operate. The following pre-defined constants are available:
/// RF12_433MHZ, RF12_868MHZ, RF12_915MHZ. You should use the one
/// matching the module you have, to get a useful TX/RX range.
/// @param g Net groups are used to separate nodes: only nodes in the same net
/// group can communicate with each other. Valid values are 1 to 212.
/// This parameter is optional, it defaults to 212 (0xD4) when omitted.
/// This is the only allowed value for RFM12 modules, only RFM12B
/// modules support other group values.
/// @param f Frequency correction to apply. Defaults to 1600, per RF12 docs.
/// This parameter is optional, and was added in February 2014.
/// @returns the nodeId, to be compatible with rf12_config().
///
/// Programming Tips
/// ----------------
/// Note that rf12_initialize() does not use the EEprom netId and netGroup
/// settings, nor does it change the EEPROM settings. To use the netId and
/// netGroup settings saved in EEPROM use rf12_config() instead of
/// rf12_initialize. The choice whether to use rf12_initialize() or
/// rf12_config() at the top of every sketch is one of personal preference.
/// To set EEPROM settings for use with rf12_config() use the RF12demo sketch.
uint8_t rf12_initialize (uint8_t id, uint8_t band, uint8_t g, uint16_t f) {
  nodeid = id;
  group = g;
  frequency = f;
  // caller should validate! if (frequency < 96) frequency = 1600;
  rf12_spiInit();
  rf12_xfer(0x0000); // initial SPI transfer added to avoid power-up problem
  rf12_xfer(RF_SLEEP_MODE); // DC (disable clk pin), enable lbd
  // wait until RFM12B is out of power-up reset, this takes several *seconds*
  rf12_xfer(RF_TXREG_WRITE); // in case we're still in OOK mode
  while (digitalRead(RFM_IRQ) == 0)
    rf12_xfer(0x0000);
  rf12_xfer(0x80C7 | (band << 4)); // EL (ena TX), EF (ena RX FIFO), 12.0pF
  rf12_xfer(0xA000 + frequency); // 96-3960 freq range of values within band
  rf12_xfer(0xC606); // approx 49.2 Kbps, i.e. 10000/29/(1+6) Kbps
  rf12_xfer(0x94A2); // VDI,FAST,134kHz,0dBm,-91dBm
  rf12_xfer(0xC2AC); // AL,!ml,DIG,DQD4
  if (group != 0) {
    rf12_xfer(0xCA83); // FIFO8,2-SYNC,!ff,DR
    rf12_xfer(0xCE00 | group); // SYNC=2DXX；
  } else {
    rf12_xfer(0xCA8B); // FIFO8,1-SYNC,!ff,DR
    rf12_xfer(0xCE2D); // SYNC=2D；
  }
  rf12_xfer(0xC483); // @PWR,NO RSTRIC,!st,!fi,OE,EN
  rf12_xfer(0x9850); // !mp,90kHz,MAX OUT
  rf12_xfer(0xCC77); // OB1，OB0, LPX,！ddy，DDIT，BW0
  rf12_xfer(0xE000); // NOT USE
  rf12_xfer(0xC800); // NOT USE
  rf12_xfer(0xC049); // 1.66MHz,3.1V
  rxstate = TXIDLE;
#if PINCHG_IRQ
#if RFM_IRQ < 8
  if ((nodeid & NODE_ID) != 0) {
    bitClear(DDRB, RFM_IRQ); // input
    bitSet(PORTB, RFM_IRQ); // pull-up
    bitSet(PCMSK0, RFM_IRQ); // pin-change
    bitSet(PCICR, PCIE0); // enable
  } else
    bitClear(PCMSK0, RFM_IRQ);
#elif RFM_IRQ < 15
  if ((nodeid & NODE_ID) != 0) {
    bitClear(DDRC, RFM_IRQ - 8); // input
    bitSet(PORTC, RFM_IRQ - 8); // pull-up
    bitSet(PCMSK1, RFM_IRQ - 8); // pin-change
    bitSet(PCICR, PCIE1); // enable
  } else
    bitClear(PCMSK1, RFM_IRQ - 8);
#else
  if ((nodeid & NODE_ID) != 0) {
    bitClear(DDRD, RFM_IRQ - 16); // input
    bitSet(PORTD, RFM_IRQ - 16); // pull-up
    bitSet(PCMSK2, RFM_IRQ - 16); // pin-change
    bitSet(PCICR, PCIE2); // enable
  } else
    bitClear(PCMSK2, RFM_IRQ - 16);
#endif
#else
  if ((nodeid & NODE_ID) != 0)
    attachInterrupt(0, rf12_interrupt, LOW);
  else
    detachInterrupt(0);
#endif
  return nodeid;
}
/// @details
/// This can be used to send out slow bit-by-bit On Off Keying signals to other
/// devices such as remotely controlled power switches operating in the 433,
/// 868, or 915 MHz bands.
///
/// To use this, you need to first call rf12initialize() with a zero node ID
/// and the proper frequency band. Then call rf12onOff() in the exact timing
/// you need for sending out the signal. Once done, either call rf12onOff(0) to
/// turn the transmitter off, or reinitialize the wireless module completely
/// with a call to rf12initialize().
/// @param value Turn the transmitter on (if true) or off (if false).
/// @note The timing of this function is relatively coarse, because SPI
/// transfers are used to enable / disable the transmitter. This will add some
/// jitter to the signal, probably in the order of 10 µsec.
void rf12_onOff (uint8_t value) {
  rf12_xfer(value ? RF_XMITTER_ON : RF_IDLE_MODE);
}
/// @details
/// This calls rf12_initialize() with settings obtained from EEPROM address
/// 0x20 .. 0x3F. These settings can be filled in by the RF12demo sketch in the
/// RFM12B library. If the checksum included in those bytes is not valid,
/// rf12_initialize() will not be called.
///
/// As side effect, rf12_config() also writes the current configuration to the
/// serial port, ending with a newline. Use rf12_configSilent() to avoid this.
/// @returns the node ID obtained from EEPROM, or 0 if there was none.
uint8_t rf12_config () {
  uint16_t crc = ~0;
  for (uint8_t i = 0; i < RF12_EEPROM_SIZE; ++i) {
    byte e = eeprom_read_byte(RF12_EEPROM_ADDR + i);
    crc = _crc16_update(crc, e);
  }
  if (crc || eeprom_read_byte(RF12_EEPROM_ADDR + 2) != RF12_EEPROM_VERSION)
    return 0;
  uint8_t nodeId = 0, group = 0;
  uint16_t frequency = 0;
  nodeId = eeprom_read_byte(RF12_EEPROM_ADDR + 0);
  group = eeprom_read_byte(RF12_EEPROM_ADDR + 1);
  frequency = eeprom_read_word((uint16_t*) (RF12_EEPROM_ADDR + 4));
  rf12_initialize(nodeId, nodeId >> 6, group, frequency);
  return nodeId & RF12_HDR_MASK;
}
/// @details
/// This function can put the radio module to sleep and wake it up again.
/// In sleep mode, the radio will draw only one or two microamps of current.
///
/// This function can also be used as low-power watchdog, by putting the radio
/// to sleep and having it raise an interrupt between about 30 milliseconds
/// and 4 seconds later.
/// @param n If RF12SLEEP (0), put the radio to sleep - no scheduled wakeup.
/// If RF12WAKEUP (-1), wake the radio up so that the next call to
/// rf12_recvDone() can restore normal reception. If value is in the
/// range 1 .. 127, then the radio will go to sleep and generate an
/// interrupt approximately 32*value miliiseconds later.
/// @todo Figure out how to get the "watchdog" mode working reliably.
void rf12_sleep (char n) {
  if (n < 0)
    rf12_control(RF_IDLE_MODE);
  else {
    rf12_control(RF_WAKEUP_TIMER | 0x0500 | n);
    rf12_control(RF_SLEEP_MODE);
    if (n > 0)
      rf12_control(RF_WAKEUP_MODE);
  }
  rxstate = TXIDLE;
}
/// @details
/// This checks the status of the RF12 low-battery detector. It will be 1 when
/// the supply voltage drops below 3.1V, and 0 otherwise. This can be used to
/// detect an impending power failure, but there are no guarantees that the
/// power still remaining will be sufficient to send or receive further packets.
char rf12_lowbat () {
  return (rf12_control(0x0000) & RF_LBD_BIT) != 0;
}
/// @details
/// Set up the easy transmission mechanism. The argument is the minimal number
/// of seconds between new data packets (from 1 to 255). With 0 as argument,
/// packets will be sent as fast as possible:
///
/// * On the 433 and 915 MHz frequency bands, this is fixed at 100 msec (10
/// packets/second).
///
/// * On the 866 MHz band, the frequency depends on the number of bytes sent:
/// for 1-byte packets, it'll be up to 7 packets/second, for 66-byte bytes of
/// data it will be around 1 packet/second.
///
/// This function should be called after the RF12 driver has been initialized,
/// using either rf12_initialize() or rf12_config().
/// @param secs The minimal number of seconds between new data packets (from 1
/// to 255). With a 0 argument, packets will be sent as fast as
/// possible: on the 433 and 915 MHz frequency bands, this is fixed
/// at 100 msec (10 packets/second). On 866 MHz, the frequency
/// depends on the number of bytes sent: for 1-byte packets, it
/// will be up to 7 packets/second, for 66-byte bytes of data it
/// drops to approx. 1 packet/second.
/// @note To be used in combination with rf12_easyPoll() and rf12_easySend().
void rf12_easyInit (uint8_t secs) {
  ezInterval = secs;
}
/// @details
/// Needs to be called often to keep the easy transmission mechanism going,
/// i.e. once per millisecond or more in normal use. Failure to poll frequently
/// enough is relatively harmless but may lead to lost acknowledgements.
/// @returns 1 = an ack has been received with actual data in it, use rf12len
/// and rf12data to access it. 0 = there is nothing to do, the last
/// send has been ack'ed or more than 8 re-transmits have failed.
/// -1 = still sending or waiting for an ack to come in
/// @note To be used in combination with rf12_easyInit() and rf12_easySend().
char rf12_easyPoll () {
  if (rf12_recvDone() && rf12_crc == 0) {
    byte myAddr = nodeid & RF12_HDR_MASK;
    if (rf12_hdr == (RF12_HDR_CTL | RF12_HDR_DST | myAddr)) {
      ezPending = 0;
      ezNextSend[0] = 0; // flags succesful packet send
      if (rf12_len > 0)
        return 1;
    }
  }
  if (ezPending > 0) {
    // new data sends should not happen less than ezInterval seconds apart
    // ... whereas retries should not happen less than RETRY_MS apart
    byte newData = ezPending == RETRIES;
    long now = millis();
    if (now >= ezNextSend[newData] && rf12_canSend()) {
      ezNextSend[0] = now + RETRY_MS;
      // must send new data packets at least ezInterval seconds apart
      // ezInterval == 0 is a special case:
      // for the 868 MHz band: enforce 1% max bandwidth constraint
      // for other bands: use 100 msec, i.e. max 10 packets/second
      if (newData)
        ezNextSend[1] = now +
          (ezInterval > 0 ? 1000L * ezInterval
           : (nodeid >> 6) == RF12_868MHZ ?
           13 * (ezSendLen + 10) : 100);
      rf12_sendStart(RF12_HDR_ACK, ezSendBuf, ezSendLen);
      --ezPending;
    }
  }
  return ezPending ? -1 : 0;
}
/// @details
/// Submit some data bytes to send using the easy transmission mechanism. The
/// data bytes will be copied to an internal buffer since the actual send may
/// take place later than specified, and may need to be re-transmitted in case
/// packets are lost of damaged in transit.
///
/// Packets will be sent no faster than the rate specified in the
/// rf12_easyInit() call, even if called more often.
///
/// Only packets which differ from the previous packet will actually be sent.
/// To force re-transmission even if the data hasn't changed, call
/// "rf12_easySend(0,0)". This can be used to give a "sign of life" every once
/// in a while, and to recover when the receiving node has been rebooted and no
/// longer has the previous data.
///
/// The return value indicates whether a new packet transmission will be started
/// (1), or the data is the same as before and no send is needed (0).
///
/// Note that you also have to call rf12_easyPoll periodically, because it keeps
/// the RFM12B logic going. If you don't, rf12_easySend() will never send out
/// any packets.
/// @note To be used in combination with rf12_easyInit() and rf12_easyPoll().
char rf12_easySend (const void* data, uint8_t size) {
  if (data != 0 && size != 0) {
    if (ezNextSend[0] == 0 && size == ezSendLen &&
        memcmp(ezSendBuf, data, size) == 0)
      return 0;
    memcpy(ezSendBuf, data, size);
    ezSendLen = size;
  }
  ezPending = RETRIES;
  return 1;
}
/// @details
/// When receiving data from other RFM12B/RFM12/RFM01 based units (Fine Offset
/// weather stations, EMR power measurement plugs etc) is is convenient to let
/// the RF12 driver handle HW interfacing but not use it's data protocol.
/// Setting a fixed packet len for reception using this function disables the
/// protocol handling when receiving data.
/// Only the global variable
/// * volatile byte rf12_data - A pointer to the received data.
/// will contain useful data when rf12_recvDone() returns success
/// The buffer will contain fixed_pkt_len bytes of data to interpreted in
/// whatever way is appropriate.
/// Setting fixed_pkt_len to 0 (the default) returns to normal protocol behaviour.
///
/// Normal use in a "bridge" JeeNode would be (in a loop):
/// rf12_initialize(...);
/// rf12_control(...); Whatever needed to match sender
/// rf12_setRawRecvMode(...);
/// while (!rf12_recvDone())
/// ;
/// ... interpret data ...
/// rf12_setRawRecvMode(0);
/// rf12_initialize(...);
/// while (!rf12_canSend())
/// ;
/// rf12_sendStart(...);
/// ... etc, ACKs or whatever ...
void rf12_setRawRecvMode(uint8_t fixed_pkt_len) {
  rf12_fixed_pkt_len = fixed_pkt_len > RF_MAX ? RF_MAX : fixed_pkt_len;
}
