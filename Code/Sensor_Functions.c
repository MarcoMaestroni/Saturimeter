/**
*   \file Sensor_Functions.c
*   \brief Source file for MAX30101
*/

#include "Global_Functions.h"

void Sensor_SetRegister(uint8 address, uint8 value)
{
    uint8 read;
    char message[20]; 
    Sensor_WriteRegister(SENSOR_ADDRESS, address, value);
    Sensor_ReadRegister(SENSOR_ADDRESS, address, &read);
    
    sprintf(message, "[0x%02X] set to 0x%02X", address, read);
    UART_PutString(message);
    if(value != read)
    {
        sprintf(message, " (ERROR: expected 0x%02X)", value);
        UART_PutString(message);
    }
    UART_PutChar('\n');
}

void Sensor_OverwriteRegister(uint8 data, uint8 address, uint8 address_eeprom, uint8 mask, uint8 pos)
{
    uint8 read;
    Sensor_ReadRegister(SENSOR_ADDRESS, address, &read);
    read = (read & ~mask) | (data << pos);
    Sensor_WriteRegister(SENSOR_ADDRESS, address, read);
    
    EEPROM_UpdateTemperature();
    EEPROM_WriteByte(read, address_eeprom);
}

uint8 Sensor_Initialization()
{
    uint8 revision_ID, part_ID;
    uint8 setting; //registers setting variable
    uint8 read; //support variable
    uint8 fifo_clean[FIFO_BYTES];    
    uint8 adc_res;
    char message[60]; //message to be sent through UART
    
    //Clear power ready flag
    Sensor_ReadRegister(SENSOR_ADDRESS, INTERRUPT_STATUS_1_ADDRESS, &read); 
    
    //Reset registers
    Sensor_WriteRegister(SENSOR_ADDRESS,MODE_CONFIG_ADDRESS, MODE_CONFIG_RESET_MASK); 
    do
    Sensor_ReadRegister(SENSOR_ADDRESS,MODE_CONFIG_ADDRESS, &read);
    while(read & 0b01000000);
    UART_PutString("\nSensor reset completed.\n");
      
    //Send PART and REVISION ID
    Sensor_ReadRegister(SENSOR_ADDRESS, REVISON_ID_ADDRESS, &revision_ID);
    Sensor_ReadRegister(SENSOR_ADDRESS, PART_ID_ADDRESS, &part_ID);
    sprintf(message, "Revision ID: 0x%02X \nPart ID: 0x%02X \n", revision_ID, part_ID);
    UART_PutString(message);
    if(part_ID != PART_ID)
    {
        sprintf(message, "Part ID different than expected (0x%02X)!\n", PART_ID);
        UART_PutString(message);
    }
    
    //Setting FIFO-almost-full values
    UART_PutString("FIFO config: ");
    Sensor_SetRegister(FIFO_CONFIG_ADDRESS, FIFO_CONFIG_MASK);
    
    //Enabling SpO2 mode (RED and IR leds)
    UART_PutString("Mode config: ");
    Sensor_SetRegister(MODE_CONFIG_ADDRESS, MODE_CONFIG_SPO2); 
    
    //Set SpO2 config
    UART_PutString("SpO2 config: ");
    setting = EEPROM_ReadByte(EEPROM_SPO2_CONFIG);
    Sensor_SetRegister(SP02_CONFIG_ADDRESS, setting); 
    adc_res = (setting & SPO2_CONFIG_LEDPW_MASK) >> SPO2_CONFIG_LEDPW_POS;
    //We have to cap the sample rate and set the averaging (proportional to sample rate)
    setting = (setting & SPO2_CONFIG_SAMPLERATE_MASK) >> SPO2_CONFIG_SAMPLERATE_POS;
    if(setting > SPO2_NUM_RATES-1) //number of unavailable spo2 sample rate configs is equal to adc_res
    {
        setting = SPO2_NUM_RATES-1;
        Sensor_OverwriteRegister(setting, SP02_CONFIG_ADDRESS, EEPROM_SPO2_CONFIG, SPO2_CONFIG_SAMPLERATE_MASK, SPO2_CONFIG_SAMPLERATE_POS); 
    }
    Sensor_ReadRegister(SENSOR_ADDRESS, FIFO_CONFIG_ADDRESS, &read);
    setting = (read & ~FIFO_CONFIG_AVERAGING_MASK) | (setting << FIFO_CONFIG_AVERAGING_POS);
    Sensor_WriteRegister(SENSOR_ADDRESS, FIFO_CONFIG_ADDRESS, setting); 
    
    //Set Red LED PA
    UART_PutString( "Red LED PA: ");
    setting = EEPROM_ReadByte(EEPROM_LED_PA_RED);
    if (setting > LED_MAX_CURRENT)
        setting = LED_MAX_CURRENT;
    Sensor_SetRegister(LED_PA_1_ADDRESS, setting); 
    
    //Set IR LED PA
    UART_PutString("IR LED PA: ");
    setting = EEPROM_ReadByte(EEPROM_LED_PA_IR);
    if (setting > LED_MAX_CURRENT)
        setting = LED_MAX_CURRENT;
    Sensor_SetRegister(LED_PA_2_ADDRESS, setting); 
    
    //Enable Temp Interrupt
    UART_PutString("Interrupt Temperature config: ");
    Sensor_SetRegister(INTERRUPT_ENABLE_2_ADDRESS, INTERRUPT_ENABLE_2_MASK);
    //Enable Leds Interrupts
    UART_PutString("Interrupt LEDs config: ");
    Sensor_SetRegister(INTERRUPT_ENABLE_1_ADDRESS, INTERRUPT_ENABLE_1_MASK);
    
    //FIFO cleaning
    Sensor_WriteRegister(SENSOR_ADDRESS,FIFO_WRITE_POINTER_ADDRESS, 0);
    Sensor_WriteRegister(SENSOR_ADDRESS,FIFO_READ_POINTER_ADDRESS, 0);
    Sensor_ReadFIFO(SENSOR_ADDRESS, FIFO_DATA_ADDRESS, FIFO_BYTES, fifo_clean);
    
    return adc_res;
}

