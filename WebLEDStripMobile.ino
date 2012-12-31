// iPhone controlled / Arduino Powered Christmas rgb LED Strip

/* https://github.com/adafruit/LPD8256 */
/* https://github.com/sirleech/Webduino */

/* Web_AjaxRGB_mobile.pde - example sketch for Webduino library */
/* -  offers web-based slider controllers for RGB led  - */

#include "SPI.h"
#include "Ethernet.h"
#include "WebServer.h"
#include "LPD8806.h"

// Number of RGB LEDs in strand:
int nLEDs = 160;

// Chose 2 pins for output; can be any valid output pins:
int dataPin  = 2;
int clockPin = 3;

// First parameter is the number of LEDs in the strand.  The LED strips
// are 32 LEDs per meter but you can extend or cut the strip.  Next two
// parameters are SPI data and clock pins:
LPD8806 strip = LPD8806(nLEDs, dataPin, clockPin);

// You can optionally use hardware SPI for faster writes, just leave out
// the data and clock pin parameters.  But this does limit use to very
// specific pins on the Arduino.  For "classic" Arduinos (Uno, Duemilanove,
// etc.), data = pin 11, clock = pin 13.  For Arduino Mega, data = pin 51,
// clock = pin 52.  For 32u4 Breakout Board+ and Teensy, data = pin B2,
// clock = pin B1.  For Leonardo, this can ONLY be done on the ICSP pins.
//LPD8806 strip = LPD8806(nLEDs);

// LED array:
int led[25][4];
// Current LED:8
int c=0;

// CHANGE THIS TO YOUR OWN UNIQUE VALUE
static uint8_t mac[6] = { 0x02, 0xAA, 0xBB, 0xCC, 0x00, 0x22 };

// CHANGE THIS TO MATCH YOUR HOST NETWORK
static uint8_t ip[4] = { 192, 168, 1, 2 }; // area 51!

/* all URLs on this server will start with /rgb because of how we
 * define the PREFIX value.  We also will listen on port 80, the
 * standard HTTP service port */
#define PREFIX "/rgb"
WebServer webserver(PREFIX, 80);

int red = 0;            //integer for red darkness
int blue = 0;           //integer for blue darkness
int green = 0;          //integer for green darkness

/* This command is set as the default command for the server.  It
 * handles both GET and POST requests.  For a GET, it returns a simple
 * page with some buttons.  For a POST, it saves the value posted to
 * the red/green/blue variable, affecting the output of the speaker */
void rgbCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  if (type == WebServer::POST)
  {
    bool repeat;
    char name[16], value[16];
    do
    {
      /* readPOSTparam returns false when there are no more parameters
       * to read from the input.  We pass in buffers for it to store
       * the name and value strings along with the length of those
       * buffers. */
      repeat = server.readPOSTparam(name, 16, value, 16);

      /* this is a standard string comparison function.  It returns 0
       * when there's an exact match.  We're looking for a parameter
       * named red/green/blue here. */
      if (strcmp(name, "red") == 0)
      {
	/* use the STRing TO Unsigned Long function to turn the string
	 * version of the color strength value into our integer red/green/blue
	 * variable */
        red = strtoul(value, NULL, 10);
      }
      if (strcmp(name, "green") == 0)
      {
        green = strtoul(value, NULL, 10);
      }
      if (strcmp(name, "blue") == 0)
      {
        blue = strtoul(value, NULL, 10);
      }
    } while (repeat);
    
    // after procesing the POST data, tell the web browser to reload
    // the page using a GET method. 
    server.httpSeeOther(PREFIX);
//    Serial.print(name);
//    Serial.println(value);

    return;
  }

  /* for a GET or HEAD, send the standard "it's all OK headers" */
  server.httpSuccess();

  /* we don't output the body for a HEAD request */
  if (type == WebServer::GET)
  {
    /* store the HTML in program memory using the P macro */
    P(message) = 
"<!DOCTYPE html><html><head>"
  "<meta charset=\"utf-8\"><meta name=\"apple-mobile-web-app-capable\" content=\"yes\" /><meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge,chrome=1\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\">"
  "<title>Keeward LED</title>"
  "<link rel=\"stylesheet\" href=\"http://code.jquery.com/mobile/1.0/jquery.mobile-1.0.min.css\" />"
  "<script src=\"http://code.jquery.com/jquery-1.6.4.min.js\"></script>"
  "<script src=\"http://code.jquery.com/mobile/1.0/jquery.mobile-1.0.min.js\"></script>"
  "<style> body, .ui-page { background: black; } .ui-body { padding-bottom: 1.5em; } div.ui-slider { width: 88%; } #red, #green, #blue { display: block; margin: 10px; } #red { background: #f00; } #green { background: #0f0; } #blue { background: #00f; } </style>"
  "<script>"
// causes the Arduino to hang quite frequently (more often than Web_AjaxRGB.pde), probably due to the different event triggering the ajax requests
    "$(document).ready(function(){ $('#red, #green, #blue').slider; $('#red, #green, #blue').bind( 'change', function(event, ui) { jQuery.ajaxSetup({timeout: 110}); /*not to DDoS the Arduino, you might have to change this to some threshold value that fits your setup*/ var id = $(this).attr('id'); var strength = $(this).val(); if (id == 'red') $.post('/rgb', { red: strength } ); if (id == 'green') $.post('/rgb', { green: strength } ); if (id == 'blue') $.post('/rgb', { blue: strength } ); });});"
  "</script>"
"</head>"
"<body>"
  "<div data-role=\"header\" data-position=\"inline\"><h1>KEEWARD led</h1></div>"
    "<div class=\"ui-body ui-body-a\">"
      "<input type=\"range\" name=\"slider\" id=\"red\" value=\"0\" min=\"0\" max=\"127\"  />"
      "<input type=\"range\" name=\"slider\" id=\"green\" value=\"0\" min=\"0\" max=\"127\"  />"
      "<input type=\"range\" name=\"slider\" id=\"blue\" value=\"0\" min=\"0\" max=\"127\"  />"
    "</div>"
  "</body>"
"</html>";

    server.printP(message);
  }
}

void setup()
{
 // Start up the LED strip
  strip.begin();

  for(int i=0;i<25 ;i++) {
    for(int j=0;j<4;j++) {
      led[i][j]=0;
    }
  }

  // Update the strip, to start they are all 'off'
  strip.show();

//  Serial.begin(9600);

  // setup the Ehternet library to talk to the Wiznet board
  Ethernet.begin(mac, ip);

  /* register our default command (activated with the request of
   * http://x.x.x.x/rgb */
  webserver.setDefaultCommand(&rgbCmd);

  /* start the server to wait for connections */
  webserver.begin();
}

void loop()
{
  // process incoming connections one at a time forever
  webserver.processConnection();
  
  //Calculates LED values
  for(int i=0;i<25;i++){
    if(i==c){
      led[i][0]=random(0, strip.numPixels());
      led[i][1]=red;
      led[i][2]=blue;
      led[i][3]=green;
    } else if(((i+1)%10) == c) {
      led[i][1]=0;
      led[i][2]=0;
      led[i][3]=0;     
    } else {
      led[i][1]/=2;
      led[i][2]/=2;
      led[i][3]/=2;  
    }
    // Set LED value:
    strip.setPixelColor(led[i][0],strip.Color(led[i][1],led[i][2],led[i][3]));
  }
  // Refresh LED states:
  strip.show();
  // Next LED:
  c=(c+1) % 25;
}
