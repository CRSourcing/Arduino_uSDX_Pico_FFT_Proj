#include <sys/_stdint.h>
/*
 * display.cpp
 * 
 * Created: May 2022
 * Author: Klaus Fensterseifer
 * https://github.com/kaefe64/Arduino_uSDX_Pico_FFT_Proj
 * 
*/

#include "Arduino.h"
#include "SPI.h"
#include "uSDR.h"  //Serialx
#include "dsp.h"
#include "TFT_eSPI.h"
#include "display_tft.h"
#include "hmi.h"


// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();
char vet_char[50];


uint16_t font_last = 0;
uint16_t color_last = 0;
uint16_t color_back_last = 0;
uint16_t size_last = 0;
uint16_t x_char_last = 0;
uint16_t y_char_last = 0;


void tft_writexy_(uint16_t font, uint16_t color, uint16_t color_back, uint16_t x, uint16_t y, uint8_t *s)
{
  if(font != font_last)
  {
    if(font == 3)
    {
      tft.setFreeFont(FONT3);                 // Select the font
      tft.setTextSize(SIZE3);  
      x_char_last = X_CHAR3;
      y_char_last = Y_CHAR3;
      size_last = SIZE3;
    }
    else if (font == 2)
    {
      tft.setFreeFont(FONT2);                 // Select the font
      tft.setTextSize(SIZE2);      
      x_char_last = X_CHAR2;
      y_char_last = Y_CHAR2;
      size_last = SIZE2;
    }
    else
    {
      tft.setFreeFont(FONT1);                 // Select the font
      tft.setTextSize(SIZE1);  //size 1 = 10 pixels, size 2 =20 pixels, and so on
      x_char_last = X_CHAR1;
      y_char_last = Y_CHAR1;
      size_last = SIZE1;
    }
    font_last = font;
  }

  if((color != color_last) || (color_back != color_back_last))
    {
    tft.setTextColor(color, color_back);
    color_last = color;
    color_back_last = color_back;
    }
    
  
  tft.drawString((const char *)s, x * x_char_last * size_last, y * y_char_last * size_last, 1);// Print the string name of the font
}




/* write text to display at line column plus a delta x and y */
void tft_writexy_plus(uint16_t font, uint16_t color, uint16_t color_back, uint16_t x, uint16_t x_plus, uint16_t y, uint16_t y_plus, uint8_t *s)
{
  if(font != font_last)
  {
    if(font == 3)
    {
      tft.setFreeFont(FONT3);                 // Select the font
      tft.setTextSize(SIZE3);  
      x_char_last = X_CHAR3;
      y_char_last = Y_CHAR3;
      size_last = SIZE3;
    }
    else if (font == 2)
    {
      tft.setFreeFont(FONT2);                 // Select the font
      tft.setTextSize(SIZE2);      
      x_char_last = X_CHAR2;
      y_char_last = Y_CHAR2;
      size_last = SIZE2;
    }
    else
    {
      tft.setFreeFont(FONT1);                 // Select the font
      tft.setTextSize(SIZE1);  //size 1 = 10 pixels, size 2 =20 pixels, and so on
      x_char_last = X_CHAR1;
      y_char_last = Y_CHAR1;
      size_last = SIZE1;
    }
    font_last = font;
  }

  if((color != color_last) || (color_back != color_back_last))
    {
    tft.setTextColor(color, color_back);
    color_last = color;
    color_back_last = color_back;
    }
    
  
  tft.drawString((const char *)s, (x * x_char_last * size_last)+x_plus, (y * y_char_last * size_last)+y_plus, 1);// Print the string name of the font
}






void tft_cursor(uint16_t font, uint16_t color, uint8_t x, uint8_t y)
{
    if(font == 3)
    {
      for(uint16_t i=1; i<((Y_CHAR3 * SIZE3)/8); i++)
      {
      tft.drawFastHLine (x * X_CHAR3 * SIZE3, ((y+1) * Y_CHAR3 * SIZE3) - i , ((X_CHAR3 * SIZE3)*9)/10, color);
      }
    }
    else if (font == 2)
    {
      for(uint16_t i=1; i<((Y_CHAR2 * SIZE2)/8); i++)
      {
      tft.drawFastHLine (x * X_CHAR2 * SIZE2, ((y+1) * Y_CHAR2 * SIZE2) - i , X_CHAR2 * SIZE2, color);
      }
    }
    else
    {
      for(uint16_t i=1; i<((Y_CHAR1 * SIZE1)/8); i++)
      {
      tft.drawFastHLine (x * X_CHAR1 * SIZE1, ((y+1) * Y_CHAR1 * SIZE1) - i , X_CHAR1 * SIZE1, color);
      }
    }
}


