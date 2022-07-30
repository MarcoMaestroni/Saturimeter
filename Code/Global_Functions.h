/**
*   \file Global_Functions.h
*   \brief Header file for general functions
*/

#ifndef _GLOBAL_FUNCTIONS_H
    #define _GLOBAL_FUNCTIONS_H
   
    #include "project.h"
    #include "Display_Functions.h"
    #include "Sensor_Functions.h"
    
    #define WATCHDOG_TRIGGER 1000000 //empirically estimated not to interfere with normal activity

    //Boolean constant
    #define TRUE 1
    #define FALSE 0

    //Temperature modes
    #define EEPROM_TEMP_MODE 0x0000
    #define CELSIUS 0
    #define FAHRENHEIT 1
    
    //Connection check
    #define CHECK_1 1
    #define CHECK_2 2
    #define CHECK_3 3
    #define CHECK_4 4
    
    //PWM
    #define PWM_1SEC 10000
    #define PERIOD_1 PWM_1SEC
    #define PERIOD_2 PWM_1SEC
    #define PERIOD_3 PWM_1SEC/2
    #define PERIOD_4 PWM_1SEC/4
    #define DC_1 100
    #define DC_2 50
    #define DC_3 50
    #define DC_4 25
        
    //General functions
    void Initialization(void);
    void Check_Connection(void); //check sensor and display connections and set PWM accordingly
    void Menu_Help();
    void Menu_Configuration(uint8 data, uint8 tail, uint8* adc_res, uint8_t* threshold);
    uint8 Temperature_Mode(uint8 mode); //set and save on eeprom the temperature mode
    void PWM_Setting(uint16 period, uint8 DC);
    uint8 I2C_IsDeviceConnected(uint8_t device_address); 
#endif

/* [] END OF FILE */
