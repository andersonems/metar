/*  04/14/2021_PRINT

DECEMBER 7 2022 modifications
This version is a modified version by Cam Anderson to work with the Airservices Australia API to pull METAR data for regional airports across Australia. All credit to the original authors, refer to comments and credits below.


  METAR Reporting with LEDs and Local WEB SERVER
  In Memory of F. Hugh Magee, brother of John Magee author of poem HIGH FLIGHT.
  https://en.wikipedia.org/wiki/John_Gillespie_Magee_Jr.

  https://youtu.be/Yg61_kyG2zE  HomebuiltHELP; The video that started me on this project.
  https://youtu.be/xPlN_Tk3VLQ  Getting Started with ESP32 video from DroneBot Workshop.
  https://www.aviationweather.gov/docs/metar/Stations.txt  List of ALL Stations Codes.

  REMARKS/COMMENTS:
  Created by John Pipe with help from David Bird.
  FULLY Configurable for YOUR application. (See the list of all Stations Codes, link above).
  No of Airports = 100 tested 6/24/20 (60+ possible with external power).
  When Error, successfully recovers. If not, press "RESET" button on ESP32.
  The code is stored on the ESP32 Development Board 2.4GHz WiFi+Bluetooth Dual Mode,
  once the code is downloaded, it will run by it's self and will automatically restart, even after a power off.
  ESP32 requires internet access (Network or Hot Spot) to download METARS.
  A computer, with FREE software https://www.arduino.cc/en/Main/Software , is needed to initially configure,
  download software and (optionally) display any program messages.
  (Watch Getting Started with ESP32 video from DroneBot Workshop, link above).

  Updates METARS approximately every six minutes, so nearly REAL TIME data, from AVIATIONWEATHER.GOV.
  A set of WS2812 LEDS show all station CATEGORIES (similar to the HomebuiltHELP video, link above).
  Then cycles through all the stations and flashes individually for:
  Wind Gusts(Cyan)[suspendable], Precipitation(Green/White), Ice(Blue), Other(Yellow) and Info(Orange).
  Then displays "RAINBOW" for all stations, for Visibility [Red Orange Pink/White], Wind Speed Gradient [Cyan],
  Temperature Gradient [Blue Green Yellow Orange Red] and Altimeter Pressure Gradient [Blue Purple].

  Recommended, VIEWABLE with a cell phone or computer connected to the SAME network:
  SUMMARY html gives a colorful overview and
  STATION html shows DECODED METAR information and much MORE.  (See Below for Improvements.)

  NOTE: To view these, you need the http address which is shown at start up if the serial monitor is swiched on.

  Makes a GREAT Christmas Tree Chain of Lights, TOO (and a Good Conversation Piece).

  MADE THINGS A LITTLE BETTER, BUG FIXES, IMPROVEMENTS, REPAIRS TO TIME-SPACE CONTINUUM, ETC, ETC.
  Includes: Decoded Metar, Current Zulu Time, Temperature in Deg F, Elevation Ft, Estimated Density Altitude Ft
  Modified Significant Weather to include Cloud Cover, RVR & Weather 12/31
  Changed to a TIMED 6 Minute METAR read and update 01/07
  Added for Ice and Hail (Blue) 01/30
  Added Capability to select ANY Airport Code 02/02
  Added Summary to HTML 03/04
  Cleaned up Update_Time & loop 03/08
  Added User 03/10
  Added Cloud_base Change Arrows 03/13
  Added Alt Pressure Change Arrows 03/13
  Added Yellow Misc Weather 03/24
  Added Observation Time 04/01
  Added Orange Info Changes 04/05
  Cleaned up Parse_Metar 04/14
  Added Wind Changes 04/30
  Added Pressure Display 05/06
  Modified Visibility Display 05/15
  Modified Variable Types 05/15
  Tweaked Rainbow Displays 05/21
  Dual Core: Main_Loop Task1 Core0; Go_Server Task2 Core1 05/24
  Modified Server Update Time 05/30
  More little tweaks 06/01
  Messing with memory storage 06/15
  Modified to HTTPS 06/23
  REMOVED from remark "Welcome User" 07/11
  Modified the URL address 10/03
  Changed Temperature Display Colors 10/29
  Address to the server with http://metar.local/ 10/29
  Added Comments 02/11/21
  Modified Dictinary 04/14/21
*/

//#include <Arduino.h>
#include "FastLED.h"
#include <WiFi.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include "ESPmDNS.h"

WiFiServer server(80);   // Set web server port number to 80
//  Configure Network
//const char*      ssid = "Network";          // your network SSID (name)
//const char*  password = "Password";        // your network password
const char*        ssid = "SSID";       // your network SSID (name)
const char*    password = "WLAN_PASS";       // your network password

// Add NAIPS credentials
String authUsername = "NAIPS_USER";
String authPassword = "NAIPS_PASS";

// Setup from Time Server
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;                // UTC Time
const int   daylightOffset_sec = 0;           // UTC Time
struct tm timeinfo;                           // Time String "%A, %B %d %Y %H:%M:%S"


// To get Station NAME and Information
// Test link: https://aviationweather.gov/adds/dataserver_current/httpparam?dataSource=Stations&requestType=retrieve&format=xml&stationString=KFLL
String    urls = "/naips/briefing-service?wsdl";
// To get Station DATA
// Test link: https://www.aviationweather.gov/adds/dataserver_current/httpparam?datasource=metars&requestType=retrieve&format=xml&mostRecentForEachStation=constraint&hoursBeforeNow=1.25&stationString=KFLL,KFXE
String    urlb = "/naips/briefing-service?wsdl";
String    host = "https://www.airservicesaustralia.com";


// Set Up LEDS
#define No_Stations          32      // Number of Stations also Number of LEDs
#define DATA_PIN              4      // Connect to pin D5/P5 with 330 to 500 Ohm Resistor
#define LED_TYPE         WS2812      // WD2811 or WS2812 or NEOPIXEL
#define COLOR_ORDER         GRB      // WD2811 are RGB or WS2812 are GRB
#define BRIGHTNESS           20      // Master LED Brightness (<12=Dim 20=ok >20=Too Bright/Much Power)
#define FRAMES_PER_SECOND   120
CRGB leds[No_Stations];              // Number of LEDs

// Define STATIONS & Global Variables
std::vector<String> PROGMEM Stations {  //   << Set Up   - Do NOT change this line
  "NULL, STATION NAME         ",        // 0 << Reserved - Do NOT change this line
  "YWBL, WARRNAMBOOL, VIC     ",     // 1
  "YHSM, HORSHAM, VIC         ",     // 2
  "YBDG, BENDIGO, VIC         ",     // 3
  "YBLT, BALLARAT, VIC        ",     // 4
  "YMAV, AVALON, VIC          ",     // 5  
  "YMMB, MOORABBIN, VIC       ",     // 6
  "YMPC, POINT COOK, VIC      ",     // 7
  "YMEN, ESSENDON, VIC        ",     // 8
  "YMML, MELBOURNE, VIC       ",     // 9
  "YMNG, MANGALORE, VIC       ",     // 10
  "YSHT, SHEPPARTON, VIC      ",     // 11
  "YWGT, WANGARATTA, VIC      ",     // 12
  "YHOT, MOUNT HOTHAM, VIC    ",     // 13
  "YBNS, BAIRNSDALE, VIC      ",     // 14
  "YMES, EAST SALE, VIC       ",     // 15
  "YLTV, LATROBE VALLEY, VIC  ",     // 16
  "YYRM, YARRAM, VIC          ",     // 17
};                                      // << Do NOT change this line

PROGMEM String         rem[No_Stations + 1];  // Remarks
PROGMEM String     comment[No_Stations + 1];  // Comments
PROGMEM String      sig_wx[No_Stations + 1];  // Significant Weather
PROGMEM float         temp[No_Stations + 1];  // temperature deg C, was float
PROGMEM float        dewpt[No_Stations + 1];  // dew point deg C, was float
PROGMEM String        wind[No_Stations + 1];  // wind speed
PROGMEM String        wdir[No_Stations + 1];  // wind direction
PROGMEM int       old_wdir[No_Stations + 1];  // old wind direction
PROGMEM float        visab[No_Stations + 1];  // visibility, was float
PROGMEM String         sky[No_Stations + 1];  // sky_cover  & cloud_base
PROGMEM int     cloud_base[No_Stations + 1];  // cloud_base
PROGMEM int old_cloud_base[No_Stations + 1];  // old cloud_base
PROGMEM float      seapres[No_Stations + 1];  // Sea Level Pressure
PROGMEM int        altim[No_Stations + 1];  // altimeter setting, was float
PROGMEM float    old_altim[No_Stations + 1];  // old altimeter setting
PROGMEM float    elevation[No_Stations + 1];  // elevation setting
PROGMEM String    category[No_Stations + 1];  // NULL   VFR    MVFR   IFR    LIFR
//..............................................Black  Green   Blue   Red    Magenta

