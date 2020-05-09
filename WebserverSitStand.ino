/*

   make more room on page - put the sitting/standing and present/absent & refresh buttons side by side
   DONE make new CSS objects that could form a graph bar and place on weekly page -not workable soultion though
   DONE make html objects that can be manipulated in size
   make working time / sitting time in a defined box
   is .genericrectangle used?
   DONE make a function to display rectangles as a graph from an array of data
     The big string of &nbsp is ugly, and doesnt render well on mobiles. fix this.
     
   DONE add ultrasonic and hall effect sensors to detect person present and desk postion
   WONT DO install run/load switch and reset button, to allow in situ programming via diagnostic port.
   DONE remove sit button, make current state bigger on web page

   DONE add last few distance readings to web page to aid in disgnosing why it misbehaves occasinally

   DONE ? runs of zeros received from distance sensors from time to time - why ? stops the whole void loop...

   optimise range sensor and add delays to make detections stable
     DONE add timers or smoothing around person present detection and map to personPresent
     Arrange for different thresholds in sitting or standing mode ?

     DONE try looking for two or more consectutive detections of present or absent before changing presence modes
         this works really well. An improvement might be to look for x valid reradings in y samples, rather than sequential

     can the min distanace for the back of a pushed back chair be sorted? might not be worth it...
   DONE Fix total time worked calcs - or log present time instead of summing sitting & standing
   Check acuracy of the timer for accumulating times. might be fast...

   DONE optimise hall effect sensor and mount magnet to desk.
   DONE intergate sensors state to web page

   Non functional changes
     tidy indents up. What would Adrian and Dylan say?
     add rest of tests around Debugmessages conditions.
     document what the debug message format is
       ~ = measurement
       > counting up or terminal count of personpresent, < counting down
     remove unnessasry variables
     remove unused code blocks
     make the 3 time counting routines a function
     standardise Camel casing on variables
     restructure both CSS and HTML so the tags are more clearly indented

   DONE Make total worked time correct - minutes and seconds can sum to > 60. Probably easier to count period person is present
   DONE put Desk up / down counts on same line of web page

     figure out how to detect a working day
     add functinality that assumes a day starts after approaching the desk after a ~5 hr break, and if I am there for > 15mins, start accumalating daily stats
     similarly, when absent for > ~5 hrs, thats the day end
     or, I could get time from an internet timeserver and assume a 0700 daily start (mon-fri)
     PARTIALLY DONE could i display contents of the working/sittiong time array as a bar graph? Hourly on daily page, daily on weekly page

     figure out how to detect a working week, add weekly stats on its own page
     have Archive stats on its own page

   arrange for eeprom write of all data (daily/weekly and Archive cumulative standing and sitting hours) every hour -
     put time since last eeprom write write on web page
     arrange to read in eeprom stored values on restart
     eeprom write of cumulative standing and sitting hours as end of working day is reached
     24 hrs * 7 days * 52 weeks is 8736 writes per year
     100000 (min read/write cycles b4 failure) / 8736 is 11.4 years so;
     figure out how to surpress eprom writes if no data changed.



   PARITALLY DONE add up/down desk cycle counter
   - add to Archive and daily/weekly totals on web page



   PARITALLY DONE add up/down desk cycle counter
   - add Archive and daily/weekly totals on web page

   BUGS
     It wont boot in desk sitting mode.
     the back of a chair is detected as a person in sitting mode.
     sometimes get a 0 distance back from the sensor

   OPTIONS
   DONE ignore values from fluffy objects (only use real reflections) and require X readings sequentailly before changing person present states
   CONSIDER are the echo values the same night and day?

*/

// Load Wi-Fi library
#include <ESP8266WiFi.h>
const char* ssid = "A_Virtual_Information";
const char* password = "BananaRock";
// Set web server port number to 80
WiFiServer server(80);
// Variable to store the HTTP request
String header;
// Auxiliary variables to store the current output state

int PageMode = 0; // 0 = Daily 1 = ProxSensor, 2 = Archive

String SitStandState = "Sitting";
unsigned long StandCounter = 0;
unsigned long SitCounter = 0;

// Assign output variables to GPIO pins

const int GPIO2 = 2;  // GPIO2 pin - on board LED of some of the ESP-01 variants.
const int HallEffectPin = 2;  //


unsigned long currentTime = millis();

unsigned long previousTime = 0;
unsigned long previousMillis = 0;

byte standingseconds ;
byte standingminutes ;
byte standinghours ;
long standingtotalseconds;
unsigned long dailystandingpercentage;

byte sittingseconds ;
byte sittingminutes ;
byte sittinghours ;
long sittingtotalseconds;
unsigned long dailysittingpercentage;

byte workingseconds ;
byte workingminutes ;
byte workinghours ;
long workingtotalseconds;


byte currentseconds = 0 ;
byte currentminutes = 55;
byte currenthours = 12 ;

