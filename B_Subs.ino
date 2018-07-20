/************************************************************************************************************************************************
 ************************************************************************************************************************************************
 **                                                          Subroutines                                                                       **
 ************************************************************************************************************************************************
 ************************************************************************************************************************************************
 */


/************************************************************************************************************************************************
 * Add5N1(messFlag,wind,rain)   : Routine to add last reading to an array of readings for wind and rain                                         *
 * messFlag = true if last message received, false if missed, wind = last received wind reading, winddirection = last wind direction,           *
 * rain = last received rain reading                                                                                                            *
 *                                                                                                                                              *
 * Signal5N1  : Calculate signal quality based on how many of the last ten messages were received on time. 10 = great, 0 = not at all           *                     
 * WindSpeedAverage   : Average of the last 33 wind speed readings  (10 minute average)                                                         *
 * WindSpeedAverage2m : Average of the last 7 wind speed readings  (2 minute average)                                                           *
 * WindSpeedPeak      : Highest of the last 33 wind speed readings  (10 minute peak)                                                            *                                                 
 * WindSpeedPeak2m    : Highest of the last 7 wind speed readings  (2 minute peak)                                                              *
 * WindPeakDirection  : Wind direction during peak wind of the last 10 minutes                                                                  *
 * WindPeakDirection2m: Wind direction during peak wind of the last 2 minutes                                                                   *
 * RainRate           : Amount of rainfall in the last 10 minutes used to estimate the hourly rate                                              *
 ************************************************************************************************************************************************
 */

void Add5N1(bool messFlag, float wind, int windDirection, float rain) {   // Add data to top of readings array, drop off oldest reading

  float OldRate = RainRate;
                                                                          // 0 is oldest, 32 will be newest
  Signal5N1 = 0;                                                          // start at zero
  WindSpeedPeak = 0;
  WindSpeedPeak2m = 0;
  WindSpeedAverage = 0;
  WindSpeedAverage2m = 0;

  for (uint8_t i = 0; i < 32; i++)                                        // move the data down
  {
    Mess5N1[i] = Mess5N1[i + 1];

    if (i > 22) {                                                         // only use the last 10
      Signal5N1 += Mess5N1[i];                                            // count up the good messages
    }

    if (i > 25) {                                                         // look at last seven readings for 2 minute calculations
      WindSpeedAverage2m += WindSpeed[i];                                 // add up the wind speeds
      if (WindSpeed[i] > WindSpeedPeak2m) {
        WindSpeedPeak2m = WindSpeed[i];                                   // update peak if this reading is higher
        WindPeakDirection2m = WindDirection[i];
      }
    }
    
    WindSpeed[i] = WindSpeed[i + 1];
    WindDirection[i] = WindDirection[i+1];
    WindSpeedAverage += WindSpeed[i];                                     // add up the windspeeds

    if (WindSpeed[i] > WindSpeedPeak) {
      WindSpeedPeak = WindSpeed[i];                                       // update peak if this reading is higher
      WindPeakDirection = WindDirection[i];
    }
    Rain[i] = Rain[i + 1];
  }

  Mess5N1[32] = messFlag;
  Signal5N1 += messFlag;

  WindSpeed[32] = wind;
  WindDirection[32] = windDirection;
  WindSpeedAverage = (WindSpeedAverage + wind) / 33;                  // average over 33 readings (10 minutes)
  if (wind > WindSpeedPeak) {
    WindSpeedPeak = wind;
    WindPeakDirection = windDirection;
  }

  WindSpeedAverage2m = (WindSpeedAverage2m + wind) / 7;               // average over 7 readings (2 minutes)
  if (wind > WindSpeedPeak2m) {
    WindSpeedPeak2m = wind;
    WindPeakDirection2m = windDirection;
  }
                                                                      
  Rain[32] = rain;
  
  if (millis() < (11 * 60 * 1000)) {                                    // Might be false rain rate if we just powered up
    RainRate = 0;
  } else {
    if ((Rain[32] - Rain[0]) < 0) {
      RainRate = (Rain[32] + RainYesterday - Rain[0]) * (3600/592);     // Don't zero the rate at midnight
    } else {
      RainRate = (Rain[32] - Rain[0]) * (3600/592);                     // convert from rain/592 seconds to rain/3600 seconds (hour)
    }
    RainRate = (RainRate + (9 * OldRate))/10;                           // calculate rain rate and weighted average with previous rate to reduce spikes
  }

};



/************************************************************************************************************************************************
 * decode_5n1(dataBytes[])    : Routine to decode the message from the 5-n-1 sensor                                                             *
 * dataBytes{] is the array holding the received message.                                                                                       *
 *                                                                                                                                              *                                                                                                                                               
 * The 5-in-1 transmits every 18 seconds, alternating between two message types                                                                 *
 * Message type 1 transmits Switch, ID, Wind, Temperature, and Humidity                                                                         *
 * Message type 2 transmits Switch, ID, Wind, Wind direction, and Rainfall                                                                      *
 * Wind updates every 18 seconds, Temperature, Humidity, Wind direction, and Rainfall update every 36 seconds                                   *
 ************************************************************************************************************************************************ 
 */
 