#define LED_BUILTIN 2  // ON Board LED GPIO 2
String naips;          // Raw data from NAIPS
String soapstatement;  // Statement to send to get the data back
String metar;          // Raw METAR data
String Time;           // Time  "HH:MM"
int Hour;              // Latest Hour
int Minute;            // Latest Minute
String Last_Up_Time;   // Last Update Time  "HH:MM"
int Last_Up_Min;       // Last Update Minute
int Update_Data = 15;   // Updates Data every 6 Minutes (Don't overload AVIATIONWEATHER.GOV) - changed to 15 mins for the Airservices version to reduce load on their API
int station_num = 1;   // Station # for Server - flash button
int httpCode;          // Error Code
String local_hwaddr;   // WiFi local hardware Address
String local_swaddr;   // WiFi local software Address
TaskHandle_t Task1;    // Main_Loop, Core 0
TaskHandle_t Task2;    // Go_Server, Core 1
const char* ServerName = "metar";   // Address to the server with http://metar.local/
const char*   FileName = "METAR_ESP32_04_14_21";

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // the onboard LED
  Serial.begin(115200);
  delay(4000);         // Time to press "Clear output" in Serial Monitor
  Serial.println("\nMETAR Reporting with LEDs and Local WEB SERVER");
  Serial.println("File Name \t: " + String(FileName) + "\n");
  Init_LEDS();         // Initialize LEDs
  if (String(ssid) == "iPhone")  Serial.println(F("** If iPhone DOESN'T CONNECT:  Select HOT SPOT on iPhone **"));
  digitalWrite(LED_BUILTIN, HIGH);  //ON
  Serial.print("WiFi Connecting to " + String(ssid) + " ");
  // Initializes the WiFi library's network settings.
  WiFi.begin(ssid, password);
  // CONNECT to WiFi network:
  while (WiFi.status() != WL_CONNECTED)   {
    delay(300);    // Wait a little bit
    Serial.print(".");
  }
  Serial.println(F(" Connected."));
  // Print the Signal Strength:
  long rssi = WiFi.RSSI() + 100;
  Serial.print("Signal Strength = " + String(rssi));
  if (rssi > 50)  Serial.println(F(" (>50 - Good)"));  else   Serial.println(F(" (Could be Better)"));
  digitalWrite(LED_BUILTIN, LOW);   //OFF
  Serial.println(F("*******************************************"));
  Serial.println("To View Decoded Station METARs from a Computer or \nCell Phone connected to the " + String(ssid) + " WiFi Network.");
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid, password);
  if (MDNS.begin(ServerName)) {     // The name that will identify your device on the network
    local_hwaddr = "http://" + WiFi.localIP().toString() + "/summary";
    Serial.println("Enter This Url Address \t: " + local_hwaddr);
    local_swaddr = "http://" + String(ServerName) + ".local/";
    Serial.println("   Or This Url Address \t: " + local_swaddr);
  }
  else {
    Serial.println(F("ERROR setting up MDNS responder"));
  }
  digitalWrite(LED_BUILTIN, HIGH);  //ON
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);   // Get Zulu Time from Server
  Update_Time();          // Set Up Time,Hour,Minute
  Serial.print(F("Date & Time \t\t: "));
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M - Zulu");
  digitalWrite(LED_BUILTIN, LOW);   //OFF
  //Task1 = Main_Loop() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore( Main_Loop, "Task1", 10000, NULL, 1, &Task1, 0);
  delay(200);          // Wait a smidgen
  //Task2 = Go_Server() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore( Go_Server, "Task2", 10000, NULL, 1, &Task2, 1);
  delay(200);          // Wait a smidgen
  Serial.println(F("Dual Core Initialized"));
  server.begin();      // Start the web server
  Serial.println(F("Web Server Started"));
  Serial.println(F("*******************************************"));
}
void loop() {
}

//  *********** Main_Loop  TASK1
void Main_Loop( void * pvParameters ) {
  for (;;) {
    Last_Up_Time = Time;
    Last_Up_Min = Minute;
    int NextCycle = Last_Up_Min + Update_Data; // Update Cycle time is 6 Minutes(Update_Data)
    GetAllMetars();                            // Get All Metars and Display Categories
    Update_Time();                             // Update Current Time
    while (NextCycle > Minute) {
      if ((NextCycle - Minute) > Update_Data)  NextCycle = -1;  else
        Display_Metar_LEDS ();                 // Display Station Metar/Show Loops
    }
  }
}

// *********** Set/Get Current Time, Hour, Minute
void Update_Time() {
  if (!getLocalTime(&timeinfo))  {
    Serial.print(F("\t\t\t**  FAILED to Get Time  **\n"));
    Network_Status ();         // WiFi Network Error
  }
  //  configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
  //  time_t now = time(nullptr);

  char TimeChar[9];
  strftime(TimeChar, 9, "%H:%M:%S", &timeinfo);
  Time  = String(TimeChar);
  Hour   = Time.substring(0, 2).toInt();
  Minute = Time.substring(3, 5).toInt();
  Time   = Time.substring(0, 5);
}

// *********** Initialize LEDs
void Init_LEDS() {
  Serial.println("Initializing LEDs for No_Stations = " + String(No_Stations));
  // Set up the strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, No_Stations).setCorrection(TypicalLEDStrip);
  // Set master brightness control (<12=Dim 20=ok >20=Too Bright)
  FastLED.setBrightness(BRIGHTNESS);
  // Set all leds to Black
  fill_solid(leds, No_Stations, CRGB::Black);
  FastLED.show();
  delay(100);    // Wait a smidgen
}

// *********** GET All Metars in Chunks
void GetAllMetars() {
  int Step = 0;             // 20 Stations at a time (change for Airservices version to do one at a time)
  for (int j = 0; j < No_Stations; j = j + 1 + Step) {
    int Start  = j;
    int Finish = Start + Step;
    if (Finish > No_Stations)  Finish = No_Stations;
    String url = "";
    for (int i = Start; i <= Finish; i++) {
      String station = Stations[i].substring(0, 5);
      //url = url + String (station);   //Comment out this line as we don't need the URL extension to identify the station.
    }
    int len = url.length();
    url = url.substring(0, len - 1);  // Remove last "comma"
    GetData(url, Start);             // GET Some Metar Data 20 Stations at a time
    for (int i = Start; i <= Finish; i++) {
      ParseMetar(i);          // Parse Metar Data one Station at a time
    }
  }
}

// *********** GET Some Metar Data/Name
void GetData(String url, int i) {
  metar = "";                          // Reset Metar Data
  String station = Stations[i].substring(0, 5); //identify which station is being sought
  int len = station.length();
  station = station.substring(0, len - 1); //remove the last comma from the station name
  if (url == "NAME") url = host + urls + Stations[0];  else   url = host + urlb + url;
  //Serial.println("url = " + url);
  //  Check for WiFi connection
  if ((wifiMulti.run() == WL_CONNECTED)) {
    digitalWrite(LED_BUILTIN, HIGH);  // ON
    HTTPClient https;
    //  Define HTTP header
    https.addHeader("Content-Type", "text/xml");
    https.addHeader("charset", "UTF-8");
    //  Build SOAP statement for posting
    soapstatement = "<SOAP-ENV:Envelope xmlns:ns0=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:ns1=\"http://www.airservicesaustralia.com/naips/xsd\" xmlns:xs1=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\"> <SOAP-ENV:Header/> <ns0:Body> <ns1:loc-brief-rqs password=\"" + authPassword + "\" requestor=\"" + authUsername + "\" source=\"atis\"> <ns1:loc>" + station + "</ns1:loc> <ns1:flags met=\"true\"/> </ns1:loc-brief-rqs> </ns0:Body> </SOAP-ENV:Envelope>"; 
    // Serial.println(soapstatement); //" added quotes here to allow colouration for the rest // debugging
    // Serial.println(url); // debugging, show URL being queried
    https.useHTTP10(true);
    https.begin(url);
    //  Start connection and send HTTP header
    httpCode = https.POST(soapstatement); 
    //Serial.println(httpCode);   
    //  httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      // and File found at server
      if (httpCode == HTTP_CODE_OK) {
        naips = https.getString();
        // Serial.println(naips.length()); //debugging - length of raw NAIPS data
        // Serial.println(naips); //debugging - raw NAIPS location briefing
        naips.remove(0,naips.indexOf("METAR")); //Locate where "METAR" appears in the location briefing and delete everything before that
        metar = naips;
      //  Serial.println(metar); // debugging - METAR for selected location
      }
      https.end();
      digitalWrite(LED_BUILTIN, LOW);  //OFF
    } else {
      https.end();        // Communication HTTP Error
      digitalWrite(LED_BUILTIN, LOW);  //OFF
      Serial.println(Time + "\tNo:" + String(i) +  + "\tCommunication Error = " + String(httpCode) + " : " + https.errorToString(httpCode).c_str());
    }
  } else {
    Network_Status ();  // WiFi Network Error
  }
}

// ***********   WiFi Network Error
void Network_Status () {
  Serial.print(Time + "\t\t\tWiFi Network : ");
  if (WiFi.status() == 0 ) Serial.println(F("Idle"));
  if (WiFi.status() == 1 ) Serial.println(F("Not Available, SSID can not be reached"));
  if (WiFi.status() == 2 ) Serial.println(F("Scan Completed"));
  if (WiFi.status() == 3 ) Serial.println(F("Connected"));
  if (WiFi.status() == 4 ) Serial.println(F("Failed, password incorrect"));
  if (WiFi.status() == 5 ) Serial.println(F("Connection Lost"));
  if (WiFi.status() == 6 ) Serial.println(F("Disconnected"));
}

