/***************************************************
  Neo Pixel Christma Tree - CC3000 Version
    
  Light up a tree with all the colors of the holidays!
  Control the color, pattern, size, and speed of animation of a
  strip of neo pixels through a web page.
  
  See the Adafruit learning system guide for more details
  and usage information:
  
  Dependencies:
  - Adafruit CC3000 Library 
    https://github.com/adafruit/Adafruit_CC3000_Library
  - Neo Pixel Library
    https://github.com/adafruit/Adafruit_NeoPixel
  
  License:
 
  This example is copyright (c) 2013 Tony DiCola (tony@tonydicola.com)
  and is released under an open source MIT license.  See details at:
    http://opensource.org/licenses/MIT
  
  This code was adapted from Adafruit CC3000 library example 
  code which has the following license:
  
  Designed specifically to work with the Adafruit WiFi products:
  ----> https://www.adafruit.com/products/1469

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
  
  Code adapté par et pour Au Fil Elec pour les besoins de l'entreprise
 ****************************************************/

#include <Adafruit_CC3000.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include "utility/debug.h"
#include "utility/socket.h"


// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   2  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  7
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an MEGA 2560, SCK = 20, MISO = 22, and MOSI = 21
#define DEBUG

Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT); // , SPI_CLOCK_DIVIDER, &Serial

#define WLAN_SSID       "Your-SSID"   // cannot be longer than 32 characters!
#define WLAN_PASS       "Your-KEY-PASS"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define LISTEN_PORT           80      // What TCP port to listen on for connections.  
                                      // The HTTP protocol uses port 80 by default.

#define MAX_ACTION            10      // Maximum length of the HTTP action that can be parsed.

#define MAX_PATH              64      // Maximum length of the HTTP request path that can be parsed.
                                      // There isn't much memory available so keep this short!

#define MAX_TYPE              10

#define MAX_CALLBACK          10

#define BUFFER_SIZE           MAX_ACTION + MAX_PATH + 20  // Size of buffer for incoming request data.
                                                          // Since only the first line is parsed this
                                                          // needs to be as large as the maximum action
                                                          // and path plus a little for whitespace and
                                                          // HTTP version.

#define TIMEOUT_MS            500    // Amount of time in milliseconds to wait for
                                     // an incoming request to finish.  Don't set this
                                     // too high or your server could be slow to respond.

// Neo pixel configuration
#define     PIXEL_PIN              4    // The pin which is connected to the neo pixel strip input.
#define     PIXEL_COUNT            60   // The number of neo pixels in the strip.

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_CC3000_Server httpServer(LISTEN_PORT);

// Color scheme definitions.
struct Color {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  
  Color(uint8_t red, uint8_t green, uint8_t blue): red(red), green(green), blue(blue) {}
  Color(): red(0), green(0), blue(0) {}
};

struct ColorScheme {
  Color* colors;
  uint8_t count;
 
  ColorScheme(Color* colors, uint8_t count): colors(colors), count(count) {} 
};

Color rgbColors[3] = { Color(255, 0, 0), Color(0, 255, 0), Color(0, 0, 255) };
ColorScheme rgb(rgbColors, 3);

Color christmasColors[2] = { Color(255, 0, 0), Color(0, 255, 0) };
ColorScheme christmas(christmasColors, 2);

Color hanukkahColors[2] = { Color(0, 0, 255), Color(255, 255, 255) };
ColorScheme hanukkah(hanukkahColors, 2);

Color kwanzaaColors[3] = { Color(255, 0, 0), Color(0, 0, 0), Color(0, 255, 0) };
ColorScheme kwanzaa(kwanzaaColors, 3);

Color rainbowColors[7] = { Color(255, 0, 0), Color(255, 128, 0), Color(255, 255, 0), Color(0, 255, 0), Color(0, 0, 255), Color(128, 0, 255), Color(255, 0, 255) };
ColorScheme rainbow(rainbowColors, 7);

Color incandescentColors[2] = { Color(255, 140, 20), Color(0, 0, 0) };
ColorScheme incandescent(incandescentColors, 2);

