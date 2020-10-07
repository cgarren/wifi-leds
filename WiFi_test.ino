#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <StreamString.h>
#include "LPD8806.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma
#ifdef __AVR_ATtiny85__
  #include <avr/power.h>
#endif

#define API_KEY "a2d12422-3227-41b1-9151-5b4523a4671c" // TODO: Change to your sinric API Key. Your API Key is displayed on sinric.com dashboard
#define SSID_NAME "1000Jeff523" // TODO: Change to your Wifi network SSID
#define WIFI_PASSWORD "dongsdisciples" // TODO: Change to your Wifi network password
#define SERVER_URL "iot.sinric.com"
#define SERVER_PORT 80 

#define HEARTBEAT_INTERVAL 300000 // 5 Minutes 

/******************************* LED STUFF ***********************************/

/*IDEAS
 *strobe
  morse code
  police
  red alert
  change colors based om time
  sync to music
  
 */

// Number of RGB LEDs in strand:
int nLEDs = 160;

// Chose 2 pins for output; can be any valid output pins:
int dataPin  = D4;
int clockPin = D5;

byte r = 0, g = 0, b = 0;
int selection;

// First parameter is the number of LEDs in the strand.  The LED strips
// are 32 LEDs per meter but you can extend or cut the strip.  Next two
// parameters are SPI data and clock pins:
LPD8806 strip = LPD8806(nLEDs, dataPin, clockPin);

/*****************************************************************************/

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient client;

uint64_t heartbeatTimestamp = 0;
bool isConnected = false;

void setPowerStateOnServer(String deviceId, String value);
void setTargetTemperatureOnServer(String deviceId, String value, String scale);


void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  /******************************* LED STUFF ***********************************/

  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
  #endif

  // Start up the LED strip
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();

  /*****************************************************************************/
  
  WiFiMulti.addAP(SSID_NAME, WIFI_PASSWORD);
  Serial.println();
  Serial.print("Connecting to Wifi: ");
  Serial.println(SSID_NAME);  

  // Waiting for Wifi connect
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if(WiFiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // server address, port and URL
  webSocket.begin(SERVER_URL, SERVER_PORT, "/");

  // event handler
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", API_KEY);
  
  // try again every 5000ms if connection has failed
  webSocket.setReconnectInterval(5000);   // If you see 'class WebSocketsClient' has no member named 'setReconnectInterval' error update arduinoWebSockets

/************************************************************************************/

}