// ***********   Parse Metar Data
void ParseMetar(int i) {
  String parsedmetar = "";
 // Serial.println("ParseMetar invoked for station " + Stations[i].substring(0, 4) + ", metar string is as follows: " + metar);
  String station = Stations[i].substring(0, 4);
  if (station == "NULL")   return;
  int data_Start = metar.indexOf(station);                     // Search for station id
  int data_end  = metar.indexOf("<", data_Start + 1);   // Search for "data end"
  if (data_Start > 0 && data_end > 0)    {
    parsedmetar = metar.substring(data_Start, data_end);       // Parse Metar Data
    // Remove found data from metar
    metar = metar.substring(0, data_Start) + metar.substring(data_end, metar.length());
    Serial.println("\nProcessing Station " + String(i) + " " + station + " in ParseMetar");
    Serial.println("Received METAR data is as follows: " + parsedmetar);
    Decodedata(i, station, parsedmetar);                       // DECODE the Station DATA
  } else {
    Serial.println("Station " + String(i) + " " + station + " Not Found");
    category[i] = "NF";             // Not Found
    sky[i] = "NF";                  // Not Found
    rem[i] = "Station Not Reporting";   // Not Found
    if (httpCode < 0)  rem[i] = "Connection Error";
    sig_wx[i] = "NF";               // Not Found
    visab[i] = 0;                   // Not Found
    wdir[i] = "NA";                 // Not Found
    wind[i] = "NA";                 // Not Found
    temp[i] = 0;                    // Not Found
    dewpt[i] = 0;                   // Not Found
    altim[i] = 0;                   // Not Found
    old_altim[i] = 0;               // Not Found
    comment[i] = "";                // Not Found
    if (WiFi.status() == 3 && httpCode > 0)       // Not Network or HTTP  Error
      Serial.println(Time + "\tNo:" + String(i) + "\t" + station + "\tStation Not Reporting, Skipping this one in ParseMetar");
    Display_LED (i, 20);            // Display Station LED
  }
}

// *********** DECODE the Station DATA
void Decodedata(int i, String station, String parsedmetar) {
  int pflag = 2;         // Print Flag 0=NO Print  1=Print ALL  2= ALL + RAW DATA
  String old_obs_time;
  old_obs_time = rem[i].substring(0, 4);                         // Last Observation Time
  if (old_obs_time == "new ")  old_obs_time = rem[i].substring(4, 8);

  // Searching Remarks
  int search_Strt = parsedmetar.indexOf(station, 0) + 7;            // Start of Remark
  int search_Raw_End = parsedmetar.indexOf("</raw_text", 0);        // End of Remark
  if (search_Strt < 7)  {
    Serial.println(Time + "\tNo:" + String(i) + "\t" + station + "\tFailed in Decodedata search_End= " + String(search_Raw_End));
    pflag = 2;
  }

  int search_End = parsedmetar.indexOf(" RMK");                        // End to RMK in US
  if (search_End < 0)  search_End = parsedmetar.indexOf(" Q") + 6;     // End to Q  Non US
  if (search_End < 6)  search_End = search_Raw_End;                    // No RMK in US

  // Append Minutes ago
  int obsh = parsedmetar.substring(7, 9).toInt();             // METAR Obs Time - Hour
  int obsm = parsedmetar.substring(9, 11).toInt();            // METAR Obs Time - Minute
  int ago = ((Hour - obsh) * 60) + Minute - obsm;             // Minutes ago
  if (ago < 0) ago = ((24 - obsh) * 60) + Minute - obsm;
  rem[i] = parsedmetar.substring(search_Strt, search_End) + " (" + String(ago) + "m&nbspago)";  // Append ago


  if (old_obs_time != rem[i].substring(0, 4))   {        // New Time : UPDATE Station
    // NEW DATA: UPDATING STATION

    rem[i] = "new " + rem[i];

    // Decoding Comments
    int search_From = 0;
    int rem_data = 0;                        // Remaining Data to be processed
    int rem_data_FLAG = 0;                   // Flag to skip the rest
    String mesg;
    String text;

    search_End = parsedmetar.indexOf("RMK") + 3;                          // End to RMK inUS
    if (search_End < 3)  search_End = parsedmetar.indexOf(" Q") + 6;      // End to Q  Non US
    if (search_End < 6)  search_End = search_Raw_End;                     // No RMK in US

    comment[i] = parsedmetar.substring(search_End, search_Raw_End) + " ";  //Adds a SPACE

    // Airservices version - removed all the crap from this line and doing it a little more simply
    // The Airservices METAR looks like this: [station_name] [time] [auto] [winddir_3chars][windspd_2chars]KT [visibility] // [clouds1] [clouds2] [clouds...] [temp]/[dewpt] Q[QNH] RMK [remarks]
    // This code will strip from the right as such:
    // Take everything to the right of RMK, put it into a varialbe, then remove it all (including the RMK string) - this takes care of the remarks
    // Take the figures after Q, put it into a variable, remove it all - this takes care of the QNH section
    // Take the five characters at the end, which will be temptemp/dewptdewpt, put them into variables, and remove - this will do temp and dewpoint
    // Anything to the right of // are then the cloud states. The first six characters will give us an idea of the lowest cloud layer for determining VFR/IFR category
    // The four chars to the right of the remainder are the visiiblity
    // etc

// Set the stuff to 0 now
    altim[i] = 0;
    dewpt[i] = 0;
    temp[i] = 0;  

    // RMK section - the section above these comments already took that info and put them into comment[i], so we just need to remove it here
    parsedmetar.remove(parsedmetar.indexOf("RMK"));

    // QNH section
    String qnh;
    parsedmetar.remove(parsedmetar.length() - 1); // remove the last space at the end
    qnh = parsedmetar.substring(parsedmetar.length() - 4,parsedmetar.length());
    parsedmetar.remove(parsedmetar.indexOf("Q"));

    // temp and dewpoint section
   String currentdewpt;
   String currenttemp;
    parsedmetar.remove(parsedmetar.length() - 1); // remove the space from the end
    currentdewpt = parsedmetar.substring(parsedmetar.length() - 2, parsedmetar.length()); //dewpt is the last two digits
    parsedmetar.remove(parsedmetar.length() - 3); // remove the last three digits (being the dewpt and the slash)
    currenttemp = parsedmetar.substring(parsedmetar.length() - 2, parsedmetar.length()); //temp is the last two digits
    parsedmetar.remove(parsedmetar.length() - 3); // remove the last three digits (being the temp and the space - anything between the double slash and the end should be the various clouds)

    // clouds. Only going to look at the first values after the "//". 
    // Read in the first three characters - that tells us the type of cloud (NCD, BKN etc)
    // If it's NCD, stop looking. If it's anything else, read in the following three characters to tell us the cloud base
    String currentcloudtype;
    String currentcloudbase;
    currentcloudbase = "999"; //set the cloudbase value as 999, being an obvious numeric null - it'll only stay this if the cloud type is NCD
    
    // Read in the three characters after the last instance of "// " (which should stop the //// // ////// strings confounding it)
      currentcloudtype = parsedmetar.substring(parsedmetar.lastIndexOf("// ") + 3,parsedmetar.lastIndexOf("// ") + 6);
      sky[i] = parsedmetar.substring(parsedmetar.lastIndexOf("// ") + 3,parsedmetar.lastIndexOf("// ") + 9); // load the entire lowest cloud statement into this variable for decoding later
    // If the cloud type is anything other than "NCD", read in the next three characters which are the cloud base
      if (currentcloudtype != "NCD") {
        currentcloudbase = parsedmetar.substring(parsedmetar.indexOf(currentcloudtype) + 3,parsedmetar.indexOf(currentcloudtype) + 6);
      }

    // Read in the visibility, being the 4 chars after "KT ". They could be slashes too, in which case we'll replace it with 9999 so it doesn't break.
    String currentvisibility;
      currentvisibility = parsedmetar.substring(parsedmetar.lastIndexOf("KT") + 3, parsedmetar.lastIndexOf("KT") + 7); //using lastIndexOf in case a station name has "KT" in it
      if (currentvisibility == "////") {
        currentvisibility = "9999";
      }

    // Read in the wind, being the five characters immediately before "KT". The first three characters are the direction, the last two are the speed.
    String currentwinddir;
    String currentwindspd;
      currentwindspd = parsedmetar.substring(parsedmetar.lastIndexOf("KT") - 2, parsedmetar.lastIndexOf("KT"));
      currentwinddir = parsedmetar.substring(parsedmetar.lastIndexOf("KT") - 5, parsedmetar.lastIndexOf("KT") - 2);
      
    // convert the String values to float and place them into the float arrays
    char temptemp[3];
    char tempdewpt[3];
    char tempaltim[5];
    char tempcloudtype[4];
    char tempcloudbase[4];
    char tempvisibility[5];
    char tempwinddir[4];
    char tempwindspd[4];
    
    currenttemp.toCharArray(temptemp,sizeof(temptemp));
    currentdewpt.toCharArray(tempdewpt,sizeof(tempdewpt));
    currentcloudtype.toCharArray(tempcloudtype,sizeof(tempcloudtype));
    currentcloudbase.toCharArray(tempcloudbase,sizeof(tempcloudbase));
    currentvisibility.toCharArray(tempvisibility,sizeof(tempvisibility));
    currentwinddir.toCharArray(tempwinddir,sizeof(tempwinddir));
    currentwindspd.toCharArray(tempwindspd, sizeof(tempwindspd));

    int intcloudbase;

    qnh.toCharArray(tempaltim,sizeof(tempaltim));
    temp[i] = atol(temptemp);
    dewpt[i] = atol(tempdewpt);
    altim[i] = atol(tempaltim);
    intcloudbase = atol(tempcloudbase); // This variable holds the entire lowest cloud level for interpretation elsewhere in the script
    visab[i] = atol(tempvisibility);
    wdir[i] = atol(tempwinddir);
    wind[i] = atol(tempwindspd);

    //send some stuff to the serial output so we can see how it's going
  //  Serial.println(temp[i]);
  //  Serial.println(dewpt[i]);
  //  Serial.println(altim[i]);
  //  Serial.println(sky[i]);
  //  Serial.println(tempcloudbase);
  //  Serial.println(visab[i]);
  //  Serial.println(wdir[i]);
  //  Serial.println(wind[i]);
  //  Serial.println(comment[i]);

    //determine the flight category (VFR, MVFR, IFR, LIFR) from what we have
    // LIFR = cloudbase <500ft ANDOR visibility < 1 mile
    // IFR = cloudbase 500-1000ft ANDOR visibility 1-3 miles
    // MVFR = cloudbase 1000-3000ft ANDOR visibility 3-5 miles
    // VFR = cloudbase >3000ft AND visibility >5 miles
      category[i] = "VFR"; //defaults to VFR unless information exists to the contrary.
      
    if (intcloudbase > 030 and visab[i] > 5000) {
      category[i] = "VFR";
    }
    if (intcloudbase >= 010 and intcloudbase < 030) {
      if (currentcloudtype == "OVC" or currentcloudtype == "BKN") {
        category[i] = "MVFR";
      }    
    }
    if (visab[i] >= 3000 and visab[i] < 5000) {
      category[i] = "MVFR";
    } 
    if (intcloudbase >= 005 and intcloudbase < 010) {
      if (currentcloudtype == "OVC" or currentcloudtype == "BKN") {      
      category[i] = "IFR";
      }
    }
    if (visab[i] >= 1000 and visab[i] < 3000) {
      category[i] = "IFR";
    }
    if (intcloudbase < 005 or visab[i] < 1000) {
      if (currentcloudtype == "OVC" or currentcloudtype == "BKN") {      
      category[i] = "LIFR";
      }      
    }

Serial.println(category[i]);    

  }
  Display_LED (i, 20);            // Display One Station LED
}

