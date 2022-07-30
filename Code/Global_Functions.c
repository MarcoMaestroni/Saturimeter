/**
*   \file Global_Functions.c
*   \brief Source file for general functions
*/

#include "Global_Functions.h"

uint16 sample_rate[4] = {50, 100, 200, 400};
uint8 adc_resolution[4] = {15, 16, 17, 18};
uint16 adc_range[4] = {2048, 4096, 8192, 16384};

void Initialization()
{
    PWM_Start();
    UART_Start();
    I2C_Start();
    EEPROM_Start();
    CyDelay(100);
    Timer_Start();
}

void Check_Connection()
{
    uint8 connection_sensor = I2C_IsDeviceConnected(SENSOR_ADDRESS);
    uint8 connection_display = I2C_IsDeviceConnected(DISPLAY_ADDRESS);
    
    if(!connection_display && !connection_sensor)
    {
        PWM_Setting(PERIOD_4,DC_4);
        UART_PutString("\nPeripherals not found. CONNECT THE PERIPHERALS.\n");
        do{
            connection_sensor = I2C_IsDeviceConnected(SENSOR_ADDRESS);
            connection_display = I2C_IsDeviceConnected(DISPLAY_ADDRESS);
        }while(!connection_display && !connection_sensor);
        UART_PutString("Peripherals connected!\n");
    }
    else if(!connection_sensor)
    {
        PWM_Setting(PERIOD_2,DC_2);
        UART_PutString("\nSensor not found. CONNECT THE SENSOR.\n"); 
        do{
            connection_sensor = I2C_IsDeviceConnected(SENSOR_ADDRESS);
        }while(!connection_sensor);
        UART_PutString("Sensor connected!\n");
    }
    else if(!connection_display)
    {
        PWM_Setting(PERIOD_3,DC_3);
        UART_PutString("\nDisplay not found. CONNECT THE DISPLAY.\n");
        do{
            connection_display = I2C_IsDeviceConnected(DISPLAY_ADDRESS);
        }while(!connection_display);
        UART_PutString("Display connected!\n");
    }
    
    PWM_Setting(PERIOD_1,DC_1);
}

void Menu_Help()
{
    uint8 read, data;
    char message[60];
    
    UART_PutString("\nPACKET STRUCTURE \nHeader byte: 0xA0 \nData byte \nTail byte: \n");
    
    read = EEPROM_ReadByte(EEPROM_SPO2_CONFIG);
    
    data = (read & SPO2_CONFIG_SAMPLERATE_MASK) >> SPO2_CONFIG_SAMPLERATE_POS;
    sprintf(message, "  0xC0: SpO2 sample rate (Current value: %d Hz)\n", sample_rate[data]);
    UART_PutString(message);
    
    data = (read & SPO2_CONFIG_LEDPW_MASK) >> SPO2_CONFIG_LEDPW_POS;
    sprintf(message, "  0xC1: ADC Resolution (Current value: %d bit)\n", adc_resolution[data]);
    UART_PutString(message);
    
    data = (read & SPO2_CONFIG_ADCRANGE_MASK) >> SPO2_CONFIG_ADCRANGE_POS;
    sprintf(message, "  0xC2: ADC Range control (Current value: %d nA)\n", adc_range[data]);
    UART_PutString(message);
    
    read = EEPROM_ReadByte(EEPROM_LED_PA_RED);
    sprintf(message, "  0xC3: Red LED current (Current value: 0x%02X)\n", read);
    UART_PutString(message);
    
    read = EEPROM_ReadByte(EEPROM_LED_PA_IR);
    sprintf(message, "  0xC4: IR LED current (Current value: 0x%02X)\n", read);
    UART_PutString(message);
    
    read = EEPROM_ReadByte(EEPROM_THRESH);
    sprintf(message, "  0xC5: Sensitivity threshold (Current value: %d)\n", read);
    UART_PutString(message);
}