void tft_cursor_plus(uint16_t font, uint16_t color, uint8_t x, uint8_t x_plus, uint8_t y, uint8_t y_plus)
{
  static uint16_t x_old = 0;
  
    if(font == 3)
    {
      for(uint16_t i=1; i<((Y_CHAR3 * SIZE3)/8); i++)
      {
        tft.drawFastHLine (x_old, (((y+1) * Y_CHAR3 * SIZE3) - i)+y_plus , ((X_CHAR3 * SIZE3)*9)/10, TFT_BACKGROUND);
        tft.drawFastHLine ((x * X_CHAR3 * SIZE3)+x_plus, (((y+1) * Y_CHAR3 * SIZE3) - i)+y_plus , ((X_CHAR3 * SIZE3)*9)/10, color);
      }
      x_old = (x * X_CHAR3 * SIZE3)+x_plus;
    }
    else if (font == 2)
    {
      for(uint16_t i=1; i<((Y_CHAR2 * SIZE2)/8); i++)
      {
        tft.drawFastHLine (x_old, (((y+1) * Y_CHAR2 * SIZE2) - i)+y_plus , X_CHAR2 * SIZE2, TFT_BACKGROUND);
        tft.drawFastHLine ((x * X_CHAR2 * SIZE2)+x_plus, (((y+1) * Y_CHAR2 * SIZE2) - i)+y_plus , X_CHAR2 * SIZE2, color);
      }
      x_old = (x * X_CHAR2 * SIZE2)+x_plus;
    }
    else
    {
      for(uint16_t i=1; i<((Y_CHAR1 * SIZE1)/8); i++)
      {
        tft.drawFastHLine (x_old, (((y+1) * Y_CHAR1 * SIZE1) - i)+y_plus , X_CHAR1 * SIZE1, TFT_BACKGROUND);
        tft.drawFastHLine ((x * X_CHAR1 * SIZE1)+x_plus, (((y+1) * Y_CHAR1 * SIZE1) - i)+y_plus , X_CHAR1 * SIZE1, color);
      }
      x_old = (x * X_CHAR1 * SIZE1)+x_plus;
    }
}



/* used to allow calling from other modules, concentrate the use of tft variable locally */
uint16_t tft_color565(uint16_t r, uint16_t g, uint16_t b)
{
  return tft.color565(r, g, b);
}


//#define bargraph_Y   (35+Y_CHAR2-3-8)   //line of display

//uint16_t Smeter_table_color[MAX_Smeter_table] = {  TFT_GREEN, TFT_GREEN, TFT_GREEN, TFT_GREEN, TFT_YELLOW, TFT_YELLOW, TFT_YELLOW, TFT_YELLOW, TFT_RED, TFT_RED, TFT_RED };
uint16_t Smeter_table_color[MAX_Smeter_table] = {  TFT_MAROON, TFT_RED, TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_YELLOW, TFT_YELLOW, TFT_GREENYELLOW, TFT_GREEN, TFT_GREEN, TFT_EMERALD};