//  *********** Decode Weather and make readable in Dictionary Function
String Decode_Weather(int i, String weather) {
  // Dictionary of CODES

  //weather.replace("A01 ", "");                          // Fixing an error
  //weather.replace("A02 ", "");                          // Fixing an error
  weather.replace("RTS ", "Routes");
  weather.replace("UTC ", "Zulu");

  weather.replace("BECMG", "CMG");                       // Rename Later
  weather.replace("TEMPO", "Temporary");
  weather.replace("CAVOK", "cavok");                     // Rename Later

  weather.replace("NOSIG", "No Signifiant Weather");
  weather.replace("NSW", "No Signifiant Weather");
  weather.replace("NSC", "No Signifiant Clouds");
  weather.replace("DEWPT", "Dew Point");
  weather.replace("CONS", "");                           //  Don't Know What This Is (Current Conditions?)

  weather.replace("OBSC", "Obscured");
  weather.replace("FROPA", "Frontal Passage");
  weather.replace("PRESFR", "Pressure Falling Rapidly");
  weather.replace("PRESRR", "Pressure Rising Rapidly");
  weather.replace("FROIN", "Frost On The Indicator");
  weather.replace("ALSTG", "Alitmeter Setting");
  weather.replace("ALTSTG", "Alitmeter Setting");
  weather.replace("SLPNO", "Sea Level Pressure NA");     // Rename Later
  weather.replace("PNO", "Rain Gauge NA");
  weather.replace("FZRANO", "Freezing Rain Sensor NA");
  weather.replace("TSNO", " Thunderstorm Sensor NA");
  weather.replace("RVRNO", " Runway VIS Sensor NA");
  weather.replace("VISNO E", "visibility Sensor NA ESTMD");
  weather.replace("PK WND", "Peak Wind");
  weather.replace("WIND", "Wind");
  weather.replace("WSHFT", "Wind Shift at ");
  weather.replace("WND", "Wind");
  weather.replace("MISG", "Missing");
  weather.replace("SHRA", "Rain Shower");
  weather.replace("SH", "Showers");
  weather.replace("DATA", "Data");
  weather.replace("AND", "and");
  weather.replace("ESTMD", "eastsimated");          // Rename Later
  weather.replace("WSHFT", "Wind Shift at");
  weather.replace("PAST", "Past");
  weather.replace("THN", "Thin");
  weather.replace("THRU", "Thru");
  weather.replace("HR", "Hour");
  weather.replace("M ", "Measured ");
  weather.replace("WSHFT", "Wind Shift");
  weather.replace("ICG", "Icing");
  weather.replace("PCPN", "Precip");
  weather.replace("MTS", "Mountains");
  weather.replace("ACC", "AC");
  weather.replace("SNINCR", "Snow Increasing Rapidily");
  weather.replace("ACFT MSHP", "Aircraft Mishap");
  weather.replace("STFD", "Staffed");
  weather.replace(" LAST", " Last");
  weather.replace("OBS", "OS");                      // Rename Later
  weather.replace("NXT", "Next");
  weather.replace("CVCTV", "Conv");                  // Rename Later
  weather.replace("FEW", "FewCLD");
  weather.replace("MDT", "Moderate");
  weather.replace("CLD", "Clouds");
  weather.replace("EMBD", "mbedded");
  weather.replace("BINOVC", "KN in OC");             // Rename Later
  weather.replace("OCNL", "Occasional");
  weather.replace("ONCL", "Occasional");
  weather.replace("FRQ", "Frequent");
  weather.replace("AND", "and");
  weather.replace("LTG ICCC", "LTG LTinC");          // Lightning in Cloud
  weather.replace("LTGICIC", "LTG LTinC");           // Lightning in Cloud
  weather.replace("LTGICCC", "LTG LTinC");           // Lightning in Cloud
  weather.replace("LTGICCG", "LTG LTinC and LTtoG"); // Lightning in Cloud and Ground
  weather.replace("LTGCGICCC", "LTG LTtoG and LTinC"); // Lightning in Cloud and Ground
  weather.replace("LTICGCG", "LTG LTtoG");           // Lightning to Ground
  weather.replace("LTGCG", "LTG LTtoG");             // Lightning to Ground
  weather.replace("LTGIC", "LTG LTinC");             // Lightning in Cloud
  weather.replace("LTG", "Lightning");
  weather.replace("LTinC", "in Clouds");
  weather.replace("LTtoG", "to Ground");
  weather.replace("ALQDS", "All Quadrents");
  weather.replace("ALQS", "All Quadrents");
  weather.replace("DIST", "Distant ");
  weather.replace("DSNT", "Distant ");
  weather.replace("MOV", " Moing");                 // Rename Later
  weather.replace("SFC", " Surface");
  weather.replace("SN", "Snow");
  weather.replace(" E", " east");                   // Rename Later
  weather.replace("E ", "east ");                   // Rename Later
  weather.replace("NE", "Neast");                   // Rename Later
  weather.replace(" SE", " Seast");                 // Rename Later
  weather.replace("E-", "east-");                   // Rename Later
  weather.replace("-E", "-east");                   // Rename Later
  weather.replace("-SE", "-Seast");                 // Rename Later
  weather.replace("BKN", "KN");                     // Rename Later
  weather.replace("TCU", "TC");                     // Rename Later
  weather.replace("SCT", "Scattered");
  weather.replace("TWR", "Tower");
  weather.replace("VIS", "visability ");            // Rename Later
  weather.replace("LWR", "Lower");
  weather.replace("OVC", "OC");
  weather.replace("DSIPTD", "Dissipated");
  weather.replace("HVY", "Heavy");
  weather.replace("LGT", "Light");
  weather.replace("VC", "In the vicinity ");
  weather.replace("OHD", "overhead");               // Rename Later
  weather.replace("OVD", "overhead");               // Rename Later
  weather.replace("VRB", "variable");               // Rename Later
  weather.replace("VRY", "very ");                  // Rename Later
  weather.replace("VSBY", "visibility ");           // Rename Later
  weather.replace("VSB", "visibile");               // Rename Later
  weather.replace("VIRGA", "virga ");               // Rename Later
  weather.replace("V", "variable");                 // Rename Later
  weather.replace("CIG", "Ceiling");
  weather.replace("ACSL", "Standing Lenticular Altocumulus Clouds ");
  weather.replace("SC", "Stratocumulus Clouds ");
  weather.replace("CB", "Cumulonimbus Clouds ");
  weather.replace("CI", "Cirrus Clouds ");
  weather.replace("CU", "Cumulus Clouds ");
  weather.replace("CF", "Cumulusfractus Clouds ");
  weather.replace("SF", "Stratus Fractus Clouds ");
  weather.replace("ST", "Stratus Clouds ");
  weather.replace("NS", "Nimbostratus Clouds ");
  weather.replace("AC", "Altocumulus Clouds ");
  weather.replace("AS", "Altostratus Clouds ");

  weather.replace("Clouds 1", "Clouds=1Oktas ");
  weather.replace("Clouds 2", "Clouds=2Oktas ");
  weather.replace("Clouds 3", "Clouds=3Oktas ");
  weather.replace("Clouds 4", "Clouds=4Oktas ");
  weather.replace("Clouds 5", "Clouds=5Oktas ");
  weather.replace("Clouds 6", "Clouds=6Oktas ");
  weather.replace("Clouds 7", "Clouds=7Oktas ");
  weather.replace("Clouds 8", "Clouds=8Oktas ");
  weather.replace("TS", "Thunderstorm");

  //weather.replace("4/", " Snow Depth on Ground=");     // (4/### Whole inches)
  //weather.replace("8/", " Amount of Precip=.");        // (4/### 1/100th inches)

  weather.replace("DZ", " Drizzle");

  weather.replace("BLU", "blu");
  weather.replace("WHT", "wht");
  weather.replace("GRN", "grn");
  weather.replace("YLO", "ylo");
  weather.replace("AMB", "amb");
  weather.replace("RED", "redd");

  weather.replace(" +", " Heavy ");
  weather.replace(" -", " Light ");
  weather.replace("BL", " blowing ");                     // Rename Later
  weather.replace("DR", " Drifting ");
  weather.replace("FZ", " Freezing ");
  weather.replace("SN", "Snow");
  weather.replace("RA", "Rain");
  weather.replace("BR", "Mist");

  weather.replace("IC", "Ice Cristals");
  weather.replace("PL", "Ice Pellets");
  weather.replace("GS", "Hail");
  weather.replace("GR", "Hail");
  weather.replace("MI", "Shallow ");
  weather.replace("PTCHY", "Patchy ");
  weather.replace("FG", "Fog");
  weather.replace("FU", "Smoke");
  weather.replace("HZY", "HZ");
  weather.replace("HZ", "Haze");
  weather.replace("DU", "Dust");
  weather.replace("UP", "Unknown Precip");
  weather.replace("MOD", "Moderate ");
  weather.replace("INC", "Increasing ");
  weather.replace("DCR", "Decreasing ");
  weather.replace("TR", "Trace ");
  weather.replace("RE", "Recent ");

  weather.replace("B", "began");                       // Rename Later
  weather.replace("E", "ended");                       // Rename Later
  weather.replace("east", "E");                        // Change it Back
  weather.replace("var", "Var");                       // Change it Back
  weather.replace("ver", "Ver");                       // Change it Back
  weather.replace("oVer", "Over");                     // Change it Back
  weather.replace("vi", "Vi");                         // Change it Back
  weather.replace("OC", "Overcast");                   // Rename
  weather.replace("KN", "Broken");                     // Rename
  weather.replace("began", "Began");                   // Change it Back
  weather.replace("blowing", "Blowing");               // Change it Back
  weather.replace("ended", "Ended");                   // Change it Back
  weather.replace("Moing", "Moving ");                 // Change it Back
  weather.replace("mbedded", "Embedded");              // Change it Back
  weather.replace("cavok", "CAVOK");                   // Change it Back
  weather.replace("Conv", "Convective");               // Rename
  weather.replace("OS", " Observation");               // Rename
  weather.replace("TC", "Towering Cumulus Clouds ");   // Rename
  weather.replace("CMG", "Becoming");                  // Rename

  weather.replace("SOG ", "Snow on the Ground");
  //weather.replace("$", " ~ Sta Needs Maintance");
  weather.replace("$", " ");
  weather.replace("RF", "Rainfall ");
  weather.replace("RMK", "Remark: ");

  weather.replace("blu blu", "blu");
  weather.replace("wht wht", "wht");
  weather.replace("grn grn", "grn");
  weather.replace("ylo ylo", "ylo");
  weather.replace("amb amb", "amb");
  weather.replace("redd redd", "red");
  weather.replace("blu", "BLUE: Cloud Base>2500ft Visibility>8km ");
  weather.replace("wht", "WHITE: Cloud Base>1500ft Visibility>5km ");
  weather.replace("grn", "GREEN: Cloud Base>700ft Visibility>3.7km ");
  weather.replace("ylo", "YELLOW: Cloud Base>300ft Visibility>1600m ");
  weather.replace("amb", "AMBER: Cloud Base>200ft Visibility>800m ");

  weather.replace("tend-", "Change -");                 // 3 Hr Pressure Tendancy
  weather.replace("tend+", "Change +");                 // 3 Hr Pressure Tendancy

  weather.replace("  ", " ");                           // Fixing an error

  // Peak Wind
  int search0 = weather.indexOf("Peak Wind");
  if (search0 >= 0) {
    int search1 = weather.indexOf("/", search0 + 9);
    String Dir = weather.substring(search1 - 5, search1 - 2) + "Deg";
    String Wind = weather.substring(search1 - 2, search1) + "KT at ";
    String Time = weather.substring(search1 + 1, search1 + 5);
    weather.replace(weather.substring(search1 - 5, search1 + 5), Dir + Wind + Time);
  }
  weather.replace("PWINO", "Peak Wind Sensor NA");
  return weather;
}