uint32 Value_LED(uint8 adc_res, uint8* fifo)
{
    uint32 value = ( (((uint32)(0b00000011 & (*fifo))) << 16) | (((uint32)(*(fifo+1))) << 8) | ((uint32)(*(fifo+2))) ) >> (3-adc_res);   
    return value;
}

void Fill_Buffer(uint32 value, uint8* buffer)
{
    *buffer = (uint8)(value >> 16);
    *(buffer+1) = (uint8)(value >> 8);
    *(buffer+2) = (uint8)value;
}

uint32_t Filter_Buffer(uint32_t* buffer, uint16 filter_size)
{
    uint8 i;
    uint32_t value = *buffer;
    
    for(i = 1; i< filter_size; i++)
        value += *(buffer+i);
    
    return value/filter_size;
}

void Sensor_ReadRegister(uint8_t device_address, uint8_t register_address, uint8_t* data)
{
    uint8_t error;
    
    do{
        error = I2C_MasterSendStart(device_address,I2C_WRITE_XFER_MODE);
        if (error == I2C_MSTR_NO_ERROR)
        {
            error = I2C_MasterWriteByte(register_address);
            if (error == I2C_MSTR_NO_ERROR)
            {
                error = I2C_MasterSendRestart(device_address, I2C_READ_XFER_MODE);
                if (error == I2C_MSTR_NO_ERROR)
                {
                    *data = I2C_MasterReadByte(I2C_NAK_DATA);
                }
            }
        }
        I2C_MasterSendStop();
        
        if(error)
            Sensor_Reconnect(device_address);   
    }while(error);
}

