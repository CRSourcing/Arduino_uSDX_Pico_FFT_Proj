#include <sys/_stdint.h>
/*
 * hmi.c
 *
 * Created: Apr 2021
 * Author: Arjan te Marvelde
 * May2022: adapted by Klaus Fensterseifer 
 * https://github.com/kaefe64/Arduino_uSDX_Pico_FFT_Proj
 * 
 * This file contains the HMI driver, processing user inputs.
 * It will also do the logic behind these, and write feedback to the LCD.
 *
 * The 4 auxiliary buttons have the following functions:
 * GP6 - Enter, confirm : Used to select menu items or make choices from a list
 * GP7 - Escape, cancel : Used to exit a (sub)menu or cancel the current action
 * GP8 - Left           : Used to move left, e.g. to select a digit
 * GP9 - Right			: Used to move right, e.g. to select a digit
 *
 * The rotary encoder (GP2, GP3) controls an up/down counter connected to some field. 
 * It may be that the encoder has a bushbutton as well, this can be connected to GP4.
 *     ___     ___
 * ___|   |___|   |___  A
 *   ___     ___     _
 * _|   |___|   |___|   B
 *
 * Encoder channel A triggers on falling edge. 
 * Depending on B level, count is incremented or decremented.
 * 
 * The PTT is connected to GP15 and will be active, except when VOX is used.
 *
 Application Menu Description
The menu uses the first line on display to show the status and the options.


Menu TUNE:
Menu position used operate the radio on normal condition. It shows the actual values for the menus: Mode, VOX, AGC and Pre.

Menu Mode:
Options: "USB","LSB","AM","CW"
Define the modulation mode used for transmition and reception.

Menu AGC:
Options: "NoAGC","Slow","Fast"
Define the Automatic Gain Control mode of actuation. (It needs improvement)

Menu Pre:
Options: "-30dB","-20dB","-10dB","0dB","+10dB"
Define the use of the attenuators and pre-amplifier on reception. It uses the relay board to switch between the options.

Menu VOX:
Options: "NoVOX","VOX-L","VOX-M","VOX-H"
Defines de use of the VOX feature. (It needs improvement)

Menu Band:
Options: "<2.5","2-6","5-12","10-24","20-40" MHz
Defines the filter used for RX and TX, and consequently the frequency band. It uses the relay board to switch between the options.

Menu Memory:
Options: "Save"
It saves the actual band setup on flash memory, so when we come back to this band after power on, it will start with the setup saved. The last band saved will be the selected band used after power on.

Menu Waterfall Gain: Press Enter and keep pressed. Change waterfall gain while pressed. Release.



 */
/*
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
*/
#include "Arduino.h"
#include "uSDR.h"
#include "relay.h"
#include "si5351.h"
#include "dsp.h"
#include "hmi.h"
#include "pico/multicore.h"
#include "SPI.h"
#include "TFT_eSPI.h"
#include "display_tft.h"

#include "CwDecoder.h"



//#define HMI_debug    10    //to release some serial print outs for debug

/*
 * GPIO assignments
 */
#define GP_ENC_A 2         // Encoder clock
#define GP_ENC_B 3         // Encoder direction
#define GP_AUX_0_Enter 6   // Enter, Confirm
#define GP_AUX_1_Escape 7  // Escape, Cancel
#define GP_AUX_2_Left 8    // Left move
#define GP_AUX_3_Right 9   // Right move
#define GP_MASK_IN ((1 << GP_ENC_A) | (1 << GP_ENC_B) | (1 << GP_AUX_0_Enter) | (1 << GP_AUX_1_Escape) | (1 << GP_AUX_2_Left) | (1 << GP_AUX_3_Right) | (1 << GP_PTT))
//#define GP_MASK_PTT	(1<<GP_PTT)




// Encoder settings

#define ENCODER_FALL 200                            //increment/decrement freq on falling of A encoder signal
#define ENCODER_TYPE ENCODER_FALL                   //choose to trigger the encoder step only on fall of A signal
#define ENCODER_CW_A_FALL_B_LOW 200                 //encoder type clockwise step when B low at falling of A
#define ENCODER_DIRECTION ENCODER_CW_A_FALL_B_HIGH  //direction related to B signal level when A signal is triggered


/*
 * Event flags
 */
//#define GPIO_IRQ_ALL		    (GPIO_IRQ_LEVEL_LOW|GPIO_IRQ_LEVEL_HIGH|GPIO_IRQ_EDGE_FALL|GPIO_IRQ_EDGE_RISE)
#define GPIO_IRQ_EDGE_ALL (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE)

/*
 * Display layout:
 *   +----------------+
 *   |USB 14074.0 R920| --> mode=USB, freq=14074.0kHz, state=Rx,S9+20dB
 *   |      Fast -10dB| --> ..., AGC=Fast, Pre=-10dB
 *   +----------------+
 * In this HMI state only tuning is possible, 
 *   using Left/Right for digit and ENC for value, Enter to commit change.
 * Press ESC to enter the submenu states (there is only one sub menu level):
 *
 * Submenu	Values								ENC		Enter			Escape	Left	Right
 * -----------------------------------------------------------------------------------------------
 * Mode		USB, LSB, AM, CW					change	commit			exit	prev	next
 * AGC		Fast, Slow, Off						change	commit			exit	prev	next
 * Pre		+10dB, 0, -10dB, -20dB, -30dB		change	commit			exit	prev	next
 * Vox		NoVOX, Low, Medium, High			change	commit			exit	prev	next
 *
 * --will be extended--
 */



//char hmi_o_menu[NUMBER_OF_MENUES][8] = {"Tune","Mode","AGC","Pre","VOX"};	// Indexed by hmi_menu  not used - menus done direct in Evaluate()
char hmi_o_mode[HMI_NUM_OPT_MODE][8] = { "USB", "LSB", "AM", "AM2", "CW " };           // Indexed by band_vars[hmi_band][HMI_S_MODE]  MODE_USB=0 MODE_LSB=1  MODE_AM=2  MODE_CW=3
char hmi_o_agc[HMI_NUM_OPT_AGC][8] = { "OFF", "Slow ", "Fast " };                      // Indexed by band_vars[hmi_band][HMI_S_AGC]
char hmi_o_pre[HMI_NUM_OPT_PRE][8] = { "-30dB", "-20dB", "-10dB", "0dB  ", "+10dB" };  // Indexed by band_vars[hmi_band][HMI_S_PRE]
char hmi_o_vox[HMI_NUM_OPT_VOX][8] = { "OFF", "LOW", "Mid", "HIGH" };                  // Indexed by band_vars[hmi_band][HMI_S_VOX]                                                            //index for NoVOX option
char hmi_o_bpf[NUMBER_OF_BANDS][16] = { "<2.5", "2-6", "5-12", "5-12", "10-24", "10-24", "10-24", "20-40", "20-40", "20-40", "20-40", "<2.5", "2-6", "5-12", "10-24", "20-40" };