void loop() {
  webSocket.loop();
  
  if(isConnected) {
      if ((selection >= 2) && (selection <= 10)) {
          everyTen(1, strip.Color(127,0,0), strip.Color(0,0,127));
          
      } else if ((selection >= 11) && (selection <= 20)) {
          //dither(strip.Color(0, 0 , 255), 10);
          
      } else if ((selection >= 21) && (selection <= 30)) {
          //dither(strip.Color(255, 0, 0), 10);
          
      } else if ((selection >= 31) && (selection <= 40)) {
          //dither(strip.Color(0, 255, 0), 10);
          
      } else if ((selection >= 41) && (selection <= 50)) {
          //dither(strip.Color(r, g, b), 10);
          
      } else if ((selection >= 51) && (selection <= 60)) {
          wave(strip.Color(r,g,b), 1, 1);
          
      } else if ((selection >= 61) && (selection <= 70)) {
          scanner(r,g,b, 20);
          
      } else if ((selection >= 71) && (selection <= 80)) {
          fullRainbow(50);
          
      } else if ((selection >= 81) && (selection <= 90)) {
          rainbowCycle(0);
          
      } else if ((selection >= 91) && (selection <= 99)) {
          //uint32_t threeColors[] = {strip.Color(0,0,127) , strip.Color(127,0,0) , strip.Color(127,127,0)};
          //TriColorWave(threeColors, 1, 10);
      } else if (selection == 1) {
          selection = random(2, 99);
          
      }
      
      uint64_t now = millis();
      
      // Send heartbeat in order to avoid disconnections during ISP resetting IPs over night. Thanks @MacSass
      if((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
          heartbeatTimestamp = now;
          webSocket.sendTXT("H");          
      }
  }
}
  
void turnOn(String deviceId) {
  if (deviceId == "5d658e8a1bddb66e363cbf5e") // Device ID of first device
  {  
     Serial.print("Turn on device id: ");
     Serial.println(deviceId);
     digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on
     // Turn all pixels to white
     colorWipe(strip.Color(127,   127,   127), 1);
     setPowerStateOnServer(deviceId, "ON");
  }     
}

void turnOff(String deviceId) {
   if (deviceId == "5d658e8a1bddb66e363cbf5e") // Device ID of first device
   {  
    Serial.print("Turn off Device ID: ");
    Serial.println(deviceId);
    digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED off
    // Turn all pixels off
    colorWipe(strip.Color(0,   0,   0), 1);
    setPowerStateOnServer(deviceId, "OFF");
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      isConnected = false;    
      Serial.printf("[webSocketEvent] Webservice disconnected from server!\n");
      break;
    case WStype_CONNECTED: {
      isConnected = true;
      Serial.printf("[webSocketEvent] Service connected to server at url: %s\n", payload);
      Serial.printf("[webSocketEvent] Waiting for commands from server ...\n");        
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[webSocketEvent] get text: %s\n", payload);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject((char*)payload); 
        String deviceId = json ["deviceId"];     
        String action = json ["action"];
        
        if(action == "action.devices.commands.OnOff") {
            // alexa, turn on tv ==> {"deviceId":"xx","action":"setPowerState","value":"ON"}
            String value = json ["value"];
            if(value == "{\"on\":true}") {
                turnOn(deviceId);
            } else {
                turnOff(deviceId);
            }        
        }
        else if(action == "action.devices.commands.BrightnessAbsolute") {
            // alexa, dim lights  ==>{"deviceId":"xxx","action":"AdjustBrightness","value":-25}
            selection = json ["value"]["brightness"];
        }       
        else if(action == "action.devices.commands.ColorAbsolute") {
           int color = json ["value"]["color"]["spectrumRGB"];
           String newcolor = String(color, HEX);
           Serial.println(newcolor);
           hexToRGB(newcolor);
           //int colorarray[] = {r, g, b};
           Serial.println(r);
           Serial.println(g);
           Serial.println(b);
           colorWipe(strip.Color(r/2, g/2, b/2), 1);  // Red
        }
        else if(action == "IncreaseColorTemperature") {
           //alexa, set the lights softer ==> {"deviceId":"xxx","action":"IncreaseColorTemperature"}
        }
        else if(action == "IncreaseColorTemperature") {
           //alexa, set the lights softer ==> {"deviceId":"xxx","action":"IncreaseColorTemperature"}
        }
        else if(action == "SetColorTemperature") {
           //alexa, set the lights softer ==> {"deviceId":"xxx","action":"SetColorTemperature","value":2200}
        }
      }
      break;
    case WStype_BIN:
      Serial.printf("[webSocketEvent] get binary length: %u\n", length);
      break;
  }
}


// If you are going to use a push button to on/off the switch manually, use this function to update the status on the server
// so it will reflect on Alexa app.
// eg: setPowerStateOnServer("deviceid", "ON")

// Call ONLY If status changed. DO NOT CALL THIS IN loop() and overload the server. 
void setPowerStateOnServer(String deviceId, String value) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["deviceId"] = deviceId;
  root["action"] = "setPowerState";
  root["value"] = value;
  StreamString databuf;
  root.printTo(databuf);
  
  webSocket.sendTXT(databuf);
}

void hexToRGB(String num) {
    char charbuf[8];
    num.toCharArray(charbuf,8);
    long int rgb=strtol(charbuf,0,16); //=>rgb=0x001234FE;
    r=(byte)(rgb>>16);
    g=(byte)(rgb>>8);
    b=(byte)(rgb);
}

/***************************** LED FUNCTIONS *********************************/

