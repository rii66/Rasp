#ifndef _NOKIA105_LCD_H
#define _NOKIA105_LCD_H

#include "Arduino.h"
#include "fonts.h"

// 9-bit SPI is implemented in software for portability.
// No MCU-specific GPIO registers and no ESP32 LEDC API are required.
#define LCD_RES_High()   digitalWrite(SPIDEVICE_RES, HIGH)
#define LCD_RES_Low()    digitalWrite(SPIDEVICE_RES, LOW)
#define LCD_CS_High()    digitalWrite(SPIDEVICE_CS, HIGH)
#define LCD_CS_Low()     digitalWrite(SPIDEVICE_CS, LOW)
#define LCD_SDA_High()   digitalWrite(SPIDEVICE_SDA, HIGH)
#define LCD_SDA_Low()    digitalWrite(SPIDEVICE_SDA, LOW)
#define LCD_SCK_High()   digitalWrite(SPIDEVICE_SCK, HIGH)
#define LCD_SCK_Low()    digitalWrite(SPIDEVICE_SCK, LOW)

//https://stackoverflow.com/questions/2660484/what-are-0x01-and-0x80-representative-of-in-c-bitwise-operations
        /*| MSB |     |     |     |     |     |     | LSB |
          |  1  |  0  |  1  |  1  |  0  |  0  |  1  |  1  |   Input
          |  1  |  1  |  0  |  0  |  1  |  1  |  0  |  1  |   Output
          |  1  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |   0x80
          |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  1  |   0x01
          |  0  |  1  |  0  |  0  |  0  |  0  |  0  |  0  |   (0x80 >> 1)
          |  0  |  0  |  0  |  0  |  0  |  0  |  1  |  0  |   (0x01 << 1)*/
/*
 * harcoded gpio for future updates
#define SPIDEVICE_CS    10
#define SPIDEVICE_RES   12          //miso
#define SPIDEVICE_SDA   11          //Mosi
#define SPIDEVICE_SCK   13
*/
//----------------------------macros to Manipulate display-----------------------------------
#ifndef NOKIA105_WIDTH
#define NOKIA105_WIDTH  128
#endif
#ifndef NOKIA105_HEIGHT
#define NOKIA105_HEIGHT 160
#endif
#ifndef NOKIA105_X_OFFSET
#define NOKIA105_X_OFFSET 2
#endif
#ifndef NOKIA105_Y_OFFSET
#define NOKIA105_Y_OFFSET 0
#endif
#define WIDTH          NOKIA105_WIDTH
#define HEIGHT         NOKIA105_HEIGHT
#define nextLineEdge   128             //printString
#define spaceBetweenScanLines  16      //printString, 2 lines ke beech ka distance
#define fullLengthVertical    160      //164 characters on display
#define rotation        0              //SCREEN ROTATION 0 by default
#define rotateBitmap90  0              //1-> no rotaion 90,0-> yes rorate 90
#define LOG             0              //to activate serial
#define totalPixals     (WIDTH*HEIGHT)   //21384
#define RGB2BGR         1              //0: RGB,1: BGR, color seems to off.means: blue become red or vice versa. green remain same

//----------predefined 16 bit colors
#define BLACK             0x0000
#define NAVY              0x000F
#define DARKGREEN         0x03E0
#define DARKCYAN          0x03EF
#define MAROON            0x7800
#define PURPLE            0x780F
#define OLIVE             0x7BE0
#define LIGHTGREY         0xC618
#define DARKGREY          0x7BEF
#define BLUE              0x001F
#define GREEN             0x07E0
#define CYAN              0x07FF
#define RED               0xF800
#define MAGENTA           0xF81F
#define YELLOW            0xFFE0
#define WHITE             0xFFFF
#define ORANGE            0xFD20
#define GREENYELLOW       0xAFE5
#define PINK              0xF81F

class Nokia105 {
	public:
  Nokia105(int SID, int SCLK, int RST, int CS);
  void setBacklightPin(int pin);
  void begin();
  void reset();
  void displayOn();
  void displayOff();
  void invertDisplay(bool invert = true);
  void setRotation(uint8_t r);
	/**********************************************************************/
  /*!
    @brief    Pin defination
    @param    SPIDEVICE_CS, SPIDEVICE_RES, SPIDEVICE_SDA or Mosi, SPIDEVICE_SCK
    spi proceed by defined gpio.
  */
  /**********************************************************************/
		
	void	initDisplay(),
  /**********************************************************************/
  /*!
    @brief    lcd initialize
    @param    
  */
  /**********************************************************************/
  
      PWMinit(),    
  /**********************************************************************/
  /*!
    @brief    start the inbuilt timers to generate pwm
    @param    
  */
  /**********************************************************************/

