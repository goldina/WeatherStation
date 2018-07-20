/************************************************************************************************************************************************
 *                                                               Setup                                                                          *
 ************************************************************************************************************************************************                                                               
 */

void setup()
{
  Serial.begin(BaudRate);
  #ifdef DebugOut                                                        // stream data out serial for debug
  Serial.println();
  Serial.println("Acurite Receiver Started...");
  #endif

 // pinMode(LED_BUILTIN, OUTPUT);                                           // LED output
 // digitalWrite(LED_BUILTIN, HIGH);                                        // start with LED off
  
  bool status;
  Wire.begin(D1, D2);
  Wire.setClock(100000);
  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin();
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
  }

  tft.init();
  tft.setRotation(2); 
  
  // Fill screen with grey so we can see the effect of printing with and without 
  // a background colour defined
  tft.fillScreen(TFT_BLACK); 

  
  // Set "cursor" at top left corner of display (0,0) and select font 2
  // (cursor will move to next line automatically during printing with 'tft.println'
  //  or stay on the line is there is room for the text with tft.print)
  tft.setCursor(0, 0, 2);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  tft.setTextColor(TFT_YELLOW,TFT_BLACK);
  tft.setTextSize(2);
  // We can now plot text on screen using the "print" class
  tft.println("Starting!"); 

  #ifdef DebugOut                                                         // stream data out serial for debug
  Serial.println();                               
  Serial.print("Connecting to ");
  Serial.println(ssid);
  #endif

  EEPROM.begin(32);                                                       // Need two bytes in EEPROM 
  RainCounter5N1 = (EEPROM.read(0) << 8) | EEPROM.read(1);                // recover rain counter from EEPROM
  CurrentRainCounter5N1 = RainCounter5N1;
  #ifdef DebugOut
  Serial.print("Rain Counter loaded from EEPROM is ");
  Serial.println(RainCounter5N1,HEX);
  #endif

  RTC.begin();
  Now = RTC.now();
  setTime(Now.unixtime()); //get the current date-time from RTC
  today = day();
  #ifdef DebugOut
  Serial.println(NTP.getTimeDateString());
  #endif


  WiFi.mode ( WIFI_STA );
  WiFi.begin(ssid, password);                                             // connect to the WiFi
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);                                                           // This needed to give control back to the esp to run its own code!  
    #ifdef DebugOut                                                       // stream data out serial for debug
    Serial.print(".");
    #endif
  }

  #ifdef DebugOut                                                          
  Serial.println("");
  Serial.println("WiFi connected");
  #endif
  
  pinMode(DATAPIN, INPUT);                                                // 433MHz radio data input
  attachInterrupt(digitalPinToInterrupt(DATAPIN), handler, CHANGE);       // interrupt on radio data signal

  #ifdef DebugOut                                                           
  Serial.println(WiFi.localIP());
  #endif

  if (mdns.begin("esp8266", WiFi.localIP())) {
    #ifdef DebugOut                                                       // stream data out serial for debug
       Serial.println("MDNS responder started");
    #endif
  }

  NTP.begin ("192.168.30.12", timeZone, true, minutesTimeZone);
  NTP.setInterval (600);

  NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) {
     if (event == 0) {
        DateTime dt(now());
        RTC.adjust(dt);
  #ifdef DebugOut
  Serial.print("RTC was adjusted: ");
  Serial.println(NTP.getTimeDateString());
  #endif
     }
      
     });
  
  webPage += "<h1>Personal Weather Server</h1>";
  webPage += "<p>debug = current debug data</p>";
  webPage += "<p>RESET = midnight reset of rain fall</p>";
  webPage += "<p>Summer Time <a href=\"SummerTimeON\"><button>ON</button></a>&nbsp;<a href=\"SummerTimeOFF\"><button>OFF</button></a></p>";
  
  server.on ( "/", handleRoot );                                          // root webpage, just identify the device
  server.on ( "/debug",handleDebug );                                     // show info for possible debugging
  server.on ( "/RESET",handleReset );                                     // midnight signal to reset daily rain amount
  server.onNotFound ( handleNotFound );
  server.on ("/SummerTimeON",handleSummerTime );
  server.on ("/SummerTimeOFF",handleWinterTime );
  
  server.begin();

                                                                          // OTA stuff
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(HostName);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else                                                      // U_SPIFFS
      type = "filesystem";
                                                              // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    detachInterrupt(digitalPinToInterrupt(DATAPIN));
  });
  ArduinoOTA.onEnd([]() {
    detachInterrupt(digitalPinToInterrupt(DATAPIN));
    delay(1000);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  });
  ArduinoOTA.onError([](ota_error_t error) {
    delay(5000);
  });
  ArduinoOTA.begin();
}



