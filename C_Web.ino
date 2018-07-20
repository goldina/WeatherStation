/************************************************************************************************************************************************
 * UpdateWeatherUnderground()   : Routine to send weather data to Weather Underground, rapidfire                                                *
 * Returns true if WU accepted the data, false if anything went wrong                                                                           *
 ************************************************************************************************************************************************
 */


/************************************************************************************************************************************************
 ************************************************************************************************************************************************
 **                                                         Webpages                                                                           **
 ************************************************************************************************************************************************
 ************************************************************************************************************************************************
 */


/************************************************************************************************************************************************
 *                                                          Root                                                                                *
 * Just identify ourselves                                                                                                                      *                                                         
 ************************************************************************************************************************************************
 */

void handleRoot() {


  server.send ( 200, "text/html", webPage );
  
 }


/************************************************************************************************************************************************
 *                                                          debug                                                                               *
 * Provide debug data                                                                                                                           *                                                         
 ************************************************************************************************************************************************
 */

void handleDebug() {

  float DTime = 0;
  float Level433 = 0;
  int last_ntp;
 
  String debugstr ="Weather Server Debug Page \r\nVersion: ";              // system
  debugstr += Version;
  debugstr += "\r\n";
  DTime = millis()/(10 * 60 * 60);
  DTime = DTime/100;                                                      // do it this way to get 1/100th, otherwise comes as an integer
  debugstr += String(DTime,2);
  
  debugstr += " hours uptime\r\n\nLocal Time: ";
  debugstr += String(NTP.getTimeDateString());
  debugstr += "\r\nLast NTP Sync:  ";                  // Radio levels
  last_ntp = NTP.getLastNTPSync();
  debugstr += String(NTP.getTimeDateString(last_ntp));
  debugstr += "\r\n\nWiFi signal level = ";                  // Radio levels
  debugstr += String(WiFi.RSSI());

  debugstr += " dBm, 433MHz receiver signal level = ";                    // Crude approximation from the MICRF211 data sheet                     
  Level433 = (((analogRead(A0) - 74)*80)/641) - 120;                      // note that the AD is apparently 0 to 3.2 Volt input range (default)
  Level433 = constrain(Level433, -120, -40);                              // 0V = 0, 3.2V = 1024
  debugstr += String(Level433,0);
  
  debugstr += " dBm\r\n";

  CurrentPressure = bme.readPressure() * 0.000295333727;                // get the air pressure

  debugstr += "\r\n\nAir Pressure = ";
  debugstr += String(CurrentPressure,2);                                  // Air pressure
  debugstr += " inHg, ";
  debugstr += String(bme.seaLevelForAltitude(204, bme.readPressure() / 100.0F));
  debugstr += " hPa; Receiver Temperature = ";
  debugstr += String(bme.readTemperature());
  debugstr += " C, ";
  debugstr += String(bme.readTemperature() * 1.8 + 32);
  debugstr += " F, Air Humidity = ";
  debugstr += String(bme.readHumidity(),2);                                // Humidity
  debugstr += " % ";

  debugstr += " \r\n\n5-in-1 Sensor:\r\nETA of next message: ";          // 5-in-1
  DTime = (Next5N1 + 18500 - millis())/1000;
  debugstr += String(DTime,0);
  debugstr += " seconds, Last good message was ";
  DTime = (millis()-LastGood5N1)/1000;
  debugstr += String(DTime,0);
  debugstr += " seconds ago, Signal quality = ";
  debugstr += String(Signal5N1,DEC);  
  debugstr += " out of 10\r\nTemperature = ";
  debugstr += String(Temperature5N1,1);
  debugstr += " C, Relative Humidity = ";
  debugstr += String(Humidity5N1,DEC);
  debugstr += "%\r\nDaily Rain = ";
  debugstr += String(CurrentRain * 25.4,2);
  debugstr += " mm, Current Rain Rate = ";
  debugstr += String(RainRate * 25.4,2);
  debugstr += " mm/hour, Rain Counter = ";
  debugstr += String(RainCounter5N1,HEX);
  debugstr += "\r\nWind = ";
  debugstr += String(CurrentWind,1);
  debugstr += " km/h, Direction = ";
  debugstr += acurite_5n1_winddirection_str[CurrentDirection];
  debugstr += ", Average = ";
  debugstr += String(WindSpeedAverage,1);
  debugstr += " km/h, Peak = ";
  debugstr += String(WindSpeedPeak,1);
  debugstr += " km/h at ";
  debugstr += acurite_5n1_winddirection_str[WindPeakDirection];
  if (BatteryLow5N1 == true) {
    debugstr += " Direction\r\nBattery Low\r\n\n";
  } else {
    debugstr += " Direction\r\n\n";
  }
  
  debugstr += "\r\n\nReading#, 5N1 Signal, Rain,   Wind Speed, Direction\r\n";
  
  for ( int i = 32; i > 0; i-- ) {                                      // signal, wind, and rain array
    debugstr += String(i,DEC);
    if (i < 10) {
      debugstr += " ";
    }
    if (Mess5N1[i] == true) {
      debugstr += "         Good       ";  
    } else {
      debugstr += "         Missed     ";
    }
    
//    if (i < 11) {                                                           // only have ten message flags for tower sensor
//      if (MessTower[i] == true) {
//        debugstr += "Good          ";  
//      } else {
//        debugstr += "Missed        ";
//      }
//    } else {
//      debugstr += "              ";  
//    }
    
    debugstr += String(Rain[i] * 25.4,2);
    debugstr += "   ";
    debugstr += String(WindSpeed[i],2);
    debugstr += "        ";
    debugstr += acurite_5n1_winddirection_str[WindDirection[i]];
    debugstr += "\r\n";
  }
  debugstr += "\r\n";

  server.send ( 200, "text/plain", debugstr );

}

/************************************************************************************************************************************************
 *                                                          RESET                                                                               *
 * Reset daily rainfall, assume it is midnight                                                                                                  *                                                         
 ************************************************************************************************************************************************
 */
  
void handleSummerTime() {
     server.send(200, "text/html", webPage);
     Serial.println("[ON]");
     if(NTP.getDayLight () == false) {
       NTP.setDayLight(true);
     }
}

void handleWinterTime() {
     server.send(200, "text/html", webPage);
     Serial.println("[OFF]");
     if(NTP.getDayLight () == true) {
       NTP.setDayLight(false);
     }
}

void handleReset() {

  String Wdata ="";  

  Wdata = " Today's total rain was ";
  Wdata += String(CurrentRain * 25.4,2);
  Wdata += " mm.";
  
  UpdateRainCounter();                                          // reset daily rain fall
  
  server.send ( 200, "text/plain", Wdata );                     // today's (yesterday's really) total rain fall

}


/************************************************************************************************************************************************
 *                                                    Requested page doesn't exist                                                              *
  ************************************************************************************************************************************************
 */

void handleNotFound() {

  server.send ( 404, "text/plain", "Page Not Found" );

}