// *********** Display One Station LED
void Display_LED(int index, int wait) {
  if (index == 0)  return;
  leds[index - 1] = 0xaf5800;    // Orange Decoding Data
  FastLED.show();
  delay(wait);
  Set_Cat_LED(index);           // Set Category for This Station LED
  FastLED.show();
}

// *********** DECODE the Station NAME
void Decode_Name(int i) {
  String Station_name = Stations[i].substring(0, 4);
  int search0 = metar.indexOf(Station_name) + 1;      // Start Search from Here
  int search1 = metar.indexOf("<site", search0);
  int search2 = metar.indexOf("</site", search0);
  if (search1 > 0 && search2 > 0)   Station_name = Station_name + ", " + metar.substring(search1 + 6, search2) + ",";
  search1 = metar.indexOf("<country", search0);
  search2 = metar.indexOf("</country", search0);
  if (metar.substring(search1 + 9, search2) == "US") {
    search1 = metar.indexOf("<state", search0);
    search2 = metar.indexOf("</state", search0);
    if (search1 > 0)   Station_name = Station_name + " " + metar.substring(search1 + 7, search2);
  }
  if (search1 > 0 && search2 > 0)   Station_name = Station_name + " " + metar.substring(search1 + 9, search2);
  Stations[i] = Station_name.c_str();
}

// ***********   Display Metar/Show Loops
void Display_Metar_LEDS () {
  int Wait_Time = 5000;             //  Delay after Loop (Seconds x 1000)
  Display_Weather_LEDS (8000);      //  Display Weather
 // Display_Vis_LEDS (Wait_Time);     //  Display Visibility [Red-Orange-White]
 // Display_Wind_LEDS (Wait_Time);    //  Display Winds [Shades of Aqua]
 // Display_Temp_LEDS (Wait_Time);    //  Alt Display Temperatures [Blue-Green-Yellow-Red]
 // Display_Alt_LEDS (Wait_Time);     //  Display Altimeter Pressure [Blue-Purple]
  Update_Time ();                   //  Update Current Time
}

