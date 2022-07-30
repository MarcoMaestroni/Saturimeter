/**
*   \file Interrupt_Routines.c
*   \brief Source file for Interrupts
*/

#include "Interrupt_Routines.h"

CY_ISR(custom_ISR_Sensor)
{
    Int_Sensor_ClearInterrupt();
    flag_int = TRUE;
}

CY_ISR(custom_ISR_UART)
{    
    if (UART_ReadRxStatus() == UART_RX_STS_FIFO_NOTEMPTY) //control over UART receiver line to verify the presence of a new byte
    {
        input_UART = UART_ReadRxData();
        
        if(flag_menu == TRUE)
        {   
            flag_received = TRUE;
            if(flag_transmission == FALSE) //we can get out of the config menu iff the transmission hasn't already started, else B or H could be feasible values
            {
                if(input_UART == 'B' || input_UART == 'b')
                {
                    flag_menu = FALSE;
                    UART_PutString("\nQuitting configuration menu.\n");
                    Timer_Init();
                }
                else if(input_UART == 'H' || input_UART == 'h')
                {
                    Menu_Help();
                }
                else if(input_UART != 0xA0)
                {
                    UART_PutString("\nWrong input! Header is 0xA0.\n");
                }
            }
        }
        else
        {
            switch(input_UART)
            {
                case 's':
                case 'S': 
                    flag_menu = TRUE;
                    UART_PutString("\nEntering configuration menu.\nPress B to quit.\nPress H for help and to display current settings.\n");
                    UART_PutString("WARNING: Increase sensitivity threshold if phantom values are displayed, decrease it if no value is displayed.\n");
                    count_menu = 0;
                    Timer_Init();
                    break;
                    
                case 'c':
                case 'C':
                    if(temp_mode != CELSIUS)
                    {
                        temp_mode = Temperature_Mode(CELSIUS);
                        flag_temp = TRUE;
                        UART_PutString("\nTemperature swithced to Celsius.\nPress F to switch back to Fahrenheit.\n");
                    }
                    else
                        UART_PutString("\nTemperature already in Celsius degrees!\n");
                    break;
                    
                case 'f':
                case 'F':
                    if(temp_mode != FAHRENHEIT)
                    {
                        temp_mode = Temperature_Mode(FAHRENHEIT);
                        flag_temp = TRUE;
                        UART_PutString("\nTemperature swithced to Fahrenheit.\nPress C to switch back to Celsius.\n");
                    }
                    else
                        UART_PutString("\nTemperature already in Fahrenheit degrees!\n");
                    break;
                    
                default: 
                    UART_PutString("\nWrong input!\nPress S to enter configuration mode.\n");
                    if(temp_mode == CELSIUS)
                        UART_PutString("Press F to change temperature units to Fahrenheit.\n");
                    else
                        UART_PutString("Press C to change temperature units to Celsius.\n");
                    break;       
            }
        }
    }
}

CY_ISR(custom_ISR_Timer)
{
    Timer_ReadStatusRegister();
    
    if(flag_logo == TRUE)
    {
        count_logo++;
        if(count_logo == TIMEOUT_LOGO)
        {
            flag_logo = FALSE;
        }
    }
    else if(flag_menu == TRUE)
    {
        count_menu++;
        if(count_menu == TIMEOUT_MENU)
        {
            flag_menu = FALSE;
            UART_PutString("\nTimeout! Quitting configuration menu.\n");
        }
    }
    else
    {
        count_temp++;
        if(count_temp == TIMEOUT_TEMP)
        {
            count_temp = 0;
            flag_timer = TRUE;
        }
    }
}

/* [] END OF FILE */