/*
    Smeter_bargraph
*/
void Smeter_bargraph(int16_t index_new)
{
  static int16_t index_old = 0;  //smeter table index = number of blocks to draw on smeter bar graph
  int16_t i;

#ifdef TST_MAX_SMETER_SWR
index_new = 10;
index_old = 0;
#endif

#ifdef DEBUG
index_new = 10;
#endif



  if(index_new >= MAX_Smeter_table)
  {
    index_new = MAX_Smeter_table - 1;
  }

  if(tx_enable_changed == true)  //if changed tx-rx = display clear
  {
    index_old = 0;  //print all bargraph
  }

  if(index_old == 0) 
  {
    //draw Smeter first block = fixed one = min one block
    tft.fillRect((bargraph_X + ((bargraph_dX + bargraph_dX_space) * 0)), bargraph_Y - 1 + bargraph_dY, bargraph_dX, 3 , Smeter_table_color[0]);
  }

  if(index_new > index_old)
  {
    //bigger signal
    for(i=index_old+1; i<=index_new; i++)
    {
      //draw blocks from actual to the new index
      tft.fillRect((bargraph_X + ((bargraph_dX + bargraph_dX_space) * i)), bargraph_Y - i + bargraph_dY, bargraph_dX, i + 2, Smeter_table_color[i]);
    }
  }
  else if(index_new < index_old)
  {
    //smaller signal
    for(i=index_new+1; i<=index_old; i++)
    {
      //erase blocks from actual to the new index
      tft.fillRect((bargraph_X + ((bargraph_dX + bargraph_dX_space) * i)), bargraph_Y - i + bargraph_dY, bargraph_dX, i + 2, TFT_BACKGROUND);
    }
  }

  index_old = index_new;

}

uint16_t TxPower_table_color[MAX_Smeter_table] = {  TFT_MAROON, TFT_MAROON, TFT_MAROON, TFT_MAROON, TFT_RED, TFT_RED, TFT_RED, TFT_RED, TFT_MAGENTA, TFT_MAGENTA, TFT_MAGENTA };


/*
    TxPower_bargraph
*/
void TxPower_bargraph(int16_t index_new)
{
  static int16_t index_old = 0;  //smeter table index = number of blocks to draw on smeter bar graph
  int16_t i;

#ifdef TST_MAX_SMETER_SWR
index_new = 10;
index_old = 0;
#endif

  if(index_new >= MAX_Smeter_table)
  {
    index_new = MAX_Smeter_table - 1;
  }

  if(tx_enable_changed == true)  //if changed tx-rx = display clear
  {
    index_old = 0;  //print all bargraph
  }

  if(index_old == 0)
  {
    //draw Smeter first block = fixed one = min one block
    tft.fillRect((bargraph_X + ((bargraph_dX + bargraph_dX_space) * 0)), bargraph_Y, bargraph_dX, bargraph_dY, TxPower_table_color[0]);
  }

  if(index_new > index_old)
  {
    //bigger signal
    for(i=index_old+1; i<=index_new; i++)
    {
      //draw blocks from actual to the new index
      tft.fillRect((bargraph_X + ((bargraph_dX + bargraph_dX_space) * i)), bargraph_Y, bargraph_dX, bargraph_dY, TxPower_table_color[i]);
    }
  }
  else if(index_new < index_old)
  {
    //smaller signal
    for(i=index_new+1; i<=index_old; i++)
    {
      //erase blocks from actual to the new index
      tft.fillRect((bargraph_X + ((bargraph_dX + bargraph_dX_space) * i)), bargraph_Y, bargraph_dX, bargraph_dY, TFT_BACKGROUND);
    }
  }

  index_old = index_new;
}




#define ABOVE_SCALE   12
#define TRIANG_TOP   (ABOVE_SCALE + 6)
#define TRIANG_WIDTH    8

int16_t triang_x_min, triang_x_max;