Color fireColors[3] = { Color(255, 0, 0), Color(255, 102, 0), Color(255, 192, 0) };
ColorScheme fire(fireColors, 3);

ColorScheme schemes[7] = { incandescent, rgb, christmas, hanukkah, kwanzaa, rainbow, fire };

// Enumeration of possible pattern types.
enum Pattern { BARS = 0, GRADIENT };

// Enumeration of possible animation types.
enum Animation { RAINBOWS = 2, RAINBOW_CYCLE, COLOR_WIPE, THEATER_CHASE, THEATER_CHASE_RAINBOW, RANDOM };

// Bar width values (in number of pixels/lights) for different size options.
int barWidthValues[3] = { 1,      // Small
                          3,      // Medium
                          6  };   // Large

// Gradient width values (in number of gradient repetitions, i.e. more repetitions equals a smaller gradient) for different size options.
int gradientWidthValues[3] = { 12,     // Small
                               6,      // Medium
                               2   };  // Large

// Speed values in amount of milliseconds to move one pixel/light.  Zero means no movement.
int speedValues[4] = { 0,       // None
                       500,     // Slow
                       250,     // Medium
                       50   };  // Fast

// Variables to hold current state.
int currentScheme = 0;
Pattern currentPattern = GRADIENT;
Animation currentAnimation = RANDOM;
int currentWidth = 0;
int currentSpeed = 20;
bool isAnimation = true;
int counter = 0; // Compteur pour la mise a jour du random
bool pause = false;

uint8_t buffer[BUFFER_SIZE+1];
int bufindex = 0;
char action[MAX_ACTION+1];
char path[MAX_PATH+1];
char type[MAX_TYPE+1];
char value[5];
char callback[MAX_CALLBACK+1];
char response[100];

void setup() {
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n")); 

  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  
  // Initialise the module
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

  // Display the IP address DNS, Gateway, etc.
  while (! displayConnectionDetails()) {
    delay(1000);
  }
  
  // Initialize the neo pixel strip.
  strip.begin();
  strip.show();
  // Initialise random numbers
  randomSeed(analogRead(A15));

  // ******************************************************
  // You can safely remove this to save some flash memory!
  // ******************************************************
  Serial.println(F("\r\nNOTE: This sketch may cause problems with other sketches"));
  Serial.println(F("since the .disconnect() function is never called, so the"));
  Serial.println(F("AP may refuse connection requests from the CC3000 until a"));
  Serial.println(F("timeout period passes.  This is normal behaviour since"));
  Serial.println(F("there isn't an obvious moment to disconnect with a server.\r\n"));
  
  // Start listening for connections
  httpServer.begin();
  
  Serial.println(F("Listening for connections..."));
}

// Compute the color of a pixel at position i using a gradient of the color scheme.  
// This function is used internally by the gradient function.
struct Color gradientColor(struct ColorScheme& scheme, int range, int gradRange, int i) {
  //wdt_enable(WDTO_1S);
  int curRange = i / range;
  int rangeIndex = i % range;
  int colorIndex = rangeIndex / gradRange;
  int start = colorIndex;
  int end = colorIndex+1;
  if (curRange % 2 != 0) {
    start = (scheme.count-1) - start;
    end = (scheme.count-1) - end;
  }
  return  Color(map(rangeIndex % gradRange, 0, gradRange, scheme.colors[start].red,   scheme.colors[end].red),
                map(rangeIndex % gradRange, 0, gradRange, scheme.colors[start].green, scheme.colors[end].green),
                map(rangeIndex % gradRange, 0, gradRange, scheme.colors[start].blue,  scheme.colors[end].blue)); 
}