void Sensor_ReadFIFO(uint8_t device_address, uint8_t register_address, uint8_t register_count, uint8_t* data)
{
    uint8_t error;
    
    do{
        error = I2C_MasterSendStart(device_address,I2C_WRITE_XFER_MODE);
        if (error == I2C_MSTR_NO_ERROR)
        {
            error = I2C_MasterWriteByte(register_address);
            if (error == I2C_MSTR_NO_ERROR)
            {
                error = I2C_MasterSendRestart(device_address, I2C_READ_XFER_MODE);
                if (error == I2C_MSTR_NO_ERROR)
                {
                    uint8_t counter = register_count;
                    while(counter>1)
                    {
                        data[register_count-counter] = I2C_MasterReadByte(I2C_ACK_DATA);
                        counter--;
                    }
                    data[register_count-1] = I2C_MasterReadByte(I2C_NAK_DATA);
                }
            }
        }
        I2C_MasterSendStop();
        
        if(error)
            Sensor_Reconnect(device_address);           
    }while(error);
}

void Sensor_WriteRegister(uint8_t device_address, uint8_t register_address, uint8_t data)
{
    uint8_t error;
    
    do{
        error = I2C_MasterSendStart(device_address,I2C_WRITE_XFER_MODE);
        if (error == I2C_MSTR_NO_ERROR)
        {
            error = I2C_MasterWriteByte(register_address);
            if (error == I2C_MSTR_NO_ERROR)
            {
                error = I2C_MasterWriteByte(data);
            }
        }
        I2C_MasterSendStop();

        if(error)
            Sensor_Reconnect(device_address);        
    }while(error);
}

void Sensor_Reconnect(uint8_t device_address)
{
    uint8_t connection;
    if(device_address == SENSOR_ADDRESS)
    {
        UART_PutString("\nSensor not found. RECONNECT THE SENSOR.\n"); 
        PWM_Setting(PERIOD_2,DC_2);
        do{
            connection = I2C_IsDeviceConnected(device_address);
        }while(!connection);
        UART_PutString("Sensor reconnected!\n");
        Sensor_Initialization();
        PWM_Setting(PERIOD_1,DC_1);
    }
}

//LIBRARY FUNCTIONS

//uch_spo2_table is approximated as  -45.060*ratioAverage* ratioAverage + 30.354 *ratioAverage + 94.845 ;
const uint8_t uch_spo2_table[184]={ 95, 95, 95, 96, 96, 96, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98, 99, 99, 99, 99, 
                                    99, 99, 99, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 
                                    100, 100, 100, 100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 98, 98, 98, 97, 97, 
                                    97, 97, 96, 96, 96, 96, 95, 95, 95, 94, 94, 94, 93, 93, 93, 92, 92, 92, 91, 91, 
                                    90, 90, 89, 89, 89, 88, 88, 87, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81, 
                                    80, 80, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68, 67, 
                                    66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 
                                    49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 31, 30, 29, 
                                    28, 27, 26, 25, 23, 22, 21, 20, 19, 17, 16, 15, 14, 12, 11, 10, 9, 7, 6, 5, 
                                    3, 2, 1 } ;

void maxim_heart_rate_and_oxygen_saturation(uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, int32_t *pn_spo2, int8_t *pch_spo2_valid, 
                                            int32_t *pn_heart_rate, int8_t *pch_hr_valid, uint8_t threshold)
