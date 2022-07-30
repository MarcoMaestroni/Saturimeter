/**
*   \file Display_Functions.c
*   \brief Source file for SSD1306 128x64
*/

#include "Global_Functions.h"
#include "Logo_Bitmap.h"

void Logo_Window()
{     
    //clear display
    display_stopscroll();
    display_clear();    
    display_update();
        
    //draw bitmap of "politecnico di milano" logo
    //bmpBuffer is the logo extracted in bitmap format throught the software provided
    gfx_drawBitmap(0,0,bmpBuffer,128,64,WHITE,BLACK);
    
    //display our names
    gfx_setTextSize(1);
    gfx_setTextColor(WHITE);
    
    gfx_setCursor(0,45);
    gfx_print("Piergiorgio"); 
    
    gfx_setCursor(0,55);
    gfx_print("Cristiano"); 
    
    gfx_setCursor(103,45);
    gfx_print("Sara"); 
    
    gfx_setCursor(98,55);
    gfx_print("Marco"); 
    
    display_update();               
}

void Reading_Window()
{
    display_clear();    
    display_update();
    
    // text settings for HR and SpO2
    gfx_setTextSize(2);
    gfx_setTextColor(WHITE);
    
    //HR text position
    gfx_setCursor(0,20); 
    gfx_print("HR"); 
    
     //SpO2 text position
    gfx_setCursor(78,20); 
    gfx_print("SpO2");  
    
    //set 'beats/min' small
    gfx_setTextSize(1);
    gfx_setCursor(40,47); 
    gfx_print("beats"); 
    gfx_drawLine(40,55,68,55,WHITE); 
    gfx_setCursor(45,57); 
    gfx_print("min");
    
    // '%' text position
    gfx_setTextSize(2);
    gfx_setCursor(118,50); 
    gfx_print("%"); 
          
    display_update(); 
}

void Configuration_Window()
{
    display_clear();    
    display_update();
                    
    gfx_setTextSize(1);
    gfx_setTextColor(WHITE);

    gfx_setCursor(10,10); 
    gfx_print("CONFIGURATION MODE"); 
    
    gfx_setCursor(20,30); 
    gfx_print("Setting"); 
    display_scroll(SCROLL_PAGE_3,SCROLL_PAGE_4,SCROLL_LEFT,SCROLL_SPEED_7);
                
    display_update(); 
} 

void Display_String(uint8_t text_size, uint16_t text_color, uint16_t cursor_x, uint16_t cursor_y, const char* string, int16_t black_w, int16_t black_h)
{    
    gfx_fillRect(cursor_x, cursor_y, black_w, black_h, BLACK);
    display_update();
    
    gfx_setTextSize(text_size);
    gfx_setTextColor(text_color);
    gfx_setCursor(cursor_x, cursor_y);

    gfx_print(string);
    display_update();
}

//LIBRARY FUNCTIONS

// display memory buffer ( === MUST INCLUDE === the preceding I2C 0x40 control byte for the display)
uint8_t SSD1306_buffer[DISPLAYHEIGHT * DISPLAYWIDTH / 8 + 1] = { 0x40 };
// pointer to actual display memory buffer
uint8_t* _displaybuf = SSD1306_buffer+1;
uint16_t _displaybuf_size = sizeof(SSD1306_buffer) - 1;

// call before first use of other functions
void display_init( uint8 i2caddr ){
    
    _i2caddr = i2caddr;
    gfx_init( DISPLAYWIDTH, DISPLAYHEIGHT );
    
    uint8 cmdbuf[] = {
        0x00,
        SSD1306_DISPLAYOFF,
        SSD1306_SETDISPLAYCLOCKDIV,
        0x80,
        SSD1306_SETMULTIPLEX,
        0x3f,
        SSD1306_SETDISPLAYOFFSET,
        0x00,
        SSD1306_SETSTARTLINE | 0x0,
        SSD1306_CHARGEPUMP,
        0x14,
        SSD1306_MEMORYMODE,
        0x00,
        SSD1306_SEGREMAP | 0x1,
        SSD1306_COMSCANDEC,
        SSD1306_SETCOMPINS,
        0x12,
        SSD1306_SETCONTRAST,
        0xcf,
        SSD1306_SETPRECHARGE,
        0xf1,
        SSD1306_SETVCOMDETECT,
        0x40,
        SSD1306_DISPLAYALLON_RESUME,
        SSD1306_NORMALDISPLAY,
        SSD1306_DISPLAYON
    };
    
    display_write_buf( cmdbuf, sizeof(cmdbuf) ); 
    
    //UART_PutString("\nDisplay initialization complete.\n");
}

