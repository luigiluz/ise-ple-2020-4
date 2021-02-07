// Based on the work by DFRobot

#include "LiquidCrystal_I2C.h"
#include <inttypes.h>
#if defined(ARDUINO) && ARDUINO >= 100

//#include "Arduino.h"

#define printIIC(args)	Wire.write(args)
inline size_t write(uint8_t value) {
	send(value, Rs);
	return 1;
}

#else
//#include "WProgram.h"



#endif
//#include "Wire.h"



/**********************/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_task_wdt.h"

#include "driver/i2c.h"

#define delay(ms) vTaskDelay( (ms) / portTICK_RATE_MS)
#define delayMicroseconds(value) for(int i=0;i<(value*52/3); i++)esp_task_wdt_reset();

/**********************/


void init_priv();
void send(uint8_t, uint8_t);
void write4bits(uint8_t);
void expanderWrite(uint8_t);
void pulseEnable(uint8_t);
uint8_t _Addr;
uint8_t _displayfunction;
uint8_t _displaycontrol;
uint8_t _displaymode;
uint8_t _numlines;
uint8_t _oled = 0;
uint8_t _cols;
uint8_t _rows;
uint8_t _backlightval;


//#define printIIC(args)	Wire.send(args)
inline void writeLCD(uint8_t value) {
	send(value, Rs);
}

// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set: 
//    DL = 1; 8-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).


#define I2C_MASTER_SCL_IO           5                /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO           4               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0        /*!< I2C port number for master dev */
#define WRITE_BIT                           I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                            I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                        0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                       0x0              /*!< I2C master will not check ack from slave */

/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init()
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = 1;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = 1;
    conf.clk_stretch_tick = 300; // 300 ticks, Clock stretch is about 210us, you can make changes according to the actual situation.
    //ESP_ERROR_CHECK(
	i2c_driver_install(i2c_master_port, conf.mode); //);
    //ESP_ERROR_CHECK(
	i2c_param_config(i2c_master_port, &conf); //);
    return ESP_OK;
}


void LiquidCrystal_I2C(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows)
{
  _Addr = lcd_Addr;
  _cols = lcd_cols;
  _rows = lcd_rows;
  _backlightval = LCD_NOBACKLIGHT;
  delay(100);
  i2c_master_init();
  
}

void oled_init(){
  _oled = 1;//true;
	init_priv();
}

void init(){
	init_priv();
}

void init_priv()
{
	//Wire.begin();
	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
	begin(_cols, _rows, LCD_5x8DOTS);  
}

void begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
	if (lines > 1) {
		_displayfunction |= LCD_2LINE;
	}
	_numlines = lines;

	// for some 1 line displays you can select a 10 pixel high font
	if ((dotsize != 0) && (lines == 1)) {
		_displayfunction |= LCD_5x10DOTS;
	}

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	delay(50); 
  
	// Now we pull both RS and R/W low to begin commands
	expanderWrite(_backlightval);	// reset expanderand turn backlight off (Bit 8 =1)
	delay(1000);

  	//put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46
	
	  // we start in 8bit mode, try to set 4 bit mode
   write4bits(0x03 << 4);
   delayMicroseconds(4500); // wait min 4.1ms
   
   // second try
   write4bits(0x03 << 4);
   delayMicroseconds(4500); // wait min 4.1ms
   
   // third go!
   write4bits(0x03 << 4); 
   delayMicroseconds(150);
   
   // finally, set to 4-bit interface
   write4bits(0x02 << 4); 


	// set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);  
	
	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	display();
	
	// clear it off
	clear();
	
	// Initialize to default text direction (for roman languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	
	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);
	
	home();
  
}

/********** high level commands, for the user! */
void clear(){
	command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
	delayMicroseconds(2000);  // this command takes a long time!
  if (_oled) setCursor(0,0);
}

void home(){
	command(LCD_RETURNHOME);  // set cursor position to zero
	delayMicroseconds(2000);  // this command takes a long time!
}

void setCursor(uint8_t col, uint8_t row){
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if ( row > _numlines ) {
		row = _numlines-1;    // we count rows starting w/0
	}
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void display() {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void noBlink() {
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void blink() {
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void scrollDisplayLeft(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void scrollDisplayRight(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void leftToRight(void) {
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void rightToLeft(void) {
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void autoscroll(void) {
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void noAutoscroll(void) {
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
// void createChar(uint8_t location, uint8_t charmap[]) {
// 	location &= 0x7; // we only have 8 locations 0-7
// 	command(LCD_SETCGRAMADDR | (location << 3));
// 	for (int i=0; i<8; i++) {
// 		write(charmap[i]);
// 	}
// }


// //createChar with PROGMEM input
// void createChar(uint8_t location, const char *charmap) {
// 	location &= 0x7; // we only have 8 locations 0-7
// 	command(LCD_SETCGRAMADDR | (location << 3));
// 	for (int i=0; i<8; i++) {
// 	    	writeLCD(pgm_read_byte_near(charmap++));
// 	}
// }


// Turn the (optional) backlight off/on
void noBacklight(void) {
	_backlightval=LCD_NOBACKLIGHT;
	expanderWrite(0);
}

void backlight(void) {
	_backlightval=LCD_BACKLIGHT;
	expanderWrite(0);
}



/*********** mid level commands, for sending data/cmds */

inline void command(uint8_t value) {
	send(value, 0);
}


/************ low level data pushing commands **********/

// write either command or data
void send(uint8_t value, uint8_t mode) {
	uint8_t highnib=value&0xf0;
	uint8_t lownib=(value<<4)&0xf0;
       write4bits((highnib)|mode);
	write4bits((lownib)|mode); 
}

void write4bits(uint8_t value) {
	expanderWrite(value);
	pulseEnable(value);
}

void expanderWrite(uint8_t _data){          
	// Wire.beginTransmission(_Addr);
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, _Addr << 1 | WRITE_BIT, ACK_CHECK_EN);
	// printIIC((int)(_data) | _backlightval);
	i2c_master_write_byte(cmd, _data | _backlightval, ACK_CHECK_EN);
	// Wire.endTransmission();
	i2c_master_stop(cmd);   
	i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
}

void pulseEnable(uint8_t _data){
	expanderWrite(_data | En);	// En high
	delayMicroseconds(1);		// enable pulse must be >450ns
	
	expanderWrite(_data & ~En);	// En low
	delayMicroseconds(50);		// commands need > 37us to settle
} 


// Alias functions

void cursor_on(){
	cursor();
}

void cursor_off(){
	noCursor();
}

void blink_on(){
	blink();
}

void blink_off(){
	noBlink();
}

// void load_custom_character(uint8_t char_num, uint8_t *rows){
// 		createChar(char_num, rows);
// }

void setBacklight(uint8_t new_val){
	if(new_val){
		backlight();		// turn backlight on
	}else{
		noBacklight();		// turn backlight off
	}
}

// void printstr(const char c[]){
// 	//This function is not identical to the function used for "real" I2C displays
// 	//it's here so the user sketch doesn't have to be changed 
// 	print(c);
// }


// unsupported API functions
/*
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void off(){}
void on(){}
void setDelay (int cmdDelay,int charDelay) {}
uint8_t status(){return 0;}
uint8_t keypad (){return 0;}
uint8_t init_bargraph(uint8_t graphtype){return 0;}
void draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_col_end){}
void draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_row_end){}
void setContrast(uint8_t new_val){}
#pragma GCC diagnostic pop
*/	
