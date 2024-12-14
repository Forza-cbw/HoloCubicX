// ST7789 240 x 240 display with no chip select line
#define USER_SETUP_ID 24

// #define TFT_PARALLEL_8_BIT
#define ST7789_DRIVER     // Configure all registers
//#define ST7789_2_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

#define CGRAM_OFFSET 
//#define TFT_SDA_READ 

//#define TFT_RGB_ORDER TFT_RGB  // Colour order Red-Green-Blue
#define TFT_RGB_ORDER TFT_BGR  // Colour order Blue-Green-Red

#define TFT_INVERSION_ON
//#define TFT_INVERSION_OFF

// ESP32 S3 pins used for the parallel interface TFT


#define ESP32_DMA


// #define TFT_MISO 13     //37
// #define TFT_MOSI 11    //35
// #define TFT_SCLK 12     //36
// #define TFT_CS    10 // Not connected
// #define TFT_DC    9
// #define TFT_RST   14  // Connect reset to ensure display initialises

// #define TFT_BL   4            // LED back-light control pin
// #define TFT_BACKLIGHT_ON LOW  // Level to turn ON back-light (HIGH or LOW).

#define TFT_MISO 13     //37
#define TFT_MOSI 42    //35
#define TFT_SCLK 41     //36
#define TFT_CS    10 // Not connected
#define TFT_DC    40
#define TFT_RST   45  // Connect reset to ensure display initialises

#define TFT_BL   46            // LED back-light control pin
#define TFT_BACKLIGHT_ON LOW  // Level to turn ON back-light (HIGH or LOW)

// #define TFT_SPI_MODE SPI_MODE0

// #define TOUCH_CS 15     // Chip select pin (T_CS) of touch screen`


#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT


// #define SPI_FREQUENCY   1000000
// #define SPI_FREQUENCY   5000000
// #define SPI_FREQUENCY  10000000
// #define SPI_FREQUENCY  20000000
// #define SPI_FREQUENCY 27000000 // Actually sets it to 26.67MHz = 80/3
#define SPI_FREQUENCY  40000000
//#define SPI_FREQUENCY  80000000

#define SPI_READ_FREQUENCY  6000000 // 6 MHz is the maximum SPI read speed for the ST7789V