// for submitting command sequences:
//  buf[0] must be 0x00
// for submitting bulk data (writing to display RAM):
//  buf[0] must be 0x40
uint32 display_write_buf( uint8* buf, uint16_t size ){ //MODIFIED

    uint32 status = TRANSFER_ERROR;
    int i;
    uint8 connection;
    uint8 error;
    
    do{
        error = I2C_MasterSendStart(_i2caddr,I2C_WRITE_XFER_MODE);
        
        if(error == I2C_MSTR_NO_ERROR)
        {
            for ( i = 0; i < size; i++)
            {
                status = I2C_MasterWriteByte(buf[i]);
                if(status != I2C_MSTR_NO_ERROR)
                {
                    error = 1;
                    I2C_MasterSendStop();
                    break;
                }
            }
        }
        
        I2C_MasterSendStop();
        
        if(error)
        {
            UART_PutString("\nDisplay not found. RECONNECT THE DISPLAY.\n"); 
            PWM_Setting(PERIOD_3,DC_3);
            do{
                connection = I2C_IsDeviceConnected(DISPLAY_ADDRESS);
            }while(!connection);
            UART_PutString("Display reconnected!\n");
            display_init(DISPLAY_ADDRESS);
            Reading_Window();
            flag_temp = TRUE;
            PWM_Setting(PERIOD_1,DC_1);
        }
    }while(error);

    return status;
}

// used by gfx_ functions. Needs to be implemented by display_
void display_setPixel( int16_t x, int16_t y, uint16_t color ){
    
    if( (x < 0) || (x >= DISPLAYWIDTH) || (y < 0) || (y >= DISPLAYHEIGHT) )
        return;

    switch( color ){
        case WHITE: 
            PIXEL_ON(x,y);
            break;
        case BLACK:
            PIXEL_OFF(x,y);
            break;
        case INVERSE: 
            PIXEL_TOGGLE(x,y);
            break;
    }
}

void display_clear(void){
    memset( _displaybuf, 0, _displaybuf_size );
    SSD1306_buffer[0] = 0x40; // to be sure its there
}

void display_update(void) {
      
    uint8 cmdbuf[] = {
        0x00,
        SSD1306_COLUMNADDR,
        0,                      // start
        DISPLAYWIDTH-1, // end
        SSD1306_PAGEADDR,
        0,                      // start
        7                       // end
    };
    display_write_buf( cmdbuf, sizeof(cmdbuf) ); 
    display_write_buf( SSD1306_buffer, sizeof(SSD1306_buffer) );
}

// draws horizontal or vertical line
// Note: no check for valid coords, this needs to be done by caller
// should only be called from gfx_hvline which is doing all validity checking
void display_line( int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color ){

    if( x1 == x2 ){
        // vertical
        uint8_t* pstart = GDDRAM_ADDRESS(x1,y1);
        uint8_t* pend = GDDRAM_ADDRESS(x2,y2);       
        uint8_t* ptr = pstart;             
        
        while( ptr <= pend ){
            
            uint8_t mask;
            if( ptr == pstart ){
                // top
                uint8_t lbit = y1 % 8;
                // bottom (line can be very short, all inside this one byte)
                uint8_t ubit = lbit + y2 - y1;
                if( ubit >= 7 )
                    ubit = 7;
                mask = ((1 << (ubit-lbit+1)) - 1) << lbit;    
            }else if( ptr == pend ){
                // top is always bit 0, that makes it easy
                // bottom
                mask = (1 << (y2 % 8)) - 1;    
            }

            if( ptr == pstart || ptr == pend ){
                switch( color ){
                    case WHITE:     *ptr |= mask; break;
                    case BLACK:     *ptr &= ~mask; break;
                    case INVERSE:   *ptr ^= mask; break;
                };  
            }else{
                switch( color ){
                    case WHITE:     *ptr = 0xff; break;
                    case BLACK:     *ptr = 0x00; break;
                    case INVERSE:   *ptr ^= 0xff; break;
                };  
            }
            
            ptr += DISPLAYWIDTH;
        }
    }else{
        // horizontal
        uint8_t* pstart = GDDRAM_ADDRESS(x1,y1);
        uint8_t* pend = pstart + x2 - x1;
        uint8_t pixmask = GDDRAM_PIXMASK(y1);    

        uint8_t* ptr = pstart;
        while( ptr <= pend ){
            switch( color ){
                case WHITE:     *ptr |= pixmask; break;
                case BLACK:     *ptr &= ~pixmask; break;
                case INVERSE:   *ptr ^= pixmask; break;
            };
            ptr++;
        }
    }
}