/**
* \brief        Calculate the heart rate and SpO2 level
* \par          Details
*               By detecting  peaks of PPG cycle and corresponding AC/DC of red/infra-red signal, the an_ratio for the SPO2 is computed.
*               Since this algorithm is aiming for Arm M0/M3. formaula for SPO2 did not achieve the accuracy due to register overflow.
*               Thus, accurate SPO2 is precalculated and save longo uch_spo2_table[] per each an_ratio.
*
* \param[in]    *pun_ir_buffer           - IR sensor data buffer
* \param[in]    n_ir_buffer_length       - data buffer length
* \param[in]    *pun_red_buffer          - Red sensor data buffer
* \param[out]    *pn_spo2                - Calculated SpO2 value
* \param[out]    *pch_spo2_valid         - 1 if the calculated SpO2 value is valid
* \param[out]    *pn_heart_rate          - Calculated heart rate value
* \param[out]    *pch_hr_valid           - 1 if the calculated heart rate value is valid
*
* \retval       None
*/
{
    int32_t an_x[n_ir_buffer_length]; //ir
    int32_t an_y[n_ir_buffer_length]; //red
    
  uint32_t un_ir_mean;
  int32_t k, n_i_ratio_count;
  int32_t i, n_exact_ir_valley_locs_count, n_middle_idx;
  int32_t n_th1, n_npks;   
  int32_t an_ir_valley_locs[15] ;
  int32_t n_peak_interval_sum;
  
  int32_t n_y_ac, n_x_ac;
  int32_t n_spo2_calc; 
  int32_t n_y_dc_max, n_x_dc_max; 
  int32_t n_y_dc_max_idx = 0;
  int32_t n_x_dc_max_idx = 0; 
  int32_t an_ratio[5], n_ratio_average; 
  int32_t n_nume, n_denom ;

  // calculates DC mean and subtract DC from ir
  un_ir_mean =0; 
  for (k=0 ; k<n_ir_buffer_length ; k++ ) 
    un_ir_mean += pun_ir_buffer[k] ;
  un_ir_mean =un_ir_mean/n_ir_buffer_length ;
    
  // remove DC and invert signal so that we can use peak detector as valley detector
  for (k=0 ; k<n_ir_buffer_length ; k++ )  
    an_x[k] = -1*(pun_ir_buffer[k] - un_ir_mean) ; 
    
//  //Moving Average
//    uint32_t value[n_ir_buffer_length];
//    for(k=0; k< n_ir_buffer_length; k++)
//        value[k] = an_x[k];
//    for(k=0; k< n_ir_buffer_length; k++)
//        an_x[k] = Filter_Buffer(&value[k], n_ir_buffer_length-k, MA_SIZE);
     
  // calculate threshold  
  n_th1=0; 
  for ( k=0 ; k<n_ir_buffer_length ;k++){
    n_th1 +=  an_x[k];
  }
  n_th1=  n_th1/ (n_ir_buffer_length);
  if( n_th1<threshold) n_th1=threshold; // min allowed
  if( n_th1>60) n_th1=60; // max allowed

  for ( k=0 ; k< UPDATE_TIME_WINDOW*(HR_MAX/60);k++) an_ir_valley_locs[k]=0;
  // since we flipped signal, we use peak detector as valley detector
  maxim_find_peaks( an_ir_valley_locs, &n_npks, an_x, n_ir_buffer_length, n_th1, SAMPLE_RATE/(HR_MAX/60), UPDATE_TIME_WINDOW*(HR_MAX/60));//peak_height, peak_distance, max_num_peaks 
  n_peak_interval_sum =0;
  if (n_npks>=2){
    for (k=1; k<n_npks; k++) n_peak_interval_sum += (an_ir_valley_locs[k] -an_ir_valley_locs[k -1] ) ;
    n_peak_interval_sum =n_peak_interval_sum/(n_npks-1);
    *pn_heart_rate =(int32_t)( (SAMPLE_RATE*60)/ n_peak_interval_sum );
    *pch_hr_valid  = 1;
  }
  else  { 
    *pn_heart_rate = -999; // unable to calculate because # of peaks are too small
    *pch_hr_valid  = 0;
  }

  // load raw value again for SPO2 calculation : RED(=y) and IR(=X)
  for (k=0 ; k<n_ir_buffer_length ; k++ )  {
      an_x[k] =  pun_ir_buffer[k] ; 
      an_y[k] =  pun_red_buffer[k] ; 
  }

  // find precise min near an_ir_valley_locs
  n_exact_ir_valley_locs_count =n_npks; 
  
  //using exact_ir_valley_locs , find ir-red DC andir-red AC for SPO2 calibration an_ratio
  //finding AC/DC maximum of raw

  n_ratio_average =0; 
  n_i_ratio_count = 0; 
  for(k=0; k< 5; k++) an_ratio[k]=0;
  for (k=0; k< n_exact_ir_valley_locs_count; k++){
    if (an_ir_valley_locs[k] > n_ir_buffer_length ){
      *pn_spo2 =  -999 ; // do not use SPO2 since valley loc is out of range
      *pch_spo2_valid  = 0; 
      return;
    }
  }
  // find max between two valley locations 
  // and use an_ratio betwen AC compoent of Ir & Red and DC compoent of Ir & Red for SPO2 
  for (k=0; k< n_exact_ir_valley_locs_count-1; k++){
    n_y_dc_max= -16777216 ; 
    n_x_dc_max= -16777216; 
    if (an_ir_valley_locs[k+1]-an_ir_valley_locs[k] >3){
        for (i=an_ir_valley_locs[k]; i< an_ir_valley_locs[k+1]; i++){
          if (an_x[i]> n_x_dc_max) {n_x_dc_max =an_x[i]; n_x_dc_max_idx=i;}
          if (an_y[i]> n_y_dc_max) {n_y_dc_max =an_y[i]; n_y_dc_max_idx=i;}
      }
      n_y_ac= (an_y[an_ir_valley_locs[k+1]] - an_y[an_ir_valley_locs[k] ] )*(n_y_dc_max_idx -an_ir_valley_locs[k]); //red
      n_y_ac=  an_y[an_ir_valley_locs[k]] + n_y_ac/ (an_ir_valley_locs[k+1] - an_ir_valley_locs[k])  ; 
      n_y_ac=  an_y[n_y_dc_max_idx] - n_y_ac;    // subracting linear DC compoenents from raw 
      n_x_ac= (an_x[an_ir_valley_locs[k+1]] - an_x[an_ir_valley_locs[k] ] )*(n_x_dc_max_idx -an_ir_valley_locs[k]); // ir
      n_x_ac=  an_x[an_ir_valley_locs[k]] + n_x_ac/ (an_ir_valley_locs[k+1] - an_ir_valley_locs[k]); 
      n_x_ac=  an_x[n_y_dc_max_idx] - n_x_ac;      // subracting linear DC compoenents from raw 
      n_nume=( n_y_ac *n_x_dc_max)>>7 ; //prepare X100 to preserve floating value
      n_denom= ( n_x_ac *n_y_dc_max)>>7;
      if (n_denom>0  && n_i_ratio_count <5 &&  n_nume != 0)
      {   
        an_ratio[n_i_ratio_count]= (n_nume*100)/n_denom ; //formular is ( n_y_ac *n_x_dc_max) / ( n_x_ac *n_y_dc_max) ;
        n_i_ratio_count++;
      }
    }
  }
  // choose median value since PPG signal may varies from beat to beat
  maxim_sort_ascend(an_ratio, n_i_ratio_count);
  n_middle_idx= n_i_ratio_count/2;

  if (n_middle_idx >1)
    n_ratio_average =( an_ratio[n_middle_idx-1] +an_ratio[n_middle_idx])/2; // use median
  else
    n_ratio_average = an_ratio[n_middle_idx ];

  if( n_ratio_average>2 && n_ratio_average <184){
    n_spo2_calc= uch_spo2_table[n_ratio_average] ;
    *pn_spo2 = n_spo2_calc ;
    *pch_spo2_valid  = 1;//  float_SPO2 =  -45.060*n_ratio_average* n_ratio_average/10000 + 30.354 *n_ratio_average/100 + 94.845 ;  // for comparison with table
  }
  else{
    *pn_spo2 =  -999 ; // do not use SPO2 since signal an_ratio is out of range
    *pch_spo2_valid  = 0; 
  }
}


