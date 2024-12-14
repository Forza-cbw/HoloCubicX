#include <Arduino.h>

/*  
 Test the tft.print() viz embedded tft.write() function

 This sketch used font 2, 4, 7

 Make sure all the display driver and pin connections are correct by
 editing the User_Setup.h file in the TFT_eSPI library folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
 #########################################################################
 */


#include <TFT_eSPI.h> // Graphics and font library for ILI9341 driver chip
#include <SPI.h>
#include "common.h"

#define TFT_GREY 0x5AEB // New colour

// TFT_eSPI tft = TFT_eSPI(240,280);  // Invoke library

int8_t getPinName(int8_t pin);
void printProcessorName(void);
setup_t user; // The library defines the type "setup_t" as a struct
              // Calling tft.getSetup(user) populates it with the settings

void setup(void) 
{
  tft.init();
  tft.setRotation(3);
//   tft.writecommand(ST7796_DISPON); //Display on
  Serial.begin(115200);


  tft.setCursor(0, 0, 4);

  // Set the font colour to be white with a black background
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // // We can now plot text on screen using the "print" class
  tft.drawString("White text",0,0);

  #if defined(CONFIG_IDF_TARGET_ESP32S3)
  Serial.println("CONFIG_IDF_TARGET_ESP32S3 = y");
  #else
  Serial.println("CONFIG_IDF_TARGET_ESP32S3 = n");
  #endif

  #if defined(CONFIG_IDF_TARGET_ESP32)
  Serial.println("CONFIG_IDF_TARGET_ESP32 = y");
  #else
  Serial.println("CONFIG_IDF_TARGET_ESP32 = n");
  #endif

  #if defined(USE_HSPI_PORT)
  Serial.println("USE_HSPI_PORT = y");
  #else
  Serial.println("USE_HSPI_PORT = n");
  #endif

  #if defined(USE_FSPI_PORT)
  Serial.println("USE_FSPI_PORT = y");
  #else
  Serial.println("USE_FSPI_PORT = n");
  #endif

  #if defined (ESP32)
  Serial.println("define ESP32");
  #endif
  
  tft.getSetup(user); //

Serial.print("\n[code]\n");

Serial.print ("TFT_eSPI ver = "); Serial.println(user.version);
printProcessorName();
#if defined (ESP32) || defined (ESP8266)
  if (user.esp < 0x32F000 || user.esp > 0x32FFFF) { Serial.print("Frequency    = "); Serial.print(ESP.getCpuFreqMHz());Serial.println("MHz"); }
#endif
#ifdef ESP8266
  Serial.print("Voltage      = "); Serial.print(ESP.getVcc() / 918.0); Serial.println("V"); // 918 empirically determined
#endif
Serial.print("Transactions = "); Serial.println((user.trans  ==  1) ? "Yes" : "No");
Serial.print("Interface    = "); Serial.println((user.serial ==  1) ? "SPI" : "Parallel");
#ifdef ESP8266
if (user.serial ==  1){ Serial.print("SPI overlap  = "); Serial.println((user.overlap == 1) ? "Yes\n" : "No\n"); }
#endif
if (user.tft_driver != 0xE9D) // For ePaper displays the size is defined in the sketch
{
  Serial.print("Display driver = "); Serial.println(user.tft_driver, HEX); // Hexadecimal code
  Serial.print("Display width  = "); Serial.println(user.tft_width);  // Rotation 0 width and height
  Serial.print("Display height = "); Serial.println(user.tft_height);
  Serial.println();
}
else if (user.tft_driver == 0xE9D) Serial.println("Display driver = ePaper\n");

if (user.r0_x_offset  != 0)  { Serial.print("R0 x offset = "); Serial.println(user.r0_x_offset); } // Offsets, not all used yet
if (user.r0_y_offset  != 0)  { Serial.print("R0 y offset = "); Serial.println(user.r0_y_offset); }
if (user.r1_x_offset  != 0)  { Serial.print("R1 x offset = "); Serial.println(user.r1_x_offset); }
if (user.r1_y_offset  != 0)  { Serial.print("R1 y offset = "); Serial.println(user.r1_y_offset); }
if (user.r2_x_offset  != 0)  { Serial.print("R2 x offset = "); Serial.println(user.r2_x_offset); }
if (user.r2_y_offset  != 0)  { Serial.print("R2 y offset = "); Serial.println(user.r2_y_offset); }
if (user.r3_x_offset  != 0)  { Serial.print("R3 x offset = "); Serial.println(user.r3_x_offset); }
if (user.r3_y_offset  != 0)  { Serial.print("R3 y offset = "); Serial.println(user.r3_y_offset); }

if (user.pin_tft_mosi != -1) { Serial.print("MOSI    = "); Serial.print("GPIO "); Serial.println(getPinName(user.pin_tft_mosi)); }
if (user.pin_tft_miso != -1) { Serial.print("MISO    = "); Serial.print("GPIO "); Serial.println(getPinName(user.pin_tft_miso)); }
if (user.pin_tft_clk  != -1) { Serial.print("SCK     = "); Serial.print("GPIO "); Serial.println(getPinName(user.pin_tft_clk)); }

#ifdef ESP8266
if (user.overlap == true)
{
  Serial.println("Overlap selected, following pins MUST be used:");

                             Serial.println("MOSI     = SD1 (GPIO 8)");
                             Serial.println("MISO     = SD0 (GPIO 7)");
                             Serial.println("SCK      = CLK (GPIO 6)");
                             Serial.println("TFT_CS   = D3  (GPIO 0)\n");

  Serial.println("TFT_DC and TFT_RST pins can be user defined");
}
#endif
String pinNameRef = "GPIO ";
#ifdef ESP8266
  pinNameRef = "PIN_D";
#endif

if (user.esp == 0x32F) {
  Serial.println("\n>>>>> Note: STM32 pin references above D15 may not reflect board markings <<<<<");
  pinNameRef = "D";
}
if (user.pin_tft_cs != -1) { Serial.print("TFT_CS   = " + pinNameRef); Serial.println(getPinName(user.pin_tft_cs)); }
if (user.pin_tft_dc != -1) { Serial.print("TFT_DC   = " + pinNameRef); Serial.println(getPinName(user.pin_tft_dc)); }
if (user.pin_tft_rst!= -1) { Serial.print("TFT_RST  = " + pinNameRef); Serial.println(getPinName(user.pin_tft_rst)); }

if (user.pin_tch_cs != -1) { Serial.print("TOUCH_CS = " + pinNameRef); Serial.println(getPinName(user.pin_tch_cs)); }

if (user.pin_tft_wr != -1) { Serial.print("TFT_WR   = " + pinNameRef); Serial.println(getPinName(user.pin_tft_wr)); }
if (user.pin_tft_rd != -1) { Serial.print("TFT_RD   = " + pinNameRef); Serial.println(getPinName(user.pin_tft_rd)); }

if (user.pin_tft_d0 != -1) { Serial.print("\nTFT_D0   = " + pinNameRef); Serial.println(getPinName(user.pin_tft_d0)); }
if (user.pin_tft_d1 != -1) { Serial.print("TFT_D1   = " + pinNameRef); Serial.println(getPinName(user.pin_tft_d1)); }
if (user.pin_tft_d2 != -1) { Serial.print("TFT_D2   = " + pinNameRef); Serial.println(getPinName(user.pin_tft_d2)); }
if (user.pin_tft_d3 != -1) { Serial.print("TFT_D3   = " + pinNameRef); Serial.println(getPinName(user.pin_tft_d3)); }
if (user.pin_tft_d4 != -1) { Serial.print("TFT_D4   = " + pinNameRef); Serial.println(getPinName(user.pin_tft_d4)); }
if (user.pin_tft_d5 != -1) { Serial.print("TFT_D5   = " + pinNameRef); Serial.println(getPinName(user.pin_tft_d5)); }
if (user.pin_tft_d6 != -1) { Serial.print("TFT_D6   = " + pinNameRef); Serial.println(getPinName(user.pin_tft_d6)); }
if (user.pin_tft_d7 != -1) { Serial.print("TFT_D7   = " + pinNameRef); Serial.println(getPinName(user.pin_tft_d7)); }

#if defined (TFT_BL)
  Serial.print("\nTFT_BL           = " + pinNameRef); Serial.println(getPinName(user.pin_tft_led));
  #if defined (TFT_BACKLIGHT_ON)
    Serial.print("TFT_BACKLIGHT_ON = "); Serial.println(user.pin_tft_led_on == HIGH ? "HIGH" : "LOW");
  #endif
#endif

Serial.println();

uint16_t fonts = tft.fontsLoaded();
if (fonts & (1 << 1))        Serial.print("Font GLCD   loaded\n");
if (fonts & (1 << 2))        Serial.print("Font 2      loaded\n");
if (fonts & (1 << 4))        Serial.print("Font 4      loaded\n");
if (fonts & (1 << 6))        Serial.print("Font 6      loaded\n");
if (fonts & (1 << 7))        Serial.print("Font 7      loaded\n");
if (fonts & (1 << 9))        Serial.print("Font 8N     loaded\n");
else
if (fonts & (1 << 8))        Serial.print("Font 8      loaded\n");
if (fonts & (1 << 15))       Serial.print("Smooth font enabled\n");
Serial.print("\n");

if (user.serial==1)        { Serial.print("Display SPI frequency = "); Serial.println(user.tft_spi_freq/10.0); }
if (user.pin_tch_cs != -1) { Serial.print("Touch SPI frequency   = "); Serial.println(user.tch_spi_freq/10.0); }

Serial.println("[/code]");

delay(3000);
  
}