void display_stopscroll(void){

    uint8 cmdbuf[] = {
        0x00,
        SSD1306_DEACTIVATE_SCROLL
    };
    display_write_buf( cmdbuf, sizeof(cmdbuf) ); 
}

void display_scroll( SCROLL_AREA start, SCROLL_AREA end, SCROLL_DIR dir, SCROLL_SPEED speed ){
   
    uint8 cmdbuf[] = {
        0x00,
        dir,                    // 0x26 or 0x2a
        0x00,                   // dummy byte
        start,                  // start page
        speed,                  // scroll step interval in terms of frame frequency 
        end,                    // end page
        0x00,                   // dummy byte
        0xFF,                   // dummy byte
        SSD1306_ACTIVATE_SCROLL // 0x2F
    };
    display_write_buf( cmdbuf, sizeof(cmdbuf) ); 
}

void gfx_init( int16_t width, int16_t height ){
    WIDTH = width;
    HEIGHT = height;
    _width = WIDTH;
    _height = HEIGHT;
    
    rotation = 0;
    cursor_y = cursor_x = 0;
    textsize = 1;
    textcolor = textbgcolor = 0xFFFF;
    wrap = 1;
}

void gfx_setCursor( int16_t x, int16_t y ){
    cursor_x = x;
    cursor_y = y;
}

void gfx_setTextSize( uint8_t size ){
    textsize = (size > 0) ? size : 1;
}

void gfx_setTextColor( uint16_t color ){
    // For 'transparent' background, we'll set the bg
    // to the same as fg instead of using a flag
    textcolor = textbgcolor = color;
}

void gfx_rotation_adjust( int16_t* px, int16_t* py ){

    int16_t y0 = *py;
    
    switch( rotation ){
        case 1:
            *py = *px;
            *px = WIDTH - y0 - 1;
            break;
        case 2:
            *px = WIDTH - *px - 1;
            *py = HEIGHT - *py - 1;
            break;
        case 3:
            *py = HEIGHT - *px - 1;
            *px = y0;
            break;
    }
}

void gfx_drawPixel( int16_t x, int16_t y, uint16_t color ){
    
    if( (x < 0) || (x >= _width) || (y < 0) || (y >= _height) )
        return;
    
    gfx_rotation_adjust( &x, &y );

    display_setPixel(x,y,color);
}

// helper function for gfx_drawLine, handles special cases of horizontal and vertical lines
void gfx_hvLine( int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color ){
    
    if( x1 != x2 && y1 != y2 ){
        // neither vertical nor horizontal
        return;
    }    
    
    // bounds check
    if( rotation == 1 || rotation == 3 ){
        if( x1 < 0 || x1 >= HEIGHT || x2 < 0 || x2 >= HEIGHT )
            return;
        if( y1 < 0 || y1 >= WIDTH || y2 < 0 || y2 >= WIDTH )
            return;
    }else{
        if( y1 < 0 || y1 >= HEIGHT || y2 < 0 || y2 >= HEIGHT )
            return;
        if( x1 < 0 || x1 >= WIDTH || x2 < 0 || x2 >= WIDTH )
            return;
    }
    
    gfx_rotation_adjust( &x1, &y1 );
    gfx_rotation_adjust( &x2, &y2 );
    
    // ensure coords are from left to right and top to bottom
    if( (x1 == x2 && y2 < y1) || (y1 == y2 && x2 < x1) ){
        // swap as needed
        int16_t t = x1; x1 = x2; x2 = t;
        t = y1; y1 = y2; y2 = t;
    }
    
    display_line( x1, y1, x2, y2, color );
}

