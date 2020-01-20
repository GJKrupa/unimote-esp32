#include <Arduino.h>
#include "SPI.h"
#include "TFT_eSPI.h"
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <IRsend.h>
#include <ArduinoJson.h>

#define BUFFER_SIZE 2048

TFT_eSPI tft = TFT_eSPI();
#define TFT_GREY 0x5AEB

const uint16_t kRecvPin = 13;
const uint16_t kIrLed = 15;
IRrecv irrecv(kRecvPin);
IRsend irsend(kIrLed);
decode_results results;

bool recording = false;
static char lineBuffer[BUFFER_SIZE];
static uint16_t lineIndex;

void displayText(const char* const text) {
  tft.fillScreen(TFT_GREY);
  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setTextSize(1);
  tft.println(text);
}

void setup() {
  Serial.begin(115200);
  irrecv.enableIRIn();  // Start the receiver
  irsend.begin();
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();
  Serial.print("IRrecvDemo is now running and waiting for IR message on Pin ");
  Serial.println(kRecvPin);

  tft.init();
  tft.setRotation(1);

  displayText("Ready");
}

void handleInput()
{
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, lineBuffer);

    if (error)
    {
      doc.clear();
      doc["ok"] = false;
      doc["err"] = error.c_str();
      serializeJson(doc, Serial);
      Serial.println();
    }
    else
    {
      const char *action = doc["act"];
      if (action == NULL)
      {
        doc.clear();
        doc["ok"] = false;
        doc["err"] = "Unknown command";
        serializeJson(doc, Serial);
        Serial.println();
      }
      else if (strcmp(action, "rec") == 0)
      {
        doc.clear();
        doc["ok"] = true;
        serializeJson(doc, Serial);
        Serial.println();
        recording = true;
        displayText("Recording...");
      }
      else if (strcmp(action, "tra") == 0)
      {
        displayText("Sending...");
        int playType = doc["typ"];
        uint64_t value = doc["val"];
        uint16_t bits = doc["bit"];
        irsend.send((decode_type_t)playType, value, bits, 3U);
        doc.clear();
        doc["ok"] = true;
        serializeJson(doc, Serial);
        Serial.println();
        displayText("Ready");
      }
      else if (strcmp(action, "can") == 0 && recording)
      {
        doc.clear();
        doc["ok"] = true;
        serializeJson(doc, Serial);
        Serial.println();
        recording = false;
        displayText("Ready");
      }
      else
      {
        doc.clear();
        doc["ok"] = false;
        doc["err"] = "Unknown command";
        serializeJson(doc, Serial);
        Serial.println();
      }
    }
}

void loop() {
  if (Serial.available() > 0)
  {
    int chr = Serial.read();

    if (chr == '\n')
    {
      if (lineIndex >= BUFFER_SIZE)
      {
        Serial.println("{\"err\":\"Buffer overflow\"");
      } 
      else 
      {
        lineBuffer[lineIndex++] = '\0';
        handleInput();
      }
      lineIndex = 0;
    }
    else
    {
      if (lineIndex < BUFFER_SIZE)
      {
        lineBuffer[lineIndex++] = chr;
      }
    }
  }

  if (irrecv.decode(&results)) {
    if (recording)
    {
      if (results.overflow)
      {
        displayText("Overflow!");
      }
      else
      {
        if (results.decode_type == UNKNOWN)
        {
          StaticJsonDocument<1024> doc;
          uint16_t *raw_array = resultToRawArray(&results);
          uint16_t size = getCorrectedRawLength(&results);
          doc["type"] = results.decode_type;
          JsonArray data = doc.createNestedArray("raw");
          for (int n=0; n<size; ++n)
          {
            data[n] = raw_array[n];
            if (n % 100 == 0) {
              yield();
            }
          }
          delete [] raw_array;
          serializeJson(doc, Serial);
          Serial.println();
        }
        else
        {
          StaticJsonDocument<200> doc;
          doc["typ"] = (int)results.decode_type;
          doc["bit"] = results.bits;
          doc["val"] = results.value;
          serializeJson(doc, Serial);
          Serial.println();
        }

        recording = false;
        displayText("Ready");
      }
    }
    irrecv.resume();  // Receive the next value
  }
  yield();
}