void loop() {
  
  // Fill screen with grey so we can see the effect of printing with and without 
  // a background colour defined
  tft.fillScreen(TFT_GREY);
  
  // Set "cursor" at top left corner of display (0,0) and select font 2
  // (cursor will move to next line automatically during printing with 'tft.println'
  //  or stay on the line is there is room for the text with tft.print)
  tft.setCursor(0, 0, 2);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(1);
  // We can now plot text on screen using the "print" class
  tft.println("Hello World!");
  
  // Set the font colour to be yellow with no background, set to font 7
  tft.setTextColor(TFT_YELLOW); tft.setTextFont(7);
  tft.println(1234.56);
  
  // Set the font colour to be red with black background, set to font 4
  tft.setTextColor(TFT_RED,TFT_BLACK);    tft.setTextFont(4);
  //tft.println(3735928559L, HEX); // Should print DEADBEEF

  // Set the font colour to be green with black background, set to font 4
  tft.setTextColor(TFT_GREEN,TFT_BLACK);
  tft.setTextFont(4);
  tft.println("Groop");
  tft.println("I implore thee,");

  // Change to font 2
  tft.setTextFont(2);
  tft.println("my foonting turlingdromes.");
  tft.println("And hooptiously drangle me");
  tft.println("with crinkly bindlewurdles,");
  // This next line is deliberately made too long for the display width to test
  // automatic text wrapping onto the next line
  tft.println("Or I will rend thee in the gobberwarts with my blurglecruncheon, see if I don't!");
  
  // Test some print formatting functions
  float fnumber = 123.45;
   // Set the font colour to be blue with no background, set to font 4
  tft.setTextColor(TFT_BLUE);    tft.setTextFont(4);
  tft.print("Float = "); tft.println(fnumber);           // Print floating point number
  tft.print("Binary = "); tft.println((int)fnumber, BIN); // Print as integer value in binary
  tft.print("Hexadecimal = "); tft.println((int)fnumber, HEX); // Print as integer number in Hexadecimal
  while(1)
  {
    Serial.println("loop");
    delay(1000);
  }
}


