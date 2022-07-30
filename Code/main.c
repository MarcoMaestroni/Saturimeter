/**
*   \file main.c
*   \brief Main source file for the final project
*/

#include "Interrupt_Routines.h"

int main(void)
{
    Initialization();
    Check_Connection();
    display_init(DISPLAY_ADDRESS);
       
    //General variables
    uint32 watchdog = 0;
    uint8 read; //support variable
    uint16 i; //iteration variable
    uint8 data; //UART variable

    //MA filters sizes (proportional to ADC resolution)
    uint8 adc_res; //ADC resolution is 15+adc_res
    uint8 red_filter[4] = {8, 12, 16, 20}; 
    uint8 ir_filter[4] = {12, 16, 20, 24};
    uint8 max_filter[4]; //used to cut buffers after filtering
    for(i=0; i<4; i++)
    {
        if(red_filter[i] > ir_filter[i])
            max_filter[i] = red_filter[i];
        else
            max_filter[i] = ir_filter[i];
    }
    
    adc_res = Sensor_Initialization();
    
    //Check temp mode in EEPROM memory
    temp_mode = EEPROM_ReadByte(EEPROM_TEMP_MODE);
    if(temp_mode!=CELSIUS && temp_mode!=FAHRENHEIT) //if EEPROM was never written or was overwritten
        temp_mode = Temperature_Mode(CELSIUS);

    //ISRs enabling
    CyGlobalIntEnable;
    ISR_UART_StartEx(custom_ISR_UART);
    ISR_Sensor_StartEx(custom_ISR_Sensor);
    ISR_Timer_StartEx(custom_ISR_Timer);
    
    //FIFO variables
    uint8 fifo[FIFO_SIZE];
    uint8 write_pointer = 0, read_pointer = 0; 
    int8 pointer_diff;
    uint32_t Red_buffer[(uint16)(UPDATE_TIME_WINDOW*SAMPLE_RATE+FIFO_NUM_SAMPLES+max_filter[3])]; //size is the maximum number of fifo samples that could be read for one measure
    uint32_t IR_buffer[(uint16)(UPDATE_TIME_WINDOW*SAMPLE_RATE+FIFO_NUM_SAMPLES+max_filter[3])];
    int32_t count_buffer = 0; //current dimension of the buffer, proportional to the sample rate
    int32_t SpO2, HR;
    int32 HR_old = 0, SpO2_old = 0;
    int8_t SpO2_valid = 0, HR_valid = 0; //variables used to check if the computation is valid
    uint8_t threshold = EEPROM_ReadByte(EEPROM_THRESH); //default value

    //LED packet for BCP
//    uint8 DataLED[LED_BUFFER_SIZE];
//    DataLED[0] = 0xA1;
//    DataLED[LED_BUFFER_SIZE-1] = 0xC0;
    
    //Temperature variables
    uint8 Temp_uint;
    uint8 Temp_frac;
    int8 Temp_int;
    float Temp_float;
    int8 Temp_int_old = 0;
    uint8 Temp_dec;
    uint8 Temp_dec_old = 0;  
    
    //Display variables
    uint8 flag_reading = FALSE; //flag used to configure only once the reading window
    uint8 flag_config = FALSE; //flag used to configure only once the configuration menu window
    uint8 state_display = READING;
    char value_display[20]; //string to display
    uint8 count_HR = 0, count_SpO2 = 0, count_temp = 0; //counts to keep track of invalid values
    
    //Flag initialization
    flag_menu = FALSE;
    flag_received = FALSE;
    flag_transmission = FALSE;
    flag_int = FALSE;
    flag_timer = FALSE;
    count_menu = 0;
    flag_logo = FALSE;
    count_logo = 0;
    flag_temp = TRUE; //set to true in order to write the unit of measurement the first time
    count_temp = 0;
    
    //Showing Logo
    Logo_Window();
    flag_logo = TRUE;
    while(flag_logo == TRUE);    

    UART_PutString("\nWELCOME!\nPress S to enter configuration mode.\n");
    if(temp_mode == CELSIUS)
        UART_PutString("Press F to change temperature units to Fahrenheit.\n");
    else
        UART_PutString("Press C to change temperature units to Celsius.\n");
           
    while(1)
    {          
        //Display configuration
        switch(state_display)
        {                                
            case READING:
                
                if(flag_reading == FALSE)
                {
                    Reading_Window();                    
                    flag_reading = TRUE;
                }
                              
                if(flag_menu)
                {
                    flag_reading = FALSE;
                    state_display = CONFIGURATION;
                }
                                          
                break;
            
            case CONFIGURATION:
                
                if(flag_config == FALSE)
                {
                    Configuration_Window(); 
                    flag_config = TRUE;
                }
                
                if(flag_menu == FALSE)
                {
                    display_stopscroll();
                    display_clear();    
                    display_update();
                    
                    flag_config = FALSE;
                    flag_temp = TRUE;
                    state_display = READING;
                    count_buffer = 0;
                }  
                
                break;   
        }

        //Configuration menu
        while(flag_menu == TRUE && flag_config == TRUE) //enters only if we have already entered in case CONFIGURATION of display switch to set config state
        {
            if(flag_received == TRUE)
            {
                flag_received = FALSE;
                if(input_UART == HEADER)
                {
                    flag_transmission = TRUE;
                    while(flag_menu == TRUE && flag_transmission == TRUE)
                    {
                        if(flag_received == TRUE)
                        {
                            data = input_UART;
                            flag_received = FALSE;
                            while(flag_menu == TRUE && flag_transmission == TRUE)
                            {
                                if(flag_received == TRUE)
                                {
                                    flag_received = FALSE;
                                    Menu_Configuration(data, input_UART, &adc_res, &threshold);
                                    
                                    flag_transmission = FALSE;
                                    UART_PutString("Returning to config menu.\nPress B to quit.\nPress H for help and to display current settings.\n");
                                    count_menu = 0;
                                    Timer_Init();     
                                }
                            }
                        }
                    }
                }
            }
        }
        
        //Sensor interrupts
        if(flag_int == TRUE && flag_config == FALSE) //enters only if we have already entered in case CONFIGURATION of display switch to set reading state
        {
            flag_int = FALSE;
            watchdog = 0; //watchdog reset
            Sensor_ReadRegister(SENSOR_ADDRESS, INTERRUPT_STATUS_1_ADDRESS, &read); //check if interrupt was triggered by fifo (if so, clears interrupt line)
                        
            if(read) //interrupt was triggered by fifo
            {
                Sensor_ReadRegister(SENSOR_ADDRESS,FIFO_READ_POINTER_ADDRESS, &read_pointer);
                Sensor_ReadRegister(SENSOR_ADDRESS,FIFO_WRITE_POINTER_ADDRESS, &write_pointer);
                pointer_diff = write_pointer-read_pointer;
                if(pointer_diff < 0)
                    pointer_diff = 32+pointer_diff;
                
                Sensor_ReadFIFO(SENSOR_ADDRESS, FIFO_DATA_ADDRESS, FIFO_SIZE, fifo); //acquiring all fifo data
                
                for(i = 0; i < pointer_diff; i++)
                {
                    Red_buffer[i+count_buffer] = Value_LED(adc_res, &fifo[6*i]);
                    IR_buffer[i+count_buffer] = Value_LED(adc_res, &fifo[6*i+3]);     
                }
                count_buffer += pointer_diff;              
                
                if(count_buffer-max_filter[adc_res] >= UPDATE_TIME_WINDOW*SAMPLE_RATE) //fifo samples needed to obtain a signal UPDATE_TIME_WINDOW long, excluding filtering
                {
                    //Moving average filter
                    for(i=0; i< count_buffer-max_filter[adc_res]; i++)
                    {
                        //Raw data
//                        Fill_Buffer(Red_buffer[i], &DataLED[1]);
//                        Fill_Buffer(IR_buffer[i], &DataLED[4]);
                        
                        Red_buffer[i] = Filter_Buffer(&Red_buffer[i], red_filter[adc_res]); 
                        IR_buffer[i] = Filter_Buffer(&IR_buffer[i], ir_filter[adc_res]);
                        
                        //Filtered data
//                        Fill_Buffer(Red_buffer[i], &DataLED[7]);
//                        Fill_Buffer(IR_buffer[i], &DataLED[10]);
//                        
//                        UART_PutArray(DataLED, LED_BUFFER_SIZE); 
                    }
   
                    //SpO2 and HR computation
                    maxim_heart_rate_and_oxygen_saturation(IR_buffer, count_buffer-max_filter[adc_res], Red_buffer, &SpO2, &SpO2_valid, &HR, &HR_valid, threshold);
                    
                    for(i=0; i < max_filter[adc_res]; i++)
                    {
                        Red_buffer[i] = Red_buffer[i+count_buffer-max_filter[adc_res]];
                        IR_buffer[i] = IR_buffer[i+count_buffer-max_filter[adc_res]];
                    }
                    count_buffer = max_filter[adc_res];
                    
                    if(SpO2_valid == 1 && SpO2 > SPO2_MIN)
                    {
                        count_SpO2 = 0;
                        
                        if(SpO2_old != SpO2) 
                        {                            
                            sprintf(value_display, "%d", (uint8)SpO2);
                            Display_String(TEXTSIZE_SPO2_HR, WHITE, X_SPO2, Y_SPO2, value_display, WIDTH_SPO2_HR,HEIGHT_SPO2_HR);
  
                            SpO2_old = SpO2;
                        }                
                    }
                    else 
                    {
                        count_SpO2++;
                        if (count_SpO2 == COUNT_ERROR)
                        {
                            count_SpO2 = 0; 
                            Display_String(TEXTSIZE_SPO2_HR, WHITE, X_SPO2, Y_SPO2, "--",WIDTH_SPO2_HR,HEIGHT_SPO2_HR);
                        }
                    }
                    
                    if(HR_valid == 1 && HR < HR_MAX && HR > HR_MIN)
                    {      
                        count_HR = 0;
                        
                        if(HR_old != HR)
                        {
                            sprintf(value_display, "%d", (uint8)HR);
                            Display_String(TEXTSIZE_SPO2_HR, WHITE, X_HR, Y_HR, value_display,WIDTH_SPO2_HR,HEIGHT_SPO2_HR);

                            HR_old = HR;
                        }                
                    }
                    else
                    {                       
                        count_HR++;
                        if (count_HR == COUNT_ERROR)
                        {
                            count_HR = 0;
                            Display_String(TEXTSIZE_SPO2_HR, WHITE, X_HR, Y_HR, "--",WIDTH_SPO2_HR,HEIGHT_SPO2_HR);
                        }
                    }  
                }
            }
            else //interrupt was triggered by temperature
            {
                Sensor_ReadRegister(SENSOR_ADDRESS,DIE_TEMP_DATA_INT_ADDRESS, &Temp_uint);
                Sensor_ReadRegister(SENSOR_ADDRESS,DIE_TEMP_DATA_FRAC_ADDRESS,&Temp_frac); //also clears interrupt line
                
                Temp_int = (int8)Temp_uint;
                Temp_float = Temp_int + Temp_frac*(float)DIE_TEMP_FRAC_COEFF;
                if (temp_mode == FAHRENHEIT)
                    Temp_float = Temp_float*9/5 + 32; //convert in °F

                if((temp_mode == CELSIUS && Temp_float > TEMP_MIN_CELSIUS && Temp_float < TEMP_MAX_CELSIUS) || 
                   (temp_mode == FAHRENHEIT && Temp_float > TEMP_MIN_FAHRENHEIT && Temp_float < TEMP_MAX_FAHRENHEIT))
                {
                    Temp_dec = Temp_float*10 - ((int16)Temp_float)*10;
                    
                    if((Temp_int_old != (int8)Temp_float) || (Temp_dec_old != Temp_dec))
                    {         
                        count_temp = 0;
                                              
                        sprintf(value_display, "%d.%d", (int16)Temp_float, Temp_dec);
                        Display_String(TEXTSIZE_TEMP, WHITE, X_TEMP, Y_TEMP, value_display, 30, 20);
                        
                        Temp_int_old = (int8)Temp_float;
                        Temp_dec_old = Temp_dec;
                    }     
                    if(flag_temp == TRUE)
                    {
                        flag_temp = FALSE;

                        if(temp_mode == CELSIUS)
                            Display_String(TEXTSIZE_TEMP, WHITE, X_TEMP_UNIT,Y_TEMP_UNIT, "C", WIDTH_TEMP, HEIGHT_TEMP); 
                        else  
                            Display_String(TEXTSIZE_TEMP, WHITE, X_TEMP_UNIT,Y_TEMP_UNIT, "F", WIDTH_TEMP, HEIGHT_TEMP);
                    }                     
                }
                else
                {
                    count_temp++;
                    if (count_temp == COUNT_ERROR)
                    {
                        count_temp = 0;    
                        Display_String(TEXTSIZE_TEMP, WHITE, X_TEMP, Y_TEMP, "--", WIDTH_TEMP, HEIGHT_TEMP);
                    }
                }    
            }
        }
        
        //Timer interrupt
        if(flag_timer == TRUE)
        {
            flag_timer = FALSE;
            Sensor_WriteRegister(SENSOR_ADDRESS,DIE_TEMP_CONFIG_ADDRESS, DIE_TEMP_CONFIG_MASK); //starts temperature acquisition
        }
        
        watchdog++;
        if(watchdog == WATCHDOG_TRIGGER)
        {
            watchdog = 0;
            UART_PutString("\nWatchdog triggered.\n");
            Sensor_Initialization();
        }
    }
}

/* [] END OF FILE */