/*********************************************************
  
*********************************************************/
void display_fft_graf_top(void) 
{
  int16_t siz, j, x;  //i, y
  uint32_t freq_graf_ini;
  uint32_t freq_graf_fim;



    //graph min freq
    freq_graf_ini = (hmi_freq - ((FFT_NSAMP/2)*FRES) )/1000;
  
    //graph max freq
    freq_graf_fim = (hmi_freq + ((FFT_NSAMP/2)*FRES) )/1000;

   
    //little triangle indicating the center freq
    switch(dsp_getmode())  //{"USB","LSB","AM","CW"}
    {
      case 0:  //USB
        triang_x_min = (display_WIDTH/2);
        triang_x_max = (display_WIDTH/2)+TRIANG_WIDTH;
        tft.fillTriangle(display_WIDTH/2, Y_MIN_DRAW - ABOVE_SCALE, display_WIDTH/2, Y_MIN_DRAW - TRIANG_TOP, (display_WIDTH/2)+TRIANG_WIDTH, Y_MIN_DRAW - ABOVE_SCALE, TFT_YELLOW);
        tft.fillTriangle((display_WIDTH/2)-1, Y_MIN_DRAW - ABOVE_SCALE, display_WIDTH/2, Y_MIN_DRAW - TRIANG_TOP, (display_WIDTH/2)-TRIANG_WIDTH, Y_MIN_DRAW - ABOVE_SCALE, TFT_BACKGROUND);
        break;
      case 1:  //LSB
        triang_x_min = (display_WIDTH/2)-TRIANG_WIDTH;
        triang_x_max = (display_WIDTH/2);
        tft.fillTriangle(display_WIDTH/2, Y_MIN_DRAW - ABOVE_SCALE, 
                         display_WIDTH/2, Y_MIN_DRAW - TRIANG_TOP, (display_WIDTH/2)-TRIANG_WIDTH, Y_MIN_DRAW - ABOVE_SCALE, 
                         TFT_YELLOW);
        tft.fillTriangle((display_WIDTH/2)+1, Y_MIN_DRAW - ABOVE_SCALE, 
                          display_WIDTH/2, Y_MIN_DRAW - TRIANG_TOP, (display_WIDTH/2)+TRIANG_WIDTH, Y_MIN_DRAW - ABOVE_SCALE, 
                          TFT_BACKGROUND);
        break;
      case 2:  //AM
        triang_x_min = (display_WIDTH/2)-TRIANG_WIDTH;
        triang_x_max = (display_WIDTH/2)+TRIANG_WIDTH;
        tft.fillTriangle((display_WIDTH/2)-TRIANG_WIDTH, Y_MIN_DRAW - ABOVE_SCALE, display_WIDTH/2, Y_MIN_DRAW - TRIANG_TOP, (display_WIDTH/2)+TRIANG_WIDTH, Y_MIN_DRAW - ABOVE_SCALE, TFT_YELLOW);
        break;
      case 3:  //CW = LSB
        triang_x_min = (display_WIDTH/2)-(TRIANG_WIDTH*2/4);
        triang_x_max = (display_WIDTH/2);   //-(TRIANG_WIDTH*1/4);
        tft.fillTriangle(display_WIDTH/2, Y_MIN_DRAW - ABOVE_SCALE, 
                         display_WIDTH/2, Y_MIN_DRAW - TRIANG_TOP, (display_WIDTH/2)-TRIANG_WIDTH, Y_MIN_DRAW - ABOVE_SCALE, 
                         TFT_YELLOW);
        tft.fillTriangle((display_WIDTH/2)+1, Y_MIN_DRAW - ABOVE_SCALE, 
                          display_WIDTH/2, Y_MIN_DRAW - TRIANG_TOP, (display_WIDTH/2)+TRIANG_WIDTH, Y_MIN_DRAW - ABOVE_SCALE, 
                          TFT_BACKGROUND);
        break;
    }

    //erase old freqs on top of scale
    tft.fillRect(0, Y_MIN_DRAW - TRIANG_TOP - Y_CHAR1 + 8, display_WIDTH, Y_CHAR1 - 8, TFT_BACKGROUND);

    //plot scale on top of waterfall
    tft.drawFastHLine (0, Y_MIN_DRAW - 11, display_WIDTH, TFT_WHITE);
    tft.fillRect(0, Y_MIN_DRAW - 10, display_WIDTH, 10, TFT_BACKGROUND);
    tft.fillRect(triang_x_min, Y_MIN_DRAW - 10, (triang_x_max - triang_x_min + 1), 11, tft.color565(25, 25, 25)); //shadow
    
    x=0;
    for(; freq_graf_ini < freq_graf_fim; freq_graf_ini+=1)
    {
      if((freq_graf_ini % 10) == 0)
      {
        tft.drawFastVLine (x, Y_MIN_DRAW - 11, 5, TFT_WHITE);
      }
      if((freq_graf_ini % 50) == 0)
      {
        tft.drawFastVLine (x-1, Y_MIN_DRAW - 11, 7, TFT_WHITE);
        tft.drawFastVLine (x, Y_MIN_DRAW - 11, 7, TFT_WHITE);
        tft.drawFastVLine (x+1, Y_MIN_DRAW - 11, 7, TFT_WHITE);
      }
      if((freq_graf_ini % 100) == 0)
      {
         tft.drawFastVLine (x-1, Y_MIN_DRAW - 11, 10, TFT_GREEN);
         tft.drawFastVLine (x, Y_MIN_DRAW - 11, 10, TFT_GREEN);
         tft.drawFastVLine (x+1, Y_MIN_DRAW - 11, 10, TFT_GREEN);
    
         //write new freq values  on top of scale
         
         //tft.setTextColor(TFT_GREEN);
         sprintf(vet_char, "%lu", freq_graf_ini);
         siz = strlen(vet_char);
         if(x < (2*X_CHAR1))   //to much to left
         {
            tft_writexy_plus(1, TFT_GREEN, TFT_BACKGROUND,0,0,7,8,(uint8_t *)vet_char);  // was MANGENTA, changed to GREEN and changed yShift 8 down
         }
         else if((x+((siz-2)*X_CHAR1)) > display_WIDTH)  //to much to right
         {
            j = display_WIDTH - (siz*X_CHAR1);
            tft_writexy_plus(1, TFT_GREEN, TFT_BACKGROUND,0,j,7,8,(uint8_t *)vet_char);  
         }
         else
         {
            j = x - (2*X_CHAR1);
            tft_writexy_plus(1, TFT_GREEN, TFT_BACKGROUND,0,j,7,8,(uint8_t *)vet_char);  
         }
      }
      x+=2;
    }
  

}