// Display a gradient of colors for the provided color scheme.
// Repeat is the number of repetitions of the gradient (pick a multiple of 2 for smooth looping of the gradient).
// SpeedMS is the number of milliseconds it takes for the gradient to move one pixel.  Set to zero for no animation.
void gradient(struct ColorScheme& scheme, int repeat = 1, int speedMS = 1000) {
  //wdt_enable(WDTO_1S);
  if (scheme.count < 2) return;
  
  int range = (int)ceil((float)PIXEL_COUNT / (float)repeat);
  int gradRange = (int)ceil((float)range / (float)(scheme.count - 1));
  
  unsigned long time = millis();
  int offset = speedMS > 0 ? time / speedMS : 0;

  Color oldColor = gradientColor(scheme, range, gradRange, PIXEL_COUNT-1+offset);
  Color currentColor;
  int rapport = time % speedMS; 
  for (int i = 0; i < PIXEL_COUNT; ++i) {
    currentColor = gradientColor(scheme, range, gradRange, i+offset);
    if (speedMS > 0) {
      // Blend old and current color based on time for smooth movement.
      strip.setPixelColor(i, map(rapport, 0, speedMS, oldColor.red,   currentColor.red),
                             map(rapport, 0, speedMS, oldColor.green, currentColor.green),
                             map(rapport, 0, speedMS, oldColor.blue,  currentColor.blue));
    }
    else {
      // No animation, just use the current color. 
      strip.setPixelColor(i, currentColor.red, currentColor.green, currentColor.blue);
    }
    oldColor = currentColor; 
  }
  strip.show(); 
}

// Display solid bars of color for the provided color scheme.
// Width is the width of each bar in pixels/lights.
// SpeedMS is number of milliseconds it takes for the bars to move one pixel.  Set to zero for no animation.
void bars(struct ColorScheme& scheme, int width = 1, int speedMS = 1000) {
  //wdt_enable(WDTO_1S);
  int maxSize = PIXEL_COUNT / scheme.count;
  if (width > maxSize) return;
  
  int offset = speedMS > 0 ? millis() / speedMS : 0;
  int colorIndex = 0;
  
  for (uint8_t i = 0; i < PIXEL_COUNT; ++i) {
    colorIndex = ((i + offset) % (scheme.count * width)) / width;
    strip.setPixelColor(i, scheme.colors[colorIndex].red, scheme.colors[colorIndex].green, scheme.colors[colorIndex].blue);
  }
  strip.show(); 
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  #ifdef DEBUG
    Serial.print(F("colorWipe wait: ")); Serial.println(wait);
  #endif
  
  for(uint8_t i=0; i < PIXEL_COUNT; i++) {
    strip.setPixelColor(i, c);
    strip.show();
    if (checkServer()) {
      return;
    }
    delay(wait);
  }
}