void maxim_find_peaks( int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, int32_t n_size, int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num )
/**
* \brief        Find peaks
* \par          Details
*               Find at most MAX_NUM peaks above MIN_HEIGHT separated by at least MIN_DISTANCE
*
* \retval       None
*/
{
  maxim_peaks_above_min_height( pn_locs, n_npks, pn_x, n_size, n_min_height );
  maxim_remove_close_peaks( pn_locs, n_npks, pn_x, n_min_distance );
  if(*n_npks>n_max_num) *n_npks = n_max_num; //MODIFIED HERE
}

void maxim_peaks_above_min_height( int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, int32_t n_size, int32_t n_min_height )
/**
* \brief        Find peaks above n_min_height
* \par          Details
*               Find all peaks above MIN_HEIGHT
*
* \retval       None
*/
{
  int32_t i = 1, n_width;
  *n_npks = 0;
  
  while (i < n_size-1){
    if (pn_x[i] > n_min_height && pn_x[i] > pn_x[i-1]){      // find left edge of potential peaks
      n_width = 1;
      while (i+n_width < n_size && pn_x[i] == pn_x[i+n_width])  // find flat peaks
        n_width++;
      if (pn_x[i] > pn_x[i+n_width] && (*n_npks) < 15 ){      // find right edge of peaks
        pn_locs[(*n_npks)++] = i;    
        // for flat peaks, peak location is left edge
        i += n_width+1;
      }
      else
        i += n_width;
    }
    else
      i++;
  }
}

