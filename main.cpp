#include <Arduino.h>
#include <RadioLib.h>
#include "boards.h"

#define PACKET_LENGTH 32

SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool wasTx = false;
bool wasRx = false;
uint8_t txSerialNumber = 0;
uint8_t message[PACKET_LENGTH];
unsigned long lastTx = 0;

void debugOutput(String lineOne, String lineTwo = "", String lineThree = "", String lineFour = "", String lineFive = "")
{
#ifdef HAS_DISPLAY
      if (u8g2)
      {
        u8g2->setFont(u8g2_font_6x10_tf);
        u8g2->clearBuffer();
        u8g2->drawStr(0, 10, lineOne.c_str());
        u8g2->drawStr(0, 20, lineTwo.c_str());
        u8g2->drawStr(0, 30, lineThree.c_str());
        u8g2->drawStr(0, 40, lineFour.c_str());
        u8g2->drawStr(0, 50, lineFive.c_str());
        u8g2->sendBuffer();
      }
#endif
}

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // we sent or received  packet, set the flag
  operationDone = true;
}

bool sendPacket()
{
  // See if it has been one second since the last packet was sent
  if (millis() - lastTx < 1000)
  {
    return false;
  }

  lastTx = millis();
  txSerialNumber++;

  for (int i = 0; i < PACKET_LENGTH; i++)
  {
    message[i] = txSerialNumber;
  }

  radio.startTransmit(message, PACKET_LENGTH);
  debugOutput("Sending packet.", std::to_string(txSerialNumber).c_str());

  return true;
}

void setup()
{
  // put your setup code here, to run once:
  // Serial.begin(9600);
  initBoard();

  // When the power is turned on, a delay is required.
  delay(1500);


  debugOutput("Starting up.", "Will bring up radio.");
  delay(1000);
  // initialize SX1280 with default settings
  //Serial.print(F("[SX1280] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE)
  {
    debugOutput("Radio up.");
  }
  else
  {
    debugOutput("Failed!", "Radio not online.");
    while (true)
      ;
  }

  // set output power to 3 dBm    !!Cannot be greater than 3dbm!!
  if (radio.setFrequency(2445.0) == RADIOLIB_ERR_INVALID_OUTPUT_POWER)
  {
    debugOutput("Failed!", "Freq. not set.");
    while (true)
      ;
  }

  // set output power to 3 dBm    !!Cannot be greater than 3dbm!!
  if (radio.setOutputPower(3) == RADIOLIB_ERR_INVALID_OUTPUT_POWER)
  {
    debugOutput("Failed!", "Invalid power level.");
    while (true)
      ;
  }


  if (radio.setHighSensitivityMode(true) != RADIOLIB_ERR_NONE)
  {
    debugOutput("Failed!", "Sensitivity mode error.");
    while (true)
      ;
  }


  if (radio.setGainControl(0) != RADIOLIB_ERR_NONE)
  {
    debugOutput("Failed!", "Didn't set gain.");
    while (true)
      ;
  }

  // set spreading factor
  if (radio.setSpreadingFactor(7) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
      debugOutput("Failed!", "Bad spreading factor.");
      while (true);
  }

  if (radio.setCodingRate(7) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
      debugOutput("Failed!", "Bad coding rate.");
      while (true);
  }


  if (radio.setBandwidth(812.5) != RADIOLIB_ERR_NONE) {
    debugOutput("Failed!", "Bad bandwidth.");
    while (true);
  }
  // set the function that will be called
  // when packet transmission is finished
  radio.setDio1Action(setFlag);

  // start listening for LoRa packets
  debugOutput("Will start receiving.");
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE)
  {
    debugOutput("Ok.", "Waiting for packets.");
  }
  else
  {
    debugOutput("Error!", "Not receiving.");
    while (true);
  }
}


void loop()
{
  if (operationDone && wasTx)
  {
    operationDone = false;
    wasTx = false;

    if (transmissionState == RADIOLIB_ERR_NONE)
    {
      debugOutput("TX Complete.", std::to_string(txSerialNumber).c_str());
      Serial.write(0x00);
      Serial.write(0x00);
      Serial.write(0x00);
      Serial.write(txSerialNumber);
      Serial.write(0x01);
      Serial.write(0xff);
      Serial.write(0xff);
      Serial.write(0xff);
    }
    else
    {
      debugOutput("TX Error", ("Error Code: " + std::to_string(transmissionState)).c_str());
      Serial.write(0x00);
      Serial.write(0x00);
      Serial.write(0x00);
      Serial.write(txSerialNumber);
      Serial.write(0x02);
      Serial.write(0xff);
      Serial.write(0xff);
      Serial.write(0xff);
    }
  }
  else if (operationDone && wasRx)
  {
    operationDone = false;
    wasRx = false;

    uint8_t buffer[32];
    int state = radio.readData(buffer, 32);
    if (state == RADIOLIB_ERR_NONE)
    {
      char packetLengthBuf[32];
      uint8_t packetLength = radio.getPacketLength();
      snprintf(packetLengthBuf, sizeof(packetLengthBuf), "Size: %u", packetLength);

      char rssiBuf[32];
      snprintf(rssiBuf, sizeof(rssiBuf), "RSSI:%.2f dBm", radio.getRSSI());

      char snrBuf[32];
      snprintf(snrBuf, sizeof(snrBuf), "SNR:%.2f dB", radio.getSNR());

      if (packetLength == 32)
      {
        debugOutput("Received packet.", packetLengthBuf, rssiBuf, snrBuf, "Forwarding packet.");
        Serial.write(0x00);
        Serial.write(0x00);
        Serial.write(0x00);
        Serial.write(buffer, 32);
        Serial.write(0xff);
        Serial.write(0xff);
        Serial.write(0xff);
        delay(500);
      }
      else
      {
        debugOutput("Received packet.", packetLengthBuf, rssiBuf, snrBuf, "Wrong length.");
      }
    }
    else if (state == RADIOLIB_ERR_CRC_MISMATCH)
    {
      // packet was received, but is malformed
      debugOutput("Bad packet.");
    }
    else
    {
      // some other error occurred
      debugOutput("Error receiving.");
    }
  }

  if (!wasTx && sendPacket())
  {
    wasRx = false;
    wasTx = true;
    transmissionState = radio.startTransmit(message, PACKET_LENGTH);
    debugOutput("Transmitting.");
  }

  if (!wasTx && !wasRx)
  {
    wasRx = true;
    radio.startReceive();
    debugOutput("Listening.", ("Last TX: " + std::to_string(txSerialNumber)).c_str());
  }
}