uint8_t vet_graf_fft[GRAPH_NUM_LINES][FFT_NSAMP];    // [NL][NCOL]
//uint16_t vet_graf_fft_pos = 0;
/*********************************************************
  
*********************************************************/
void display_fft_graf(uint16_t freq)    //receive the actual freq to move the waterfall as changing the dial
{                                       //consider 1kHz for each pixel on horizontal
  int16_t x, y;
  uint16_t extra_color;
  static uint16_t freq_old = 7080;
  int16_t freq_change;

  freq_change = ((int16_t)freq - (int16_t)freq_old);  //how much then dial (freq) changed

  //erase waterfall area
  //tft.fillRect(0, Y_MIN_DRAW, GRAPH_NUM_COLS, GRAPH_NUM_LINES, TFT_BACKGROUND);

  extra_color = tft.color565(25, 25, 25);  //light shadow on center freq

  //plot waterfall
  //vet_graf_fft[GRAPH_NUM_LINES][GRAPH_NUM_COLS]   [NL][NCOL]
  for(y=0; y<GRAPH_NUM_LINES; y++)   // y=0 is the line at the bottom   y=(GRAPH_NUM_LINES-1) is the new line
  {
    //erase one waterfall line
    tft.drawFastHLine (0, (GRAPH_NUM_LINES + Y_MIN_DRAW - y), GRAPH_NUM_COLS, TFT_DARKBLUE);// use darkblue background

#ifdef WATERFALL_IN_BLOCK   // all lines in the waterfall move with the freq change (not only the new line)
    //move the waterfall in block according the freq change
    //if (freq changed) and (its is not the last line) // new line does not move, it is in the right position
    if((freq_change != 0) && (y < (GRAPH_NUM_LINES-1)))
    {      
      //  move the waterfall line to left or right  according to the freq change
      if(freq_change > 0)    // new freq greater than old
      {
        if(freq_change < GRAPH_NUM_COLS)   //change in the visible area
        {
          for(x=0; x<GRAPH_NUM_COLS; x++)
          {
            if((x + freq_change) < GRAPH_NUM_COLS)
            {
              vet_graf_fft[y][x] = vet_graf_fft[y][x + freq_change];   //move to left
            }
            else
            {
              vet_graf_fft[y][x] = 0;  //fill with empty
            }
          }    
        }
        else  //big change, make a empty line (erase the old data)
        {
          vet_graf_fft[y][x] = 0;
        }
      }
      else   // new freq smaller than old = freq_change is negative
      {
        if((-freq_change) < GRAPH_NUM_COLS)   //change in the visible area
        {
          for(x=GRAPH_NUM_COLS-1; x>=0; x--)
          {
            if((x + freq_change) > 0)
            {
              vet_graf_fft[y][x] = vet_graf_fft[y][x + freq_change];   //move to right
            }
            else
            {
              vet_graf_fft[y][x] = 0;  //fill with empty
            }
          }    
        }
        else  //big change, make a empty line (erase the old data)
        {
          vet_graf_fft[y][x] = 0;
        }
      }
    }      
#endif

    //plot waterfall line
    for(x=0; x<GRAPH_NUM_COLS; x++)
    {
#ifdef DEBUG    //random lines for debugging                   
static int lockX = 100;
static long pixelCount = 320;

if (pixelCount <= 0) {
  lockX = random(0, 320);             
  pixelCount = random(20000);       
}


    uint8_t brightness = random( 256);


    uint8_t r = brightness >> 3;      // 5 bits
    uint8_t g = brightness >> 2;      // 6 bits
    uint8_t b = brightness >> 3;      // 5 bits
    uint16_t shade = (r << 11) | (g << 5) | b;


tft.drawPixel(lockX, (GRAPH_NUM_LINES + Y_MIN_DRAW - y), shade);
pixelCount--;

#endif

      //plot one waterfall line
      if(vet_graf_fft[y][x] > 0)
      {
        if((x>=triang_x_min) && (x<=triang_x_max))  //tune shadow area
        {
          tft.drawPixel(x, (GRAPH_NUM_LINES + Y_MIN_DRAW - y), TFT_WHITE|extra_color); 
        }
        else
        {
          tft.drawPixel(x, (GRAPH_NUM_LINES + Y_MIN_DRAW - y), TFT_WHITE); 
        }
      }
      else
      {
        if((x>=triang_x_min) && (x<=triang_x_max))  //tune shadow area
        {
          tft.drawPixel(x, (GRAPH_NUM_LINES + Y_MIN_DRAW - y), TFT_BACKGROUND|extra_color); 
        }
      }
    }
  }



  //move graph data array one line up to open space for next FFT (new line will be the last line)
  for(y=0; y<(GRAPH_NUM_LINES-1); y++)
  {
    for(x=0; x<GRAPH_NUM_COLS; x++)
    {
      vet_graf_fft[y][x] = vet_graf_fft[y+1][x];
    }
  }


  freq_old = freq;  //take note of the freq center on last waterfall printed on display
}