// ***********  Display Weather
void Display_Weather_LEDS (int wait) {
  Display_Cat_LEDS ();             //  Display All Categories
//  for (int i = 1; i < (No_Stations + 1); i++)  {
//    if (sig_wx[i].length() > 4)   {    // IF NOT "None" in Sig Wx,  Display Weather
//      station_num = i;                 // Station # for Server - flash button
//      //  Twinkle for Flashing Weather   (index,  red,green,blue, pulses, on, off time)
//      if (sig_wx[i].indexOf("KT") > 0) Twinkle(i, 0x00, 0xff, 0xff, 1, 500, 500); //Gusts   Aqua
//      if (rem[i].indexOf("FZ") > 0)    Twinkle(i, 0x00, 0x00, 0x70, 3, 300, 400); //Freezing Blue
//      if (rem[i].indexOf("FG") > 0)    Twinkle(i, 0x30, 0x40, 0x26, 3, 400, 500); //Fog   L Yellow
//      if (rem[i].indexOf("BR") > 0)    Twinkle(i, 0x20, 0x50, 0x00, 3, 500, 500); //Mist  L Green
//      if (rem[i].indexOf("DZ") > 0)    Twinkle(i, 0x22, 0x50, 0x22, 3, 300, 300); //Drizzle L Green
  //    if (rem[i].indexOf("RA") > 0)    Twinkle(i, 0x22, 0xFC, 0x00, 5, 300, 300); //Rain    Green
  //    if (rem[i].indexOf("HZ") > 0)    Twinkle(i, 0x20, 0x20, 0x20, 3, 400, 400); //Haze    White/purple
  //    if (rem[i].indexOf("SN") > 0)    Twinkle(i, 0xaf, 0xaf, 0xaf, 4, 300, 600); //Snow    White
  //    if (rem[i].indexOf("SG") > 0)    Twinkle(i, 0xaf, 0xaf, 0xaf, 4, 300, 600); //Snow    White
  //    if (rem[i].indexOf("TS") > 0)    Twinkle(i, 0xff, 0xff, 0xff, 4,  10, 900); //Thunder White
  //    if (rem[i].indexOf("GS") > 0)    Twinkle(i, 0x88, 0x88, 0x00, 4, 100, 800); //S Hail  Yellow
  //    if (rem[i].indexOf("GR") > 0)    Twinkle(i, 0x88, 0x88, 0x00, 5, 100, 800); //L Hail  Yellow
  //    if (rem[i].indexOf("IC") > 0)    Twinkle(i, 0x00, 0x00, 0x80, 3, 300, 400); //Ice C   Blue
  //    if (rem[i].indexOf("PL") > 0)    Twinkle(i, 0x00, 0x00, 0xa0, 3, 300, 400); //Ice P   Blue
  //    if (rem[i].indexOf("VCSH") > 0)  Twinkle(i, 0x22, 0xfc, 0x00, 2, 300, 300); //Showers Green
  //    if (rem[i].indexOf("TCU") > 0)   Twinkle(i, 0x30, 0x30, 0x30, 2, 300, 300); //CU      Grey
  //    if (rem[i].indexOf("CB") > 0)    Twinkle(i, 0x30, 0x30, 0x40, 2, 300, 300); //CB      Grey
      //  Rest of Weather = YELLOW
  //    if (rem[i].indexOf("DU") > 0)    Twinkle(i, 0x88, 0x88, 0x00, 3, 500, 500); //Dust    Yellow
  //    if (rem[i].indexOf("FU") > 0)    Twinkle(i, 0x88, 0x88, 0x00, 3, 500, 500); //Smoke   Yellow
  //    if (rem[i].indexOf("FY") > 0)    Twinkle(i, 0x88, 0x88, 0x00, 3, 500, 500); //Spray   Yellow
  //    if (rem[i].indexOf("SA") > 0)    Twinkle(i, 0x88, 0x88, 0x00, 3, 500, 500); //Sand    Yellow
  //    if (rem[i].indexOf("PO") > 0)    Twinkle(i, 0x88, 0x88, 0x00, 3, 500, 500); //Dust&Sand Yellow
  //    if (rem[i].indexOf("SQ") > 0)    Twinkle(i, 0x88, 0x88, 0x00, 3, 500, 500); //Squalls Yellow
  //    if (rem[i].indexOf("VA") > 0)    Twinkle(i, 0x88, 0x88, 0x00, 3, 500, 500); //Volcanic Ash Yellow
  //    if (rem[i].indexOf("UP") > 0)    Twinkle(i, 0x88, 0x88, 0x00, 3, 500, 500); //Unknown Yellow
  //    // Notification of changes = ORANGE
  //    if (sig_wx[i] == "Info:")        Twinkle(i, 0xaf, 0x58, 0x00, 1, 200, 800); //Flash Orange
  //    Set_Cat_LED (i);  // Set Category for This Station LED
  //    FastLED.show();
  //  }
 // }
  delay(wait);
}
// ***********  Twinkle for Flashing Weather
void Twinkle (int index, byte red, byte green, byte blue, int pulses, int on_time, int off_time) {
  leds[index - 1].r = 0x00;    //  Red   Off
  leds[index - 1].g = 0x00;    //  Green Off
  leds[index - 1].b = 0x00;    //  Blue  Off
  FastLED.show();
  delay(100);    // Wait a smidgen
  for (int i = 0; i < pulses; i++) {
    leds[index - 1].r = red;   //  Red   On
    leds[index - 1].g = green; //  Green On
    leds[index - 1].b = blue;  //  Blue  On
    FastLED.show();
    delay(on_time);
    leds[index - 1].r = 0x00;  //  Red   Off
    leds[index - 1].g = 0x00;  //  Green Off
    leds[index - 1].b = 0x00;  //  Blue  Off
    FastLED.show();
    delay(off_time);
  }
  delay(300);    // Wait a little bit for flash button
}
// *********** Display ALL Categories
void Display_Cat_LEDS () {
  for (int i = 1; i < (No_Stations + 1); i++) {
    Set_Cat_LED (i);  // Set Category for This Station LED
  }
  FastLED.show();
  delay(200);        // Wait a smidgen
}
// *********** Set Category for One Station LED
void Set_Cat_LED (int i)  {
  if (category[i] == "NF" )  leds[i - 1] = CRGB(20, 20, 0); // DIM  Yellowish
  if (category[i] == "NA" )  leds[i - 1] = CRGB(20, 20, 0); // DIM  Yellowish
  if (category[i] == "VFR" ) leds[i - 1] = CRGB::DarkGreen;
  if (category[i] == "MVFR") leds[i - 1] = CRGB::DarkBlue;
  if (category[i] == "IFR" ) leds[i - 1] = CRGB::DarkRed;
  if (category[i] == "LIFR") leds[i - 1] = CRGB::DarkMagenta;
}
// *********** Display Visibility [Red Orange Pink/White]
void Display_Vis_LEDS (int wait) {
  for (int i = 1; i < (No_Stations + 1); i++) {
    int hue     =       visab[i] * 5; // (red yellow white)
    int sat     = 170 - visab[i] * 6;
    int bright  = 200 - visab[i] * 4;
    leds[i - 1] = CHSV(hue, sat, bright);
    if (category[i].substring(0, 1) == "N")  leds[i - 1] = CHSV( 0, 0, 0);
  }
  FastLED.show();
  delay(wait);
}
// *********** Display Winds [Aqua]
void Display_Wind_LEDS (int wait) {
  for (int i = 1; i < (No_Stations + 1); i++) {
    int Wind = wind[i].toInt();
    leds[i - 1] = CRGB(0, Wind * 7, Wind * 7);
    if (category[i].substring(0, 1) == "N" || wind[i] == "NA")  leds[i - 1] = CRGB(0, 0, 0);
  }
  FastLED.show();
  delay(wait);
}

// *********** Display Temperatures [Blue Green Yellow Orange Red]
void Display_Temp_LEDS (int wait) {
  for (int i = 1; i < (No_Stations + 1); i++) {
    //int hue = 176 - temp[i] * 4.2;     //  purple blue green yellow orange red
    int hue = 160 - temp[i] * 4;     //  purple blue green yellow orange red
    //Serial.println("Station=" + Stations[i].substring(0, 4) + "  Temp=" + String(temp[i]) + "  Hue=" + String(hue));
    leds[i - 1] = CHSV( hue, 180, 150);
    if (temp[i] == 0)  leds[i - 1] = CHSV( 0, 0, 0);
  }
  FastLED.show();
  delay(wait);
}
// *********** Display Altimeter Pressure [Blue Purple]
void Display_Alt_LEDS (int wait) {
  for (int i = 1; i < (No_Stations + 1); i++) {
    int hue = (altim[i] - 27.92) * 100;     //  blue purple
    leds[i - 1] = CHSV( hue - 30, 130, 150);
    if (category[i].substring(0, 1) == "N")   leds[i - 1] = CHSV( 0, 0, 0);
  }
  FastLED.show();
  delay(wait);
}