void printProcessorName(void)
{
  Serial.print("Processor    = ");
  if ( user.esp == 0x8266) Serial.println("ESP8266");
  if ( user.esp == 0x32)   Serial.println("ESP32");
  if ( user.esp == 0x32F)  Serial.println("STM32");
  if ( user.esp == 0x2040) Serial.println("RP2040");
  if ( user.esp == 0x0000) Serial.println("Generic");
}

// Get pin name
int8_t getPinName(int8_t pin)
{
  // For ESP32 and RP2040 pin labels on boards use the GPIO number
  if (user.esp == 0x32 || user.esp == 0x2040) return pin;

  if (user.esp == 0x8266) {
    // For ESP8266 the pin labels are not the same as the GPIO number
    // These are for the NodeMCU pin definitions:
    //        GPIO       Dxx
    if (pin == 16) return 0;
    if (pin ==  5) return 1;
    if (pin ==  4) return 2;
    if (pin ==  0) return 3;
    if (pin ==  2) return 4;
    if (pin == 14) return 5;
    if (pin == 12) return 6;
    if (pin == 13) return 7;
    if (pin == 15) return 8;
    if (pin ==  3) return 9;
    if (pin ==  1) return 10;
    if (pin ==  9) return 11;
    if (pin == 10) return 12;
  }

  if (user.esp == 0x32F) return pin;

  return -1; // Invalid pin
}