byte RectHeight = 110;
byte RectWidth = 10;
String RectColor = "Black";
byte percentagegraphscalingfactor = 1;
String SittingColor = "Blue";
String StandingColor = "LightBlue";
String WorkingColor = "DarkBlue";

// Define web page timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

int PersonPresentSmoothingCount = 0;
byte distancereportcounter = 0;
byte PersonPresentSmoothingCountThreshold = 60;// It takes this many valid distance readings to infer a person has left the workdesk
byte WallDetectedStateCount = 0;
byte PersonDetectedStateCount = 0;
byte WallDetectedStateCountThreshold = 3;
byte PersonDetectedStateCountThreshold = 1;

boolean connectWifi();
bool PersonDetectedState = false;  // True or HIGH when someone is detected
bool PersonPresentState = false;  // True or HIGH when someone is present - this is a buffered version of PersonDetectedState
bool PrevPersonDetectedState = false;
bool HallEffectState = false;
bool PrevHallEffectState = false;
bool DeskPostitionState = false; // UP is logic HIGH
bool PrevPersonPresentState = false;
bool InvalidDistanceReading = false;
bool WallDetectedState = false;

unsigned long UpDownCounter = 0;
int WeeklyUpDownCounter = 0;

int DailyUpCounter = 0;
int DailyDownCounter = 0;
//const int PersonDetectedlowerthreshold = 10; //cm.  Readings under this value are discarded as invalid

int LastReading ;
int PenultimateReading ;
int SubPenultimateReading ;

/* These arent use - they are in the select case statement...
  const int PersonDetectedmidthreshold = 20; //cm.  Readings over this value indicate someone is present
  // values in this range mean someone is there
  const int PersonDetectedupperthreshold = 90; //cm. Readings over this value mean no one is there
  // bed is 100 away in sitting mode
  //window is 260 away in standing mode.
  const int PersonDetectedthresholdfurrymode = 270; // cm. sometimes values in the range 500 - 2500 are received which are always from fluffy materials
*/
const int MaxValidDistance = 275;

boolean wifiConnected = false;
bool DebugMessages = true;

long duration, distance; // Duration used to calculate distance

const int echoPin = 3;  // 3 is the RX pin
const int trigPin = 1; // 1 is the TX Pin

int DailySitStandArray[50] ; // 24 hrs, plus sit/stand for each index [0] is the sitting minutes for the first hour (00-01) two unused slots...
byte DailySitStandArrayIndex = 0;
byte CurrentSittingMinutes = 0;
byte CurrentStandingMinutes = 0;

/*  WIRING DETAILS
    CPU TXD connects to the ultrasonic Trigger pin
    CPU RXD connects to the ultrasonic Echo pin
    Hall effect sensor connected to GPIO2

*/

void setup()
{


  pinMode(GPIO2, OUTPUT); // it changes to an input later...

  Serial.begin(9600);


  Serial.println("void setup starting ");
  Serial.println("Booting...");
  delay(2000);
  Serial.println("flashing LED on GPIO2...");
  //flash fast a few times to indicate CPU is booting
  digitalWrite(GPIO2, LOW);
  delay(100);
  digitalWrite(GPIO2, HIGH);
  delay(100);
  digitalWrite(GPIO2, LOW);
  delay(100);
  digitalWrite(GPIO2, HIGH);
  delay(100);
  digitalWrite(GPIO2, LOW);
  delay(100);
  digitalWrite(GPIO2, HIGH);

  Serial.println("Delaying a bit...");
  delay(2000);

  // Initialise wifi connection
  wifiConnected = connectWifi();

  if (wifiConnected)
  {
    Serial.println("flashing slow to indicate wifi connected...");
    //flash slow a few times to indicate wifi connected OK
    digitalWrite(GPIO2, LOW);
    delay(1000);
    digitalWrite(GPIO2, HIGH);
    delay(1000);
    digitalWrite(GPIO2, LOW);
    delay(1000);
    digitalWrite(GPIO2, HIGH);
    delay(1000);
    digitalWrite(GPIO2, LOW);
    delay(1000);
    digitalWrite(GPIO2, HIGH);

    // Print local IP address and start web server
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();

    Serial.println("End of - Void Setup & if (wifiConnected)");

  }

  Serial.println("Making RX pin into an Digital INPUT"); // used to receive ultrasonic pulse legnth to infer if someone is present
  pinMode(echoPin, FUNCTION_3);
  pinMode(echoPin, INPUT); // The ultrasonic detector uses timing pulse length to indicate distance, not serial.

  Serial.println("Making GPIO pin 2 into an Digital INPUT"); // used to detect Hall Effect sensor state for desk up/down
  pinMode(HallEffectPin, FUNCTION_3);
  pinMode(HallEffectPin, INPUT); //

  DailySitStandArray[0] = 10; // 00 hr sitting
  DailySitStandArray[1] = 20; // 00 hr Standing
  DailySitStandArray[2] = 50;
  DailySitStandArray[3] = 2;
  DailySitStandArray[12] = 30; // 6rd hr
  DailySitStandArray[13] = 59;
  DailySitStandArray[24] = 30; // 12th hr
  DailySitStandArray[25] = 59;
  DailySitStandArray[36] = 30; // 18th hr
  DailySitStandArray[37] = 59;
  DailySitStandArray[47] = 30; // 24th hr
  DailySitStandArray[48] = 59;

}

