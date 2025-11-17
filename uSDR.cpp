#include <sys/_stdint.h>
/*
 * uSDR.c
 *
 * Created: Mar 2021
 * Author: Arjan te Marvelde 
 * May2022: adapted by Klaus Fensterseifer 
 * https://github.com/kaefe64/Arduino_uSDX_Pico_FFT_Proj
 * 
 * The application main loop
 * This initializes the units that do the actual work, and then loops in the background. 
 * Other units are:
 * - dsp.c, containing all signal processing in RX and TX branches. This part runs on the second processor core.
 * - si5351.c, containing all controls for setting up the si5351 clock generator.
 * - lcd.c, contains all functions to put something on the LCD
 * - hmi.c, contains all functions that handle user inputs
 */

#include "Arduino.h"

#include <Wire.h>
#include "uSDR.h"
#include "hmi.h"
#include "dsp.h"
#include "si5351.h"
#include "monitor.h"
#include "relay.h"
#include "TFT_eSPI.h"
#include "display_tft.h"




uint16_t tim_loc;     // local time 

void uSDR_setup0(void)  //main
{
  display_intro();

}



void uSDR_setup(void)  //main
{
  //gpio_init_mask(1<<14);  
  //gpio_set_dir(14, GPIO_OUT); 
  

  
  Serialx.println("uSDR_setup   Wire.begin");
	/*
	 * i2c0 is used for the si5351 interface
	 * i2c1 is used for the LCD and all other interfaces
	 */
#if TX_METHOD == I_Q_QSE   //original project
  Wire.begin();            //i2c0 master to Si5351
  //Wire.setClock(200000);   // Set i2c0 clock speed (default=100k)
#endif
  Wire1.begin();           //i2c1   used for switching band and atten/LNA
  //Wire1.setTimeout(1000);  // sets maximum milliseconds to wait for stream data, default is 1 second

	/* Initialize units */


  Serialx.println("uSDR_setup   mon_init");
	mon_init();										// Monitor shell on stdio
  
// si_init causes the blank display of nothing else is connected

 Serialx.println("uSDR_setup   si_init");
 si_init();										// VFO control unit



Serialx.println("uSDR_setup   relay_init");
 relay_init();



  Serialx.println("uSDR_setup   hmi_init");
	hmi_init();										// HMI user inputs
  
 Serialx.println("uSDR_setup   dsp_init");
 dsp_init();                   // Signal processing unit


 Serialx.println("uSDR_setup   display_static_elements");
 display_static_elements();   //moved to setup0 to write into display from the begining 


hmi_init0(); 



Serialx.println("uSDR_setup   finished");




  tim_loc = tim_count;          // local time for main loop
  //digitalWrite(14, LOW);


}



void uSDR_loop(void)
{ 


  if((uint16_t)(tim_count - tim_loc) >= (uint16_t)LOOP_MS)  //run the tasks every 100ms  (LOOP_MS = 100)
  {

   uint32_t st = millis(); 
    
    hmi_evaluate();               // Refresh HMI
    //Serialx.println("hmi_evaluate   finished");
   
    si_evaluate();                // Refresh VFO settings
    //Serialx.println("si_evaluate   finished");
    
    mon_evaluate();               // Check monitor input
    //Serialx.println("mon_evaluate   finished");
    
    //Serialx.println("dsp_loop  started\n");
    dsp_loop();  //spend more time here for FFT and graphic
    //Serialx.println("dsp_loop   finished\n");

    //Serialx.println("tft_loop  started\n");
    display_tft_loop();           // Refresh display
   // Serialx.println("tft_loop  finished\n");
    //it takes 50ms for the tasks (most in hmi_evaluate() to plot the waterfall)
    
  uint32_t diff = millis() - st;

  char s [20];


if (diff < 1000) {
  sprintf(s, "%lums", diff);

  tft.setFreeFont(FONT1);
  tft.setCursor(250,14);
  tft.fillRect(250,0,70,22,TFT_BLACK);
  tft.print(s);
}

    tim_loc += LOOP_MS;
  }

}
