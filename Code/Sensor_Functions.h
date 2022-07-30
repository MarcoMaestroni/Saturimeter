/**
*   \file Sensor_Functions.h
*   \brief Header file for MAX30101
*   \credits Maxim Integrated Products, Inc. for library functions (copyright notice at the end)
*/

#ifndef _SENSOR_FUNCTIONS_H
    #define _SENSOR_FUNCTIONS_H
    
    #include "project.h"
    #include "stdio.h"

    #define SENSOR_ADDRESS 0x57
    #define PART_ID_ADDRESS 0xFF
    #define PART_ID 0x15
    #define REVISON_ID_ADDRESS 0xFE

    //Interrupts registers
    #define INTERRUPT_STATUS_1_ADDRESS 0x00
    #define INTERRUPT_ENABLE_1_ADDRESS 0x02
    #define INTERRUPT_ENABLE_1_MASK 0x80 //interrupts on almost full fifo
    #define INTERRUPT_ENABLE_2_ADDRESS 0x03
    #define INTERRUPT_ENABLE_2_MASK 0X02 //interrupt on temperature ready

    //FIFO registers
    #define FIFO_WRITE_POINTER_ADDRESS 0x04
    #define FIFO_OVERFLOW_COUNTER_ADDRESS 0x05
    #define FIFO_READ_POINTER_ADDRESS 0x06
    #define FIFO_DATA_ADDRESS 0x07
    #define FIFO_CONFIG_ADDRESS 0x08 
    #define FIFO_CONFIG_AVERAGING_MASK 0b11100000
    #define FIFO_CONFIG_AVERAGING_POS 5
    #define FIFO_CONFIG_MASK 0X04 //Rollover disabled
    
    //Operating state register
    #define MODE_CONFIG_ADDRESS 0x09 
    #define MODE_CONFIG_SPO2 0x03
    #define MODE_CONFIG_RESET_MASK 0x40

    //Registers to be set by the configuration menu
    #define SP02_CONFIG_ADDRESS 0x0A
    #define SPO2_CONFIG_SAMPLERATE_MASK 0b00011100
    #define SPO2_CONFIG_SAMPLERATE_POS 2
    #define SPO2_CONFIG_LEDPW_MASK 0b00000011
    #define SPO2_CONFIG_LEDPW_POS 0
    #define SPO2_CONFIG_ADCRANGE_MASK 0b01100000
    #define SPO2_CONFIG_ADCRANGE_POS 5
    #define LED_PA_1_ADDRESS 0x0C //Red
    #define LED_PA_2_ADDRESS 0x0D //IR
    #define LED_PA_CONFIG_MASK 0b11111111
    #define LED_PA_CONFIG_POS 0
    #define LED_MAX_CURRENT 0x1F 

    //Temperature data registers
    #define DIE_TEMP_DATA_INT_ADDRESS 0x1F
    #define DIE_TEMP_DATA_FRAC_ADDRESS 0x20
    #define DIE_TEMP_CONFIG_ADDRESS 0x21
    #define DIE_TEMP_CONFIG_MASK 0x01 //starts temperature reading (returns to 0 when reading is concluded)
    #define DIE_TEMP_FRAC_COEFF 0.0625 //coefficient of fractional temperature data increments

    //Data transmission
    #define NUM_LEDS 2
    #define LED_BUFFER_SIZE (6*NUM_LEDS+2)
    #define FIFO_BYTES (3*NUM_LEDS)
    #define FIFO_NUM_SAMPLES 32
    #define FIFO_SIZE (FIFO_BYTES*FIFO_NUM_SAMPLES)
    #define SPO2_NUM_RATES 4
    #define UPDATE_TIME_WINDOW 2.0 //HR and SpO2 are updated every UPDATE_TIME sec (the sytem can detect an heart rate down to 60/UPDATE_TIME_WINDOW bpm)
    
    //Valid values
    #define HR_MAX 220
    #define HR_MIN 30
    #define SPO2_MIN 75
    #define TEMP_MAX_CELSIUS 85 //taken from datasheet
    #define TEMP_MAX_FAHRENHEIT 185
    #define TEMP_MIN_CELSIUS (-40) //taken from datasheet
    #define TEMP_MIN_FAHRENHEIT (-40)
    #define COUNT_ERROR 3 //number of consecutive permissable invalid values before showing error                   
    #define THRESH_MAX 30
    
    //EEPROM 
    #define EEPROM_SPO2_CONFIG 0x0001
    #define EEPROM_LED_PA_RED 0x0002
    #define EEPROM_LED_PA_IR 0x0003
    #define EEPROM_THRESH 0x0004

    //UART config menu
    #define HEADER 0xA0
    #define TAIL_0 0xC0
    #define TAIL_1 0xC1
    #define TAIL_2 0xC2
    #define TAIL_3 0xC3
    #define TAIL_4 0xC4
    #define TAIL_5 0xC5

    //Functions declarations
    uint8 Sensor_Initialization(); //reset and set sensor registers, returns ADC resolution
    void Sensor_SetRegister(uint8 address, uint8 value); //set a register and display its value
    void Sensor_OverwriteRegister(uint8 data, uint8 address, uint8 address_eeprom, uint8 mask, uint8 pos); //overwrite register in positions specified by the mask
                                                                                                           //pos is the position of the lsb of data in the register
    uint32 Value_LED(uint8 adc_res, uint8* fifo); //computes the value of the led from fifo data
    void Fill_Buffer(uint32 value, uint8* buffer);
    uint32_t Filter_Buffer(uint32_t* buffer, uint16 filter_size); //MA filter
    void Sensor_ReadRegister(uint8_t device_address, uint8_t register_address, uint8_t* data);
    void Sensor_ReadFIFO(uint8_t device_address, uint8_t register_address, uint8_t register_count, uint8_t* data);
    void Sensor_WriteRegister(uint8_t device_address, uint8_t register_address, uint8_t data); 
    void Sensor_Reconnect(uint8 device_address);

    //LIBRARY
    
    #define SAMPLE_RATE 50 //actual sample rate is constant because of averaging
    
    void maxim_heart_rate_and_oxygen_saturation(uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, int32_t *pn_spo2, int8_t *pch_spo2_valid, 
                                                int32_t *pn_heart_rate, int8_t *pch_hr_valid, uint8_t threshold);
    void maxim_find_peaks(int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, int32_t n_size, int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num);
    void maxim_peaks_above_min_height(int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, int32_t n_size, int32_t n_min_height);
    void maxim_remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_min_distance);
    void maxim_sort_ascend(int32_t  *pn_x, int32_t n_size);
    void maxim_sort_indices_descend(int32_t  *pn_x, int32_t *pn_indx, int32_t n_size);
#endif

/*
******************************************************************************
* Copyright (C) 2015 Maxim Integrated Products, Inc., All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of Maxim Integrated
* Products, Inc. shall not be used except as stated in the Maxim Integrated
* Products, Inc. Branding Policy.
*
* The mere transfer of this software does not imply any licenses
* of trade secrets, proprietary technology, copyrights, patents,
* trademarks, maskwork rights, or any other form of intellectual
* property whatsoever. Maxim Integrated Products, Inc. retains all
* ownership rights.
*******************************************************************************
*/

/* [] END OF FILE */
