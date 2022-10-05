#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"
#include "hardware/gpio.h"

/*Encoder GPIO*/
// GPIO 10 is Encoder phase A,  
// GPIO 11 is Encoder phase B,
// GPIO 12 is the encoder push botton switch.
// change these as needed

#define ENC_A   10
#define ENC_B   11
#define ENC_SW  12


#define WS2812_PIN 3
#define repeat(x) for(int i = x; i--;)



#define IS_RGBW false
#define NUM_PIXELS 12
#define brightness 50

#define repeat(x) for(int i = x; i--;)

// commands
const int LCD_CLEARDISPLAY = 0x01;
const int LCD_RETURNHOME = 0x02;
const int LCD_ENTRYMODESET = 0x04;
const int LCD_DISPLAYCONTROL = 0x08;
const int LCD_CURSORSHIFT = 0x10;
const int LCD_FUNCTIONSET = 0x20;
const int LCD_SETCGRAMADDR = 0x40;
const int LCD_SETDDRAMADDR = 0x80;

// flags for display entry mode
const int LCD_ENTRYSHIFTINCREMENT = 0x01;
const int LCD_ENTRYLEFT = 0x02;

// flags for display and cursor control
const int LCD_BLINKON = 0x01;
const int LCD_CURSORON = 0x02;
const int LCD_DISPLAYON = 0x04;

// flags for display and cursor shift
const int LCD_MOVERIGHT = 0x04;
const int LCD_DISPLAYMOVE = 0x08;

// flags for function set
const int LCD_5x10DOTS = 0x04;
const int LCD_2LINE = 0x08;
const int LCD_8BITMODE = 0x10;

// flag for backlight control
const int LCD_BACKLIGHT = 0x08;

const int LCD_ENABLE_BIT = 0x04;

// By default these LCD display drivers are on bus address 0x27
static int addr = 0x27;

// Modes for lcd_send_byte
#define LCD_CHARACTER  1
#define LCD_COMMAND    0

#define MAX_LINES      2
#define MAX_CHARS      16

/* Quick helper function for single byte transfers */
void i2c_write_byte(uint8_t val) {
#ifdef i2c_default
    i2c_write_blocking(i2c_default, addr, &val, 1, false);
#endif
}

void lcd_toggle_enable(uint8_t val) {
    // Toggle enable pin on LCD display
    // We cannot do this too quickly or things don't work
#define DELAY_US 600
    sleep_us(DELAY_US);
    i2c_write_byte(val | LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
    i2c_write_byte(val & ~LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
}

// The display is sent a byte as two separate nibble transfers
void lcd_send_byte(uint8_t val, int mode) {
    uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
    uint8_t low = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;

    i2c_write_byte(high);
    lcd_toggle_enable(high);
    i2c_write_byte(low);
    lcd_toggle_enable(low);
}

void lcd_clear(void) {
    lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
}

// go to location on LCD
void lcd_set_cursor(int line, int position) {
    int val = (line == 0) ? 0x80 + position : 0xC0 + position;
    lcd_send_byte(val, LCD_COMMAND);
}

static void inline lcd_char(char val) {
    lcd_send_byte(val, LCD_CHARACTER);
}

void lcd_string(const char *s) {
    while (*s) {
        lcd_char(*s++);
    }
}

void lcd_init() {
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x02, LCD_COMMAND);

    lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
    lcd_send_byte(LCD_FUNCTIONSET | LCD_2LINE, LCD_COMMAND);
    lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
    lcd_clear();
}
uint16_t adcresult = 1;

int light = 1.2;




static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);
}

void bootanimation(){
    sleep_ms(250);
    int a=0;
    int b=0;
    int x=0;
    repeat(12){
        put_pixel(urgb_u32(0,10,0));

    }
    sleep_ms(50);
    while (a<13){
        repeat(x){
            put_pixel(urgb_u32(10,0,0));
            //x++;

        }
        a++;
        sleep_ms(50);
        x++;

    }
    x=0;
    while (b<13){
        repeat(x){
            put_pixel(urgb_u32(0,0,10));
            //x++;

        }
        b++;
        sleep_ms(50);
        x++;
    }
}







/* Encoder Callback*/
/*
        "LEVEL_LOW",  // 0x1
        "LEVEL_HIGH", // 0x2
        "EDGE_FALL",  // 0x4
        "EDGE_RISE"   // 0x8
*/
void encoder_callback(uint gpio, uint32_t events) 
{

    uint32_t gpio_state = 0;

    gpio_state = (gpio_get_all() >> 10) & 0b0111;   // get all GPIO them mask out all but bits 10, 11, 12
                                                    // This will need to change to match which GPIO pins are being used.


    static bool ccw_fall = 0;  //bool used when falling edge is triggered
    static bool cw_fall = 0;

    uint8_t enc_value = 0;
    enc_value = (gpio_state & 0x03);


    if (gpio == ENC_A) 
    {
        if ((!cw_fall) && (enc_value == 0b10)) // cw_fall is set to TRUE when phase A interrupt is triggered
            cw_fall = 1; 

        if ((ccw_fall) && (enc_value == 0b00)) // if ccw_fall is already set to true from a previous B phase trigger, the ccw event will be triggered 
        {
            cw_fall = 0;
            ccw_fall = 0;
            //do something here,  for now it is just printing out CW or CCW
            printf("CCW \r\n");
        }

    }   


    if (gpio == ENC_B) 
    {
        if ((!ccw_fall) && (enc_value == 0b01)) //ccw leading edge is true
            ccw_fall = 1;

        if ((cw_fall) && (enc_value == 0b00)) //cw trigger
        {
            cw_fall = 0;
            ccw_fall = 0;
            //do something here,  for now it is just printing out CW or CCW
            printf("CW \r\n");
        }

    }

}

int main()
{
    stdio_init_all();
     // GPIO Setup for Encoder
    gpio_init(ENC_SW);                  //Initialise a GPIO for (enabled I/O and set func to GPIO_FUNC_SIO)
    gpio_set_dir(ENC_SW,GPIO_IN);
    gpio_disable_pulls(ENC_SW);

    gpio_init(ENC_A);
    gpio_set_dir(ENC_A,GPIO_IN);
    gpio_disable_pulls(ENC_A);

    gpio_init(ENC_B);
    gpio_set_dir(ENC_B,GPIO_IN);
    gpio_disable_pulls(ENC_B);

    gpio_set_irq_enabled_with_callback(ENC_SW, GPIO_IRQ_EDGE_FALL, true, &encoder_callback);
    gpio_set_irq_enabled(ENC_A, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(ENC_B, GPIO_IRQ_EDGE_FALL, true);

    while (1) 
    {
    // do something here forever
    sleep_ms(1000);
    }

    // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    lcd_init();


    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
    //reset all pixels
    for (uint i=0; i<12; i++){
        put_pixel(0);
    }
    
    
    bootanimation();

}