void decode_5n1(byte dataBytes[])   {                                             // 5-n-1 should transmit 3 times every 18 seconds.
                                                                                  // 36 seconds before each message type repeats.
                                                                                  // Effectively, wind is every 18 seconds
  int messtype = dataBytes[2] & 0x3F;

#ifdef DebugOut
  Serial.print(millis() - LastGood5N1);
  Serial.print("ms - ");
  Serial.print("5n1 - ");
  Serial.print(acurite_txr_getSensorId(dataBytes[0], dataBytes[1]), HEX);
  Serial.print(" CH:");
  Serial.print(GetLetter(dataBytes[0]));
  Serial.print(" N:");
  Serial.print(GetMessageNo(dataBytes[0]));
  Serial.print("; W:");
  Serial.print(acurite_getWindSpeed_kph(dataBytes[3], dataBytes[4]));
  if (messtype == MT_WS_WD_RF)  {                                               // Wind Speed, Direction and Rainfall
    Serial.print("; D:");
    Serial.print(acurite_5n1_winddirection_str[dataBytes[4] & 0x0F]);
    Serial.print("; R:");
    Serial.print(acurite_getRainfall(dataBytes[5], dataBytes[6]));
    Serial.print(";");
  } else if (messtype == MT_WS_T_RH) {                                          // Wind speed, Temp, Humidity
    Serial.print("; T:");
    Serial.print(acurite_getTemp_5n1(dataBytes[4], dataBytes[5]));
    Serial.print("; H:");
    Serial.print(acurite_getHumidity(dataBytes[6]));
    Serial.print("%;");
  }
    if ((dataBytes[2] & 0x40) != 0x40) {                                       // Check for low battery warning
      Serial.print("Low Battery ");
    }

/*  if ((Temperature5N1 > 90) || (Temperature5N1 < 40) || (CurrentWind > 20) || (Humidity5N1 < 1) || (Humidity5N1 > 99)) {
    Serial.print("****");                                                           // easier to search for problem in debug log
  } */
#endif
  if ((acurite_txr_getSensorId(dataBytes[0], dataBytes[1]) == ID5N1) || (ID5N1 == 0)) {               // is this our 5N1?
    if (((millis() - LastGood5N1) > 500) || (Signal5N1 == 0))  {                    // make sure this isn't a redundant message (came in too soon after last good one)
  
      CurrentWind = acurite_getWindSpeed_kph(dataBytes[3], dataBytes[4]);
  
      if (messtype == MT_WS_WD_RF)  {                                               // Wind Speed, Direction and Rainfall
    
        CurrentDirection =  dataBytes[4] & 0x0F;
        CurrentRain = acurite_getRainfall(dataBytes[5], dataBytes[6]);
      } else if (messtype == MT_WS_T_RH) {                                          // Wind speed, Temp, Humidity
    
        Temperature5N1 = acurite_getTemp_5n1(dataBytes[4], dataBytes[5]);
        Humidity5N1 = acurite_getHumidity(dataBytes[6]);\
      }
      if ((dataBytes[2] & 0x40) != 0x40) {                                          // Check for low battery warning
        BatteryLow5N1 = true;
      } else {
        BatteryLow5N1 = false;
      }
      Next5N1 = millis();                                                            // reset the message timers
      LastGood5N1 = millis();
      Add5N1(true, CurrentWind, CurrentDirection, CurrentRain);                       // log readings
    } else {
      #ifdef DebugOut
      Serial.print("- Redundant");
      #endif
    }
  } else {
    #ifdef DebugOut
    Serial.print(" - Wrong ID");
    #endif
  }
  #ifdef DebugOut
  Serial.println();
  #endif
}



/************************************************************************************************************************************************
 * displayBitTiming()   : Routine to show the bit timings for a message                                                                         *
 ************************************************************************************************************************************************
 */

#ifdef DebugOut
void displayBitTiming()                                               // Print the bit stream for debugging.
{
  unsigned int ringIndex;

  Serial.print("IchangeCount = ");
  Serial.println(IchangeCount);
  Serial.print("bytesReceived = ");
  Serial.println(bytesReceived);
  Serial.print("syncIndex = ");
  Serial.println(syncIndex);

  ringIndex = (syncIndex - (SYNCPULSEEDGES - 1)) % RING_BUFFER_SIZE;

  for ( int i = 0; i < (SYNCPULSECNT + (bytesReceived * 8)); i++ )
  {
    int bit = convertTimingToBit( pulseDurations[ringIndex % RING_BUFFER_SIZE],
                                  pulseDurations[(ringIndex + 1) % RING_BUFFER_SIZE] );
    Serial.print("bit ");
    Serial.print( i );
    Serial.print(" = ");
    Serial.print(bit);
    Serial.print(" t1 = ");
    Serial.print(pulseDurations[ringIndex % RING_BUFFER_SIZE]);
    Serial.print(" t2 = ");
    Serial.println(pulseDurations[(ringIndex + 1) % RING_BUFFER_SIZE]);


    ringIndex += 2;
  }

}
#endif