      setLcdBrightness(uint16_t PWM),    
  /**********************************************************************/
  /*!
    @brief    simple map the input 16 bit values to counter
    @param    
  */
  /**********************************************************************/
     
  setDrawPosition(unsigned char x, unsigned char y),
  /**********************************************************************/
  /*!
    @brief    set window cursor to push colors
    @param    x-> number of pixels in x axis or horizontal, y>x-> number of pixels in y axis or vertical
  */
  /**********************************************************************/
        
  setDrawPositionAxis(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1),
  /**********************************************************************/
  /*!
    @brief    set window cursor to push colors
    @param    x-> number of pixels in x axis or horizontal, y>x-> number of pixels in y axis or vertical
    x0: start position | x1: end position 
    y0: start position | y1: end position
  */
  /**********************************************************************/
  
  drawPixel(int16_t x, int16_t y, uint16_t color), 
	/**********************************************************************/
  /*!
    @brief    as function name says, it drae 1 pixel on screen
    @param    x: horizonal position, y: vertical position,color: 16 bit color in hex
  */
  /**********************************************************************/
	
	image1d (uint16_t w, uint16_t h, uint16_t shiftX,uint16_t shiftY, const uint16_t image[] ),
	/**********************************************************************/
  /*!
    @brief    Pin defination
    @param    
  */
  /**********************************************************************/
	
	/*image2d (int w, int h, int shiftX,int shiftY, const uint16_t image[][80] ),*/
  /**********************************************************************/
  /*!
    @brief    Pin defination
    @param    
  */
  /**********************************************************************/
				
	printDigit(unsigned int a, int16_t x, int16_t y,uint16_t forgroundColor,uint16_t backgroundColor),
  /**********************************************************************/
  /*!
    @brief    digit print working upto 10,000 only unsigned integers 
    @param    
  */
  /**********************************************************************/

	drawtext(unsigned char c, unsigned char x, unsigned char y ,uint16_t color),
  /**********************************************************************/
  /*!
    @brief    as per function name. it draw the text but it is in beta.
    @param    
  */
  /**********************************************************************/
				
	fillRectangle (int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color),
  /**********************************************************************/
  /*!
    @brief    rectanglle  shape color 
    @param    
  */
  /**********************************************************************/
	
	smpteTest(),
  /**********************************************************************/
  /*!
    @brief    colorfull rectangles
    @param    
  */
  /**********************************************************************/
	
	printBitmap(int16_t x, int16_t y, const uint8_t bitmap[],int16_t w, int16_t h, uint16_t color),
	/**********************************************************************/
  /*!
    @brief    bitmap 
    @param    
  */
  /**********************************************************************/
	
	backgroundColor(uint16_t c),
  /**********************************************************************/
  /*!
    @brief    fill the screen by passing the color value
    @param    
  */
  /**********************************************************************/
	
	colorPalletTest(),
  /**********************************************************************/
  /*!
    @brief    colors flash on screen
    @param   
  */
  /**********************************************************************/
	
	lineHorixontal(int16_t x, int16_t y, int16_t h, uint16_t color),
  /**********************************************************************/
  /*!
    @brief    horizontal line
    @param    
  */
  /**********************************************************************/
	
	lineVertical(int16_t x, int16_t y, int16_t w,uint16_t color),
  /**********************************************************************/
  /*!
    @brief    vertical line
    @param    
  */
  /**********************************************************************/
				
	circle(int16_t x0, int16_t y0, int16_t r, uint16_t color),
  /**********************************************************************/
  /*!
    @brief    draw circle
    @param    
  */
  /**********************************************************************/
	
	printSingleChar( unsigned char c,unsigned char x, unsigned char y,uint16_t forgroundColor, uint16_t backgroundColor),
  /**********************************************************************/
  /*!
    @brief    single charatcer only
    @param   
  */
  /**********************************************************************/
	
	printStringChar ( const char *String,unsigned char x,unsigned char y,uint16_t forgroundColor, uint16_t backgroundColor),
  /**********************************************************************/
  /*!
    @brief    string of data without next line feature
    @param    .
  */
  /**********************************************************************/
	
	printString(const char *str,uint8_t x,uint8_t y,uint16_t forgroundColor, uint16_t backgroundColor),
  /**********************************************************************/
  /*!
    @brief    Print String of character with next line feature
    @param    
  */
  /**********************************************************************/
	
	displayClear();
  /**********************************************************************/
  /*!
    @brief    Display Clear
    @param    
  */
  /**********************************************************************/

	private:
  void writeNokiaCommand(unsigned char c);
  void writeNokiaData(unsigned char c);

  int SPIDEVICE_CS;
  int SPIDEVICE_RES;
  int SPIDEVICE_SDA;
  int SPIDEVICE_SCK;
  int backLightPin;
  uint8_t rotationValue;
};
#endif