void display_aud_graf_var(uint16_t aud_pos, uint16_t aud_var, uint16_t color)
{  
  int16_t x;
  int16_t aud; 
  
  for(x=0; x<AUD_GRAPH_NUM_COLS; x++)
  {
    aud = (aud_samp[aud_var][x+aud_pos]);
    
    if(aud < AUD_GRAPH_MIN)  //check boundaries
    {
      tft.drawPixel((x + X_MIN_AUD_GRAPH), (Y_MIN_AUD_GRAPH + AUD_GRAPH_MAX - AUD_GRAPH_MIN), color);    //lower line    
    }
    else if(aud > AUD_GRAPH_MAX)  //check boundaries
    {
      tft.drawPixel((x + X_MIN_AUD_GRAPH), (Y_MIN_AUD_GRAPH + AUD_GRAPH_MAX - AUD_GRAPH_MAX), color);    //upper line 
    }
    else
    {
      tft.drawPixel((x + X_MIN_AUD_GRAPH), (Y_MIN_AUD_GRAPH + AUD_GRAPH_MAX - aud), color);        
    }
  }
  
}




void display_aud_graf(void)
{
uint16_t aud_pos;
int16_t x;
int16_t aud_samp_trigger;
  
  //erase graphic area
  //tft.fillRect(0, Y_MIN_DRAW - 10, display_WIDTH, 10, TFT_BACKGROUND);
  tft.fillRect(X_MIN_AUD_GRAPH, Y_MIN_AUD_GRAPH, AUD_GRAPH_NUM_COLS, (AUD_GRAPH_MAX - AUD_GRAPH_MIN + 3), TFT_NAVY);

for (int y = Y_MIN_AUD_GRAPH; y < Y_MIN_AUD_GRAPH + (AUD_GRAPH_MAX - AUD_GRAPH_MIN + 3); y += 5) {
  for (int x = X_MIN_AUD_GRAPH; x < X_MIN_AUD_GRAPH + AUD_GRAPH_NUM_COLS; x += 5) {
    tft.drawPixel(x, y, TFT_DARKGREY);  // Added grid
  }
}

 // if(tx_enabled)
  {
 //   aud_samp_trigger = AUD_SAMP_MIC;  
  }
//  else
  {
    aud_samp_trigger = AUD_SAMP_I;
  }

  //find trigger point to start ploting
  for(x=0; x<AUD_GRAPH_NUM_COLS; x++)
  {
     if((aud_samp[aud_samp_trigger][x+0] > 0) &&
        (aud_samp[aud_samp_trigger][x+1] > 0) &&
        (aud_samp[aud_samp_trigger][x+2] > 0) &&
        (aud_samp[aud_samp_trigger][x+3] > 0) &&
        (aud_samp[aud_samp_trigger][x+4] > 0))
        {
          break;
        }
  }
  for(; x<AUD_GRAPH_NUM_COLS; x++)
  {
     if(aud_samp[aud_samp_trigger][x] < 0)
        {
          break;
        }
  }
  aud_pos = x;

  //plot each variable
  display_aud_graf_var(aud_pos, AUD_SAMP_I, TFT_GREEN);
  display_aud_graf_var(aud_pos, AUD_SAMP_Q, TFT_CYAN);
  display_aud_graf_var(aud_pos, AUD_SAMP_MIC, TFT_RED);
  display_aud_graf_var(aud_pos, AUD_SAMP_A, TFT_PINK);
  display_aud_graf_var(aud_pos, AUD_SAMP_PEAK, TFT_YELLOW);
  display_aud_graf_var(aud_pos, AUD_SAMP_GAIN, TFT_MAGENTA);

}


