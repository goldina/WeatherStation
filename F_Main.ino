/************************************************************************************************************************************************
 *                                                             MAIN LOOP                                                                        *
 ************************************************************************************************************************************************                                                               
 */

/*
   Main Loop
   Wait for received to be true, meaning a sync stream plus
   all of the data bit edges have been found.
   Convert all of the pulse timings to bits and calculate
   the results.
*/
void loop() {
  
  ArduinoOTA.handle();                                        // for over the air updates
  
  server.handleClient();                                      // webserver

  if ( received == true )                                     // There's a messsage ready
  {

    unsigned int ringIndex;                                   // where in the bit buffer are we
    bool fail = false;                                        // flag to indicate good message or not
    byte dataBytes[bytesReceived];                            // Decoded message bytes go here  

    fail = false;                                             // reset bit decode error flag 

#ifdef DISPLAY_BIT_TIMING
    displayBitTiming();
#endif 

// ******************* Decode the message bit timings into bytes *******************************

    for ( int i = 0; i < bytesReceived; i++ )                 // clear the data bytes array
    {
      dataBytes[i] = 0;
    }

    ringIndex = (syncIndex + 1) % RING_BUFFER_SIZE;

    for ( int i = 0; i < bytesReceived * 8; i++ )             // Decode the timing into message bytes
    {
      int bit = convertTimingToBit( pulseDurations[ringIndex % RING_BUFFER_SIZE],
                                    pulseDurations[(ringIndex + 1) % RING_BUFFER_SIZE] );

      if ( bit < 0 )
      {
        fail = true;                                          // fails if any invalid bits
        break;                                                // exit loop
      }
      else
      {
        dataBytes[i / 8] |= bit << (7 - (i % 8));             // build the bytes
      }

      ringIndex += 2;
    }

#ifdef DebugOut
                                                            // Display the raw data received in hex
    if (fail)
    {
      Serial.println("Data Byte Display : Decoding error.");
    } else {
      for( int i = 0; i < bytesReceived; i++ )
      {
        Serial.print(dataBytes[i], HEX);
        Serial.print(",");
      }
    }
#endif

// *************** Validate the message via CRC and PARITY checks ******************************
    if (!fail) {
      fail = !acurite_parity(dataBytes, bytesReceived);      // check parity of each byte
    }

    if (!fail) {
      fail = !acurite_crc(dataBytes, bytesReceived);         // Check checksum of message
    }

// ********************* Determine which sensor it is from *************************************
    if (fail)  {

    } else if ((bytesReceived == 8)) {                
      decode_5n1(dataBytes);                          //5n1 sensor

    } else {
      #ifdef DebugOut
      Serial.println("Unknown or Corrupt Message"); // shouldn't be possible, only 7, 8, or 9 byte messages should get here.
      #endif      
    }
    received = false;                                                                
  }                                                                        // Done with message, if we got one
    
  else {

    if ((millis() - Next5N1) > 20000) {                                    // check to see if we missed a message, we have if over 20 seconds since the last one

      Add5N1(false, CurrentWind, CurrentDirection, CurrentRain);           // missed the next 18.5 second update message
      Next5N1 = millis() - 1500;                                           // go back 1.5 seconds in time, since we were 1.5 seconds overdue 
      #ifdef DebugOut
      Serial.println("Missed 5n1"); 
      #endif
    }
  }

  if (today != day()) {                         // time to reset rain fall?
#ifdef DebugOut
    Serial.println("Reseting Rain Counter");
#endif
    UpdateRainCounter();                                                  // reset daily rain fall
    today = day();
  }                                                   

  if((millis() - update_screen) > 1000)
  {
      tft.setCursor(0, 0, 2);
  // We can now plot text on screen using the "print" class
      tft.print("     ");
      tft.print(NTP.getTimeStr()); 
      tft.setCursor(0, 30, 2);
      tft.print("   ");
      tft.print(NTP.getDateStr()); 
      tft.setCursor(0, 80, 2);
      tft.print("P: ");
      tft.print(bme.seaLevelForAltitude(204, bme.readPressure() / 100.0F),1); 
      tft.print(" hPa");
      tft.setCursor(0, 110, 2);
      tft.print("W: ");
      tft.print(CurrentWind,1); 
      tft.print(" km/h   ");
      tft.setCursor(0, 140, 2);
      tft.print("R: ");
      tft.print(RainRate * 25.4,0); 
      tft.print(" mm/h");
      tft.setCursor(0, 170, 2);
      tft.print("DR: ");
      tft.print(CurrentRain * 25.4,2); 
      tft.print(" mm");
      tft.setCursor(0, 210, 2);
      tft.print("     Out     In");
      tft.setCursor(0, 240, 2);
      tft.print("T:   ");
      if (Temperature5N1 < -100) {
        tft.print("  ");
      }
      else {
        if(Temperature5N1 < 10){
          tft.print(" ");
        }
        tft.print(Temperature5N1, 0);
      }
      tft.print("      ");
      tft.print(bme.readTemperature(), 0);
      tft.print("  ");
       
      tft.setCursor(0, 270, 2);
      tft.print("H:   ");
      if (Humidity5N1 == 0) {
        tft.print("  ");
      }
      else {
        if(Humidity5N1 < 10){
            tft.print(" ");
        }
        tft.print(Humidity5N1, 1);
      }
      tft.print("      ");
      tft.print(bme.readHumidity(), 0);
      tft.print("  ");
      
      update_screen = millis();
  }

//  if ((millis() > WUtime) && ((millis() - Next5N1) < 14000) && ((millis() - NextTower) < 12000) && (WUsend)) { 
//                                                                          
//    CurrentPressure = bme.readPressure() * 0.000295333727;                // get the air pressure
//    WUupdate = UpdateWeatherUnderground();                                // send data to Weather Underground
//  }
  
//  if ((millis()-displaylast) > 1000) { UpdateDisplay(); }                 // update display once a second

}


