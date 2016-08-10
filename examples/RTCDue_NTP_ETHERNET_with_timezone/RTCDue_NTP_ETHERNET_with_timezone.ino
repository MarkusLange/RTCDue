/*
  Simple NTP Ethernet Example for Arduino Due

  Demonstrates the use of the RTC library for the Arduino Due

  This example code is in the public domain

  created by Markus Lange
  10 Feb 2016
*/

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <TimeLib.h>        //http://www.arduino.cc/playground/Code/Time
#include <Timezone.h>       //https://github.com/JChristensen/Timezone modified for the Due https://github.com/MarkusLange/Timezone
#include <RTCDue.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

unsigned int localPort = 2390;        // local port to listen for UDP packets

IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server

const int NTP_PACKET_SIZE = 48;       // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[NTP_PACKET_SIZE];   // buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

// Select the Slowclock source
//RTCDue rtc(RC);
RTCDue rtc(XTAL);

const char* daynames[]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
const unsigned long seventyYears = 2208988800UL;
unsigned long actual_unixtime;

//Australia Eastern Time Zone (Sydney, Melbourne)
TimeChangeRule aEDT = {"AEDT", First, Sun, Oct, 2, 660};    //UTC + 11 hours
TimeChangeRule aEST = {"AEST", First, Sun, Apr, 3, 600};    //UTC + 10 hours
Timezone ausET(aEDT, aEST);

//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET  = {"CET ", Last, Sun, Oct, 3, 60};      //Central European Standard Time
Timezone CE(CEST, CET);

//United Kingdom (London, Belfast)
TimeChangeRule BST = {"BST", Last, Sun, Mar, 1, 60};        //British Summer Time
TimeChangeRule GMT = {"GMT", Last, Sun, Oct, 2, 0};         //Standard Time
Timezone UK(BST, GMT);

//US Eastern Time Zone (New York, Detroit)
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  //Eastern Daylight Time = UTC - 4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   //Eastern Standard Time = UTC - 5 hours
Timezone usET(usEDT, usEST);

//US Central Time Zone (Chicago, Houston)
TimeChangeRule usCDT = {"CDT", Second, dowSunday, Mar, 2, -300};
TimeChangeRule usCST = {"CST", First, dowSunday, Nov, 2, -360};
Timezone usCT(usCDT, usCST);

//US Mountain Time Zone (Denver, Salt Lake City)
TimeChangeRule usMDT = {"MDT", Second, dowSunday, Mar, 2, -360};
TimeChangeRule usMST = {"MST", First, dowSunday, Nov, 2, -420};
Timezone usMT(usMDT, usMST);

//Arizona is US Mountain Time Zone but does not use DST
Timezone usAZ(usMST, usMST);

//US Pacific Time Zone (Las Vegas, Los Angeles)
TimeChangeRule usPDT = {"PDT", Second, dowSunday, Mar, 2, -420};
TimeChangeRule usPST = {"PST", First, dowSunday, Nov, 2, -480};
Timezone usPT(usPDT, usPST);

TimeChangeRule *tcr;                  // pointer to the time change rule, use to get the TZ abbrev
time_t utc;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  // start Ethernet and UDP
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forever more:
    while (1) {
      Serial.println("Failed to configure Ethernet using DHCP");

      // wait 10 seconds for connection:
      delay(10000);
    }
  }
  
  Serial.println("Connected to ethernet");
  printEthernetStatus();
  
  Serial.println("Starting connection to server...");
  Udp.begin(localPort);
  recieveNTP();
  
  rtc.begin();
  rtc.setClock(actual_unixtime);
  
  setSyncProvider(getArduinoDueTime);
}

time_t getArduinoDueTime() {
  return rtc.unixtime();
}

void loop() {
  Serial.println();
  utc = now();
  printTime(ausET.toLocal(utc, &tcr), tcr -> abbrev, "Sydney");
  printTime(CE.toLocal(utc, &tcr), tcr -> abbrev, "Frankfurt, Paris");
  printTime(UK.toLocal(utc, &tcr), tcr -> abbrev, " London");
  printTime(utc, "UTC", " Universal Coordinated Time");
  printTime(usET.toLocal(utc, &tcr), tcr -> abbrev, " New York");
  printTime(usCT.toLocal(utc, &tcr), tcr -> abbrev, " Chicago");
  printTime(usMT.toLocal(utc, &tcr), tcr -> abbrev, " Denver");
  printTime(usAZ.toLocal(utc, &tcr), tcr -> abbrev, " Phoenix");
  printTime(usPT.toLocal(utc, &tcr), tcr -> abbrev, " Los Angeles");
  delay(2000);
}

void printTime(time_t t, const char *tz, const char *loc) {
  digitprint(hour(t), 2);
  Serial.print(":");
  digitprint(minute(t), 2);
  Serial.print(":");
  digitprint(second(t), 2);
  Serial.print(" ");
  Serial.print(dayShortStr(weekday(t)));
  Serial.print(": ");
  digitprint(day(t), 2);
  Serial.print(".");
  digitprint(month(t), 2);
  Serial.print(".");
  Serial.print(year(t));
  Serial.print(" ");
  Serial.print(tz);
  Serial.print(" ");
  Serial.print(loc);
  Serial.println();
}

void recieveNTP(){
  sendNTPpacket(timeServer); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  Serial.println( Udp.parsePacket() );
  if ( Udp.parsePacket() ) {
    Serial.println("packet received");
		
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    
    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord  = word(packetBuffer[42], packetBuffer[43]);
    
		// combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    actual_unixtime = secsSince1900 - seventyYears;
  }
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address){
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0]  = 0b11100011;  // LI, Version, Mode
  packetBuffer[1]  = 0;           // Stratum, or type of clock
  packetBuffer[2]  = 6;           // Polling Interval
  packetBuffer[3]  = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void digitprint(int value, int lenght){
  for (int i = 0; i < (lenght - numdigits(value)); i++){
    Serial.print("0");
  }
  Serial.print(value);
}

int numdigits(int i){
  int digits;
  if (i < 10)
    digits = 1;
  else
    digits = (int)(log10((double)i)) + 1;
  return digits;
}

void printEthernetStatus(){
  // print your local IP address:
  Serial.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
}