/*********************************************************
  Initial msgs on display  (after reset)
*********************************************************/
void display_intro(void) {
  char s[32];
  
  tft.init();
  tft.setRotation(ROTATION_SETUP);
 
  tft.fillScreen(TFT_BACKGROUND);

//  sprintf(s, "uSDR Pico");
  sprintf(s, "Arjan-5");  //name changed from uSDR Pico FFT
  tft_writexy_plus(3, TFT_YELLOW, TFT_BACKGROUND, 2,10,1,0,(uint8_t *)s);

  //tft.fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color)
  //tft.drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color)
  tft.drawRoundRect(35, 25, 250, 70, 15, TFT_YELLOW);

//  sprintf(s, "uSDR Pico FFT");  //name changed from uSDR Pico FFT
  sprintf(s, "16 Band SSB/AM/CW");  //name changed from uSDR Pico FFT
//  tft_writexy_plus(2, TFT_YELLOW, TFT_BACKGROUND, 0,0,3,10,(uint8_t *)s);
  tft_writexy_plus(1, TFT_YELLOW, TFT_BACKGROUND, 3,0,5,0,(uint8_t *)s);

  sprintf(s, "HF Transceiver");  //name changed from uSDR Pico FFT
//  tft_writexy_plus(2, TFT_YELLOW, TFT_BACKGROUND, 0,10,4,10,(uint8_t *)s);
  tft_writexy_plus(1, TFT_YELLOW, TFT_BACKGROUND, 4,0,6,6,(uint8_t *)s);

  tft_writexy_plus(1, TFT_YELLOW, TFT_BACKGROUND, 9,0,7,12,(uint8_t *)SW_VERSION);

//#ifdef PY2KLA_setup
//  sprintf(s, "by");
//  tft_writexy_plus(1, TFT_LIGHTGREY, TFT_BACKGROUND, 0,0,5,0,(uint8_t *)s);
//  sprintf(s, "PE1ATM");
//  sprintf(s, "by");
//  tft_writexy_plus(1, TFT_LIGHTGREY, TFT_BACKGROUND, 0,0,7,0,(uint8_t *)s);
  sprintf(s, "Arjan te Marvelde");
  tft_writexy_plus(1, TFT_SKYBLUE, TFT_BACKGROUND, 3,0,9,0,(uint8_t *)s);
//  sprintf(s, "and");
//  tft_writexy_plus(1, TFT_LIGHTGREY, TFT_BACKGROUND, 0,0,8,0,(uint8_t *)s);
// sprintf(s, "PY2KLA");
//  sprintf(s, "and");
//  tft_writexy_plus(1, TFT_LIGHTGREY, TFT_BACKGROUND, 0,0,9,0,(uint8_t *)s);
  sprintf(s, "Klaus Fensterseifer");
  tft_writexy_plus(1, TFT_SKYBLUE, TFT_BACKGROUND, 2,0,10,0,(uint8_t *)s);
//#endif  
}