const char hmi_o_band_name_displayed[NUMBER_OF_BANDS][20] = { "Amateur-160m", "Amateur-80m", "Amateur-40m", "Amateur-30m", "Amateur-20m", "Amateur-17m", "Amateur-15m", "Amateur-12m", "Citizen Band", "Amateur-10m/1", "Amateur-10m/2", "AM Radio", "2-6MHz", "6-12MHz", "12-24MHz", "24-40MHz" };



#define NUMBER_OF_BANDS 16

// Lower frequency limits in Hz
const uint32_t band_lower_limit[NUMBER_OF_BANDS] = {
  1800000,   // Amateur-160m
  3500000,   // Amateur-80m
  7000000,   // Amateur-40m
  10100000,  // Amateur-30m
  14000000,  // Amateur-20m
  18068000,  // Amateur-17m
  21000000,  // Amateur-15m
  24890000,  // Amateur-12m
  26965000,  // Citizen Band (CB)
  28000000,  // Amateur-10m/1
  28500000,  // Amateur-10m/2
  520000,    // AM Radio
  2000000,   // Shortwave 2–6 MHz
  6000000,   // Shortwave 6–12 MHz
  12000000,  // Shortwave 12–24 MHz
  24000000   // Shortwave 24–40 MHz
};

// Upper frequency limits in Hz
const uint32_t band_upper_limit[NUMBER_OF_BANDS] = {
  2000000,   // Amateur-160m
  4000000,   // Amateur-80m
  7300000,   // Amateur-40m
  10150000,  // Amateur-30m
  14350000,  // Amateur-20m
  18168000,  // Amateur-17m
  21450000,  // Amateur-15m
  24990000,  // Amateur-12m
  27700000,  // Citizen Band (CB)
  28500000,  // Amateur-10m/1
  29700000,  // Amateur-10m/2
  2000000,   // AM Radio
  6000000,   // Shortwave 2–6 MHz
  12000000,  // Shortwave 6–12 MHz
  24000000,  // Shortwave 12–24 MHz
  40000000   // Shortwave 24–40 MHz
};







// Map option to setting
uint8_t hmi_pre[5] = { REL_ATT_30, REL_ATT_20, REL_ATT_10, REL_ATT_00, REL_PRE_10 };
uint8_t hmi_bpf[16] = { REL_LPF2, REL_BPF6, REL_BPF12, REL_BPF12, REL_BPF24, REL_BPF24, REL_BPF24, REL_BPF40,
                        REL_BPF40, REL_BPF40, REL_BPF40, REL_LPF2, REL_BPF6, REL_BPF12, REL_BPF24, REL_BPF40 };

uint8_t hmi_menu;               // menu section 0=Tune/cursor 1=Mode 2=AGC 3=Pre 4=VOX 5=Band 6=Mem  (old hmi_state)
int8_t hmi_menu_opt_display;    // current menu option showing on display (it will be copied to band vars on <enter>)  (old hmi_option) // was uint, changed to int to loop the cursor
uint8_t hmi_band = START_BAND;  // actual band, start with predefined band


//                                             10 Presets with 6 menus    0=Tune/cursor 1=Mode 2=AGC 3=Pre 4=VOX 5=Band
uint8_t band_vars[NUMBER_OF_BANDS][NUMBER_OF_MENUES] = {
  { 4, 1, 2, 3, 0, 0 },
  { 4, 1, 2, 3, 0, 1 },
  { 4, 1, 2, 3, 0, 2 },
  { 4, 0, 2, 3, 0, 3 },
  { 4, 0, 2, 3, 0, 4 },
  { 4, 0, 2, 3, 0, 5 },
  { 4, 0, 2, 3, 0, 6 },
  { 4, 0, 2, 3, 0, 7 },
  { 4, 0, 2, 3, 0, 8 },
  { 4, 0, 2, 3, 0, 9 },
  { 4, 0, 2, 3, 0, 10 },
  { 4, 2, 2, 3, 0, 11 },
  { 4, 2, 2, 3, 0, 12 },
  { 4, 2, 2, 3, 0, 13 },
  { 4, 2, 2, 3, 0, 14 },
  { 4, 2, 2, 3, 0, 15 }
};

uint32_t band_starting_freq[NUMBER_OF_BANDS] = { b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15 };


uint32_t hmi_freq;  // Frequency from Tune state

const uint32_t hmi_step[HMI_NUM_OPT_TUNE] = { 10000000L, 1000000L, 100000L, 10000L, 1000L, 100L, 10L };  // Frequency digit increments (tune option = cursor position)

const uint32_t hmi_minfreq[NUMBER_OF_BANDS] = { 0000000L, 2000000L, 5000000L, 5000000L, 10000000L, 10000000L, 10000000L, 20000000L, 20000000L, 20000000L, 20000000L, 000000L, 2000000L, 5000000L, 10000000L, 20000000L };      // min freq for each band from pass band filters
const uint32_t hmi_maxfreq[NUMBER_OF_BANDS] = { 2500000L, 6000000L, 12000000L, 12000000L, 24000000L, 24000000L, 24000000L, 40000000L, 40000000L, 40000000L, 40000000L, 2500000L, 6000000L, 12000000L, 24000000L, 40000000L };  // max freq for each band from pass band filters

//#ifdef PY2KLA_setup
//#define HMI_MULFREQ 4  // Factor between HMI and actual frequency
//#else
#define HMI_MULFREQ 4  // Factor between HMI and actual frequency \
                       // Set to 1, 2 or 4 for certain types of mixer
//#endif


char s[80];  //aux to print to the screen

bool tx_enabled = false;
bool tx_enable_changed = true;
bool ptt_internal_active = false;  //PTT output = true for vox, mon and mem
bool ptt_external_active = false;  //external = from mike
//these inputs will generate the tx_enabled to transmit
bool ptt_vox_active = false;  //if vox whants to transmit
bool ptt_mon_active = false;
bool ptt_aud_active = false;

uint8_t sel_graph = 0;  // select scope graphic

uint16_t tox = 0, toy = 0;  //touch coordinates
uint8_t touch_delay = 0;    // block get_touch if needed
int16_t fft_gain_old = 0;


//***********************************************************************
//
// get info from actual band = freq -> and store it at band_vars
// when switching back to that band, it will be at the same freq and setup
//
//
//***********************************************************************
void Store_Last_Band(uint8_t band) {


  band_starting_freq[band] = hmi_freq;
}


//***********************************************************************
//
// get band info from band_vars -> and set  freq
//
//***********************************************************************
void Setup_Band(uint8_t band) {
  uint16_t j;


  hmi_freq = band_starting_freq[band];

  band_vars[hmi_band][HMI_S_TUNE] = 4;  // start every band with a fixed 1KHz step

  if (hmi_freq > hmi_maxfreq[band])  //checking boundaries
  {
    //Serialx.print("hmi_freq > hmi_maxfreq[band");
    hmi_freq = hmi_maxfreq[band];
  } else if (hmi_freq < hmi_minfreq[band]) {
    //Serialx.print("hmi_freq < hmi_minfreq[band");
    hmi_freq = hmi_minfreq[band];
  }


  fft_gain = BAND_RELATED_FFT_GAIN;  // increase fft gain with freq since signal strength and atmospheric noise decreases

  print_Band(band);


  //set the new band to display and freq

  SI_SETFREQ(0, HMI_MULFREQ * hmi_freq);  // Set freq to hmi_freq (MULFREQ depends on mixer type)
  SI_SETPHASE(0, 1);                      // Set phase to 90deg (depends on mixer type)

  //ptt_state = 0;
  ptt_external_active = false;

  dsp_setmode(band_vars[band][HMI_S_MODE]);  //MODE_USB=0 MODE_LSB=1  MODE_AM=2  MODE_CW=3
  dsp_setvox(band_vars[band][HMI_S_VOX]);
  dsp_setagc(band_vars[band][HMI_S_AGC]);
  relay_setattn(hmi_pre[band_vars[band][HMI_S_PRE]]);
  relay_setband(hmi_bpf[band_vars[band][HMI_S_BPF]]);
}