void loop()
{


  //Serial.println("void loop starting ");

  HallEffectState = digitalRead(HallEffectPin);
  delay(100);
  if (HallEffectState == LOW) {
    if (PrevHallEffectState == HIGH) {
      Serial.println(" Desk is DOWN (Logic LOW)");
      DailyDownCounter = DailyDownCounter + 1;
    }
  }

  if (HallEffectState == HIGH) {
    if (PrevHallEffectState == LOW) {
      Serial.println("Desk is UP (Logic HIGH)");
      DailyUpCounter = DailyUpCounter + 1;
    }
  }

  PrevHallEffectState = HallEffectState; // edge detection of Hall Effect state

  DeskPostitionState = HallEffectState; // allowing some timing/buffereing/contact bounce to happen here if needed...

  if (DeskPostitionState == HIGH) { // Desk is UP

    SitStandState = "Standing";
  }
  else { // Desk is DOWN
    SitStandState = "Sitting";
  }


  measuredistance();
  logdistance();

  SubPenultimateReading = PenultimateReading;
  PenultimateReading = LastReading;
  LastReading = distance;

  /*
    this code block counts up or down every second using PersonDetectedState to/from PersonPresentSmoothingCountThreshold to set or reset PersonPresentState

    if (distance < MaxValidDistance ) { // if its greater than the distance to the window, throw that reading away as invalid



    switch (distance) { // only here is distance is < MaxValidDistance

    case 0 : // Invalid responce, might need to reboot if this persists?

    //PersonDetectedState = false;
    break;
    case 1 ... 9: // Invalid responce, measure again
    //PersonDetectedState = false;
    break;
    case 10 ... 22: // The keyboard shelf
    //PersonDetectedState = false;
    break;
    case 23 ... 95: // The active zone for working at the desk.
    // someone sitting on a chair is 36 - 60
    // Someone is there
    PersonDetectedState = true;
    break;
    case 96 ... 270: // background things on the bed, or the window behind the desk
    // the bed is 102 away in sitting mode
    // the back of an unoccupied chair is about 80 in sitting mode. You also sometimes get values like 2256 for this
    // the window is 261 away in standing mode
    //no-one is there
    PersonDetectedState = false;
    break;
    case 271 ... 499: // the lowish random numbers from fluffy objects . These dont happen often
    //no-one is there
    //PersonDetectedState = false;
    break;
    case 500 ... 3000: // the medium - hi random numbers from fluffy objects. Sometimes this is a person wearing clothes at the desk
    // this is the trouble some zone
    //PersonDetectedState = true;

    break;
    }



    if (PersonDetectedState == true) {
    PersonPresentSmoothingCount = PersonPresentSmoothingCountThreshold ; // this will bias the system to assume person present
    Serial.print(">");
    // if (PersonPresentSmoothingCount > PersonPresentSmoothingCountThreshold) {

    // PersonPresentSmoothingCount = PersonPresentSmoothingCountThreshold;
    PersonPresentState = true;
    //}
    }

    if (PersonDetectedState == false) {

    PersonPresentSmoothingCount = PersonPresentSmoothingCount -1;
    Serial.print("<");
    if (PersonPresentSmoothingCount < 0) {
    PersonPresentSmoothingCount = 0;
    PersonPresentState = false;
    }
    }








    } //if (distance < MaxValidDistance )
  */

  /*
      This code block looks for consectutive runs of values of Distance before setting/resetting PersonPresentState
  */


  switch (distance) {

    case 0 : //
      InvalidDistanceReading = true;
      WallDetectedStateCount = 0;
      PersonDetectedStateCount = 0;
      WallDetectedState = false;
      PersonDetectedState = false;
      break;
    case 1 ... 22: // // The keyboard shelf
      InvalidDistanceReading = true;
      WallDetectedStateCount = 0;
      PersonDetectedStateCount = 0;
      WallDetectedState = false;
      PersonDetectedState = false;
      break;
    case 23 ... 95: // The active zone for working at the desk.
      // someone sitting on a chair is 36 - 60
      // Someone is there
      PersonDetectedStateCount = PersonDetectedStateCount + 1;
      Serial.print(">");
      WallDetectedStateCount = 0;
      InvalidDistanceReading = false;
      break;
    case 96 ... 270: // background things on the bed, or the window behind the desk
      // the bed is 102 away in sitting mode
      // the back of an unoccupied chair is about 80 in sitting mode. You also sometimes get values like 2256 for this
      // the window is 261 away in standing mode
      //no-one is there
      WallDetectedStateCount = WallDetectedStateCount + 1;
      Serial.print("<");
      PersonDetectedStateCount = 0;
      InvalidDistanceReading = false;
      break;
    case 271 ... 3000: // Sometimes this is a person wearing clothes at the desk
      // this is the trouble some zone
      InvalidDistanceReading = true;
      WallDetectedStateCount = 0;
      PersonDetectedStateCount = 0;
      WallDetectedState = false;
      PersonDetectedState = false;

      break;
  }

  /* here once per pass
      Only one of these states is true InvalidDistanceReading, WallDetectedState or PersonDetectedState
  */

  if (WallDetectedStateCount >= WallDetectedStateCountThreshold) {
    WallDetectedStateCount = WallDetectedStateCountThreshold;
    PersonPresentState = false;
  }

  if (PersonDetectedStateCount >= PersonDetectedStateCountThreshold) {
    PersonDetectedStateCount = PersonDetectedStateCountThreshold;
    PersonPresentState = true;
  }


  delay(100);
  if (PersonPresentState == true) {
    if (PrevPersonPresentState == false) {
      Serial.println("Person has just arrived");
    }
  }

  if (PersonPresentState == false) {
    if (PrevPersonPresentState == true) {
      Serial.println("Person has just left");
    }
  }

  PrevPersonPresentState = PersonPresentState; // edge detection of Person Present state

  WiFiClient client = server.available();   // Listen for incoming clients
  if (client)
  { // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();


            if (header.indexOf("GET /dumptimer") >= 0) {
              Serial.println("Dump Stats Request");
              standingseconds = 0;
              standingminutes = 0;
              standinghours = 0;
              standingtotalseconds = 0;

              sittingseconds = 0;
              sittingminutes = 0;
              sittinghours = 0;
              sittingtotalseconds = 0;

              workingseconds = 0;
              workingminutes = 0;
              workinghours = 0;
              workingtotalseconds = 0;

              DailyUpCounter = 0;
              DailyDownCounter = 0;

              for (DailySitStandArrayIndex = 0; DailySitStandArrayIndex < 49; DailySitStandArrayIndex = DailySitStandArrayIndex + 1) {

                DailySitStandArray[DailySitStandArrayIndex] = 0;
              }

            }
            if (header.indexOf("GET /IncHrs") >= 0) {
              Serial.println("Inc Hrs request");
              currenthours = currenthours + 1;
            }
            if (header.indexOf("GET /DecHrs") >= 0) {
              Serial.println("Dec Hrs request");
              currenthours = currenthours - 1;
            }
            if (header.indexOf("GET /IncMins") >= 0) {
              Serial.println("Inc Mins request");
              currentminutes = currentminutes + 1;
            }
            if (header.indexOf("GET /DecMins") >= 0) {
              Serial.println("Dec Mins request");
              currentminutes = currentminutes - 1;
            }

            if (header.indexOf("GET /Daily") >= 0) {
              PageMode = 0;
            }
            if (header.indexOf("GET /ProxSensor") >= 0) {
              PageMode = 1;
            }
            if (header.indexOf("GET /Archive") >= 0) {
              PageMode = 2;
            }
            if (header.indexOf("GET /Setup") >= 0) {
              PageMode = 3;
            }

            //Display the comman parts of the web page

            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            //client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("<style>html { font-family: Helvetica; display: inline; margin: 0; margin-top:0px; padding: 0; text-align: center;}");

            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");

            //client.println(".redrectangle { background-color: red; border: none; color: black; padding: 40px 10px;}");
            //client.println(".purplerectangle { background-color: purple; border: none; color: black; padding: 100px 10px;}");
            client.println(".genericrectangle { background-color: brown; border: none; color: white; padding: " + String(RectHeight) + "px " + String(RectWidth) + "px;}");
            //client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");


            client.println(".button2 {background-color: #77878A;}");
            client.println(".buttonStanding {background-color:" + String(StandingColor) + ";}");
            client.println(".buttonSitting {background-color:" + String(SittingColor) + ";}</style></head>");

            //client.println(".buttonsmall { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            //client.println("text-decoration: none; font-size: 10px; margin: 2px; cursor: pointer;}");

            // Web Page Heading
            client.println("<body><h1>Sit / Stand Timer</h1>");

            // Display current state, and ON/OFF SIT/STAND buttons
            //client.println("<p>Current State " + SitStandState + "</p>");

            if (SitStandState == "Standing") {
              client.println("<p><a href=\"/2/on\"><button class=\"button buttonStanding\">STANDING</button></a> </p>");
            } else {
              client.println("<p><a href=\"/2/off\"><button class=\"button buttonSitting\">SITTING</button></a></p>");
            }

            if (PersonPresentState == false) {
              client.println("<p><a href=\"/2/on\"><button class=\"button button2\">ABSENT</button></a><a href=\"/refresh\"><button class=\"button\">REFRESH</button></a></p>");
            } else {
              client.println("<p><a href=\"/2/off\"><button class=\"button \">PRESENT</button></a><a href=\"/refresh\"><button class=\"button\">REFRESH</button></a></p>");
            }
            // Display last 3 distances recorded
            //client.println("<p>Distances "  + String(SubPenultimateReading) + "cm  -  " + String(PenultimateReading)+ "cm  -  " + String(LastReading) + "cm" + " </p>");

            //Display Refresh Button
            //client.println("<p><a href=\"/refresh\"><button class=\"button\">REFRESH</button></a></p>");



            switch (PageMode) { // { 0 =  Daily, 1 = ProxSensor, 2 = Archive , 3 = Setup

              case 0: // "Daily" :
                // Display the daily stats default HTML web page

                // Display current time of day
                client.println("<p>Current Time is " + String(currenthours) + ":" + String(currentminutes) + ":" + String(currentseconds) + ":" + "</p>");


                // Display Sitting Time
                //client.println("<p>Sitting Time Today " + String(sittinghours) +" hrs " + String(sittingminutes) +" mins "+ String(sittingseconds) +" secs " + String(dailysittingpercentage)+ " %" + "</p>");
                client.println("<p <span style=color:white;background-color:" + (SittingColor) + ">Sitting Time Today " + String(sittinghours) + " hrs " + String(sittingminutes) + " mins " + String(sittingseconds) + " secs " + String(dailysittingpercentage) + " %" + "</p>");


                // Display Standing Time
                //client.println("<p>Standing Time Today " + String(standinghours) +" hrs " + String(standingminutes) +" mins "+ String(standingseconds) +" secs " + String(dailystandingpercentage) + " %" + "</p>");
                //client.println("<p> style=color:white; background-color:" + (StandingColor) + ";" + "Standing Time Today " + String(standinghours) +" hrs " + String(standingminutes) +" mins "+ String(standingseconds) +" secs " + String(dailystandingpercentage) + " %" + "</p>");
                //client.println("<p style=\"color:blue\">My Name is: <span style=\"color:red\">Tintinecute</span> </p>");
                //client.println("<p <span style=\"color:blue\">Standing Time Today</span> </p>");
                //client.println("<p <span style=\"color:blue\">Standing Time Today " + String(standinghours) +" hrs " + String(standingminutes) +" mins "+ String(standingseconds) +" secs " + String(dailystandingpercentage) + " %" + "</p>");
                client.println("<p <span style=color:white;background-color:" + (StandingColor) + ">Standing Time Today " + String(standinghours) + " hrs " + String(standingminutes) + " mins " + String(standingseconds) + " secs " + String(dailystandingpercentage) + " %" + "</p>");
                //try adding this to limit the stripe across the screen? .boxed {  border: 1px solid green

                // Show the current day split percentages
                RectWidth = 50;
                RectColor = SittingColor;
                RectHeight = (dailysittingpercentage * percentagegraphscalingfactor);
                client.print("<div id=\"genericrectangle\" style=\"display: inline-block; width:" + String(RectWidth) + "px; height:" + String(RectHeight) + "px; background-color:" + (RectColor) + "\"></div>");

                RectColor = StandingColor;
                RectHeight = (dailystandingpercentage * percentagegraphscalingfactor);
                client.print("<div id=\"genericrectangle\" style=\"display: inline-block; width:" + String(RectWidth) + "px; height:" + String(RectHeight) + "px; background-color:" + (RectColor) + "\"></div>"); // + > + String(dailystandingpercentage) +
                client.println("<p> % Split Today </p>");

                // show current Hours sit/stand minutes
                client.println("<p <span style=color:white;background-color:" + (StandingColor) + ">Last Hours Standing Time " + String(CurrentSittingMinutes) + " mins " + "</p>");
                client.println("<p <span style=color:white;background-color:" + (SittingColor) + ">Last Hours Sitting Time " + String(CurrentStandingMinutes) + " mins " + "</p>");
                //<span style=\"color:red\">Tintinecute</span>

                //client.print("<div id=\"genericrectangle\" style=\"display: inline-block; width:"+ String(RectWidth) +"px; height:"+ String(RectHeight) +"px; background-color:" + (RectColor)+"\">00%</div>"); // + > + String(dailystandingpercentage) +
                //client.print("<div id=\"genericrectangle\" style=\"display: inline-block; width:"+ String(RectWidth) +"px; height:"+ String(RectHeight) +"px; background-color:" + (RectColor)+"\"> + String(dailystandingpercentage)</div>"); // + > + String(dailystandingpercentage) +


                //RectColor = SittingColor;
                //RectHeight = DailySitStandArray[23];
                //client.print("<div id=\"genericrectangle\" style=\"display: inline-block; width:"+ String(RectWidth) +"px; height:"+ String(RectHeight) +"px; background-color:" + (RectColor)+"\"></div>");

                //RectColor = StandingColor;
                //RectHeight = DailySitStandArray[24];
                //client.print("<div id=\"genericrectangle\" style=\"display: inline-block; width:"+ String(RectWidth) +"px; height:"+ String(RectHeight) +"px; background-color:" + (RectColor)+"\"></div>"); // + > + String(dailystandingpercentage) +

                // Graph the SitStand minutes for the current day
                for (DailySitStandArrayIndex = 0; DailySitStandArrayIndex < 49; DailySitStandArrayIndex = DailySitStandArrayIndex + 1) {
                  RectWidth = 12;
                  if ((DailySitStandArrayIndex % 2) == 0) { //if its even...
                    // here if even
                    RectColor = SittingColor;
                  } else {
                    // here if odd
                    RectColor = StandingColor;
                  }
                  RectHeight = DailySitStandArray[DailySitStandArrayIndex];
                  client.print("<div id=\"genericrectangle\" style=\"display: inline-block; width:" + String(RectWidth) + "px; height:" + String(RectHeight) + "px; background-color:" + (RectColor) + "\"></div>");

                }
                // need to figure out element spacing client.println("<p>align="justify"00hrs           06hrs              12hrs             18hrs              24hrs</p>");
                client.println("<p align=\"justify/left/right/center\">00:00&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;06:00&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;12:00&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;18:00&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;24:00</p>");
                //client.println("<p>......Timeline.....</p>");

                // Display Working Time
                client.println("<p>Working Time Today " + String(workinghours) + " hrs " + String(workingminutes) + " mins " + String(workingseconds) + " secs " + "</p>");
                // Display Up/Down Counters
                client.println("<p>Desk Up counter " + String(DailyUpCounter) + "  -  " + "Desk Down counter " + String(DailyDownCounter) + "  </p>");



                //Display Dump Stats & ProxSensor  & Archive Buttons
                client.println("<p><a href=\"/dumptimer\"><button class=\"buttonsmall\">Dump Stats</button></a> <a href=\"/ProxSensor\"><button class=\"buttonsmall\">ProxSensor</button></a> <a href=\"/Archive\"><button class=\"buttonsmall\">Archive</button></a> <a href=\"/Setup\"><button class=\"buttonsmall\">Setup</button></a></p>");

                break;
              case 1: // "ProxSensor" :
                // Display last 3 distances recorded
                client.println("<p>Distances "  + String(SubPenultimateReading) + "cm  -  " + String(PenultimateReading) + "cm  -  " + String(LastReading) + "cm" + " </p>");

                //client.println("<p>ProxSensor data will appear here </p>");
                //client.println("<div id=\"rectangle\"></div>"); // doesnt work, references rectanngle in CSS section so prob not a starter anyway

                //client.println("<p><a href=\"/test1\"><button class=\"redrectangle\">?</button></a><a href=\"/test1\"><button class=\"purplerectangle\">?</button></a><button class=\"genericrectangle\">?</button></a></p>");

                //client.print("<div id=\"genericrectangle\" style=\"width:10px; height:50px; background-color:blue\"></div>");
                //body { margin: 0; padding: 0;}
                //client.println("<p>Distances "  + String(SubPenultimateReading) + "cm  -  " + String(PenultimateReading)+ "cm  -  " + String(LastReading) + "cm" + " </p>");
                RectWidth = 20;
                if (SubPenultimateReading >= 200) {
                  RectHeight = 200;
                } else {
                  RectHeight = SubPenultimateReading;
                }
                RectColor = "Green";
                //RectHeight = 20;
                //PrintRectangle(RectWidth,RectHeight,RectColor);

                client.print("<div id=\"genericrectangle\" style=\"display: inline-block; width:" + String(RectWidth) + "px; height:" + String(RectHeight) + "px; background-color:" + (RectColor) + "\"></div>");

                if (PenultimateReading >= 200) {
                  RectHeight = 200;
                } else {
                  RectHeight = PenultimateReading;
                }
                RectColor = "Red";
                //RectHeight = 30;
                client.print("<div id=\"genericrectangle\" style=\"display: inline-block; width:" + String(RectWidth) + "px; height:" + String(RectHeight) + "px; background-color:" + (RectColor) + "\"></div>");

                if (LastReading >= 200) {
                  RectHeight = 200;
                } else {
                  RectHeight = LastReading;
                }
                RectColor = "Orange";
                //RectHeight = 40;
                client.print("<div id=\"genericrectangle\" style=\"display: inline-block; width:" + String(RectWidth) + "px; height:" + String(RectHeight) + "px; background-color:" + (RectColor) + "\"></div>");

                //Display Daily, ProxSensor & Archive Buttons
                client.println("<p><a href=\"/Daily\"><button class=\"buttonsmall\">Daily</button></a> <a href=\"/ProxSensor\"><button class=\"buttonsmall\">ProxSensor</button></a> <a href=\"/Archive\"><button class=\"buttonsmall\">Archive</button></a> <a href=\"/Setup\"> <button class=\"buttonsmall\">Setup</button></a></p>");

                break;
              case 2: // "Archive":
                client.println("<p>Archive data will appear here </p>");
                //Display Daily, ProxSensor , Archive, Setup Buttons
                client.println("<p><a href=\"/Daily\"><button class=\"buttonsmall\">Daily</button></a> <a href=\"/ProxSensor\"><button class=\"buttonsmall\">ProxSensor</button></a> <a href=\"/Archive\"><button class=\"buttonsmall\">Archive</button></a> <a href=\"/Setup\"><button class=\"buttonsmall\">Setup</button></a></p>");

                break;

              case 3: // "Setup":

                // Display current time of day
                client.println("<p>Current Time is " + String(currenthours) + ":" + String(currentminutes) + ":" + String(currentseconds) + ":" + "</p>");


                //Display Clock set controls
                client.println("<p><a href=\"/IncHrs\"><button class=\"buttonsmall\">Inc Hrs</button></a></p>");
                client.print("<p><a href=\"/DecHrs\"><button class=\"buttonsmall\">Dec Hrs</button></a></p>");
                client.print("<p><a href=\"/IncMins\"><button class=\"buttonsmall\">Inc Mins</button></a></p>");
                client.print("<p><a href=\"/DecMins\"><button class=\"buttonsmall\">Dec Mins</button></a></p>");
                client.println();
                // trying to get buttons side by side client.println("<p><a href=\"/IncHrs\"><button class=\"buttonsmall\">Inc Hrs</button></a></p>" + "<p><a href=\"/DecHrs\"><button class=\"buttonsmall\">Dec Hrs</button></a></p>");

                client.println();

                //Display Daily, ProxSensor , Archive, Setup Buttons
                client.println("<p><a href=\"/Daily\"><button class=\"buttonsmall\">Daily</button></a> <a href=\"/ProxSensor\"><button class=\"buttonsmall\">ProxSensor</button></a> <a href=\"/Archive\"><button class=\"buttonsmall\">Archive</button></a> <a href=\"/Setup\"><button class=\"buttonsmall\">Setup</button></a></p>");


                break;

            }

            client.println("</body></html>");
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

  // Count Standing / Sitting Time
  if (millis() >= (previousMillis))
  {
    previousMillis = previousMillis + 1000;  // use 100000 for uS

    if (PersonPresentState == true) {
      //counttime(workinghours,workingminutes,workingseconds); This function needs to return multiple values, doesnt work yet..
      workingseconds = workingseconds + 1;
      workingtotalseconds = workingtotalseconds + 1;
      if (workingseconds == 60)
      {
        workingseconds = 0;
        workingminutes = workingminutes + 1;
      }
      if (workingminutes == 60)
      {
        workingminutes = 0;
        workinghours = workinghours + 1;
      }
      if (SitStandState == "Standing") {
        //Serial.print ("Accumulating Standing Time");
        standingseconds = standingseconds + 1;
        standingtotalseconds = standingtotalseconds + 1;
        if (standingseconds == 60)
        {
          standingseconds = 0;
          standingminutes = standingminutes + 1;
          CurrentStandingMinutes = CurrentStandingMinutes + 1;
        }
        if (standingminutes == 60)
        {
          standingminutes = 0;
          standinghours = standinghours + 1;
        }
        /*
          if (standinghours == 24)
          {
          standinghours = 0;
          }
          if (standinghours < 10)
          {
          //Serial.print("0");
          }
          //Serial.print (standinghours, DEC);
          //Serial.print ("h");
          if (standingminutes < 10)
          {
          //Serial.print("0");
          }
          //Serial.print (standingminutes,DEC);
          //Serial.print ("m");
          if (standingseconds < 10)
          {
          //Serial.print("0");
          }
          //Serial.print(standingseconds,DEC);
          //Serial.println("s");
        */
      } else {
        //Serial.print ("Accumulating Sitting Time");
        sittingseconds = sittingseconds + 1;
        sittingtotalseconds = sittingtotalseconds + 1;
        if (sittingseconds == 60)
        {
          sittingseconds = 0;
          sittingminutes = sittingminutes + 1;
          CurrentSittingMinutes = CurrentSittingMinutes + 1;
        }
        if (sittingminutes == 60)
        {
          sittingminutes = 0;
          sittinghours = sittinghours + 1;
        }
        /*
          if (sittinghours == 24)
          {
          sittinghours = 0;
          }
          if (sittinghours < 10)
          {
          //Serial.print("0");
          }
          //Serial.print (sittinghours, DEC);
          //Serial.print ("h");
          if (sittingminutes < 10)
          {
          // Serial.print("0");
          }
          //Serial.print (sittingminutes,DEC);
          //Serial.print ("m");
          if (sittingseconds < 10)
          {
          // Serial.print("0");
          }
          //Serial.print(sittingseconds,DEC);
          //Serial.println("s");
        */
      }




      //calculate percentages - this calc will cause  run time crashes without the if's in place. Divide by zero...
      // val1 = (val2 / 255.0) * 100; // does this get around using a byte to hold the ansqwer?
      if (workingtotalseconds > 0) {
        dailystandingpercentage = round((standingtotalseconds * 100) / workingtotalseconds);
      }
      if (workingtotalseconds > 0) {
        dailysittingpercentage = round((sittingtotalseconds * 100) / workingtotalseconds);
      }


    }   // is PersonPresentState
    // Maintain RTC and do archive functions

    currentseconds = currentseconds + 1;
    if (currentseconds == 60)
    {
      currentseconds = 0;
      currentminutes = currentminutes + 1;
    }
    if (currentminutes == 60)
    {
      // Put current hours worth of standing and sittting minutes into the array

      DailySitStandArrayIndex = (currenthours * 2); // work out the array index - 2 values stored per hour
      DailySitStandArray[DailySitStandArrayIndex] = CurrentSittingMinutes;
      Serial.print("Writing Sitting mins to Array ");
      Serial.println(DailySitStandArrayIndex);
      Serial.println(CurrentSittingMinutes);
      DailySitStandArray[(DailySitStandArrayIndex + 1)] = CurrentStandingMinutes;
      Serial.print("Writing Standing mins to Array ");
      Serial.println(DailySitStandArrayIndex + 1);
      Serial.println(CurrentStandingMinutes);
      CurrentStandingMinutes = 0;
      CurrentSittingMinutes = 0;
      currentminutes = 0;
      currenthours = currenthours + 1;

    }
    if (currenthours == 24)
    {
      currentseconds = 0;
      currentminutes = 0;
      currenthours = 0;
      // do archive here?
      // test for day start / end here?
    }

  } // end 1 second
} // end void loop


// connect to wifi – returns true if successful or false if not
boolean connectWifi() {
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi Network");

  Serial.println(ssid);

  // Wait for connection
  Serial.print("Connecting ...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(5000);
    Serial.print(".");
    if (i > 10) {
      state = false;
      break;
    }
    i++;
  }

  if (state) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("");
    Serial.println("Connection failed. Bugger");
  }

  return state;
}