/*********************************************************
  
*********************************************************/
void display_oscilloscope_legend(void) {

uint16_t xPos = 205, yPos = 95;

  tft.fillScreen(TFT_BACKGROUND);

  tft.setFreeFont(NULL);

  tft.setTextSize(1);
 /*
tft.setCursor(xPos, yPos);
tft.setTextColor(TFT_GREEN);
tft.print("I");

tft.setCursor(xPos + 10, yPos);
tft.setTextColor(TFT_CYAN, TFT_BACKGROUND);
tft.print("Q");

tft.setCursor(xPos + 20, yPos);
tft.setTextColor(TFT_PINK, TFT_BACKGROUND);
tft.print("A");


tft.setCursor(xPos, yPos+ 10);
tft.setTextColor(TFT_RED, TFT_BACKGROUND);
tft.print("MIC");

tft.setCursor(xPos,yPos + 20);
tft.setTextColor(TFT_YELLOW, TFT_BACKGROUND);
tft.print("PEAK");

tft.setCursor(xPos,yPos + 30);
tft.setTextColor(TFT_MAGENTA, TFT_BACKGROUND);
tft.print("GAIN"); 

*/

  tft.setFreeFont(NULL);

  tft.setTextSize(1);
  
tft.setCursor(xPos, yPos);

tft.setTextColor(TFT_GREEN);
tft.print("I ");

tft.setTextColor(TFT_CYAN, TFT_BACKGROUND);
tft.print("Q ");

tft.setTextColor(TFT_PINK, TFT_BACKGROUND);
tft.print("A ");

tft.setTextColor(TFT_RED, TFT_BACKGROUND);
tft.print("MIC ");

tft.setTextColor(TFT_YELLOW, TFT_BACKGROUND);
tft.print("PEAK ");

tft.setTextColor(TFT_MAGENTA, TFT_BACKGROUND);
tft.print("GAIN "); 


} 




/*********************************************************
  Initial msgs on display  (after reset)
*********************************************************/
void display_tft_countdown(bool show, uint16_t val) 
{
  char s[32];

  if(show == true)
  {
    tft.drawRoundRect(260, 100, 50, 40, 10, TFT_ORANGE);
    sprintf(s, "%d", val);  //name changed from uSDR Pico FFT

    //tft.fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color)
    if(val < 10)
    {
      tft_writexy_plus(1, TFT_ORANGE, TFT_BACKGROUND, 20,4,5,0,(uint8_t *)s);
      //tft.drawRoundRect(275, 100, 35, 40, 10, TFT_ORANGE);
    }
    else
    {
      tft_writexy_plus(1, TFT_ORANGE, TFT_BACKGROUND, 19,4,5,0,(uint8_t *)s);
      //tft.drawRoundRect(260, 100, 50, 40, 10, TFT_ORANGE);
    }
  }
  else  //fill with black = erase = close the window
  {
    tft.fillRoundRect(260, 100, 50, 40, 10, TFT_BACKGROUND);
  }
}





void display_tft_loop(void) 
{
  static uint32_t hmi_freq_fft;

  if (tx_enabled == false)  //waterfall only during RX
  {
    if (fft_display_graf_new == 1)    //design a new graphic only when a new line is ready from FFT
    {
      if(hmi_freq == hmi_freq_fft)
      {
        //plot waterfall graphic     
        display_fft_graf((uint16_t)(hmi_freq/500));  // warefall 110ms
      }
      else
      {
        //plot waterfall graphic     
        display_fft_graf((uint16_t)(hmi_freq_fft/500));  // warefall 110ms
        hmi_freq_fft = hmi_freq;
      }

      fft_display_graf_new = 0;  
      fft_samples_ready = 2;  //ready to start new sample collect
    }
  }

}