void Menu_Configuration(uint8 data, uint8 tail, uint8* adc_res, uint8_t* threshold)
{
    uint8 read;
    char message [60];
    
    switch(tail)
    {                                      
        case TAIL_0:
            if(data > SPO2_NUM_RATES-1) 
            {
                data = 3;
                sprintf(message, "\nValue not allowed! ADC range set to %d Hz.", sample_rate[data]);
                UART_PutString(message);
            }
            Sensor_OverwriteRegister(data, SP02_CONFIG_ADDRESS, EEPROM_SPO2_CONFIG, SPO2_CONFIG_SAMPLERATE_MASK, SPO2_CONFIG_SAMPLERATE_POS);         
            UART_PutString("\nSpO2 sample rate set. \n");
            Display_String(1, WHITE, X_MENU, Y_MENU, " SpO2 sample rate set", WIDTH_MENU, HEIGHT_MENU);
            //Setting averaging too
            Sensor_ReadRegister(SENSOR_ADDRESS, FIFO_CONFIG_ADDRESS, &read);
            data = (read & ~FIFO_CONFIG_AVERAGING_MASK) | (data << FIFO_CONFIG_AVERAGING_POS);
            Sensor_WriteRegister(SENSOR_ADDRESS, FIFO_CONFIG_ADDRESS, data);
            break;
            
        case TAIL_1:
            if(data > 3)
            {
                data = 3;
                sprintf(message, "\nValue not allowed! ADC range set to %d bit.", adc_resolution[data]);
                UART_PutString(message);
            }   
            *adc_res = data;
            Sensor_OverwriteRegister(data, SP02_CONFIG_ADDRESS, EEPROM_SPO2_CONFIG, SPO2_CONFIG_LEDPW_MASK, SPO2_CONFIG_LEDPW_POS);
            UART_PutString("\nLED pulse width set. \n"); //ADC resolution
            Display_String(1, WHITE, X_MENU, Y_MENU, "      LED PW set", WIDTH_MENU, HEIGHT_MENU);
            break;
            
        case TAIL_2:
            if(data > 3)
            {
                data = 3;
                sprintf(message, "\nValue not allowed! ADC range set to %d nA.", adc_range[data]);
                UART_PutString(message);
            }
            Sensor_OverwriteRegister(data, SP02_CONFIG_ADDRESS, EEPROM_SPO2_CONFIG, SPO2_CONFIG_ADCRANGE_MASK, SPO2_CONFIG_ADCRANGE_POS);
            UART_PutString("\nSpO2 ADC range set. \n");
            Display_String(1, WHITE, X_MENU, Y_MENU, "  SpO2 ADC range set", WIDTH_MENU, HEIGHT_MENU);
            break;
            
        case TAIL_3:
            if (data > LED_MAX_CURRENT)
            {
                data = LED_MAX_CURRENT;
                sprintf(message, "\nValue not allowed! Red LED current capped to 0x%02X.", data);
                UART_PutString(message);
            }
            Sensor_OverwriteRegister(data, LED_PA_1_ADDRESS, EEPROM_LED_PA_RED, LED_PA_CONFIG_MASK, LED_PA_CONFIG_POS); 
            UART_PutString("\nRed LED current set. \n");
            Display_String(1, WHITE, X_MENU, Y_MENU, " Red LED current set", WIDTH_MENU, HEIGHT_MENU);
            break;
            
        case TAIL_4:
            if (data > LED_MAX_CURRENT)
            {
                data = LED_MAX_CURRENT;
                sprintf(message, "\nValue not allowed! IR LED current capped to 0x%02X.", data);
                UART_PutString(message);
            }
            Sensor_OverwriteRegister(data, LED_PA_2_ADDRESS, EEPROM_LED_PA_IR, LED_PA_CONFIG_MASK, LED_PA_CONFIG_POS);
            UART_PutString("\nIR LED current set. \n");
            Display_String(1, WHITE, X_MENU, Y_MENU, "  IR LED current set", WIDTH_MENU, HEIGHT_MENU);
            break;
            
        case TAIL_5:
            if (data > THRESH_MAX)
            {
                data = THRESH_MAX;
                sprintf(message, "\nValue not allowed! Sensitivity threshold capped to %d.", data);
                UART_PutString(message);
            }
            *threshold = data;
            EEPROM_UpdateTemperature();
            EEPROM_WriteByte(data, EEPROM_THRESH);
            UART_PutString("\nSensitivity threshold set. \n");
            Display_String(1, WHITE, X_MENU, Y_MENU, "    Threshold set", WIDTH_MENU, HEIGHT_MENU);
            break;
            
        default:
            UART_PutString("\nWrong tail byte. \n");
            break;
    }
}

uint8 Temperature_Mode(uint8 mode)
{
    EEPROM_UpdateTemperature();
    EEPROM_WriteByte(mode, EEPROM_TEMP_MODE);
    return mode;
}

void PWM_Setting(uint16 period, uint8 DC)
{
    PWM_WritePeriod(period);
    PWM_WriteCompare(period-period*DC/100);
}

uint8_t I2C_IsDeviceConnected(uint8_t device_address)
{
    uint8_t error = I2C_MasterSendStart(device_address, I2C_WRITE_XFER_MODE);
    I2C_MasterSendStop();
    
    if (error == I2C_MSTR_NO_ERROR)
        return 1;
    
    return 0;
} 

/* [] END OF FILE */