void measuredistance() {
  delay(100); //pause to stabilise

  /* The following trigPin/echoPin cycle is used to determine the
    distance of the nearest object by bouncing soundwaves off of it.
    The ESP-01 running on a USB adaptor gets confused easily by power supply noise
    with this code. Beware!
    especally important to run the ultrasonic sensor on its own PSU - it gets
    very septic running on the USB power as well. A grunty capacitor across
    its PSU pins helps as well.
    YMMV
  */
  Serial.print("~"); // this will trigger the ultrasonic to take a measurement

  duration = pulseIn(echoPin, HIGH);
  //Calculate the distance (in cm) based on the speed of sound.
  distance = duration / 58.2;

}

void logdistance() {
  if (distancereportcounter < 12) {
    Serial.print (distance, DEC);
    Serial.print("-");
    Serial.print ("W");
    Serial.print(WallDetectedStateCount);
    Serial.print ("P");
    Serial.print(PersonDetectedStateCount);
    Serial.print (" ");
    //Serial.print (distancereportcounter);
    distancereportcounter = distancereportcounter + 1;
  } else {
    distancereportcounter = 0;
    Serial.println  (distance, DEC);
    Serial.print("-");
    Serial.print ("W");
    Serial.print(WallDetectedStateCount);
    Serial.print ("P");
    Serial.print(PersonDetectedStateCount);
    Serial.print (" ");
  }
}
/*
  void PrintRectangle(int RectWidth,int RectHeight,String RectColor) {

  client.print("<div id=\"genericrectangle\" style=\"display: inline-block; width:"+ String(RectWidth) +"px; height:"+ String(RectHeight) +"px; background-color:"+ (RectColor)+"\"></div>");
  }
*/


int counttime(int hours, int minutes, int seconds) { // This function needs to return multiple values, doesnt work yet..

  seconds = seconds + 1;
  if (seconds == 60)
  {
    seconds = 0;
    minutes = minutes + 1;
  }
  if (minutes == 60)
  {
    minutes = 0;
    hours = hours + 1;
  }

}

