#include <sys/_stdint.h>
#ifndef __HMI_H__
#define __HMI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * hmi.h
 *
 * Created: Apr 2021
 * Author: Arjan te Marvelde
 *
 * See hmi.c for more information 
 */


#include <stdint.h>

/*
 * Some macros
 */
#ifndef MIN
#define MIN(x, y)        ((x)<(y)?(x):(y))  // Get min value
#endif
#ifndef MAX
#define MAX(x, y)        ((x)>(y)?(x):(y))  // Get max value
#endif




//#define   SMETER_TEST   10     //uncomment this line to see the audio level direct on display (used to generate the S Meter levels)
//#define HMI_debug

/* Menu definitions (band vars array position) */
#define HMI_S_TUNE		0
#define HMI_S_MODE		1
#define HMI_S_AGC			2
#define HMI_S_PRE			3
#define HMI_S_VOX			4
#define HMI_S_BPF			5
#define HMI_S_FFT			6
#define HMI_S_OSC			7

#define NUMBER_OF_MENUES			8  //number of possible menus

/* Event definitions */
#define HMI_E_NOEVENT		0
#define HMI_E_INCREMENT		1
#define HMI_E_DECREMENT		2
#define HMI_E_ENTER			3
#define HMI_E_SUBMENU		4
#define HMI_E_LEFT			5
#define HMI_E_RIGHT			6
#define HMI_E_PTTON			7
#define HMI_E_PTTOFF		8
#define HMI_PTT_ON      9
#define HMI_PTT_OFF     10
#define HMI_E_ENTER_RELEASE			11
#define HMI_NEVENTS			12  //number of events


/* Sub menu AMOUNT of options (string sets) */
/*
#define HMI_NUM_OPT_TUNE	8  // = num pos cursor
#define HMI_NUM_OPT_MODE	4
#define HMI_NUM_OPT_AGC	3
#define HMI_NUM_OPT_PRE	5
#define HMI_NUM_OPT_VOX	4
#define NUMBER_OF_BANDS	5
#define HMI_NUM_OPT_DFLASH	2
*/

#define HMI_NUM_OPT_TUNE	8  // = amount of fields to position cursor, tune step
#define HMI_NUM_OPT_MODE	5 // now 5 modes
#define HMI_NUM_OPT_AGC	3
#define HMI_NUM_OPT_PRE	5
#define HMI_NUM_OPT_VOX	4


//"USB","LSB","AM","CW"
#define MODE_USB  0
#define MODE_LSB  1
#define MODE_AM   2
#define MODE_AM2  3
#define MODE_CW   4

#define USE_TOUCH_SCREEN
#define BAND_RELATED_FFT_GAIN 32+ hmi_freq / 1000 / 250; // This increases fft_gain automatically when switching to a higher fband


#define NUMBER_OF_BANDS	16
extern uint8_t  band_vars[NUMBER_OF_BANDS][NUMBER_OF_MENUES];

#define START_BAND 11 // band to start transceiver

// Starting freqs when band gets called

#define b0  1910000LU
#define b1  3800000LU
#define b2  7200000LU 
#define b3  10100000LU 
#define b4  14200000LU 
#define b5  18100000LU 
#define b6  21300000LU
#define b7  24900000LU
#define b8  27455000LU
#define b9  28074000LU
#define b10 28500000LU
#define b11  870000LU
#define b12 5000000LU
#define b13 9000000LU
#define b14 13820000LU
#define b15 24000000LU



#define GP_PTT		15

//extern uint8_t  hmi_sub[NUMBER_OF_MENUES];							// Stored option selection per state
extern uint32_t hmi_freq;  
extern uint8_t  hmi_band;	
extern bool tx_enabled;
extern bool tx_enable_changed;
extern bool ptt_internal_active;    //PTT output = true for vox, mon and mem
extern bool ptt_external_active;
extern bool ptt_vox_active;	
extern bool ptt_mon_active;
extern bool ptt_aud_active;


extern uint8_t sel_graph; // select scope graphics
extern uint16_t tox, toy; // touch coordinates
extern uint8_t touch_delay; // blocks touch for a while

void Setup_Band(uint8_t band);
void hmi_init0(void);
void hmi_init(void);
void hmi_evaluate(void);
void print_current_mode (char *s); 
void print_Band(uint8_t band);
void touch_evaluate(void);
static inline uint16_t swapBytes(uint16_t);

#ifdef __cplusplus
}
#endif
#endif