void rainbow(uint8_t wait) {
  int i, j;
   
  for (j=0; j < 192; j=j+2) {     // 3 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( (i + j) % 384));
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  
  for (j=0; j < 384 * 1; j++) {     // 5 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel( ((i * 384 / strip.numPixels()) + j) % 384) );
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

void fullRainbow(uint8_t wait) {
  uint16_t i, j;
  
  for (j=0; j < 384 * 1; j++) {     // 5 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel( ((i * 1 / strip.numPixels()) + j) % 384) );
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

void everyTen(uint8_t wait, uint32_t c1, uint32_t c2) {
  uint16_t i,j = 0;
  //for (int q=0; q < 2; q++) {
    for(i=0; i<strip.numPixels(); i++) {
      if (j < 21) {
        strip.setPixelColor(i, c1); // red
      } else {
        strip.setPixelColor(i, c2); // blue
        if (j >= 40) {
          j = j - 40;
        }
      }
      j++;
      delay(wait);
      strip.show();
    }
    // Refresh LED states
    //j = j + 10;
  //}
}

// Fill the dots progressively along the strip.
void colorWipe(uint32_t c, uint8_t wait) {
  int i;

  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

// Chase one dot down the full strip.
void colorChase(uint32_t c, uint8_t wait) {
  int i;

  // Start by turning all pixels off:
  for(i=0; i<strip.numPixels(); i++) strip.setPixelColor(i, 0);

  // Then display one pixel at a time:
  for(i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c); // Set new pixel 'on'
    strip.show();              // Refresh LED states
    strip.setPixelColor(i, 0); // Erase pixel, but don't refresh!
    delay(wait);
  }

  strip.show(); // Refresh to turn off last pixel
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<2; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();
     
      delay(wait);
     
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
  
}// An "ordered dither" fills every pixel in a sequence that looks
// sparkly and almost random, but actually follows a specific order.
void dither(uint32_t c, uint8_t wait) {

  // Determine highest bit needed to represent pixel index
  int hiBit = 0;
  int n = strip.numPixels() - 1;
  for(int bit=1; bit < 0x8000; bit <<= 1) {
    if(n & bit) hiBit = bit;
  }

  int bit, reverse;
  for(int i=0; i<(hiBit << 1); i++) {
    // Reverse the bits in i to create ordered dither:
    reverse = 0;
    for(bit=1; bit <= hiBit; bit <<= 1) {
      reverse <<= 1;
      if(i & bit) reverse |= 1;
    }
    strip.setPixelColor(reverse, c);
    strip.show();
    delay(wait);
  }
  delay(250); // Hold image for 1/4 sec
}

// "Larson scanner" = Cylon/KITT bouncing light effect
void scanner(uint8_t r, uint8_t g, uint8_t b, uint8_t wait) {
  int i, j, pos, dir;

  pos = 0;
  dir = 1;

  for(i=0; i<((strip.numPixels()-1) * 2); i++) {
    // Draw 5 pixels centered on pos.  setPixelColor() will clip
    // any pixels off the ends of the strip, no worries there.
    // we'll make the colors dimmer at the edges for a nice pulse
    // look
    strip.setPixelColor(pos - 7, strip.Color(r/4, g/4, b/4));
    strip.setPixelColor(pos - 6, strip.Color(r/2, g/2, b/2));
    strip.setPixelColor(pos - 5, strip.Color(r, g, b));
    strip.setPixelColor(pos - 4, strip.Color(r, g, b));
    strip.setPixelColor(pos - 3, strip.Color(r, g, b));
    strip.setPixelColor(pos - 2, strip.Color(r, g, b));
    strip.setPixelColor(pos - 1, strip.Color(r, g, b));
    strip.setPixelColor(pos, strip.Color(r, g, b));
    strip.setPixelColor(pos + 1, strip.Color(r, g, b));
    strip.setPixelColor(pos + 2, strip.Color(r, g, b));
    strip.setPixelColor(pos + 3, strip.Color(r, g, b));
    strip.setPixelColor(pos + 4, strip.Color(r, g, b));
    strip.setPixelColor(pos + 5, strip.Color(r, g, b));
    strip.setPixelColor(pos + 6, strip.Color(r/2, g/2, b/2));
    strip.setPixelColor(pos + 7, strip.Color(r/4, g/4, b/4));

    strip.show();
    delay(wait);
    // If we wanted to be sneaky we could erase just the tail end
    // pixel, but it's much easier just to erase the whole thing
    // and draw a new one next time.
    for(j=-2; j<= 2; j++) 
        strip.setPixelColor(pos+j, strip.Color(0,0,0));
    // Bounce off ends of strip
    pos += dir;
    if(pos < 0) {
      pos = 1;
      dir = -dir;
    } else if(pos >= strip.numPixels()-1) {
      pos = strip.numPixels() - 2;
      dir = -dir;
    }
  }
}

// Sine wave effect
#define PI 3.14159265
void wave(uint32_t c, int cycles, uint8_t wait) {
  float y;
  byte  r, g, b, r2, g2, b2;

  // Need to decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 

  for(int x=0; x<(strip.numPixels()*2); x++)
  {
    for(int i=0; i<strip.numPixels(); i++) {
      y = sin(PI * (float)cycles * (float)(x + i) / (float)strip.numPixels());
      if(y >= 0.0) {
        // Peaks of sine wave are white
        y  = 1.0 - y; // Translate Y to 0.0 (top) to 1.0 (center)
        r2 = 127 - (byte)((float)(127 - r) * y);
        g2 = 127 - (byte)((float)(127 - g) * y);
        b2 = 127 - (byte)((float)(127 - b) * y);
      } else {
        // Troughs of sine wave are black
        y += 1.0; // Translate Y to 0.0 (bottom) to 1.0 (center)
        r2 = (byte)((float)r * y);
        g2 = (byte)((float)g * y);
        b2 = (byte)((float)b * y);
      }
      strip.setPixelColor(i, r2, g2, b2);
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 384; j=j+2) {     // cycle all 384 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, Wheel( (i+j) % 384));    //turn every third pixel on
        }
        strip.show();
       
        delay(wait);
       
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
}

void TriColorWave(uint32_t colors[], int cycles, uint16_t wait) {
  float y = 0;
  float z = 0;
  byte  r, g, b, r2, g2, b2, r3, g3, b3, r4, g4, b4;

  // Need to decompose color into its r, g, b elements
  g = (colors[0] >> 16) & 0x7f;
  r = (colors[0] >>  8) & 0x7f;
  b =  colors[0]        & 0x7f; 
  g2 = (colors[1] >> 16) & 0x7f;
  r2 = (colors[1] >>  8) & 0x7f;
  b2 =  colors[1]        & 0x7f; 
  g3 = (colors[2] >> 16) & 0x7f;
  r3 = (colors[2] >>  8) & 0x7f;
  b3 =  colors[2]        & 0x7f;
  
  for(int x=0; x<(nLEDs*cycles); x++)
  {
    for(byte i=0; i<nLEDs; i++) {
      //still need to figure out how to get the number of waves in here
      int B = ((nLEDs-i) + x)%(nLEDs);
      z = ((float)B/nLEDs);
      if(z < 0.33) {
        y  = (z*3);
        r4 = (byte)(r*((float)y)+(r2*(float)(1-y)));
        g4 = (byte)(g*((float)y)+(g2*(float)(1-y)));
        b4 = (byte)(b*((float)y)+(b2*(float)(1-y)));
        strip.setPixelColor(i, r4, g4, b4);
      } else if(z<0.66){
        // Troughs of sine wave are black
        y  = (z-0.33)*3;
        r4 = (byte)(r3*((float)y)+(r*(float)(1-y)));
        g4 = (byte)(g3*((float)y)+(g*(float)(1-y)));
        b4 = (byte)(b3*((float)y)+(b*(float)(1-y)));
        strip.setPixelColor(i, r4, g4, b4);
      }else{
        y = (z-0.66)*3;
        r4 = (byte)(r2*((float)y)+(r3*(float)(1-y)));
        g4 = (byte)(g2*((float)y)+(g3*(float)(1-y)));
        b4 = (byte)(b2*((float)y)+(b3*(float)(1-y)));
        strip.setPixelColor(i, r4, g4, b4);
      }
      strip.show();
    }
    float time = wait/nLEDs;
    if (time > 1) {
      delay(wait/nLEDs);
    }
  }
}

/* Helper functions */

//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g -b - back to r

uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128;   //Red down
      g = WheelPos % 128;      // Green up
      b = 0;                  //blue off
      break; 
    case 1:
      g = 127 - WheelPos % 128;  //green down
      b = WheelPos % 128;      //blue up
      r = 0;                  //red off
      break; 
    case 2:
      b = 127 - WheelPos % 128;  //blue down 
      r = WheelPos % 128;      //red up
      g = 0;                  //green off
      break; 
  }
  return(strip.Color(r,g,b));
}