//***********************************************************************
void print_Band(uint8_t band) {


  if (band < 12)  // Amateur bands
    sprintf(s, "%s: "
               "%lu-%lu KHz",
            hmi_o_band_name_displayed[band], band_lower_limit[band] / 1000, band_upper_limit[band] / 1000);
  else
    sprintf(s, " Shortwave %s ", hmi_o_band_name_displayed[band]);

  tft.fillRect(0, 72, 240, 8, TFT_BACKGROUND);  //clear menu selection
  tft.setTextFont(1);
  tft.setCursor(0, 72);
  if (hmi_o_band_name_displayed[hmi_menu_opt_display][7] == '-') {  // Amateur band label 7th characer is -, for example Amateur-20m


    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print(s);
  }

  else {

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.print(s);
  }

  tft.setFreeFont(FONT1);
  // Serialx.println(s);
}



//*********************************************************************************//

/*
 * HMI State Machine,
 * Handle event according to current state
 * Code needs to be optimized
 */





void hmi_handler(uint8_t event) {


  uint8_t hmi_menu_last = HMI_S_BPF;  //Always start with bands menue

#ifdef USE_TOUCH_SCREEN

  if (event == 99) {

    // Simulate encoder
    if (toy > 135 && toy < 160) {
      if (tox > 160 && tox < 200) {
        event = HMI_E_DECREMENT;
      } else if (tox > 200 && tox < 240) {
        event = HMI_E_INCREMENT;
      }
    }


    if (toy > 80 && toy < 115 && tox > 160 && tox < 240) {  // simulate Enter
      event = HMI_E_SUBMENU;                               
      touch_delay = 1;
    }


    if (toy > 115 && toy < 135) {  // simulate L/R

      if (tox > 200 && tox < 240)
        event = HMI_E_RIGHT;

      if (tox > 160 && tox < 200)
        event = HMI_E_LEFT;

      touch_delay = 2;
    }


    if (tox > 250 && toy > 100 && toy < 170) {  // snippet to select an oscilloscope trace by touching the screen
      if (sel_graph < 6)
        sel_graph++;
      else
        sel_graph = 0;
      touch_delay = 3;
      return;
    }


    if (toy > 190) {  // snippet to adjust hmi_freq when tapping on waterfall
      if (tox < 160)
        hmi_freq -= (80 - tox / 2) * 1000;
      if (tox > 160)
        hmi_freq += (tox / 2 - 80) * 1000;
      touch_delay = 2;

      if (band_vars[hmi_band][HMI_S_TUNE] != 4) {
        hmi_menu_opt_display = 4;
        band_vars[hmi_band][HMI_S_TUNE] = hmi_menu_opt_display;  // set to 1KHz step
      }
      return;
    }

    if (toy > 150 && toy < 170 && tox < 160)  // snippet to ajust fft gain
      fft_gain = tox;
  }


  // snippet to set cursor with touch
  if (toy > 15 && toy < 50) {

    touch_delay = 2;

    if (tox > 30 && tox < 50)
      hmi_menu_opt_display = 1;

    if (tox > 60 && tox < 80)
      hmi_menu_opt_display = 2;

    if (tox > 90 && tox < 110)
      hmi_menu_opt_display = 3;

    if (tox > 115 && tox < 135)
      hmi_menu_opt_display = 4;

    if (tox > 170 && tox < 190)
      hmi_menu_opt_display = 5;

    if (tox > 200 && tox < 220)
      hmi_menu_opt_display = 6;


    band_vars[hmi_band][HMI_S_TUNE] = hmi_menu_opt_display;


    if (tox > 250)
      hmi_freq = hmi_freq / 1000 * 1000;  // round down to full KHz when tapping on KHz
  }

#endif

  if ((event == HMI_PTT_ON) && (ptt_internal_active == false))  //if internal is taking the ptt control, not from mike, ignores mike
  {
    ptt_external_active = true;
  } else if (event == HMI_PTT_OFF) {
    ptt_external_active = false;
    ;
  }

  /* Special case for TUNE state */
  if (hmi_menu == HMI_S_TUNE)  //We are on main menu
  {


#ifdef USE_TOUCH_SCREEN
    if ((event == HMI_E_ENTER) || (tox > 250 && (toy > 45 && toy < 80))) {  // set mode through touch
      touch_delay = 5;
#else
    if (event == HMI_E_ENTER)  // ENTER now sets mode
    {
#endif
      band_vars[hmi_band][HMI_S_MODE]++;
      if (band_vars[hmi_band][HMI_S_MODE] > 4)
        band_vars[hmi_band][HMI_S_MODE] = 0;
      hmi_menu_opt_display = band_vars[hmi_band][HMI_S_MODE];
    }

    if (event == HMI_E_SUBMENU)
    // Enter submenus
    {

      //band_vars[hmi_band][hmi_menu] = hmi_menu_opt_display;							// Store cursor position on TUNE
      hmi_menu = hmi_menu_last;  // go to last menu selected before TUNE



      hmi_menu_opt_display = band_vars[hmi_band][hmi_menu];  // Restore selection of new menu
    }

    else if (event == HMI_E_INCREMENT || event == HMI_E_DECREMENT) {


      uint8_t step_index = band_vars[hmi_band][HMI_S_TUNE];  // fixes step change bug
      int32_t step = hmi_step[step_index];

      // Apply increment or decrement
      if (event == HMI_E_INCREMENT) {
        hmi_freq += step;
        if (hmi_freq > band_upper_limit[hmi_band]) {
          hmi_freq = band_lower_limit[hmi_band];  // wrap
        }
      } else {  // HMI_E_DECREMENT
        hmi_freq -= step;
        if (hmi_freq < band_lower_limit[hmi_band]) {
          hmi_freq = band_upper_limit[hmi_band];  // wrap
        }
      }
    }


    if (event == HMI_E_RIGHT) {  // cursor position
      // Move selection to the right, but don't exceed max
      if (hmi_menu_opt_display < HMI_NUM_OPT_TUNE - 1) {
        hmi_menu_opt_display++;
      }

      // Wrap around if above 6
      if (hmi_menu_opt_display > 6) {
        hmi_menu_opt_display = 1;
      }

      // Update band variable
      band_vars[hmi_band][HMI_S_TUNE] = hmi_menu_opt_display;
    }

    if (event == HMI_E_LEFT) {

      if (hmi_menu_opt_display > 0) {
        hmi_menu_opt_display--;
      }

      // Wrap around if at 0
      if (hmi_menu_opt_display == 0) {
        hmi_menu_opt_display = 6;
      }

      // Update band variable
      band_vars[hmi_band][HMI_S_TUNE] = hmi_menu_opt_display;
    }


  } else  //in submenus

  {




    /* Submenu states */
    switch (hmi_menu) {


        // menu section 0=Tune/cursor 1=Mode 2=AGC 3=Pre 4=VOX 5=Band


      case HMI_S_MODE:

        if (band_vars[hmi_band][HMI_S_MODE] == MODE_CW)  // need to make room for CW decoder
          tft.fillRect(0, 0, 360, 16, TFT_BACKGROUND);

      case HMI_S_AGC:
        if (event == HMI_E_INCREMENT)
          hmi_menu_opt_display = (hmi_menu_opt_display < HMI_NUM_OPT_AGC - 1) ? hmi_menu_opt_display + 1 : HMI_NUM_OPT_AGC - 1;
        else if (event == HMI_E_DECREMENT)
          hmi_menu_opt_display = (hmi_menu_opt_display > 0) ? hmi_menu_opt_display - 1 : 0;
        break;
      case HMI_S_PRE:
        if (event == HMI_E_INCREMENT)
          hmi_menu_opt_display = (hmi_menu_opt_display < HMI_NUM_OPT_PRE - 1) ? hmi_menu_opt_display + 1 : HMI_NUM_OPT_PRE - 1;
        else if (event == HMI_E_DECREMENT)
          hmi_menu_opt_display = (hmi_menu_opt_display > 0) ? hmi_menu_opt_display - 1 : 0;
        break;
      case HMI_S_VOX:
        if (event == HMI_E_INCREMENT)
          hmi_menu_opt_display = (hmi_menu_opt_display < HMI_NUM_OPT_VOX - 1) ? hmi_menu_opt_display + 1 : HMI_NUM_OPT_VOX - 1;
        else if (event == HMI_E_DECREMENT)
          hmi_menu_opt_display = (hmi_menu_opt_display > 0) ? hmi_menu_opt_display - 1 : 0;
        break;
      case HMI_S_BPF:
        if (event == HMI_E_INCREMENT)
          hmi_menu_opt_display = (hmi_menu_opt_display < NUMBER_OF_BANDS - 1) ? hmi_menu_opt_display + 1 : NUMBER_OF_BANDS - 1;
        else if (event == HMI_E_DECREMENT)
          hmi_menu_opt_display = (hmi_menu_opt_display > 0) ? hmi_menu_opt_display - 1 : 0;
        break;
      case HMI_S_FFT:
        if (event == HMI_E_INCREMENT)
          fft_gain += 2;
        else if (event == HMI_E_DECREMENT)
          fft_gain -= 2;
        break;
      case HMI_S_OSC:  // selects trace in the mini oscilloscope
        if (event == HMI_E_INCREMENT && sel_graph == 6)
          sel_graph = 0;
        else if (event == HMI_E_INCREMENT && sel_graph < 6)
          sel_graph++;

        else if (event == HMI_E_DECREMENT && sel_graph)
          sel_graph--;

        break;
    }

    /* General actions for all submenus */




    if (event == HMI_E_ENTER) {
      ptt_external_active = !ptt_external_active;  // for tx debugging
      tx_enabled = !tx_enabled;
      tx_enable_changed = true;
      return;
    }

    if (hmi_menu == HMI_S_BPF)
      hmi_band = hmi_menu_opt_display;  //band changed
    else {

      hmi_menu = constrain(hmi_menu, 2, 7);  // 2=AGC 3=Pre 4=VOX 5=Band

      band_vars[hmi_band][hmi_menu] = hmi_menu_opt_display;  // Store selected option
    }

    if (event == HMI_E_SUBMENU) {

      {

        hmi_menu_last = hmi_menu;
        hmi_menu = HMI_S_TUNE;                                 // Leave submenus
        hmi_menu_opt_display = band_vars[hmi_band][hmi_menu];  // Restore selection of new state
      }
    } else if (event == HMI_E_RIGHT) {
      hmi_menu = (hmi_menu < NUMBER_OF_MENUES - 1) ? (hmi_menu + 1) : 1;  // Change submenu
      hmi_menu_opt_display = band_vars[hmi_band][hmi_menu];               // Restore selection of new state
    } else if (event == HMI_E_LEFT) {
      hmi_menu = (hmi_menu > 1) ? (hmi_menu - 1) : NUMBER_OF_MENUES - 1;  // Change submenu
      hmi_menu_opt_display = band_vars[hmi_band][hmi_menu];               // Restore selection of new state
    }
  }
}



/*
 * GPIO IRQ callback routine
 * Sets the detected event and invokes the HMI state machine
 */
void hmi_callback(uint gpio, uint32_t events) {
  uint8_t evt = HMI_E_NOEVENT;

  switch (gpio) {
    case GP_ENC_A:  // Encoder
      if (events & GPIO_IRQ_EDGE_FALL) {
#if ENCODER_DIRECTION == ENCODER_CW_A_FALL_B_HIGH
        evt = gpio_get(GP_ENC_B) ? HMI_E_INCREMENT : HMI_E_DECREMENT;
#else
        evt = gpio_get(GP_ENC_B) ? HMI_E_DECREMENT : HMI_E_INCREMENT;
#endif
      }
#if ENCODER_TYPE == ENCODER_FALL_AND_RISE
      else if (events & GPIO_IRQ_EDGE_RISE) {
#if ENCODER_DIRECTION == ENCODER_CW_A_FALL_B_HIGH
        evt = gpio_get(GP_ENC_B) ? HMI_E_DECREMENT : HMI_E_INCREMENT;
#else
        evt = gpio_get(GP_ENC_B) ? HMI_E_INCREMENT : HMI_E_DECREMENT;
#endif
      }
#endif


      break;
    case GP_AUX_0_Enter:  // Enter
      if (events & GPIO_IRQ_EDGE_FALL) {
        evt = HMI_E_ENTER;
      }
      break;
    case GP_AUX_1_Escape:  // Escape
      if (events & GPIO_IRQ_EDGE_FALL) {
        evt = HMI_E_SUBMENU;
      }
      break;
    case GP_AUX_2_Left:  // Previous
      if (events & GPIO_IRQ_EDGE_FALL) {
        evt = HMI_E_LEFT;
      }
      break;
    case GP_AUX_3_Right:  // Next
      if (events & GPIO_IRQ_EDGE_FALL) {
        evt = HMI_E_RIGHT;
      }
      break;

    case GP_PTT:  // PTT TX
                  /*
    if (events&GPIO_IRQ_EDGE_ALL)
    {
      evt = gpio_get(GP_PTT)?HMI_PTT_OFF:HMI_PTT_ON;
    }
*/
      if (events & GPIO_IRQ_EDGE_FALL) {
        evt = HMI_PTT_ON;
      }
      if (events & GPIO_IRQ_EDGE_RISE) {
        evt = HMI_PTT_OFF;
      }

      break;
    default:
      return;
  }




  hmi_handler(evt);  // Invoke state machine
}



/*
 * Initialize the User interface
 * It could take some time to read all DFLASH hmi data, so make it when display is showing title
 */
void hmi_init0(void) {
  // Initialize LCD and set VFO
  Setup_Band(hmi_band);  //                                                             why was this commented out?
                         //  menu position = Tune  and  cursor position = hmi_menu_opt_display
  hmi_menu = HMI_S_TUNE;
  hmi_menu_opt_display = band_vars[hmi_band][HMI_S_TUNE];  // option on Tune is the cursor position

  Serialx.println("TUNE " + String(hmi_band));
  Serialx.println("hmi_menu  " + String(hmi_menu));
  Serialx.println("hmi_menu_opt_display " + String(hmi_menu_opt_display));


  CwDecoder_InicTable();  //fill table on running time
}


/*
 * Initialize the User interface
 */


void hmi_init(void) {
  /*
	 * Notes on using GPIO interrupts: 
	 * The callback handles interrupts for all GPIOs with IRQ enabled.
	 * Level interrupts don't seem to work properly.
	 * For debouncing, the GPIO pins should be pulled-up and connected to gnd with 100nF.
	 * PTT has separate debouncing logic
	 */

  // Init input GPIOs
  gpio_init_mask(GP_MASK_IN);  //	Initialise multiple GPIOs (enabled I/O and set func to GPIO_FUNC_SIO)
                               //  Clear the output enable (i.e. set to input). Clear any output value.

  // Enable pull-ups on input pins
  gpio_pull_up(GP_ENC_A);
  gpio_pull_up(GP_ENC_B);
  gpio_pull_up(GP_AUX_0_Enter);
  gpio_pull_up(GP_AUX_1_Escape);
  gpio_pull_up(GP_AUX_2_Left);
  gpio_pull_up(GP_AUX_3_Right);
  gpio_pull_up(GP_PTT);
  /*	
  gpio_set_dir_in_masked(GP_MASK_IN);   //don't need,  already input by  gpio_init_mask()

for(;;)
{
      gpio_put(GP_PTT, 0);      //drive PTT low (active)
      gpio_set_dir(GP_PTT, GPIO_OUT);   // PTT output
      delay(2000);

      //gpio_put(GP_PTT, 0);      //drive PTT low (active)
      gpio_set_dir(GP_PTT, GPIO_IN);   // PTT output
      delay(2000);
}
*/


  // Enable interrupt on level low
  gpio_set_irq_enabled(GP_ENC_A, GPIO_IRQ_EDGE_ALL, true);
  gpio_set_irq_enabled(GP_AUX_0_Enter, GPIO_IRQ_EDGE_ALL, true);
  gpio_set_irq_enabled(GP_AUX_1_Escape, GPIO_IRQ_EDGE_ALL, true);
  gpio_set_irq_enabled(GP_AUX_2_Left, GPIO_IRQ_EDGE_ALL, true);
  gpio_set_irq_enabled(GP_AUX_3_Right, GPIO_IRQ_EDGE_ALL, true);
  //	gpio_set_irq_enabled(GP_PTT, GPIO_IRQ_EDGE_ALL, false);
  gpio_set_irq_enabled(GP_PTT, GPIO_IRQ_EDGE_ALL, true);

  // Set callback, one for all GPIO, not sure about correctness!
  gpio_set_irq_enabled_with_callback(GP_ENC_A, GPIO_IRQ_EDGE_ALL, true, hmi_callback);

#ifdef USE_TOUCH_SCREEN
  uint16_t calData[5] = { 379, 3519, 197, 3591, 1 };  // modify as needed
  tft.setTouch(calData);
#endif
}




#define x_RT 10  //position for R+smeter and T+power
#define y_RT 80

//#define x_xGain ((1*X_CHAR1))  //position for fft gain
//#define y_yGain ((3*Y_CHAR1)+8-8)

#define x_xGain (0)  //position for fft gain
#define y_yGain (150)


#define x_plus1 (x_RT + (2 * X_CHAR2))  //position for the first +
#define y_plus1 (y_RT + 6)

#define x_plus2 (x_RT + (2 * X_CHAR2) + 13)  //position for the second +
#define y_plus2 (y_RT - 3)



/*
S Meter  	Antenna input
Reading   uVrms @ 50R
S9+20	    500
S9+10	    160
S9 	       50
S8 	       25
S7 	       12,5
S6 	       6,25
S5 	       3,125
S4 	       1,5625
S3 	       0,78125
S2 	       0,39063
S1 	       0,19531
*/
//                                             S  1  2  3  4   5   6   7    8    9   9+  9++
int16_t Smeter_table_level[MAX_Smeter_table] = { 1, 2, 4, 9, 18, 35, 75, 150, 300, 400, 600 };  //audio signal value after filters for each antenna level input



// SMeter adjust for "-30dB","-20dB","-10dB","0dB","+10dB"
//                    x22     x8.48   x2.8    x1     x2
const int32_t smeter_pre_mult[HMI_NUM_OPT_PRE] = { 353, 136, 46, 1, 2 };  // S level =  (max_a_sample * smeter_pre_mult) >> smeter_pre_shift
const int16_t smeter_pre_shift[HMI_NUM_OPT_PRE] = { 4, 4, 4, 0, 0 };      // it makes "shift" instead of "division", tries to save processing
int16_t rec_level;
int16_t rec_level_old = 1;

/**************************************************************************************
    hmi_smeter - writes the S metr value on display
**************************************************************************************/
void hmi_smeter(void) {

  //  static int16_t agc_gain_old = 1;
  int16_t Smeter_index_new;
  static int16_t Smeter_index = 0;  //smeter table index = number of blocks to draw on smeter bar graph


  /*
    if(agc_gain_old != agc_gain)
    {
      rec_level = AGC_GAIN_MAX - agc_gain;
      sprintf(s, "%d", rec_level);
      tft_writexy_(2, TFT_GREEN, TFT_BACKGROUND, 1,2,(uint8_t *)s);
      agc_gain_old = agc_gain;
    }
*/
  if (smeter_display_time >= MAX_SMETER_DISPLAY_TIME)  //new value ready to display, and avoid to write variable at same time on int
  {
    //correcting input ADC value with attenuators
    max_a_sample = (max_a_sample * smeter_pre_mult[band_vars[hmi_band][HMI_S_PRE]]) >> smeter_pre_shift[band_vars[hmi_band][HMI_S_PRE]];

    //look for smeter table index
    for (Smeter_index_new = (MAX_Smeter_table - 1); Smeter_index_new > 0; Smeter_index_new--) {
      if (max_a_sample > Smeter_table_level[Smeter_index_new]) {
        break;
      }
    }
    Smeter_bargraph(Smeter_index_new);

    rec_level = Smeter_index_new + 1;  // S level = index + 1

#ifdef TST_MAX_SMETER_SWR
    rec_level = 11;
    rec_level_old = 6;
    //tft.fillRect(x_plus1, y_plus1, X_CHAR1-1, Y_CHAR1-4, TFT_LIGHTGREY); //TFT_BACKGROUND);
    //tft.fillRect(x_plus2, y_plus2, X_CHAR1-1, Y_CHAR1-4, TFT_LIGHTGREY); //TFT_BACKGROUND);
#endif

    // if (tx_enable_changed == true)  //if changed tx-rx = display clear
    //{
    // rec_level_old = 0;  //print all
    //  fft_gain_old = 0;
    // }

/*
      rec_level = rec_level_old;
      if(++contk > 5)
      {
        rec_level = rec_level_old + 1;
        if (rec_level > 11)  rec_level = 1;
        contk = 0;
      }
*/
#ifdef SMETER_TEST  //used to get the audio level for a RF input signal -> to fill the Smeter_table_level[]
    //prints the audio level to display,
    sprintf(s, "%3d", max_a_sample);
    //s[3] = 0;  //remove the low digit
    tft_writexy_plus(1, TFT_GREEN, TFT_BACKGROUND, 20, 5, 3, 2, (uint8_t *)s);
#endif

    display_a_sample = max_a_sample;  //save the last value printed on display
    max_a_sample = 0;                 //restart the search for big signal
    smeter_display_time = 0;
  }

  if (fft_gain_old != fft_gain) {

    if (!tox) {  // touch was not used
      tft.setTextColor(TFT_MAGENTA);
      sprintf(s, "Set FFT gain: %d      ", fft_gain);
      tft.fillRect(0, 0, 320, 15, TFT_BLACK);
      tft_writexy_(1, TFT_MAGENTA, TFT_BACKGROUND, 0, 0, (uint8_t *)s);
    }


    sprintf(s, "%d", fft_gain);
    s[3] = 0;
    tft.setFreeFont(FONT1);
    tft.fillRect(0, 142, 160, 15, TFT_DARKPURPLE);
    tft.setCursor(15, 155);
    tft.setTextColor(TFT_ORANGE);
    tft.print("FFTGAIN:");
    tft.print(s);
    fft_gain_old = fft_gain;
  }
}

/*
  if(smeter_display_time < MAX_SMETER_DISPLAY_TIME)   //for some time, look for the bigger signal
  {
    if(avg_a_sample > max_a_sample)  //bigger than displayed (if the value is bigger, print on display right now)
    {
      max_a_sample = avg_a_sample;  //save the bigger audio signal received
      if(max_a_sample > display_a_sample)  //bigger than displayed (if the value is bigger, print on display right now)
      {
        smeter_display_time = MAX_SMETER_DISPLAY_TIME;  //indicate the new (bigger) value is ready to show at display
      }
    }
    else
    {
      smeter_display_time++;  //count time
    }
  }
*/


#if I2C_Arduino_Pro_Mini == 1  //only used when together with Arduino Pro Mini for relays control (and allow SWR reading)


#define TIME_SHOW 6  // x 100ms = "some time"

/**************************************************************************************
    hmi_power_show - used to show the recent bigger power value on the display for "some time"
**************************************************************************************/
bool hmi_power_show(int16_t pow) {
  static int16_t pow_big;
  static int16_t time = 0;
  bool ret;

  if (++time > TIME_SHOW)  //count every 100ms
  {
    time = 0;
    pow_big = 0;
    ret = true;  // indicates to show the actual value on display  as the "old" big value already stayed at display for "some time"
  } else {
    if (pow > pow_big) {
      pow_big = pow;  //holds the new bigger value
      time = 0;       // start the timer to keep the value some time
      ret = true;     // indicates to show the actual value on display right now  as it is bigger
    } else {
      ret = false;  // indicates to keep the value already showed
    }
  }
  return ret;
}


#define SWR_POW_MAX 10                                                                    //max power in watts on display (same number of ADC values at swr_pow table)
#define SWR_BARGRAPH_MAX 10                                                               //number of steps for power on bragraph
const int16_t power_adc[SWR_POW_MAX] = { 20, 40, 60, 80, 100, 120, 140, 160, 180, 200 };  //ADC 160 = 8W measured (values not calibraterd)


/**************************************************************************************
    hmi_power_swr - reads the swr and put on display
**************************************************************************************/
void hmi_power_swr(void)  //read the swr from Arduino Pro Mini I2C
{
  static uint8_t i2c_data[3];
  static uint8_t i2c_data0_old = 0;
  int16_t ret;
  int16_t pow;

  ret = i2c_read_blocking(i2c1, I2C_SWR, i2c_data, 3, false);  // get 3 bytes: SWR, FOR and REF

  ret = 3;
  if (ret == 3)  //number os bytes received  (<0 if no answer from I2C slave)

  {

    if (tx_enable_changed == true)  //if changed tx-rx = display clear
    {
      
       tft.fillRect(0,0, 180,16, TFT_BLACK);
      i2c_data0_old = 0;  //print all
    }


    /*
    //prints the SWR level to display, 
    sprintf(s, "%d %02x %02x %02x  ", ret, i2c_data[0], i2c_data[1], i2c_data[2]);
    tft_writexy_plus(1, TFT_GREEN, TFT_BACKGROUND, 6, 5, 4, 5, (uint8_t *)s); 
*/
    // converts from AD reading to power using power_adc[] table
    for (pow = 0; pow < SWR_POW_MAX; pow++)  //only 10 steps on the bargraph
      if (i2c_data[1] < power_adc[pow])
        break;
        //pow results 0-10W

#ifdef TST_MAX_SMETER_SWR
    pow = random(12);
    i2c_data[0] = random(0xFF);  // S level max = 15.9 = F9
#endif





    if (hmi_power_show(pow) == true)  //if it is time to show the new power (swr value follows the power moment)
    {

     tft.fillRect(10,0, 180,16, TFT_BLACK);
      if (pow < 10) {
        sprintf(s, "PWR: %d ", pow);
      } else {
        sprintf(s, "PWR: %d", pow);
      }
      //tft_writexy_(2, TFT_RED, TFT_BACKGROUND, 1,2,(uint8_t *)s);
      //tft_writexy_plus(15, TFT_RED, TFT_BACKGROUND, 0, x_RT + (1 * X_CHAR2), 0, y_RT, (uint8_t *)s);

      tft.setTextFont(2);
      tft.setCursor(10, 0);
      tft.setTextColor(TFT_WHITE);
      tft.print(s);

        TxPower_bargraph(pow);

      if (i2c_data0_old != i2c_data[0]) {
        /*
        if((i2c_data[0]>>4) < 10)
        {
        sprintf(s, " %d.%d", (i2c_data[0]>>4),(i2c_data[0]&0x0f));
        }
        else
        {
        sprintf(s, "%d.%d", (i2c_data[0]>>4),(i2c_data[0]&0x0f));
        }
  */
        sprintf(s, "SWR: %d.%d ", (i2c_data[0] >> 4), (i2c_data[0] & 0x0f));

        tft.setCursor(100, 0);
        tft.print(s);
        i2c_data0_old = i2c_data[0];
      }

      tft.setFreeFont(FONT1);
    }
  }
}

#endif





//int16_t contk = 0;
/*
 * Redraw the display, representing current state
 * This function is called regularly from the main loop.
 */
void hmi_evaluate(void)  //hmi loop
{
  static uint8_t band_vars_old[NUMBER_OF_MENUES] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };  // Stored last option selection
  static uint32_t hmi_freq_old = 0xff;
  static uint8_t hmi_band_old = hmi_band;
  static bool tx_enable_old = true;
  static uint8_t hmi_menu_old = 0xff;
  static uint8_t hmi_menu_opt_display_old = 0xff;



#ifdef HMI_debug
  uint16_t ndata;
  for (ndata = 1; ndata < NUMBER_OF_MENUES - 2; ndata++) {
    if (band_vars[hmi_band][ndata] != band_vars_old[ndata])
      break;
  }
  if (ndata < NUMBER_OF_MENUES - 2) {
    Serialx.print("evaluate Band " + String(hmi_band) + "  band vars changed   ");
    for (ndata = 0; ndata < NUMBER_OF_MENUES; ndata++) {
      Serialx.print(" " + String(band_vars_old[ndata]));
    }
    Serialx.print("  ->  ");
    for (ndata = 0; ndata < NUMBER_OF_MENUES; ndata++) {
      Serialx.print(" " + String(band_vars[hmi_band][ndata]));
    }
    Serialx.println("\n");
  }
#endif

  // if band_var changed (after <enter>), set parameters accordingly

  if (hmi_freq_old != hmi_freq) {

    if (hmi_freq > 40000000L)
      hmi_freq = 40000000L;  // limit to 40MHz

    SI_SETFREQ(0, HMI_MULFREQ * hmi_freq);
    //freq  (from encoder)


    double K = (double)(hmi_freq / 1000000.0) * 1000;
    sprintf(s, "%7.2f", K);  // Always 7 characters wide
    char oldFreq[20];
    tft.setFreeFont(FONT3);
    tft.setTextColor(TFT_GREEN);

    int fixedRightX = 220;
    int textWidth = tft.textWidth(s);
    int startX = fixedRightX - textWidth;


#define X_CHAR3 28
#define Y_CHAR3 42

    int digitWidth = X_CHAR3;  // reduce flicker by only rewriting the digit that has changed
    int fontHeight = Y_CHAR3;

    int x = startX;
    int y = 45;


    if (hmi_freq < 10000000 && hmi_freq_old >= 10000000)
      tft.fillRect(0, y - fontHeight + 10, digitWidth, fontHeight - 5, TFT_BLACK);

    for (int i = 0; i < strlen(s); i++) {
      if (s[i] != oldFreq[i]) {
        tft.fillRect(x + i * digitWidth, y - fontHeight + 10, digitWidth, fontHeight - 5, TFT_BLACK);
        tft.setCursor(x + i * digitWidth, y);
        tft.print(s[i]);
      }
    }
    strcpy(oldFreq, s);
    tft.setFreeFont(FONT1);

    //cursor (writing the freq erase the cursor)
    //tft_cursor_plus(3, TFT_BLUE, 0 + (band_vars[hmi_band][HMI_S_TUNE] > 4 ? band_vars[hmi_band][HMI_S_TUNE] + 1 : band_vars[hmi_band][HMI_S_TUNE]), 0, 0, 12);
    display_fft_graf_top();
    hmi_freq_old = hmi_freq;
  }




  if (band_vars_old[HMI_S_MODE] != band_vars[hmi_band][HMI_S_MODE])  //mode (SSB AM CW)
  {
    dsp_setmode(band_vars[hmi_band][HMI_S_MODE]);  //MODE_USB=0 MODE_LSB=1  MODE_AM=2  MODE_CW=3

    display_fft_graf_top();  //scale freqs, mode changes the triangle

    if (band_vars[hmi_band][HMI_S_MODE] == MODE_CW) {
      CwDecoder_Inic();
    }
    if (band_vars_old[HMI_S_MODE] == MODE_CW) {
      CwDecoder_Exit();
    }

    band_vars_old[HMI_S_MODE] = band_vars[hmi_band][HMI_S_MODE];
  }



  if (band_vars_old[HMI_S_VOX] != band_vars[hmi_band][HMI_S_VOX]) {
    dsp_setvox(band_vars[hmi_band][HMI_S_VOX]);
    band_vars_old[HMI_S_VOX] = band_vars[hmi_band][HMI_S_VOX];
  }
  if (band_vars_old[HMI_S_AGC] != band_vars[hmi_band][HMI_S_AGC]) {
    dsp_setagc(band_vars[hmi_band][HMI_S_AGC]);
    band_vars_old[HMI_S_AGC] = band_vars[hmi_band][HMI_S_AGC];
  }
  if (hmi_band_old != hmi_band) {
    Store_Last_Band(hmi_band_old);  // store data from old band (save freq to have it when back to this band)

    //relay_setband(hmi_band);  // = hmi_band
    sleep_ms(1);              // I2C doesn't work without...
    Setup_Band(hmi_band);     // = hmi_band  get the new band data
    hmi_band_old = hmi_band;  // = hmi_band

    //sprintf(s, "B:%d", band_vars[hmi_band][HMI_S_BPF]);  // hmi_menu_opt_display = band_vars[hmi_band][HMI_S_BPF] // removed display band
  }
  if (band_vars_old[HMI_S_PRE] != band_vars[hmi_band][HMI_S_PRE]) {
    relay_setattn(hmi_pre[band_vars[hmi_band][HMI_S_PRE]]);
    band_vars_old[HMI_S_PRE] = band_vars[hmi_band][HMI_S_PRE];
  }


  //T or R  (using letters instead of arrow used on original project)
  if (tx_enable_old != tx_enabled) {
    //erase the area for T or R, infos and the bar graph area
    //tft.fillRect(x_RT, y_RT, (6*X_CHAR1), (3*Y_CHAR1), TFT_BACKGROUND);   // TFT_LIGHTGREY);  TFT_BACKGROUND);

    tft.fillRect(210, 55, 25, 16, TFT_BLACK);

    if (tx_enabled == true) {
      tft.fillRect(0,0, 180,16, TFT_BLACK); // overwrite highest row
      tft.setTextColor(TFT_RED);
      tft.setCursor(210, 65);
      tft.print("TX");

#if I2C_Arduino_Pro_Mini == 1  //using Arduino Pro Mini for relays control (and allow SWR reading) \
                               //tft_writexy_plus(1, TFT_RED, TFT_BACKGROUND, 0, x_xGain, 0, y_yGain, (uint8_t *)"#0.0");
#endif
    } else {
       tft.fillRect(0,0, 180,16, TFT_BLACK); 
      tft.setTextColor(TFT_GREEN);
      tft.setCursor(210, 65);
      tft.print("RX");
    }
    rec_level_old = rec_level + 1;

    tx_enable_changed = true;  //signal to init values at display

    tx_enable_old = tx_enabled;
  }




  if (tx_enabled == false) /* RX */
  {


    if (band_vars[hmi_band][HMI_S_MODE] == MODE_CW) {
      CwDecoder_array_in();
    }

    hmi_smeter();  //during RX, print Smeter on display only when ! CW decoding


  } else /* TX */
  {
#if I2C_Arduino_Pro_Mini == 1  //using Arduino Pro Mini for relays control (and allow SWR reading)
    hmi_power_swr();           //during TX, read the SWR, and print it on display
#endif
  }


  tx_enable_changed = false;  //signal to init values at display


  // if menu changed, print new value

  if ((hmi_menu_old != hmi_menu) || (hmi_menu_opt_display_old != hmi_menu_opt_display)) {


#ifdef HMI_debug
    Serialx.println("evaluate Band " + String(hmi_band));
    Serialx.println("hmi_menu  " + String(hmi_menu_old) + " -> " + String(hmi_menu));
    Serialx.println("hmi_menu_opt_display " + String(hmi_menu_opt_display_old) + " -> " + String(hmi_menu_opt_display));
#endif

    tft.fillRect(0, 0, 320, 15, TFT_BACKGROUND);  //clear menu information

    tft.setTextColor(TFT_MAGENTA, TFT_BACKGROUND);

    switch (hmi_menu) {
      case HMI_S_TUNE:

        tft.setFreeFont(NULL);

        sprintf(s, " %s ", hmi_o_mode[band_vars[hmi_band][HMI_S_MODE]]);
        print_current_mode(s);  //

        tft.fillRect(0, 85, 160, 58, TFT_DARKPURPLE);
        //tft.fillRoundRect(0,85, 170, 70, 5, TFT_DARKPURPLE);
        //menu

        sprintf(s, "BPF: %sMHz", hmi_o_bpf[hmi_band]);  //
        tft.setCursor(15, 95);
        tft.print(s);

        tft.setCursor(15, 110);
        sprintf(s, "AGC:    %s", hmi_o_agc[band_vars[hmi_band][HMI_S_AGC]]);
        tft.print(s);

        tft.setCursor(15, 125);
        sprintf(s, "ATTEN.: %s ", hmi_o_pre[band_vars[hmi_band][HMI_S_PRE]]);
        tft.print(s);

        sprintf(s, "VOX:    %s ", hmi_o_vox[band_vars[hmi_band][HMI_S_VOX]]);
        tft.setCursor(15, 140);
        tft.print(s);

        tft.setFreeFont(FONT1);
        tft_cursor_plus(3, TFT_BLUE, 0 + (band_vars[hmi_band][HMI_S_TUNE] > 4 ? band_vars[hmi_band][HMI_S_TUNE] + 1 : band_vars[hmi_band][HMI_S_TUNE]), 0, 0, 12);  // CURSOR
        break;


      //case HMI_S_MODE:                                                       Mode has seperate button now
      //  sprintf(s, "Set Mode: %s        ", hmi_o_mode[hmi_menu_opt_display]);
      // tft_writexy_(1, TFT_MAGENTA, TFT_BACKGROUND, 0, 0, (uint8_t *)s);
      // break;
      case HMI_S_AGC:
        sprintf(s, "Set AGC: %s     ", hmi_o_agc[hmi_menu_opt_display]);
        tft_writexy_(1, TFT_MAGENTA, TFT_BACKGROUND, 0, 0, (uint8_t *)s);
        break;
      case HMI_S_PRE:
        sprintf(s, "Set Pre: %s     ", hmi_o_pre[hmi_menu_opt_display]);
        tft_writexy_(1, TFT_MAGENTA, TFT_BACKGROUND, 0, 0, (uint8_t *)s);
        break;
      case HMI_S_VOX:
        sprintf(s, "Set VOX: %s      ", hmi_o_vox[hmi_menu_opt_display]);
        tft_writexy_(1, TFT_MAGENTA, TFT_BACKGROUND, 0, 0, (uint8_t *)s);
        break;
      case HMI_S_BPF:
        sprintf(s, "Set Band          ");  //
        tft.setCursor(0, 10);
        tft.print(s);
        break;
      case HMI_S_FFT:
        sprintf(s, "Set FFT gain: %d      ", fft_gain);
        tft_writexy_(1, TFT_MAGENTA, TFT_BACKGROUND, 0, 0, (uint8_t *)s);
        break;

      case HMI_S_OSC:
        sprintf(s, "Select trace");
        tft_writexy_(1, TFT_MAGENTA, TFT_BACKGROUND, 0, 0, (uint8_t *)s);
        break;

        tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
        tft.fillRect(0, 85, 160, 16, TFT_DARKPURPLE);  // update information panel
        sprintf(s, "BPF: %sMHz", hmi_o_bpf[hmi_menu_opt_display]);
        tft.setCursor(15, 95);
        tft.print(s);

        sprintf(s, " %s", hmi_o_mode[band_vars[hmi_band][HMI_S_MODE]]);  // update mode
        print_current_mode(s);                                           //
        break;
    }

    hmi_menu_old = hmi_menu;
    hmi_menu_opt_display_old = hmi_menu_opt_display;
  }




  if (aud_samples_state == AUD_STATE_SAMP_RDY)  //design a new graphic only when data is ready
  {
    //plot audio graphic
    display_aud_graf();

    aud_samples_state = AUD_STATE_SAMP_IN;
  }


  CwDecoder_Loop();  //task on 100ms loop
}