//  *********** Go_Server HTML  TASK 2
void Go_Server ( void * pvParameters ) {
  int diff_in_clouds = 2750;      // Significant CHANGE IN CLOUD BASE
  float diff_in_press  = 0.045;   // Significant CHANGE IN PRESSURE
  String header;                  // Header for Server
  int sta_n = 1;
  int wx_flag;
  int station_flag = 1;
  int summary_flag = 1;
  float TempF;
  String User;
  String Browser;

  for (;;) {
    WiFiClient client = server.available();   // Listen for incoming clients
    if (client) {                             // If a new client connects,
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
          //Serial.write(c);                  // print it out the serial monitor
          header += c;
          if (c == '\n') {                    // if the byte is a newline character
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              // HTTP headers always Start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-type:text/html"));
              client.println(F("Connection: close"));
              //client.println(F("Refresh: 60"));  // refresh the page automatically every 60 sec
              client.println();
               Serial.println(header);

              // Checking if new username / password was requested
              int search2 = header.indexOf("GET /get?new_username=");
              if (search2 >= 0) {
                String new_username = header.substring(search2 + 22, header.indexOf("&new_password="));
                String new_password = header.substring(header.indexOf("&new_password="));
                Serial.println("Updated credentials submitted, username " + new_username + ", password " + new_password);
                }                            
              
              // Checking if AIRPORT CODE was Entered
              int search = header.indexOf("GET /get?Airport_Code=");
              if (search >= 0) {
                String Airport_Code = header.substring(search + 22, search + 26);
                Airport_Code.toUpperCase();    // Changes all letters to UPPER CASE
                sta_n = -1;
                for (int i = 0; i < (No_Stations + 1); i++) {    // Check if Airport_Code is in Data base
                  if (Airport_Code == Stations[i].substring(0, 4))    sta_n = i;
                }
                if (sta_n == -1)  {            // Airport_Code NOT in Data base, Add as Station[0]
                  sta_n = 0;
                  Stations[sta_n] = Airport_Code.c_str();
                  cloud_base[sta_n] = 0;
                  altim[sta_n] = 0;
                  old_cloud_base[sta_n] = 0;
                  old_altim[sta_n] = 0;
                  //old_wdir[sta_n] = 0.0;
                  wdir[sta_n] = "";
                  rem[sta_n] = "";
                  GetData("NAME", 0);          // GET Some Metar /NAME
                  Decode_Name(sta_n);          // Decode Station NAME
                  GetData(Airport_Code, 0);    // GET Some Metar /DATA
                  ParseMetar(sta_n);           // Parse Metar DATA
                }
                summary_flag = 0;
                station_flag = 1;
              }
              // Checking which BUTTON was Pressed
              search = header.indexOf("GET /back HTTP/1.1");
              if (search >= 0) {
                sta_n = sta_n - 1;
                if (sta_n < 0)  sta_n = No_Stations;
                if (sta_n == 0 && Stations[0].substring(0, 4) == "NULL") sta_n = No_Stations;
                summary_flag = 0;
                station_flag = 1;
              }
              search = header.indexOf("GET /next HTTP/1.1");
              if (search >= 0) {
                sta_n = sta_n + 1;
                if (sta_n > No_Stations) sta_n = 0;
                if (sta_n == 0 && Stations[0].substring(0, 4) == "NULL") sta_n = 1;
                summary_flag = 0;
                station_flag = 1;
              }
              search = header.indexOf("GET /flash HTTP/1.1");
              if (search >= 0) {
                sta_n = station_num;             // From  Display Weather
                if (sta_n < 1 || sta_n > No_Stations) sta_n = 1;
                summary_flag = 0;
                station_flag = 1;
              }
              search = header.indexOf("GET /summary HTTP/1.1");
              if (search >= 0) {
                summary_flag = 1;
                station_flag = 1;
              }
              if (summary_flag == 1)  {
                // *********** DISPLAY SUMMARY ***********
                client.print(F("<!DOCTYPE html><html>"));     // Display the HTML web page
                // Responsive in any web browser, Header
                client.print(F("<HEAD><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"));
                client.print(F("<title>METAR</title>"));      // TITLE
                client.print(F("<style> html { FONT-family: Helvetica; display: inline-block; margin: 0px auto; text-align: left;}"));
                client.print(F("</style></HEAD>"));           // Closes Style & Header
                // Web Page Body
                client.print(F("<BODY><h2>METAR Summary</h2>"));
                client.print("Summary of Weather Conditions - Last Update : " + Last_Up_Time + " Zulu &nbsp&nbsp Next Update in " + String(Update_Data) + " Minutes<BR>");

                // Display SUMMARY Table ***********
                client.print(F("<TABLE BORDER='2' CELLPADDING='5'>"));
                String Deg = "Deg F";
                client.print("<TR><TD>No:</TD><TD>ID</TD><TD>CAT</TD><TD>SKY<BR>COVER</TD><TD>VIS<BR>Miles</TD><TD>WIND<BR>from</TD><TD>WIND<BR>Speed</TD><TD>TEMP<BR>" + Deg + "</TD><TD>ALT<BR>in&nbspHg</TD><TD>REMARKS</TD></TR>");
                for (int i = 0; i < (No_Stations + 1); i++) {
                  String color = "<TD><FONT COLOR=";
                  if (category[i] == "VFR" ) color = color + "'Green'>";
                  if (category[i] == "MVFR") color = color + "'Blue'>";
                  if (category[i] == "IFR" ) color = color + "'Red'>";
                  if (category[i] == "LIFR") color = color + "'Magenta'>";
                  if (category[i] == "NA")   color = color + "'Black'>";
                  if (category[i] == "NF")   color = color + "'Orange'>";

                  if (Stations[i].substring(0, 4) != "NULL")    {           // Display Station
                    if (i == sta_n )   {
                      client.print("<TR><TD BGCOLOR='Yellow'>" + String(i) + "</TD>");
                      client.print("<TD BGCOLOR='Yellow'>" + color.substring(4, color.length()) + Stations[i].substring(0, 4) + "</TD></FONT>");
                    } else {
                      client.print("<TR><TD>" + String(i) + "</TD>");                       // Display Station Number
                      client.print(color + Stations[i].substring(0, 4) + "</TD></FONT>");   // Display Station Id
                    }
                    client.print(color + category[i] + "</FONT></TD>");   // Display Category
                    if (sky[i].length() < 11)   {
                      client.print(color + sky[i]);                         // Display Sky Cover
                    } else {
                      if (old_cloud_base[i] > 0 && sig_wx[i].substring(0, 4) == "None") {
                        if (cloud_base[i] >= old_cloud_base[i] + diff_in_clouds)      // Significant INCREASE in Cloud Base
                          sig_wx[i] = "Info:";                                        // Triggers Orange Flashing & MistyRose
                        if (cloud_base[i] <= old_cloud_base[i] - diff_in_clouds)      // Significant DECREASE in Cloud Base
                          sig_wx[i] = "Info:";                                        // Triggers Orange Flashing & MistyRose
                      }
                      if (sig_wx[i] == "Info:") client.print(F("<TD BGCOLOR='MistyRose'><FONT COLOR='Purple'>"));  else  client.print(color);
                      client.print(sky[i].substring(0, 3) + "&nbspat<BR>" + cloud_base[i] + "&nbspFt&nbsp");
                      if (old_cloud_base[i] > 0) {
                        if (cloud_base[i]  > old_cloud_base[i])  client.print(F("&uArr;"));  //up arrow
                        if (cloud_base[i]  < old_cloud_base[i])  client.print(F("&dArr;"));  //down arrow
                        if (cloud_base[i] == old_cloud_base[i])  client.print(F("&rArr;"));  //right arrow
                      }
                    }
                    client.print(F("</FONT></TD>"));
                    client.print(color + String(visab[i]) + "</FONT></TD>");             // Display Visibility
                    client.print(color + wdir[i] + "</FONT></TD>");                      // Display Wind Direction
                    client.print(color + wind[i] + "</FONT></TD>");                      // Display Wind Speed
                    TempF = temp[i] * 1.8 + 32;  // deg F
                    if (temp[i] == 0 && dewpt[i] == 0)  client.print(color + "NA</FONT></TD>"); else if (Deg == "Deg C")   client.print(color + String(temp[i], 1) + "</FONT></TD>"); else
                      client.print(color + String(TempF, 1) + "</FONT></TD>");
                    wx_flag = 0;
                    if (old_altim[i] > 0.1)   {
                      if (altim[i] >= old_altim[i] + diff_in_press)  wx_flag = 1;  // Significant INCREASE in Pressure
                      if (altim[i] <= old_altim[i] - diff_in_press)  wx_flag = 1;  // Significant DECREASE in Pressure
                      if (wx_flag == 1) client.print("<TD BGCOLOR='MistyRose'><FONT COLOR='Purple'>" + String(altim[i]));
                      else  client.print(color + String(altim[i]));
                      if (altim[i] > old_altim[i])   client.print(F("<BR>&uArr;")); //up arrow
                      if (altim[i] < old_altim[i])   client.print(F("<BR>&dArr;")); //down arrow
                      if (altim[i] == old_altim[i])  client.print(F("<BR>&rArr;")); //right arrow
                    }  else  {
                      client.print(color + String(altim[i]));
                    }
                    client.print(F("</FONT></TD>"));                                    // Display Remarks
                    if (rem[i].substring(0, 3) == "new")  client.print(F("<TD><FONT COLOR='Purple'>"));  else  client.print(color);
                    client.print(rem[i]);
                    if (comment[i].length() > 1)   client.print("<BR>" + comment[i]);   // Display Comment
                    client.print("</FONT></TD></TR>");
                  }
                }
                client.print(F("</TABLE>"));
              }
              if (station_flag == 1)  {

                // *********** DISPLAY STATION  ***********
                client.print(F("<!DOCTYPE html><html>"));    // Display the HTML web page
                // Responsive in any web browser.
                client.print(F("<HEAD><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"));
                client.print(F("<title>METAR</title>"));    // TITLE
                client.print(F("<style> html { FONT-family: Helvetica; display: inline-block; margin: 0px auto; text-align: left;}"));
                client.print(F("</style></HEAD>"));         // Closes Style & Header
                // Web Page Body
                client.print(F("<BODY><h2>METAR Station</h2>"));
              }
              if (station_flag == 1 || summary_flag == 1)  {
                client.print(F("<P>"));
                client.print("For&nbsp:&nbsp" +  Stations[sta_n] + "&nbsp&nbsp#&nbsp&nbsp" + String(sta_n) + "<BR>");
                // Display BUTTONS: the ESP32 receives a request in the header ("GET /back HTTP/1.1")
                client.print(F("<a href=\'/back\'><INPUT TYPE=button VALUE='Previous Station' onClick= sta_n></a>"));
                client.print(F("<a href=\'/flash\'><INPUT TYPE=button VALUE='Flashing LED' onClick= sta_n></a>"));
                client.print(F("<a href=\'/next\'><INPUT TYPE=button VALUE='Next Station' onClick= sta_n></a>"));
                client.print(F("&nbsp&nbsp&nbsp&nbsp<a href=\'/summary\'><INPUT TYPE=button VALUE='Summary of Stations' onClick= sta_n></a>"));
                client.print(F("<BR>&nbsp&nbsp&nbsp&nbspPress <B>'BUTTON'</B> above, when <B>LED is Flashing</B><BR>"));
                // Display TABLE/FORM: the ESP32 receives a request in the header ("GET /get?Airport_Code=")
                client.print(F("<TABLE BORDER='0' CELLPADDING='1'>"));
                client.print(F("<TR><TD>ENTER AIRPORT ID CODE:</TD><TD>"));
                client.print(F("<FORM action='/get' METHOD='get'>"));
                client.print(F("<INPUT TYPE='text' NAME='Airport_Code' SIZE='5'>"));
                client.print(F("</FORM></TD></TR></TABLE>"));
                Display_LED (sta_n, 300);  // Display One Station LED
                Update_Time();             //  GET Current Time
                String C_Time = "Current Time : " + Time + " Zulu&nbsp&nbsp&nbsp&nbsp&nbsp&nbspNext Update in less than 0 Minutes<BR>";
                int upd = (Last_Up_Min + Update_Data) - Minute;
                if (Last_Up_Min + Update_Data > 60) upd = 60 - Minute;
                if (upd > 0 && upd < Update_Data + 1)  {
                  C_Time.replace("than 0", "than " + String(upd));
                  if (upd == 1)  C_Time.replace("Minutes", "Minute");
                }
                client.print(C_Time);
                // Display STATION Table  ***********
                String Bcol = "BORDERCOLOR=";
                if (category[sta_n] == "VFR" ) Bcol = Bcol + "'Green'";
                if (category[sta_n] == "MVFR") Bcol = Bcol + "'Blue'";
                if (category[sta_n] == "IFR" ) Bcol = Bcol + "'Red'";
                if (category[sta_n] == "LIFR") Bcol = Bcol + "'Magenta'";
                if (category[sta_n] == "NA")   Bcol = Bcol + "'Black'";
                if (category[sta_n] == "NF")   Bcol = Bcol + "'Orange'";
                String color = "<TD><FONT " + Bcol.substring(6, Bcol.length()) + ">";
                client.print("<TABLE " + Bcol + " BORDER='3' CELLPADDING='5'>");
                client.print("<TR><TD>Flight Category</TD>" + color + "<B>" + category[sta_n] + "</B></FONT>  for " + Stations[sta_n]  + "</TD></TR>");
                client.print(F("<TR><TD>Remarks</TD>"));
                if (rem[sta_n].substring(0, 3) == "new" ) client.print("<TD><FONT COLOR='Purple'>" + rem[sta_n] + "</FONT>"); else client.print("<TD>" + rem[sta_n]);
                client.print(F("</TD></TR>"));

                client.print("<TR><TD>Significant Weather</TD>" + color + sig_wx[sta_n] + "</FONT>");

                //  Comments for Weather and Cloud Base
                if (sky[sta_n].substring(0, 3) == "OVC")
                  client.print("<BR><FONT " + Bcol.substring(6, Bcol.length()) + ">Overcast Cloud Layer</FONT>");
                if (cloud_base[sta_n] > 0 && cloud_base[sta_n] <= 1200)
                  client.print("<BR><FONT " + Bcol.substring(6, Bcol.length()) + ">Low Cloud Base</FONT>");
                if (old_cloud_base[sta_n] > 0 && old_altim[sta_n] > 0)    {
                  if (cloud_base[sta_n] >= old_cloud_base[sta_n] + diff_in_clouds) // INCREASE
                    client.print(F("<BR><FONT COLOR='Orange'>Significant Increase in Cloud Base</FONT>"));
                  if (cloud_base[sta_n] <= old_cloud_base[sta_n] - diff_in_clouds) // DECREASE
                    client.print(F("<BR><FONT COLOR='Orange'>Significant Decrease in Cloud Base</FONT>"));
                  if (cloud_base[sta_n] > old_cloud_base[sta_n] && altim[sta_n] > old_altim[sta_n])
                    client.print(F("<BR><FONT COLOR='Purple'>Weather is Getting Better</FONT>"));
                  if (cloud_base[sta_n] < old_cloud_base[sta_n] && altim[sta_n] < old_altim[sta_n])
                    client.print(F("<BR><FONT COLOR='Blue'>Weather is Getting Worse</FONT>"));
                  if (sig_wx[sta_n] == "Info:")   sig_wx[sta_n] = "Info";          // Reset Orange Flashing
                }

                if (comment[sta_n].length() > 1)   // Comment
                  client.print("<BR><FONT COLOR='Purple'>" + comment[sta_n] + "</FONT>");

                client.print("</TD></TR><TR><TD>Sky Cover</TD>" + color);
                if (sky[sta_n].length() > 11)  {
                  client.print(sky[sta_n].substring(0, 3) + "&nbspClouds&nbsp" + sky[sta_n].substring(4, sky[sta_n].length()) + "&nbsp&nbsp&nbsp");
                  if (old_cloud_base[sta_n] > 0)   {
                    if (cloud_base[sta_n] > old_cloud_base[sta_n])  {
                      client.print(F("&uArr; from ")); //up arrow
                      client.print(old_cloud_base[sta_n]);
                    }
                    if (cloud_base[sta_n] < old_cloud_base[sta_n])  {
                      client.print(F("&dArr; from ")); //down arrow
                      if (old_cloud_base[sta_n] > 99998)  client.print("CLEAR"); else
                        client.print(old_cloud_base[sta_n]);
                    }
                    if (cloud_base[sta_n] == old_cloud_base[sta_n]) {
                      client.print(F("&rArr; no change")); //right arrow
                    }
                  }
                }  else  client.print(sky[sta_n]);
                client.print(F("</FONT></TD></TR>"));

                client.print("<TR><TD>Visibility</TD>" + color + visab[sta_n] + " Statute miles</FONT></TD></TR>");

                client.print(F("<TR><TD>Wind from</TD><TD>"));
                int newdir = wdir[sta_n].toInt();
                int olddir = old_wdir[sta_n];
                if (wind[sta_n] == "CALM")  client.print(wind[sta_n]);
                else client.print(wdir[sta_n] + " Deg at " + wind[sta_n]);
                if (newdir != 0 && olddir != 0) {
                  if (newdir == olddir)  client.print(" : no change");
                  if (newdir > olddir + 30 || newdir < olddir - 30)
                    client.print("<FONT COLOR='Orange'> : Significant Change from " + String(olddir) + " Deg</FONT>");
                  else if (newdir != olddir)  client.print(" : previously " + String(olddir) + " Deg");
                }
                client.print(F("</TD></TR>"));

                TempF = temp[sta_n] * 1.8 + 32;  // deg F
                client.print(F("<TR><TD>Temperature</TD><TD>"));
                if (temp[sta_n] <= 0) client.print(F("<FONT COLOR='Blue'>"));  else  client.print(F("<FONT COLOR='Black'>"));
                if (temp[sta_n] == 0 && dewpt[sta_n] == 0)  client.print(F("NA</FONT>")); else
                  client.print(String(temp[sta_n], 1) + " Deg C&nbsp&nbsp&nbsp:&nbsp&nbsp&nbsp" + String(TempF, 1) + " Deg F</FONT>");
                if (TempF >= 95.0)  client.print(F("<FONT SIZE='-1' FONT COLOR='Red'><I> and Too HOT</I></FONT>"));
                client.print(F("</TD></TR><TR><TD>Dew Point</TD><TD>"));
                if (dewpt[sta_n] <= 0) client.print(F("<FONT COLOR='Blue'>"));  else  client.print(F("<FONT COLOR='Black'>"));
                if (dewpt[sta_n] == 0)  client.print(F("NA")); else
                  client.print(String(dewpt[sta_n], 1) + " Deg C</TD></TR>");
                client.print("</FONT><TR><TD>Altimeter</TD><TD>" + String(altim[sta_n]) + "&nbspin&nbspHg&nbsp&nbsp");
                if (old_altim[sta_n] > 0)  {
                  if (altim[sta_n] > old_altim[sta_n]) {
                    if (altim[sta_n] > old_altim[sta_n] + diff_in_press)  client.print(F("<FONT COLOR='Orange'>Significant&nbsp</FONT>"));
                    client.print(F("&uArr; from ")); //up arrow
                    client.print(old_altim[sta_n], 2);
                  }
                  if (altim[sta_n] < old_altim[sta_n]) {
                    if (altim[sta_n] < old_altim[sta_n] - diff_in_press)  client.print(F("<FONT COLOR='Orange'>Significant&nbsp</FONT>"));
                    client.print(F("&dArr; from ")); //down arrow
                    client.print(old_altim[sta_n], 2);
                  }
                  if (altim[sta_n] == old_altim[sta_n])  client.print(F("&rArr; Steady")); //right arrow
                }
                client.print(F("</TD></TR>"));
                float Elevation = elevation[sta_n] * 3.28; //feet
                client.print("<TR><TD>Elevation</TD><TD>" + String(elevation[sta_n], 1) + " m&nbsp&nbsp&nbsp:&nbsp&nbsp&nbsp" + String(Elevation, 1) + " Ft</TD></TR>");
                float PressAlt = Elevation + (1000 * (29.92 - altim[sta_n])); //feet
                float DensityAlt = PressAlt + (120 * (temp[sta_n] - (15 - abs(2 * Elevation / 1000)))); // feet
                client.print(F("<TR><TD>Estimated Density Altitude</TD><TD>"));
                if (temp[sta_n] == 0 || altim[sta_n] == 0)  client.print(F("NA")); else client.print(String(DensityAlt, 1) + " Ft</TD></TR>");
                client.print(F("</TABLE>"));

                //add NAIPS username / password fields, for updating NAIPS credentials
                client.print(F("<BR><TABLE BORDER='0' CELLPADDING='1'>"));
                client.print(F("<TR><TD>Update NAIPS credentials (username / password):</TD><TD>"));
                client.print(F("<FORM action='/get' METHOD='get'>"));
                client.print(F("<INPUT TYPE='text' NAME='new_username' SIZE='5'>"));
                client.print(F("<INPUT TYPE='text' NAME='new_password' SIZE='5'>"));
                //client.print(F("&nbsp&nbsp&nbsp&nbsp<a href=\'/?newuser=" + new_username + "&newpass=" + new_password + "\'><INPUT TYPE=button VALUE='Submit' onClick= updateauth></a>"));
                client.print(F("</FORM></TD></TR></TABLE>"));                

                client.print("<BR><FONT SIZE='-1'>File Name: " + String(FileName));
                client.print("<BR>H/w Address : " + local_hwaddr);
                client.print("<BR>Url Address : " + local_swaddr);
                client.print(F("<BR><B>Dedicated to: F. Hugh Magee</B>"));
                client.print(F("</FONT></BODY></html>"));
              }
              client.println();    // The HTTP response ends with another blank line
              break;               // Break out of the while loop
            } else {               // if you got a newline, then clear currentLine
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }
        }                          // Client Not Available
      }                            // Client Disconnected
      header = "";                 // Clear the header variable
      client.stop();               // Close the connection
    }
  }
}