void maxim_remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_min_distance)
/**
* \brief        Remove peaks
* \par          Details
*               Remove peaks separated by less than MIN_DISTANCE
*
* \retval       None
*/
{
    
  int32_t i, j, n_old_npks, n_dist;
    
  /* Order peaks from large to small */
  maxim_sort_indices_descend( pn_x, pn_locs, *pn_npks );

  for ( i = -1; i < *pn_npks; i++ ){
    n_old_npks = *pn_npks;
    *pn_npks = i+1;
    for ( j = i+1; j < n_old_npks; j++ ){
      n_dist =  pn_locs[j] - ( i == -1 ? -1 : pn_locs[i] ); // lag-zero peak of autocorr is at index -1
      if ( n_dist > n_min_distance || n_dist < -n_min_distance )
        pn_locs[(*pn_npks)++] = pn_locs[j];
    }
  }

  // Resort indices int32_to ascending order
  maxim_sort_ascend( pn_locs, *pn_npks );
}

void maxim_sort_ascend(int32_t  *pn_x, int32_t n_size) 
/**
* \brief        Sort array
* \par          Details
*               Sort array in ascending order (insertion sort algorithm)
*
* \retval       None
*/
{
  int32_t i, j, n_temp;
  for (i = 1; i < n_size; i++) {
    n_temp = pn_x[i];
    for (j = i; j > 0 && n_temp < pn_x[j-1]; j--)
        pn_x[j] = pn_x[j-1];
    pn_x[j] = n_temp;
  }
}

void maxim_sort_indices_descend(  int32_t  *pn_x, int32_t *pn_indx, int32_t n_size)
/**
* \brief        Sort indices
* \par          Details
*               Sort indices according to descending order (insertion sort algorithm)
*
* \retval       None
*/ 
{
  int32_t i, j, n_temp;
  for (i = 1; i < n_size; i++) {
    n_temp = pn_indx[i];
    for (j = i; j > 0 && pn_x[n_temp] > pn_x[pn_indx[j-1]]; j--)
      pn_indx[j] = pn_indx[j-1];
    pn_indx[j] = n_temp;
  }
}

/* [] END OF FILE */
