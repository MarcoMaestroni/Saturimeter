/**
*   \file Interrupt_Routines.h
*   \brief Header file for Interrupts
*/

#ifndef _INTERRUPT_ROUTINES_H
    #define _INTERRUPT_ROUTINES_H
    
    #include "Global_Functions.h"
    
    #define TIMEOUT_LOGO 3
    #define TIMEOUT_MENU 5 
    #define TIMEOUT_TEMP 2

    volatile uint8 flag_int;
    volatile uint8 temp_mode;
    volatile uint8 count_temp;

    //Timer variables
    volatile uint8 flag_logo; //this flag signals that the logo is showing
    volatile uint8 count_logo;
    volatile uint8 flag_timer; //this flag is used fot the temperature interrupt timing
    volatile uint8 count_menu;
    
    //UART variables
    volatile uint8 flag_menu; //this flag signals that we entered the configuration menu
    volatile uint8 flag_received; //this flag signals that a character was received on the UART
    volatile uint8 flag_transmission; //this flag signals that the packet transmission is going on
    volatile uint8 input_UART;

    CY_ISR_PROTO(custom_ISR_Sensor);
    CY_ISR_PROTO(custom_ISR_UART);
    CY_ISR_PROTO(custom_ISR_Timer);
#endif

/* [] END OF FILE */