void rainbows(uint8_t wait) {
  uint16_t i, j;
  
  #ifdef DEBUG
    Serial.print(F("rainbow wait: ")); Serial.println(wait);
  #endif
  
  for(j=0; j<256; j++) {
    for(i=0; i<PIXEL_COUNT; i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    if (checkServer()) {
      return;
    }
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j, numbers;
  numbers = 256 * 5;
  
  #ifdef DEBUG
    Serial.print(F("rainbowCycle wait: ")); Serial.println(wait);
  #endif

  for(j=0; j < numbers; j++) { // 5 cycles of all colors on wheel
    for(i=0; i < PIXEL_COUNT; i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / PIXEL_COUNT) + j) & 255));
    }
    strip.show();
    if (checkServer()) {
      return;
    }
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  #ifdef DEBUG
    Serial.print(F("theaterChase wait: ")); Serial.println(wait);
  #endif
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < PIXEL_COUNT; i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      if (checkServer()) {
        return;
      }
      delay(wait*2.5);

      for (int i=0; i < PIXEL_COUNT; i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  #ifdef DEBUG
    Serial.print(F("theaterChaseRainbow wait: ")); Serial.println(wait);
  #endif
  
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (int i=0; i < PIXEL_COUNT; i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();
  
      if (checkServer()) {
        return;
      }
      delay(wait*2.5);

      for (int i=0; i < PIXEL_COUNT; i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void set_speed(uint8_t wait) {
  if (!isAnimation) {
    if (wait > 3) {
      wait = uint8_t(wait / 33.33);
    }
    currentSpeed = speedValues[constrain(wait, 0, 3)];
  } else {
    currentSpeed = constrain(wait, 0, 1023);
  }
  #ifdef DEBUG
    Serial.print(F("CurrentSpeed: ")); Serial.println(currentSpeed);
  #endif
}

bool checkServer() {
  return httpServer.available().available();
}

void togglePause(void) {
  pause = !pause;
  
  if (pause) {
    for(uint8_t i=0; i < PIXEL_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
      strip.show();
    }
  }
}

void loop() {
  //wdt_enable(WDTO_2S);
  // Try to get a client which is connected.
  Adafruit_CC3000_ClientRef client = httpServer.available();
  if (client) {
    Serial.println(F("Client connected."));
    // Process this request until it completes or times out.
    // Note that this is explicitly limited to handling one request at a time!

    // Clear the incoming data buffer and point to the beginning of it.
    bufindex = 0;
    memset(&buffer, 0, sizeof(buffer));
    
    // Clear action and path strings.
    memset(&action,   0, sizeof(action));
    memset(&path,     0, sizeof(path));
    memset(&type,     0, sizeof(type));
    memset(&value,    0, sizeof(value));
    memset(&callback, 0, sizeof(callback));
    memset(&response, 0, sizeof(response));

    // Set a timeout for reading all the incoming data.
    unsigned long endtime = millis() + TIMEOUT_MS;
    
    // Read all the incoming data until it can be parsed or the timeout expires.
    bool parsed = false;
    while (!parsed && (millis() < endtime) && (bufindex < BUFFER_SIZE)) {
      if (client.available()) {
        buffer[bufindex++] = client.read();
      }
      parsed = parseRequest(buffer, bufindex, action, path, type, value, callback);
    }
    
    // Handle the request if it was parsed.
    if (parsed) {
      #ifdef DEBUG
        Serial.println(F("Processing request"));
        Serial.print(F("Action: ")); Serial.println(action);
        Serial.print(F("Path: ")); Serial.println(path);
        Serial.print(F("Type: ")); Serial.println(type);
        Serial.print(F("Value: ")); Serial.println(value);
        Serial.print(F("Callback: ")); Serial.println(callback);
        Serial.println(F(""));
      #endif
      // Check the action to see if it was a GET request.
      if (strcmp(action, "GET") == 0) {
        // Parse a single digit value.
        int val = atoi(value);
        // Update appropriate state for the associated command and value.
        if (strcmp(type, "scheme") == 0) {
          currentScheme = constrain(val, 0, 6);
          currentPattern = GRADIENT;
          isAnimation = false;
          set_speed(currentSpeed);
          #ifdef DEBUG
            PRINT_DEBUG("currentScheme: ", value);
          #endif
        }
        else if (strcmp(type, "pattern") == 0) {
          currentPattern = (Pattern)constrain(val, 0, 1);
          isAnimation = false;
          set_speed(currentSpeed);
          #ifdef DEBUG
            PRINT_DEBUG("currentPattern: ", value);
          #endif
        }
        else if (strcmp(type, "animation") == 0) {
          currentAnimation = (Animation)constrain(val, 2, 7);
          isAnimation = true;
          #ifdef DEBUG
            PRINT_DEBUG("Animation: ", value);
          #endif
        }
        else if (strcmp(type, "width") == 0) {
          currentWidth = constrain(val, 0, 2);
          #ifdef DEBUG
            PRINT_DEBUG("currentWidth: ", value);
          #endif
        }
        else if (strcmp(type, "speed") == 0) {
          set_speed(val);
          #ifdef DEBUG
            PRINT_DEBUG("currentSpeed: ", value);
          #endif
        }
        else if (strcmp(type, "pause") == 0) {
          togglePause();
          #ifdef DEBUG
            Serial.println(F("Toggle pause"));
          #endif
        }
        if (pause && strcmp(type, "pause") != 0 && strcmp(type, "status") != 0) {
          togglePause();
          #ifdef DEBUG
            Serial.println(F("Toggle pause auto"));
          #endif
        }
        
        if (callback != NULL) {
          if (!isAnimation) {
            sprintf(response, "%s({\"scheme\":%d,\"pattern\":%d,\"width\":%d,\"speed\":%d})", 
                    callback, currentScheme, currentPattern, currentWidth, currentSpeed);
          } else {
            sprintf(response, "%s({\"animation\":%d,\"speed\":%d})", 
                    callback, currentAnimation, currentSpeed);
          }
        } else {
          if (!isAnimation) {
            sprintf(response, "{\"scheme\":%d,\"pattern\":%d,\"width\":%d,\"speed\":%d}",
                    currentScheme, currentPattern, currentWidth, currentSpeed);
          } else {
            sprintf(response, "{\"animation\":%d,\"speed\":%d}",
                    currentAnimation, currentSpeed);
          }
        }
        char resLength[15];
        sprintf(resLength, "%d", strlen(response));
        
        // Respond with the path that was accessed.
        // First send the success response code.
        client.fastrprintln(F("HTTP/1.1 200 OK"));
        // Then send a few headers to identify the type of data returned and that
        // the connection will not be held open.
        client.fastrprintln(F("Content-Type: application/json"));
        client.fastrprintln(F("Connection: close"));
        client.fastrprintln(F("Server: Adafruit CC3000"));
        client.fastrprint(F("Content-Length: ")); client.fastrprintln(resLength);
        // Send an empty line to signal start of body.
        client.fastrprintln(F(""));
        // Now send the response data.
        client.fastrprintln(response);
        
        #ifdef DEBUG
          PRINT_DEBUG("Response: ", response);
        #endif
      }
      else {
        // Unsupported action, respond with an HTTP 405 method not allowed error.
        client.fastrprintln(F("HTTP/1.1 405 Method Not Allowed"));
        client.fastrprintln(F("Connection: close"));
        client.fastrprintln(F("Server: Adafruit CC3000"));
        client.fastrprintln(F(""));
      }
    } else {
      // Unsupported action, respond with an HTTP 405 method not allowed error.
      client.fastrprintln(F("HTTP/1.1 412 Precondition Failed"));
      client.fastrprintln(F("Connection: close"));
      client.fastrprintln(F("Server: Adafruit CC3000"));
      client.fastrprintln(F(""));
    }

    // Wait a short period to make sure the response had time to send before
    // the connection is closed (the CC3000 sends data asyncronously).
    delay(200);

    // Close the connection when done.
    Serial.println(F("Client disconnected"));
    client.close();
  }
  if (!pause) {
    // Update pixels based on current state.
    if (!isAnimation) {
      if (currentPattern == BARS) {
        bars(schemes[currentScheme], barWidthValues[currentWidth], currentSpeed);
      } else if (currentPattern == GRADIENT) {
        gradient(schemes[currentScheme], gradientWidthValues[currentWidth], currentSpeed);
      }
    } else {
      if (currentAnimation == RANDOM) {
        // Some example procedures showing how to display to the pixels:
        colorWipe(strip.Color(255, 0, 0), 50); // Red
        colorWipe(strip.Color(0, 255, 0), 50); // Green
        colorWipe(strip.Color(0, 0, 255), 50); // Blue
        // Send a theater pixel chase in...
        theaterChase(strip.Color(127, 127, 127), 50); // White
        theaterChase(strip.Color(127, 0, 0), 50); // Red
        theaterChase(strip.Color(0, 0, 127), 50); // Blue
      
        rainbows(20);
        rainbowCycle(20);
        theaterChaseRainbow(50);
      } else {
        switch(currentAnimation) {
          case RAINBOWS:
            rainbows(currentSpeed);
            break;
          case RAINBOW_CYCLE:
            rainbowCycle(currentSpeed);
            break;
          case COLOR_WIPE:
            colorWipe(Wheel(randomByte()), currentSpeed);
            break;
          case THEATER_CHASE:
            theaterChase(Wheel(randomByte()), currentSpeed); // Blue
            break;
          case THEATER_CHASE_RAINBOW:
            theaterChaseRainbow(currentSpeed);
            break;
        }
      }
    }
  }
}

uint8_t randomByte(void) {
  uint8_t rand = random(256);
  if (counter < 1000) counter++;
  else {
    randomSeed(analogRead(A15));
    counter = 0;
    #ifdef DEBUG
      Serial.println(F("Counter reset"));
    #endif
  }
  return rand;
}

// Return true if the buffer contains an HTTP request.  Also returns the request
// path and action strings if the request was parsed.  This does not attempt to
// parse any HTTP headers because there really isn't enough memory to process
// them all.
// HTTP request looks like:
//  [method] [path] [version] \r\n
//  Header_key_1: Header_value_1 \r\n
//  ...
//  Header_key_n: Header_value_n \r\n
//  \r\n
bool parseRequest(uint8_t* buf, int bufSize, char* action, char* path, char* type, char* value, char* callback) {
  //wdt_enable(WDTO_1S);
  // Check if the request ends with \r\n to signal end of first line.
  if (bufSize < 2)
    return false;
  if (buf[bufSize-2] == '\r' && buf[bufSize-1] == '\n') {
    return parseFirstLine((char*)buf, action, path, type, value, callback);
  }
  return false;
}

// Parse the action and path from the first line of an HTTP request.
bool parseFirstLine(char* line, char* action, char* path, char* type, char* value, char* callback) {
  PRINT_DEBUG("Line; ", line);
  // Parse first word up to whitespace as action.
  char* lineaction = strtok(line, " ");
  if (lineaction == NULL) {
    PRINT_DEBUG("Action not found", "");
    return false;
  }
  strncpy(action, lineaction, MAX_ACTION);
  
  // Parse second word up to whitespace as path.
  char* linepath = strtok(NULL, " ");
  if (linepath != NULL) {
    strncpy(path, linepath, MAX_PATH);
    parsePath(path, type, value, callback);
  } else {
    PRINT_DEBUG("Path not found", "");
    return false;
  }
}

bool parsePath(const char* line, char* type, char* value, char* callback) {
  PRINT_DEBUG("Path; ", (char*)line);
  char tmp[MAX_PATH];
  strcpy(tmp, line);
  char* keyword = strtok(tmp, "/");
  if (keyword == NULL || strncmp(keyword, "arduino", 7) != 0) {
    PRINT_DEBUG("Keyword not found or invalid", keyword);
    return false;
  }
  char* linetype = strtok(NULL, "/");
  if (linetype == NULL) {
    PRINT_DEBUG("Type is NULL", "");
    return false;
  }
  strncpy(type, linetype, MAX_TYPE);
  
  char* linevalue = strtok(NULL, "/");
  PRINT_DEBUG("LineValue1: ", linevalue);
  if (linevalue == NULL) {
    PRINT_DEBUG("Value is NULL", "");
    return false;
  }
  linevalue = strtok(linevalue, "?");
  PRINT_DEBUG("LineValue1: ", linevalue);
  if (linevalue == NULL) {
    PRINT_DEBUG("Value is NULL", "");
    strcpy(value, "0");
  } else {
    strncpy(value, linevalue, 5);
  }
  
  char* lineParams = strtok(NULL, "?");
  if (lineParams != NULL)
    parseParameters(lineParams, callback);

  return true;
}

void parseParameters(const char* lineParams, char* callback) {
  PRINT_DEBUG("Parameters: ", (char*)lineParams);
  char tmp[MAX_PATH];
  strcpy(tmp, lineParams);
  
  char* param = strtok(tmp, "&=");
  
  while (param != NULL) {
    if (param != NULL && strncmp(param, "callback", 8) == 0) {
      strcpy(callback, strtok(NULL, "&="));
      PRINT_DEBUG("Callback found: ", callback);
      break;
    }
    param = strtok(NULL, "&=");
  }
}

void PRINT_DEBUG(const char* key, char* fstr)
{
  #ifdef DEBUG
    if(!key) return;
    Serial.print(key);
    Serial.println(fstr);
  #endif
}

// Tries to read the IP address and other connection details
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}
