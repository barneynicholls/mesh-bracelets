#include <Arduino.h>

const int ledPin = 2; // Onboard LED is connected to GPIO2

#include <FastLED.h>

#define LED_PIN D4
#define NUM_LEDS 10
#define BRIGHTNESS 64
#define LED_TYPE WS2812B
#define COLOR_ORDER RGB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

uint8_t lastSecond = 99;
uint8_t lastSecondHand = 99;
uint8_t startIndex = 0;

CRGBPalette16 currentPalette;
TBlendType currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

#include "painlessMesh.h"

#define MESH_PREFIX "Mesh_username"
#define MESH_PASSWORD "mesh_password"
#define MESH_PORT 5555

bool isController = false; // flag to designate that this is the current controller

Scheduler userScheduler;
painlessMesh mesh;

void sendmsg(int newPallette)
{
  Serial.printf("Broadcasting Pallette: %d", newPallette);
  Serial.println();
  mesh.sendBroadcast(String(newPallette));
}

void SetupTotallyRandomPalette()
{
  for (int i = 0; i < 16; ++i)
  {
    currentPalette[i] = CHSV(random8(), 255, random8());
  }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid(currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
  CRGB purple = CHSV(HUE_PURPLE, 255, 255);
  CRGB green = CHSV(HUE_GREEN, 255, 255);
  CRGB black = CRGB::Black;

  currentPalette = CRGBPalette16(
      green, green, black, black,
      purple, purple, black, black,
      green, green, black, black,
      purple, purple, black, black);
}

void SwitchPallete(uint8_t pallette)
{
  Serial.printf("Switching to Pallette: %d", pallette);
  Serial.println();
  
  switch (pallette)
  {
  case 0:
    currentPalette = RainbowColors_p;
    currentBlending = LINEARBLEND;
    break;
  case 1:
    currentPalette = RainbowStripeColors_p;
    currentBlending = NOBLEND;
    break;
  case 2:
    currentPalette = RainbowStripeColors_p;
    currentBlending = LINEARBLEND;
    break;
  case 3:
    SetupPurpleAndGreenPalette();
    currentBlending = LINEARBLEND;
    break;
  case 4:
    SetupTotallyRandomPalette();
    currentBlending = LINEARBLEND;
    break;
  case 5:
    SetupBlackAndWhiteStripedPalette();
    currentBlending = NOBLEND;
    break;
  case 6:
    SetupBlackAndWhiteStripedPalette();
    currentBlending = LINEARBLEND;
    break;
  case 7:
    currentPalette = CloudColors_p;
    currentBlending = LINEARBLEND;
    break;
  case 8:
    currentPalette = PartyColors_p;
    currentBlending = LINEARBLEND;
    break;
  case 9:
    currentPalette = myRedWhiteBluePalette_p;
    currentBlending = NOBLEND;
    break;
  case 10:
    currentPalette = myRedWhiteBluePalette_p;
    currentBlending = LINEARBLEND;
    break;
  }

  startIndex = 0;
}

void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("IsCtrl: %d Received from %u Received Pallette=%s\n", isController, from, msg.c_str());
  SwitchPallete(msg.toInt());
}

void newConnectionCallback(uint32_t nodeId)
{
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback()
{
  Serial.printf("changedConnectionCallback\n");
  SimpleList<uint32_t> nodes;
  uint32_t myNodeID = mesh.getNodeId();
  uint32_t lowestNodeID = myNodeID;

  Serial.printf("Changed connections %s\n", mesh.subConnectionJson().c_str());

  nodes = mesh.getNodeList();
  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  for (SimpleList<uint32_t>::iterator node = nodes.begin(); node != nodes.end(); ++node)
  {
    Serial.printf(" %u", *node);
    if (*node < lowestNodeID)
      lowestNodeID = *node;
  }

  Serial.println();

  if (lowestNodeID == myNodeID)
  {
    Serial.printf("Id: %u I am the controller now", myNodeID);
    Serial.println();
    isController = true;
  }
  else
  {
    Serial.printf("Id: %u is the controller now", lowestNodeID);
    Serial.println();
    isController = false;
  }
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void FillLEDsFromPaletteColors(uint8_t colorIndex)
{
  uint8_t brightness = 255;

  for (int i = 0; i < NUM_LEDS; ++i)
  {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}

// This function fills the palette with totally random colors.

void ChangePalettePeriodically()
{
  uint8_t secondHand = (millis() / 1000) % 60;

  if (lastSecond != secondHand)
  {
    lastSecond = secondHand;

    if (secondHand == 0 || secondHand % 5 == 0)
    {
      int newPallette = random(11);
      sendmsg(newPallette);
      SwitchPallete(newPallette);
    }
  }
}

// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
    {
        CRGB::Red,
        CRGB::Gray, // 'white' is too bright compared to red and blue
        CRGB::Blue,
        CRGB::Black,

        CRGB::Red,
        CRGB::Gray,
        CRGB::Blue,
        CRGB::Black,

        CRGB::Red,
        CRGB::Red,
        CRGB::Gray,
        CRGB::Gray,
        CRGB::Blue,
        CRGB::Blue,
        CRGB::Black,
        CRGB::Black};

void setup()
{
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT); // Set the LED pin as an output
  digitalWrite(ledPin, LOW);

  mesh.setDebugMsgTypes(ERROR | STARTUP);

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // userScheduler.addTask(taskSendmsg);
  // taskSendmsg.enable();

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
}

void loop()
{
  mesh.update();
  digitalWrite(ledPin, LOW);

  startIndex = startIndex + 1; /* motion speed */

  FillLEDsFromPaletteColors(startIndex);

  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);

  if (isController)
  {
    ChangePalettePeriodically();
  }
}