// always use this function for line drawing
void gfx_drawLine( int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color ){
 
    if( x0 == x1 || y0 == y1 ){
        // vertical and horizontal lines can be drawn faster
        gfx_hvLine( x0, y0, x1, y1, color );
        return; 
    }
    
    int16_t t;
    
    //ADDED THESE 10 LINES WHICH ALLOW NOT USING abs FUNCTION IN THE FOLLOWING AND SO NOT IMPORTING stdlib.h
    int16_t y = y1-y0;
    if(y >= 0)
        y = y;
    else
        y = -y;
    
    int16_t x = x1-x0;
    if(x >= 0)
        x = x;
    else
        x = -x;
    
    int16_t steep = y > x;
    if( steep ){
        t = x0; x0 = y0; y0 = t;
        t = x1; x1 = y1; y1 = t;
    }
    if( x0 > x1 ){
        t = x0; x0 = x1; x1 = t;
        t = y0; y0 = y1; y1 = t;
    }
    int16_t dx, dy;
    dx = x1 - x0;
    dy = y;
    int16_t err = dx / 2;
    int16_t ystep;
    if( y0 < y1 ){
        ystep = 1;
    }else{
        ystep = -1;
    }
    for( ; x0<=x1; x0++ ){
        if( steep ){
            gfx_drawPixel( y0, x0, color );
        }else{
            gfx_drawPixel( x0, y0, color );
        }
        err -= dy;
        if( err < 0 ){
            y0 += ystep;
            err += dx;
        }
    }
}

void gfx_fillRect( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color ){
    int16_t i = 0;
    if( h > w ){
        for( i = x ; i < x+w ; i++ ){
            gfx_drawLine( i, y, i, y+h-1, color );
        }
    }else{
        for( i = y ; i < y+h ; i++ ){
            gfx_drawLine( x, i, x+w-1, i, color );
        }
    }
}

// Draw a character
void gfx_drawChar( int16_t x, int16_t y, unsigned char c,uint16_t color, uint16_t bg, uint8_t size) {
    if( (x >= _width) || // Clip right
        (y >= _height) || // Clip bottom
        ((x + 6 * size - 1) < 0) || // Clip left
        ((y + 8 * size - 1) < 0)) // Clip top
        return;

    int8_t i = 0;
    for( i = 0 ; i < 6 ; i++ ){
        uint8_t line;
        if( i == 5 )
            line = 0x0;
        else
           line = font[(c*5)+i];
        int8_t j = 0;
        for( j = 0; j < 8 ; j++ ){
            if( line & 0x1 ){
                if( size == 1 ) // default size
                    gfx_drawPixel( x+i, y+j, color );
                else { // big size
                    gfx_fillRect( x+(i*size), y+(j*size), size, size, color );
                }
            } else if( bg != color ){
                if( size == 1 ) // default size
                    gfx_drawPixel( x+i, y+j, bg );
                else { // big size
                    gfx_fillRect( x+i*size, y+j*size, size, size, bg );
                }
            }
            line >>= 1;
        }
    }
}

void gfx_write( uint8_t ch ){
    if( ch == '\n' ){
        cursor_y += textsize*8;
        cursor_x = 0;
    }else if( ch == '\r' ){
        // skip em
    }else{
        gfx_drawChar(cursor_x, cursor_y, ch, textcolor, textbgcolor, textsize);
        cursor_x += textsize*6;
        if( wrap && (cursor_x > (_width - textsize*6)) ){
            cursor_y += textsize*8;
            cursor_x = 0;
        }
    }
}

void gfx_print( const char* s ){
    
    unsigned int len = strlen( s );
    unsigned int i = 0; 
    for( i = 0 ; i < len ; i++ ){
        gfx_write( s[i] );
    }
}

// Draw a 1-bit color bitmap at the specified x, y position from the
// provided bitmap buffer (must be PROGMEM memory) using color as the
// foreground color and bg as the background color.
void gfx_drawBitmap(int16_t x, int16_t y,
            const uint8_t *bitmap, int16_t w, int16_t h,
            uint16_t color, uint16_t bg) {

  int16_t i, j, byteWidth = (w + 7) / 8;
  
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
        gfx_drawPixel(x+i, y+j, color);
      }
      else {
      	gfx_drawPixel(x+i, y+j, bg);
      }
    }
  }
}

/* [] END OF FILE */