//#################################################################################################//


void print_current_mode(char *s) {

  tft.setFreeFont(NULL);
  tft.fillRoundRect(245, 54, 60, 25, 5, TFT_SILVER);
  tft.setTextColor(TFT_BLACK, TFT_SILVER);
  tft.setTextSize(2);
  tft.setCursor(245, 60);
  tft.print(s);
  tft.setFreeFont(FONT1);
  tft.setTextSize(1);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
}


//#################################################################################################//


void touch_evaluate() {
  // uses raw touch functions with reduced sampling to save time.
  // tft.getTouch() is too slow, so we need less oversampling + convert + filter.


  if (touch_delay) {
    touch_delay--;
    return;
  }

  const int samples = 10;
  uint16_t xs[samples], ys[samples];
  tft.getTouchRawZ();
  tox = toy = 0;
  // Collect raw samples quickly
  int valid = 0;
  for (int i = 0; i < samples; i++) {
    uint16_t x, y;
    if (tft.getTouchRaw(&x, &y)) {
      tft.convertRawXY(&x, &y);
      xs[valid] = x;
      ys[valid] = y;
      valid++;
    }
    delayMicroseconds(50);  // ADC settle time
  }

  if (valid == 0)
    return;  // no touch

  // --- Median filter ---
  for (int i = 1; i < valid; i++) {
    for (int j = i; j > 0 && xs[j] < xs[j - 1]; j--) {
      uint16_t tmp = xs[j];
      xs[j] = xs[j - 1];
      xs[j - 1] = tmp;
    }
    for (int j = i; j > 0 && ys[j] < ys[j - 1]; j--) {
      uint16_t tmp = ys[j];
      ys[j] = ys[j - 1];
      ys[j - 1] = tmp;
    }
  }

  uint16_t x = xs[valid / 2];
  uint16_t y = ys[valid / 2];



  // --- Reject obvious false positives ---
  if (x > 320 || y > 240) {
    tox = toy = 0;
    return;
  }

  tox = x;
  toy = y;


  //char s[30];
  // sprintf(s, " x%d  y%d", x, y);
  // Serialx.println(s);



  hmi_handler(99);
}


//#################################################################################################//

//#################################################################################################//
