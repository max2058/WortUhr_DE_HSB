/*
 * Projekt: WortUhr / WordClock 
 * Ersterstellung: 10.01.2022
 *
 * Version: 1.3.0.6
 * Aenderungsdatum: 26.12.2022
 * Autor: Auerbach Maximilian - max.auerbach@gmx.net  
 * 
 * Changelog:
 * --- Bugfixes
 * - Fehler Stundenzahl erhöhen ab HALB
 * - Fehler Dimmwert für BLAU berechnen
 * - Fehler Sonderfall EIN/EINS
 * - Fehler Issue #2 unklare sporadische Zerstörung des ESP8266. (siehe GitHub)
 * --- Neuerungen
 * + Ergänzung Ost-Variante ("viertel vor" vs. "dreiviertel") User kann bei Inbetriebnahme (über DNS Server) auswahl treffen.
 * 
 *
*/

//Externe Bibliotheken
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include "Parameter.h"
#include "HTML.h"



//******************************************************** PARAMETER ********************************************************
//********************************************************** WS2812B ********************************************************

//Pin des ESP8266
const uint16_t Pin = D6;

//Bitte auswählen, welcher WordClock-Typ verwendet wird
// TYPE  0  => für eine WordClock ohne Minutenanzeige
// TYPE  1  => für eine WordClock mit Minutenanzeige
#define TYPE 1

//WordClock ohne Minutenanzeige
#if TYPE == 0
  bool WordClockType = LOW;
  //Anzahl der LED's
  const uint16_t Numbers = 110;
#endif

//WordClock mit Minutenanzeige
#if TYPE == 1
  bool WordClockType = HIGH;
  //Anzahl der LED's
  const uint16_t Numbers = 114;
#endif

//***************************************************************************************************************************
//*********************************************** ACCESS POINT & DNS SERVER *************************************************

//Zugangsdaten Access Point
//SSID
const char SSID_AccessPoint[] = "WortUhr";
//Passwort
const char Password_AccessPoint[] = "wortuhr123";
//URL DNS Server
//www.wortuhr.local
const char WebserverURL[] = "www.wortuhr.local";

//***************************************************************************************************************************
//******************************************* FARBWERTE UND HELLIGKEIT DER LED's ********************************************

//Beispiel: RGB-Farbschema Rot = 50; Grün = 50; Blau = 50 => ergibt einen weißen Farbton.
//Tipp: Der blaue Anteil muss bei den WS2812B-LED's etwas reduziert werden, da dieser dominanter ausfällt.
//Helligkeit der LED's kann mit Werten zwischen 0 - 255 eingestellt werden.
//Beispiel 1: Rot = 255;  Grün = 255; Blau = 255 => weißer Farbton mit maximaler Helligkeit aller LED's
//Beispiel 2: Rot = 0;    Grün = 0;   Blau = 255 => blauer Farbton mir maximaler Helligkeit der blauen LED
//Beispiel 3: Rot = 127;  Grün = 127; Blau = 127 => weißer Farbton mit 50% der Helligkeit aller LED's

//Farbwert ROT
const unsigned int RGBValueRed = 60;
//Farbwert GRÜN
const unsigned int RGBValueGreen = 60;
//Farbwert BLAU
const unsigned int RGBValueBlue = 50;


//***************************************************************************************************************************
//******************************************************** Modi *************************************************************
//******************************************************** Ost Oder West ****************************************************

//Variable für Ost Oder West - im Verlauf:
// TYPE  0  => für (viertel Vor Drei, viertel nach zwei)
// TYPE  1  => für (dreivirtel drei, viertel drei)
unsigned int EastWest;

//******************************************************** NACHTMODUS *******************************************************
//Display kann in einem definiertem Zeitfenster (24h-Format) ausgeschalten werden.
//Enable Display On/Off => Freigabe der Funktion
//Display Off => Uhrzeit wann das Display wieder ausgeschaltet werden soll
//Display On => Uhrzeit wann das Display wieder eingeschaltet werden soll

//Freigabe der Funktion
unsigned int EnableNightMode = 0;
//Uhrzeit Display ausschalten
unsigned int TimeDisplayOff = 0;
//Uhrzeit Display einschalten
unsigned int TimeDisplayOn = 0;


//***************************************************************************************************************************
//***************************************************** LOKALE UHRZEIT ******************************************************

//Wird die Uhr ohne eine WLAN-Verbindung gestartet, kann mit den folgenden Parametern eine Uhrzeit eingestellt werden.
//Die Uhrzeit wird als UTC-Zeit eingestellt. Sommerzeit (UTC+2) oder Winterzeit (UTC+1) können eingestellt werden.

//Stundenzahl in UTC (24h-Format)
const unsigned int localutcaktstunde = 11;
//Minutenzahl in UTC
const unsigned int localutcaktminute = 0;
//Sekundenzahl in UTC
const unsigned int localutcaktsekunde = 0;
//Einstellung Sommer-/Winterzeit
const unsigned int localutcx = 1;

//***************************************************************************************************************************
//******************************************************** DIAGNOSE *********************************************************

//Serielle Schnittstelle für die Diagnose öffnen
const bool debug = LOW;

//***************************************************************************************************************************
//******************************************************* KONSTANTEN ********************************************************

//Portnummern
const unsigned int localPort = 2390;
const unsigned int DNSPort = 53;
const unsigned int HTTPPort = 80;

//Adresse des NTP-Serverpools
const char* NtpServerName = "de.pool.ntp.org";

//HTTP Status Codes
const int HTTPRequest = 200;
const int HTTPPost = 303;
const int HTTPNotFound = 404;

//Größe des NTP-Datenpaket
const unsigned int NtpPaketSize = 48;
byte NtpPaketBuffer[NtpPaketSize];

//Größe des UserInterface-Datenpaket
const unsigned int UserInterfacePaketSize = 12;
byte UserInterfacePaketBuffer[UserInterfacePaketSize];

//ID-Nummern
const int UdpIdHandshakeRequest = 100;
const int UdpIdHandshakeAnswer = 101;
const int UdpIdRgb = 102;
const int UdpIdTime = 103;
const int UdpIdDate = 104;
const int UdpIdDisplay = 105;

//Zeitintervalle in Millisekunden
const unsigned long secondperday = 86400;
const unsigned long secondperhour = 3600;
const unsigned long secondperminute = 60;
const unsigned long minuteperhour = 60;
const unsigned long millisecondpersecond = 1000;
const unsigned long IntervallUdp = 20;
const unsigned long IntervallDebug = 10000;
const unsigned long IntervallFade = 3;


//Anzahl der Tage im Monat
unsigned int tageImMonat[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };


//Adresse der ersten und letzten LED
const int ersteLED = 0;
const int letzteLED = Numbers - 1;


//Faktor zur Berechnung der Stromaufnahme
const float currentPerRgbValue = 0.0795;


//Größe der Adressbereiche
const int NumberOfHours = 12;
const int NumberOfMinute = 5;
const int NumberOfAddMinutes = 4;
const int NumberOfSpecialChar = 2;


//Adressen der Stunden
const int HourAddr[NumberOfHours][2] = { { 49, 53 },    //Zwölf
                                         { 60, 63 },    //Eins
                                         { 62, 65 },    //Zwei
                                         { 67, 70 },    //Drei
                                         { 77, 80 },    //Vier
                                         { 73, 76 },    //Fünf
                                         { 104, 108 },  //Sechs
                                         { 55, 60 },    //Sieben
                                         { 89, 92 },    //Acht
                                         { 81, 84 },    //Neun
                                         { 93, 96 },    //Zehn
                                         { 85, 87 } };  //Elf

//Adressen der Minuten
const int MinuteAddr[NumberOfMinute][2] = { { 7, 10 },     //Fünf 0
                                            { 18, 21 },    //Zehn 1
                                            { 26, 32 },    //Viertel 2
                                            { 11, 17 },    //Zwanzig 3
                                            { 22, 32 } };  //Dreiviertel 4

//Adressen Vor/Nach
const int ToPastAddr[NumberOfSpecialChar][2] = { { 38, 41 },    //Nach
                                                 { 35, 37 } };  //Vor

//Adressen Halb/Uhr
const int HalfFullHourAddr[NumberOfSpecialChar][2] = { { 44, 47 },     //Halb
                                                       { 99, 101 } };  //Uhr

//Adressen Es/Ist
const int EsIstAddr[NumberOfSpecialChar][2] = { { 0, 1 },    //Es 0
                                                { 3, 5 } };  //Ist 1

//Adressen der Minutenanzeige
const int AddMinutesAddr[NumberOfAddMinutes][2] = { { 110, 110 },    //Minute +1
                                                    { 111, 111 },    //Minute +2
                                                    { 112, 112 },    //Minute +3
                                                    { 113, 113 } };  //Minute +4

//Zeittabelle mit Takt +2/-2 Minuten
#if TYPE == 0
int TimeTable[60][4] = { { 0, 0, 0, 2 }, { 1, 0, 0, 2 }, { 2, 0, 0, 2 }, { 3, 1, 1, 0 }, { 4, 1, 1, 0 }, { 5, 1, 1, 0 }, { 6, 1, 1, 0 }, { 7, 1, 1, 0 }, { 8, 2, 1, 0 }, { 9, 2, 1, 0 }, { 10, 2, 1, 0 }, { 11, 2, 1, 0 }, { 12, 2, 1, 0 }, { 13, 3, 1, 0 }, { 14, 3, 1, 0 }, { 15, 3, 1, 0 }, { 16, 3, 1, 0 }, { 17, 3, 1, 0 }, { 18, 4, 1, 0 }, { 19, 4, 1, 0 }, { 20, 4, 1, 0 }, { 21, 4, 1, 0 }, { 22, 4, 1, 0 }, { 23, 1, 2, 1 }, { 24, 1, 2, 1 }, { 25, 1, 2, 1 }, { 26, 1, 2, 1 }, { 27, 1, 2, 1 }, { 28, 0, 0, 1 }, { 29, 0, 0, 1 }, { 30, 0, 0, 1 }, { 31, 0, 0, 1 }, { 32, 0, 0, 1 }, { 33, 1, 1, 1 }, { 34, 1, 1, 1 }, { 35, 1, 1, 1 }, { 36, 1, 1, 1 }, { 37, 1, 1, 1 }, { 38, 4, 2, 0 }, { 39, 4, 2, 0 }, { 40, 4, 2, 0 }, { 41, 4, 2, 0 }, { 42, 4, 2, 0 }, { 43, 3, 2, 0 }, { 44, 3, 2, 0 }, { 45, 3, 2, 0 }, { 46, 3, 2, 0 }, { 47, 3, 2, 0 }, { 48, 2, 2, 0 }, { 49, 2, 2, 0 }, { 50, 2, 2, 0 }, { 51, 2, 2, 0 }, { 52, 2, 2, 0 }, { 53, 1, 2, 0 }, { 54, 1, 2, 0 }, { 55, 1, 2, 0 }, { 56, 1, 2, 0 }, { 57, 1, 2, 0 }, { 58, 0, 0, 2 }, { 59, 0, 0, 2 } };
#endif

//Zeittabelle mit Takt volle 5 Minuten
#if TYPE == 1
int TimeTable[60][4] = { { 0, 0, 0, 2 }, { 1, 0, 0, 2 }, { 2, 0, 0, 2 }, { 3, 0, 0, 2 }, { 4, 0, 0, 2 }, { 5, 1, 1, 0 }, { 6, 1, 1, 0 }, { 7, 1, 1, 0 }, { 8, 1, 1, 0 }, { 9, 1, 1, 0 }, { 10, 2, 1, 0 }, { 11, 2, 1, 0 }, { 12, 2, 1, 0 }, { 13, 2, 1, 0 }, { 14, 2, 1, 0 }, { 15, 3, 1, 0 }, { 16, 3, 1, 0 }, { 17, 3, 1, 0 }, { 18, 3, 1, 0 }, { 19, 3, 1, 0 }, { 20, 4, 1, 0 }, { 21, 4, 1, 0 }, { 22, 4, 1, 0 }, { 23, 4, 1, 0 }, { 24, 4, 1, 0 }, { 25, 1, 2, 1 }, { 26, 1, 2, 1 }, { 27, 1, 2, 1 }, { 28, 1, 2, 1 }, { 29, 1, 2, 1 }, { 30, 0, 0, 1 }, { 31, 0, 0, 1 }, { 32, 0, 0, 1 }, { 33, 0, 0, 1 }, { 34, 0, 0, 1 }, { 35, 1, 1, 1 }, { 36, 1, 1, 1 }, { 37, 1, 1, 1 }, { 38, 1, 1, 1 }, { 39, 1, 1, 1 }, { 40, 4, 2, 0 }, { 41, 4, 2, 0 }, { 42, 4, 2, 0 }, { 43, 4, 2, 0 }, { 44, 4, 2, 0 }, { 45, 3, 2, 0 }, { 46, 3, 2, 0 }, { 47, 3, 2, 0 }, { 48, 3, 2, 0 }, { 49, 3, 2, 0 }, { 50, 2, 2, 0 }, { 51, 2, 2, 0 }, { 52, 2, 2, 0 }, { 53, 2, 2, 0 }, { 54, 2, 2, 0 }, { 55, 1, 2, 0 }, { 56, 1, 2, 0 }, { 57, 1, 2, 0 }, { 58, 1, 2, 0 }, { 59, 1, 2, 0 } };
#endif

//***************************************************************************************************************************
//******************************************************** VARIABLEN ********************************************************

int i = 0;
int z = 0;
int x = 0;
int y = 0;

unsigned long newsecsSince1900 = 0;
unsigned long oldsecsSince1900 = 0;

unsigned int oldstunde = 99;
unsigned int oldmezstunde = 0;
unsigned int oldminute = 99;

unsigned int utcaktstunde = 0;
unsigned int utcaktminute = 0;
unsigned int utcaktsekunde = 0;

unsigned int mezaktstunde = 0;
unsigned int mezaktstunde24 = 0;
unsigned int mezaktminute = 0;
unsigned int mezaktsekunde = 0;

unsigned long newtageSeit1900 = 0;
unsigned long oldtageSeit1900 = 0;
unsigned int newschaltTage = 0;

unsigned int wochenTag = 0;

unsigned int datumTag = 0;
unsigned int datumMonat = 0;
unsigned int datumJahr = 0;

unsigned int tagNummer = 0;

unsigned int utcx = 0;

unsigned int oldschaltTage = 0;
bool schaltJahr = LOW;
bool sonderFall366 = LOW;

unsigned long lastmillisDebug = 0;
unsigned long lastmillisSecond = 0;
unsigned long lastmillisdifferenz = 0;
unsigned long lastmillisFade = 0;
unsigned long LastMillisUdp = 0;

unsigned int UdpId = 0;

unsigned int UdpHour = 0;
unsigned int UdpMinute = 0;
unsigned int UdpSecond = 0;

unsigned int UdpDay = 0;
unsigned int UdpMonth = 0;
unsigned int UdpYear = 0;

unsigned int red = 0;
unsigned int green = 0;
unsigned int blue = 0;

float newredminus = 0.0;
float newgreenminus = 0.0;
float newblueminus = 0.0;

float newredplus = 0.0;
float newgreenplus = 0.0;
float newblueplus = 0.0;

float fadevaluered = 1.0;
float fadevaluegreen = 1.0;
float fadevalueblue = 1.0;

unsigned int currentPerLed = 0;

unsigned int pointer = 0;

unsigned int newletter[Numbers];
unsigned int showletter[Numbers];
unsigned int changeletter[Numbers];

bool newdisplay = LOW;
bool setDisplayOff = LOW;

bool NoWifi = LOW;

//***************************************************************************************************************************

//IP-Adressen
IPAddress TimeServerIP;
IPAddress UserInterfaceIP;
//IP Adressen Access Point
IPAddress IPAdress_AccessPoint(192, 168, 10, 2);
IPAddress Gateway_AccessPoint(192, 168, 10, 0);
IPAddress Subnetmask_AccessPoint(255, 255, 255, 0);

//Wifi UDP
WiFiUDP UDP;

//Neopixel
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(Numbers, Pin, NEO_GRB + NEO_KHZ800);

//Webserver
ESP8266WebServer Webserver(HTTPPort);

//DNS Server
DNSServer DnsServer;

//Parameter
Parameter_t Parameter;



//SETUP
void setup() {

  //Array initialisieren
  for (i = ersteLED; i <= letzteLED; i++) {
    newletter[i] = 0;
  }

  //Array initialisieren
  for (i = ersteLED; i <= letzteLED; i++) {
    showletter[i] = 0;
  }

  //Array initialisieren
  for (i = ersteLED; i <= letzteLED; i++) {
    changeletter[i] = 2;
  }


  //Serielle Schnittstelle für Debuging öffen
  if (debug == HIGH) {
    Serial.begin(115200);
  }


  //Leds initialisieren
  pixels.begin();


  //Paramater aus dem EEPROM lesen
  LoadParameter();

  //Startwerte laden, falls ein Fehler im EEPROM vorliegt bzw keine gespeicherten Daten
  if (Parameter.ConfigSaved != 1) {

    //Startwerte als Parameter speichern
    Parameter.RGBValueRed = RGBValueRed;
    Parameter.RGBValueGreen = RGBValueGreen;
    Parameter.RGBValueBlue = RGBValueBlue;
    Parameter.EastWest = EastWest;
    Parameter.EnableNightMode = EnableNightMode;
    Parameter.TimeDisplayOff = TimeDisplayOff;
    Parameter.TimeDisplayOn = TimeDisplayOn;
    Parameter.ConfigSaved = 1;

    //Parameter im EEPROM speichern
    SaveParameter();

  } else {  //wenn es gespeicherte Daten hingegen gegeben hatte, können diese ja zur Laufzeit auch genutzt werden..

    EastWest = Parameter.EastWest;
  }

  //Start: Aufbau der WLAN-Verbindung

  //Wifi Verbindung aufbauen
  WiFi.mode(WIFI_STA);
  WiFi.begin(Parameter.Ssid, Parameter.Password);

  //Wartezeit damit die Verbindung aufgebaut wird
  delay(3000);

  //Kontrolle ob die Verbindung aufgebaut wurde
  do {

    //Kein WLAN vorhanden
    if (x >= 2) {
      NoWifi = HIGH;
      break;
    }

    //Start: Test alle LEDs bei Neustart der Uhr

    //Testdurchlauf => alle LEDs nacheinander einschalten
    for (i = ersteLED; i <= letzteLED; i++) {
      pixels.setPixelColor(i, pixels.Color(8, 8, 8));
      pixels.show();
      delay(30);
    }

    delay(400);

    //Testdurchlauf => alle LEDs umgekehrt ausschalten
    for (i = letzteLED; i >= ersteLED; i--) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      pixels.show();
      delay(30);
    }

    //Ende: Test alle LEDs bei Neustart der Uhr

    //Anzahl der Durchläufe zählen
    x++;

  } while (WiFi.status() != WL_CONNECTED);

  //Ende: Aufbau der WLAN-Verbindung



  //Kein WLAN gefunden
  //Access Point & Webserver starten
  if (NoWifi == HIGH) {

    //Access Point aufbauen
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAdress_AccessPoint, Gateway_AccessPoint, Subnetmask_AccessPoint);
    WiFi.softAP(SSID_AccessPoint, Password_AccessPoint);

    //DNS Server starten
    DnsServer.setTTL(300);
    DnsServer.start(DNSPort, WebserverURL, IPAdress_AccessPoint);

    //Webserver starten
    Webserver.on("/", HandlerWebpage);
    Webserver.on("/refresh", HTTP_POST, HandlerRefresh);
    Webserver.on("/save", HTTP_POST, HandlerSave);
    Webserver.onNotFound(HandlerNotFound);
    Webserver.begin();

    //Lokale Uhrzeit als Startzeit setzen
    utcaktstunde = localutcaktstunde;
    utcaktminute = localutcaktminute;
    utcaktsekunde = localutcaktsekunde;
    utcx = localutcx;

    //UDP-Protokoll
    UDP.begin(localPort);

  }


  //WLAN-Verbindung aufgebaut
  else {

    //UDP-Protokoll
    UDP.begin(localPort);

    //Start: NTP-Anfrage und NTP-Antwort

    //Zeitwerte vom NTP-Server abrufen
    do {
      SendNtpRequest();
      UdpDataReceive();
    } while (newsecsSince1900 == 0);

    //Ende: NTP-Anfrage und NTP-Antwort

    //Systemzeit speichern für den nächsten Vergleich
    lastmillisSecond = millis();

    //Berechnung der Jahreszahl
    datumJahr = datumJahr_calc(newsecsSince1900);

    //Berechnung der Anzahl der Schalttage seit dem 1. Januar 1900
    for (i = 1900; i < datumJahr; i++) {

      //Aufruf der Funktion zur Erkennung ob ein Schaltjahr vorliegt
      schaltJahr = schaltJahrJN(i);

      if (schaltJahr == HIGH) {
        newschaltTage++;
      }
    }

    newschaltTage = newschaltTage - 1;
  }


  //Start: Farbwerte zum Dimmen der LED's berechnen

  fadevaluered = FadeValueRed(Parameter.RGBValueRed, Parameter.RGBValueGreen, Parameter.RGBValueBlue);
  fadevaluegreen = FadeValueGreen(Parameter.RGBValueRed, Parameter.RGBValueGreen, Parameter.RGBValueBlue);
  fadevalueblue = FadeValueBlue(Parameter.RGBValueRed, Parameter.RGBValueGreen, Parameter.RGBValueBlue);

  //Farbwerte um x Nachkommastellen verschieben
  for (i = 0; i <= 10; i++) {

    if (fadevaluered < 1.0 && fadevaluegreen < 1.0 && fadevalueblue < 1.0) {
      break;
    }

    fadevaluered = fadevaluered / 10.0;
    fadevaluegreen = fadevaluegreen / 10.0;
    fadevalueblue = fadevalueblue / 10.0;
  }

  //Ende: Farbwerte zum Dimmen der LED's berechnen


  //Farbwerte übernehmen
  newredminus = Parameter.RGBValueRed;
  newgreenminus = Parameter.RGBValueGreen;
  newblueminus = Parameter.RGBValueBlue;


  //Wort "ES" anzeigen
  WriteArray(0, EsIstAddr, newletter, 1);
  //Wort "IST" anzeigen
  WriteArray(1, EsIstAddr, newletter, 1);
}


//LOOP
void loop() {


  //Start: Webserver

  //Webserver Client
  if (NoWifi == HIGH && newdisplay == LOW) {
    Webserver.handleClient();
    DnsServer.processNextRequest();
  }

  //Ende: Webserver



  //Start: UDP-Datenpakete empfangen

  //UDP-Daten in einem festen Zeitintervall abfragen
  if (millis() - LastMillisUdp > IntervallUdp && newdisplay == LOW) {

    //Systemzeit speichern für den nächsten Vergleich
    LastMillisUdp = millis();

    UdpDataReceive();
  }

  //Ende: UDP-Datenpakete empfangen



  //Start: NTP-Anfrage

  //Zeitwerte bei jedem Stundenwechsel aktualisieren
  if (mezaktstunde != oldmezstunde && newdisplay == LOW && NoWifi == LOW) {

    //Zeit für den nächsten Vergleich speichern
    oldmezstunde = mezaktstunde;

    //Zeitwerte vom NTP-Server anfragen
    SendNtpRequest();
  }

  //Ende: NTP-Anfrage



  //Start: Datum und Uhrzeit ermitteln

  //Kontrolle ob eine Änderung bei dem Zeitstempel stattgefunden hat
  if (newsecsSince1900 > oldsecsSince1900) {

    //Änderung abspeichern für den nächsten Vergleich
    oldsecsSince1900 = newsecsSince1900;

    //Tage seit dem 1. Januar 1900; 00:00:00 Uhr
    newtageSeit1900 = newsecsSince1900 / secondperday;


    //Start: Datum ermitteln

    //Kontrolle ob eine Änderung bei den Tagen seit dem 1. Januar 1900 stattgefunden hat
    if (newtageSeit1900 > oldtageSeit1900) {

      //Änderung abspeichern für den nächsten Vergleich
      oldtageSeit1900 = newtageSeit1900;


      //Aufruf der Funktion zur Berechnung des Wochentages
      wochenTag = wochenTag_calc(newtageSeit1900);


      //Berechnug der Jahreszahl
      datumJahr = ((newtageSeit1900 - newschaltTage) / 365.002f) + 1900;


      //Aufruf der Funktion zur Erkennung ob ein Schaltjahr vorliegt
      schaltJahr = schaltJahrJN(datumJahr);


      //Funktion fürs Schaltjahr
      if (schaltJahr == HIGH) {

        tageImMonat[2] = 29;


        //Anzahl der Schalttage seit dem 1. Januar 1900 !ERST! am Ende des jeweiligen Schaltjahres hochzählen
        //Sonderfall: Zur Berechnung des 366. Tages wird der "alte" Wert der Schalttage benötigt, die Berechnung
        //            der Jahreszahl am 366. Tag muss aber mit dem neuen Wert durchgeführt werden

        //Berechnung der Anzahl der Schalttage seit dem 1. Januar 1900
        if (datumMonat == 12 && datumTag == 30) {

          oldschaltTage = newschaltTage;

          newschaltTage = newschaltTage + 1;

          sonderFall366 = HIGH;
        }
      }

      else {
        tageImMonat[2] = 28;
      }


      //Berechnung der wieviele Tag im Jahr
      //Sonderfall: Die Berechnung des 366. Tages im Schaltjahr muss mit dem "alten" Wert der Schalttage (seit 1900) berechnet werden
      if (sonderFall366 == HIGH) {

        tagNummer = (newtageSeit1900 - ((datumJahr - 1900) * 365) - oldschaltTage);

        sonderFall366 = LOW;
      }

      //Ansonsten muss mit dem aktuellen Wert der Schalttage (seit 1900) gerechnet werden
      else {
        tagNummer = (newtageSeit1900 - ((datumJahr - 1900) * 365) - newschaltTage);
      }

      //Aufruf der Funktion zur Berechnung des Monates
      datumMonat = welcherMonat(tagNummer, schaltJahr);

      datumTag = 0;

      //Berechnung der bereits vergangenen Tagen im Jahr
      for (i = 0; i < datumMonat; i++) {
        datumTag = datumTag + tageImMonat[i];
      }

      //Tag-Nummer minus bereits vergangener Tage im Jahr = aktueller Tag
      datumTag = tagNummer - datumTag;
    }

    //Ende: Datum ermitteln



    //Start: Sommer-/ Winterzeit einstellen

    //am letzten Sonntag im März (03) wird von UTC+1 auf UTC+2 umgestellt; 02:00 -> 03:00
    //am letzten Sonntag im Oktober (10) wird von UTC+2 auf UTC+1 umgestellt; 03:00 -> 02:00

    //Aufruf jeden Sonntag im März
    if (datumMonat == 3 && wochenTag == 7) {

      //Letzter Sonntag im März
      if ((tageImMonat[3] - datumTag) < 7) {

        //Umstellung auf Sommerzeit UTC+2
        utcx = 2;
      }
    }

    //Aufruf jeden Sonntag im Oktober
    if (datumMonat == 10 && wochenTag == 7) {

      //Letzter Sonntag im Oktober
      if ((tageImMonat[10] - datumTag) < 7) {

        //Umstellung auf Winterzeit UTC+1
        utcx = 1;
      }
    }

    //Sommer-/Winterzeit bei Neustart bestimmen
    if (utcx == 0) {

      //Neustart im Monat März
      if (datumMonat == 3) {

        //Neustart in der letzten Woche im März
        if ((tageImMonat[3] - datumTag) < 7) {

          //Beispiel Jahr 2020: März 31 Tage; Neustart 26. März 2020 (Donnerstag = Wochentag = 4); 5 Tage Rest; Letzter Sonntag 29. März 2020
          //Berechnung: 31 - 26 = 5; 5 + 4 = 9;
          //Ergebnis: Letzter Tag im März ist ein Dienstag. Es folgt noch ein weiterer Sonntag im Oktober => Winterzeit einstellen

          //Beispiel Jahr 2021: März 31 Tage; Neustart 30. März 2021 (Dienstag = Wochentag = 2); 1 Tage Rest; Letzter Sonntag 28. März 2021
          //Berechnung: 31 - 30 = 1; 1 + 2 = 3;
          //Ergebnis: Letzter Tag im März ist ein Mittwoch. Umstellung auf Sommerzeit bereits erfolgt => Sommerzeit einstellen

          //Es folgt innerhalb der letzten Woche im März noch ein weiterer Sonntag => Winterzeit einstellen
          if (((tageImMonat[3] - datumTag) + wochenTag) >= 7) {
            utcx = 1;
          }

          //Letzter Sonntag im März bereits vergangen => Sommerzeit einstellen
          else {
            utcx = 2;
          }
        }

        //Neustart in den ersten drei Wochen im März => Winterzeit einstellen
        else {
          utcx = 1;
        }
      }

      //Neustart im Monat Oktober
      if (datumMonat == 10) {

        //Neustart in der letzten Woche im Oktober
        if ((tageImMonat[10] - datumTag) < 7) {

          //Beispiel Jahr 2020: Oktober 31 Tage; Neustart 26. Oktober 2020 (Montag = Wochentag = 1); 5 Tage Rest; Letzter Sonntag 25. Oktober 2020
          //Berechnung: 31 - 26 = 5; 5 + 1 = 6;
          //Ergebnis: Letzter Tag im Oktober ist ein Samstag. Umstellung auf Winterzeit bereits erfolgt => Winterzeit einstellen

          //Beispiel Jahr 2021: Oktober 31 Tage; Neustart 26. Oktober 2021 (Dienstag = Wochentag = 2); 5 Tage Rest; Letzter Sonntag 31. Oktober 2021
          //Berechnung: 31 - 26 = 5; 5 + 2 = 7;
          //Ergebnis: Letzter Tag im Oktober ist ein Sonntag. Es folgt noch ein weiterer Sonntag im Oktober => Sommerzeit einstellen

          //Es folgt innerhalb der letzten Woche im Oktober noch ein weiterer Sonntag => Sommerzeit einstellen
          if (((tageImMonat[10] - datumTag) + wochenTag) >= 7) {
            utcx = 2;
          }

          //Letzter Sonntag im Oktober bereits vergangen => Winterzeit einstellen
          else {
            utcx = 1;
          }
        }

        //Neustart in den ersten drei Wochen im Oktober => Sommerzeit einstellen
        else {
          utcx = 2;
        }
      }

      //Falls Neustart der Uhr innerhalb der Sommerzeit (Ausnahme März/Oktober)
      if (datumMonat > 3 && datumMonat < 10) {
        utcx = 2;
      }

      //Falls Neustart der Uhr innerhalb der Winterzeit (Ausnahme März/Oktober)
      if (datumMonat < 3 || datumMonat > 10) {
        utcx = 1;
      }
    }

    //Ende: Sommer-/ Winterzeit einstellen



    //Start: Aktuelle Uhrzeit hh:mm:ss aus dem NTP-Paket ermitteln

    //Beispiel: Berechnung der aktuellen Stunde
    //Sekunden seit dem 1. Januar 1900 (3812811794) / Sekunden pro Tag (86400) = 44129 volle Tage seit dem 1. Januar 1900
    //Tage seit dem 1. Januar (44129) * Sekunden pro Tag (86400) = 3812745600 Sekunden der vollen Tage seit dem 1. Januar 1900
    //Sekunden seit dem 1. Januar 1900 (3812811794) - Sekunden der vollen Tage (3812745600) = 66194 Sekunden am aktuellen Tag vergangen
    //Sekunden des aktuellen Tages (66194) / Sekunden pro Stunde (3600) = 18 Stunden am aktuellen Tag vergangen => Aktuelle Uhrzeit 18:mm:ss

    //Berechnung der aktuellen Stunde
    utcaktstunde = ((newsecsSince1900 % secondperday) / secondperhour);

    //Berechnung der aktuellen Minute
    utcaktminute = (newsecsSince1900 % secondperhour) / secondperminute;

    //Berechnung der aktuellen Sekunde
    utcaktsekunde = (newsecsSince1900 % secondperminute);

    //Ende: Aktuelle Uhrzeit hh:mm:ss aus dem NTP-Paket ermitteln
  }

  //Ende: Datum und Uhrzeit ermitteln



  //Start: Uhrzeit über den millis()-Timer berechnen

  //Alle 1000 ms die Sekunden hochzählen
  if (millis() - lastmillisSecond > millisecondpersecond) {

    //Zeitdifferenz
    lastmillisdifferenz = lastmillisdifferenz + ((millis() - lastmillisSecond) - millisecondpersecond);

    if (lastmillisdifferenz > millisecondpersecond) {
      lastmillisdifferenz = 0;
      utcaktsekunde++;
    }

    //Systemzeit speichern für den nächsten Vergleich
    lastmillisSecond = millis();

    utcaktsekunde++;
  }

  //Alle 60 s den Minutenzähler hochzählen
  if (utcaktsekunde >= secondperminute) {

    //Sekunden zurücksetzen
    utcaktsekunde = 0;

    //Minuten hochzählen
    utcaktminute++;
  }

  //Alle 60 min den Stundenzähler hochzählen
  if (utcaktminute >= minuteperhour) {

    //Minuten zurücksetzen
    utcaktminute = 0;

    //Stunden hochzählen
    utcaktstunde++;

    //Wechsel von 24 Uhr zu 1 Uhr
    if (utcaktstunde == 25) {
      utcaktstunde = 1;
    }
  }

  //Ende: Uhrzeit über den millis()-Timer berechnen



  //Start: Von UTC nach MEZ

  //Sekunden und Minuten unverändert von UTC übernehmen
  mezaktminute = utcaktminute;
  mezaktsekunde = utcaktsekunde;

  //Umrechnung von UTC nach MEZ mittels Zeitverschiebung
  mezaktstunde = utcaktstunde + utcx;

  //Aktuelle Stundenzahl in MEZ als 24h Format speichern
  mezaktstunde24 = mezaktstunde;


  //Stundenzahl erhöhen
  //Takt mit vollen 5 Minuten
  if (WordClockType == HIGH) {

    if (EastWest == 0) {
      //Nach "Halb" Stundenzahl erhöhen => Beispiel: 16:25:00 => Fünf vor halb Fünf
      if (mezaktminute >= 25 && mezaktminute <= 59) {
        mezaktstunde = mezaktstunde + 1;
      }
    } else if (EastWest == 1) {  //in Ostschreibweise wird die volle stunde schon ab Minute 15 erhöt um +1
      if (mezaktminute >= 15 && mezaktminute <= 59) {
        mezaktstunde = mezaktstunde + 1;
      }
    }
  }

  //Stundenzahl erhöhen
  //Takt mit +2/-2 Minuten
  if (WordClockType == LOW) {

    if (EastWest == 0) {
      //Nach "Halb" Stundenzahl erhöhen => Beispiel: 16:25:00 => Fünf vor halb Fünf
      if (mezaktminute >= 23 && mezaktminute <= 59) {
        mezaktstunde = mezaktstunde + 1;
      }
    } else if (EastWest == 1) {  //in Ostschreibweise wird die volle stunde schon ab Minute 15 erhöt um +1
      if (mezaktminute >= 15 && mezaktminute <= 25) {
        mezaktstunde = mezaktstunde + 1;
      }
    }
  }


  //Beispiel: UTC             = 22:15:00
  //          MEZ (Winter +1) = 23:15:00 => 23:15:00 => 11:15:00 => viertel nach Elf
  //          MEZ (Sommer +2) = 24:15:00 => 00:15:00 => viertel nach Zwölf

  //Beispiel: UTC             = 23:15:00
  //          MEZ (Winter +1) = 24:15:00 => 00:15:00 => viertel nach Zwölf
  //          MEZ (Sommer +2) = 25:15:00 => 01:15:00 => viertel nach Eins

  //Problemstellung 1: Stundenzahlen über 23 abfangen
  if (mezaktstunde >= 24) {
    mezaktstunde = mezaktstunde - 24;
  }


  //Beispiel: UTC             = 10:47:00
  //          MEZ (Winter +1) = 11:15:00 => 11:15:00 => viertel nach Elf
  //          MEZ (Sommer +2) = 12:15:00 => 00:15:00 => viertel nach Zwölf

  //Beispiel: UTC             = 11:47:00
  //          MEZ (Winter +1) = 12:15:00 => 00:15:00 => viertel nach Zwölf
  //          MEZ (Sommer +2) = 13:15:00 => 01:15:00 => viertel nach Eins

  //Problemstellung 2: Stundenzahlen auf den Bereich "Eins" bis "Zwölf" begrenzen
  if (mezaktstunde >= 12) {
    mezaktstunde = mezaktstunde - 12;
  }

  //Ende: Von UTC nach MEZ



  //Start: Stundenzahl ausgeben

  if (mezaktstunde != oldstunde) {

    //Stundenzahl für den nächsten Vergleich speichern
    oldstunde = mezaktstunde;

    //Adressbereich auf Null setzen
    ResetArray(NumberOfHours, HourAddr, newletter);

    //Anzeige der Stunden
    WriteArray(mezaktstunde, HourAddr, newletter, 1);
  }

  //Ende: Stundenzahl ausgeben



  //Start: Minutenzahl ausgeben

  if (mezaktminute != oldminute) {

    //Minutenzahl für den nächsten Vergleich speichern
    oldminute = mezaktminute;

    //Adressbereich auf Null setzen
    ResetArray(NumberOfMinute, MinuteAddr, newletter);

    //Adressbereich auf Null setzen
    ResetArray(NumberOfSpecialChar, ToPastAddr, newletter);

    //Adressbereich auf Null setzen
    ResetArray(NumberOfSpecialChar, HalfFullHourAddr, newletter);


    //WordClock mit zusätzlicher Minutenanzeige
    if (WordClockType == HIGH) {

      //Pointer hat einen Wert zwischen 0-4
      pointer = mezaktminute % 5;

      //Adressbereich auf Null setzen
      if (pointer == 0) {
        ResetArray(NumberOfAddMinutes, AddMinutesAddr, newletter);
      }
      //Minutenanzeige +1 +2 +3 +4
      else {
        for (i = 0; i < pointer; i++) {
          WriteArray(i, AddMinutesAddr, newletter, 1);
        }
      }
    }


    //Pointer zurücksetzen
    pointer = 99;

    //TimeTable[60][4]
    //{0,0,0,0} = Minutenzahl, Pointer_MinuteAddr, Pointer_ToPastAddr, Pointer_HalfFullHourAddr
    //Wert '0' bedeutet, dass das Array übersprungen wird und die Adresse nicht angezeigt wird

    //Anzeige der Minuten
    for (i = 0; i <= 59; i++) {

      //Aktuelle Minutenzahl mit der Zeittabelle vergleichen
      if (mezaktminute == TimeTable[i][0]) {

        //MinuteAddr
        if (TimeTable[i][1] != 0) {
          pointer = TimeTable[i][1] - 1;
          WriteArray(pointer, MinuteAddr, newletter, 1);
        }

        //OstMode
        if (EastWest == 1) {
          //Das ist die Frage
          if (mezaktminute >= 15 && mezaktminute <= 19) {
          }                                                     //"Nach" fällt werg; wenn es viertel 9 ist ist es NICHT viertel NACH Acht..
          else if (mezaktminute >= 20 && mezaktminute <= 24) {  //Es ist ZEHN VOR HALB XXX

            //Adressbereich auf Null setzen
            ResetArray(NumberOfMinute, MinuteAddr, newletter);
            //Adressbereich auf Wert setzen
            WriteArray(1, MinuteAddr, newletter, 1);              // fünf, ZEHN, zwanzig, viertel, dreiviertel
            WriteArray(1, ToPastAddr, newletter, 1);              // nach, VOR
            WriteArray(0, HalfFullHourAddr, newletter, 1);        // HALB, Um
          } else if (mezaktminute >= 40 && mezaktminute <= 44) {  //Es ist ZEHN NACH HALB

            //Adressbereich auf Null setzen
            ResetArray(NumberOfMinute, MinuteAddr, newletter);
            //Adressbereich auf Wert setzen
            WriteArray(1, MinuteAddr, newletter, 1);        // fünf, ZEHN, zwanzig, viertel, dreiviertel
            WriteArray(0, ToPastAddr, newletter, 1);        // NACH, vor
            WriteArray(0, HalfFullHourAddr, newletter, 1);  // HALB, Um

          } else if (mezaktminute >= 45 && mezaktminute <= 49) {  // DREIVIERTEL

            //Wort "dreiviertel" anzeigen
            WriteArray(4, MinuteAddr, newletter, 1);

          } else {
            //ToPastAddr
            if (TimeTable[i][2] != 0) {
              pointer = TimeTable[i][2] - 1;
              WriteArray(pointer, ToPastAddr, newletter, 1);
            }
          }
        } else  //alles wie üblich Westmodus
        {
          //ToPastAddr
          if (TimeTable[i][2] != 0) {
            pointer = TimeTable[i][2] - 1;
            WriteArray(pointer, ToPastAddr, newletter, 1);
          }
        }

        //HalfFullHourAddr
        if (TimeTable[i][3] != 0) {
          pointer = TimeTable[i][3] - 1;
          WriteArray(pointer, HalfFullHourAddr, newletter, 1);
        }

        break;
      }
    }

    //Beispiel: ES IST FÜNF VOR EINS
    if (mezaktstunde == 1) {
      newletter[HourAddr[1][0]] = 1;
    }

    //Sonderfall: ES IST EIN(S) UHR
    if (mezaktstunde == 1 && newletter[HalfFullHourAddr[1][0]] == 1 && newletter[HalfFullHourAddr[1][1]] == 1) {
      newletter[HourAddr[1][0]] = 0;
    }
  }

  //Ende: Minutenzahl ausgeben



  //Start: Altes Bild mit neuem Bild vergleichen

  for (i = ersteLED; i <= letzteLED; i++) {

    //showletter = 1; newletter = 0 => Wort ausschalten
    if (showletter[i] > newletter[i]) {
      newdisplay = HIGH;
      changeletter[i] = 0;
    }

    //showletter = 0; newletter = 1 => Wort einschalten
    if (showletter[i] < newletter[i]) {
      newdisplay = HIGH;
      changeletter[i] = 1;
    }

    if (showletter[i] == newletter[i]) {
      changeletter[i] = 2;
    }
  }

  //Ende: Altes Bild mit neuem Bild vergleichen



  //Start: LED-Display in einem definierten Zeitfenster ausschalten

  if (Parameter.EnableNightMode == 1) {

    //Beispiel: Display On: 5 Uhr; Display Off: 2 Uhr
    if (Parameter.TimeDisplayOff < Parameter.TimeDisplayOn) {
      if (mezaktstunde24 >= Parameter.TimeDisplayOff && mezaktstunde24 < Parameter.TimeDisplayOn) {

        for (i = ersteLED; i <= letzteLED; i++) {

          //Angezeigte Wörter im ersten Aufruf ausschalten
          if (setDisplayOff == LOW) {
            if (showletter[i] == 1 || newletter[i] == 1) {
              changeletter[i] = 0;
              newdisplay = HIGH;
            }
          }

          //Display im Zeitfenster ausgeschaltet lassen
          else {
            changeletter[i] = 2;
            showletter[i] = 0;
            newdisplay = LOW;
          }
        }
      }

      else {
        setDisplayOff = LOW;
      }
    }

    //Beispiel: Display On: 6 Uhr; Display Off: 23 Uhr
    else {
      if (mezaktstunde24 >= Parameter.TimeDisplayOff || mezaktstunde24 < Parameter.TimeDisplayOn) {

        for (i = ersteLED; i <= letzteLED; i++) {

          //Angezeigte Wörter im ersten Aufruf ausschalten
          if (setDisplayOff == LOW) {
            if (showletter[i] == 1 || newletter[i] == 1) {
              changeletter[i] = 0;
              newdisplay = HIGH;
            }
          }

          //Display im Zeitfenster ausgeschaltet lassen
          else {
            changeletter[i] = 2;
            showletter[i] = 0;
            newdisplay = LOW;
          }
        }
      }

      else {
        setDisplayOff = LOW;
      }
    }
  }

  else {
    setDisplayOff = LOW;
  }

  //Ende: LED-Display in einem definierten Zeitfenster ausschalten



  //Start: Ausgabe des Bildes

  if (newdisplay == HIGH) {

    //Farbwerte in dem eingestellten Intervall dimmen
    if (millis() - lastmillisFade > IntervallFade) {

      //Systemzeit speichern für den nächsten Vergleich
      lastmillisFade = millis();

      //Farbwerte reduzieren "herunterdimmen"
      if (newredminus > 0.0) {
        newredminus = newredminus - fadevaluered;
        if (newredminus <= 0.0) {
          newredminus = 0.0;
          newgreenminus = 0.0;
          newblueminus = 0.0;
        }
      }

      //Farbwerte reduzieren "herunterdimmen"
      if (newgreenminus > 0.0) {
        newgreenminus = newgreenminus - fadevaluegreen;
        if (newgreenminus <= 0.0) {
          newredminus = 0.0;
          newgreenminus = 0.0;
          newblueminus = 0.0;
        }
      }

      //Farbwerte reduzieren "herunterdimmen"
      if (newblueminus > 0.0) {
        newblueminus = newblueminus - fadevalueblue;
        if (newblueminus <= 0.0) {
          newredminus = 0.0;
          newgreenminus = 0.0;
          newblueminus = 0.0;
        }
      }

      //Farbwerte erhöhen "hochdimmen"
      if (newredplus < Parameter.RGBValueRed) {
        newredplus = newredplus + fadevaluered;
        if (newredplus >= Parameter.RGBValueRed) {
          newredplus = Parameter.RGBValueRed;
          newgreenplus = Parameter.RGBValueGreen;
          newblueplus = Parameter.RGBValueBlue;
        }
      }

      //Farbwerte erhöhen "hochdimmen"
      if (newgreenplus < Parameter.RGBValueGreen) {
        newgreenplus = newgreenplus + fadevaluegreen;
        if (newgreenplus >= Parameter.RGBValueGreen) {
          newredplus = Parameter.RGBValueRed;
          newgreenplus = Parameter.RGBValueGreen;
          newblueplus = Parameter.RGBValueBlue;
        }
      }

      //Farbwerte erhöhen "hochdimmen"
      if (newblueplus < Parameter.RGBValueBlue) {
        newblueplus = newblueplus + fadevalueblue;
        if (newblueplus >= Parameter.RGBValueBlue) {
          newredplus = Parameter.RGBValueRed;
          newgreenplus = Parameter.RGBValueGreen;
          newblueplus = Parameter.RGBValueBlue;
        }
      }

      //Neue Farbwerte an die LED's übermitteln
      for (i = ersteLED; i <= letzteLED; i++) {

        red = newredminus;
        green = newgreenminus;
        blue = newblueminus;

        if (changeletter[i] == 0) {
          pixels.setPixelColor(i, pixels.Color(red, green, blue));
        }

        red = newredplus;
        green = newgreenplus;
        blue = newblueplus;

        if (changeletter[i] == 1) {
          pixels.setPixelColor(i, pixels.Color(red, green, blue));
        }
      }

      //LED's aktualisieren
      pixels.show();
    }


    //Bild wurde komplett aktualisiert
    if (newredminus <= 0.0 && newgreenminus <= 0.0 && newblueminus <= 0.0 && newredplus >= Parameter.RGBValueRed && newgreenplus >= Parameter.RGBValueGreen && newblueplus >= Parameter.RGBValueBlue) {

      //Arrays kopieren für den nächsten Vergleich
      for (i = ersteLED; i <= letzteLED; i++) {
        showletter[i] = newletter[i];
      }


      //Start: Aktuelle Stromaufnahme ermitteln

      currentPerLed = 0;

      //Aktuelle Stromfaufnahme mittels RGB-Wert und Anzahl der aktiven LED's ermitteln
      //Die Stromaufnahme wird dabei nicht gemessen, sondern mit einem Faktor und der Anzahl der aktiven LED's berechnet
      for (i = ersteLED; i <= letzteLED; i++) {

        if (showletter[i] == 1) {
          currentPerLed = currentPerLed + round((float)(currentPerRgbValue * (red + green + blue)));
        }
      }

      //Ende: Aktuelle Stromaufnahme ermitteln


      //Aktualisierung des Bildes beenden
      newdisplay = LOW;
      setDisplayOff = HIGH;


      //Farbwerte für die nächste Aktualisierung zurücksetzen
      newredminus = Parameter.RGBValueRed;
      newgreenminus = Parameter.RGBValueGreen;
      newblueminus = Parameter.RGBValueBlue;
      newredplus = 0.0;
      newgreenplus = 0.0;
      newblueplus = 0.0;
    }
  }

  //Ende: Ausgabe des Bildes



  //Start: Ausgabe fürs Debuging

  if (debug == HIGH && newdisplay == LOW) {

    if (millis() - lastmillisDebug > IntervallDebug) {

      //Systemzeit speichern für den nächsten Vergleich
      lastmillisDebug = millis();

      Serial.print("SSID: ");
      Serial.println(Parameter.Ssid);

      Serial.print("Passwort: ");
      Serial.println(Parameter.Password);

      Serial.print("IP-Adresse: ");
      Serial.println(WiFi.localIP());

      Serial.print("WLAN Daten gespeichert: ");
      Serial.println(Parameter.LoginSaved);

      Serial.print("Parameter gespeichert: ");
      Serial.println(Parameter.ConfigSaved);

      Serial.print("NTP-Paket: ");
      Serial.println(newsecsSince1900);

      Serial.print("Zeitdifferenz: ");
      Serial.print(lastmillisdifferenz);
      Serial.println("ms");

      Serial.print("Uhrzeit UTC: ");
      Serial.print(utcaktstunde);
      Serial.print(":");
      Serial.print(utcaktminute);
      Serial.print(":");
      Serial.println(utcaktsekunde);

      Serial.print("Zeitverschiebung: +");
      Serial.println(utcx);

      Serial.print("Uhrzeit MEZ: ");
      Serial.print(mezaktstunde24);
      Serial.print(":");
      Serial.print(mezaktminute);
      Serial.print(":");
      Serial.println(mezaktsekunde);

      Serial.print("Datum: ");
      Serial.print(datumTag);
      Serial.print(".");
      Serial.print(datumMonat);
      Serial.print(".");
      Serial.println(datumJahr);

      Serial.print("Wochentag: ");
      Serial.println(wochenTag);

      Serial.print("Schaltjahr: ");
      Serial.println(schaltJahr);

      Serial.print("Anzahl Schalttage: ");
      Serial.println(newschaltTage);

      Serial.print("OstWestModus: ");
      Serial.println(Parameter.EastWest);

      Serial.print("Nachtmodus aktiv: ");
      Serial.println(Parameter.EnableNightMode);

      Serial.print("Display ausschalten um: ");
      Serial.print(Parameter.TimeDisplayOff);
      Serial.println(" Uhr");

      Serial.print("Display einschalten um: ");
      Serial.print(Parameter.TimeDisplayOn);
      Serial.println(" Uhr");

      Serial.print("RGB-Farbwert Rot: ");
      Serial.print(Parameter.RGBValueRed);
      Serial.println("/255");

      Serial.print("RGB-Farbwert Grün: ");
      Serial.print(Parameter.RGBValueGreen);
      Serial.println("/255");

      Serial.print("RGB-Farbwert Blau: ");
      Serial.print(Parameter.RGBValueBlue);
      Serial.println("/255");

      Serial.print("Faktor Dimmen Rot: ");
      Serial.println(fadevaluered);

      Serial.print("Faktor Dimmen Grün: ");
      Serial.println(fadevaluegreen);

      Serial.print("Faktor Dimmen Blau: ");
      Serial.println(fadevalueblue);

      Serial.print("Stromaufnahme: ");
      Serial.print(currentPerLed);
      Serial.println("mA");

      Serial.println();
    }
  }

  //Ende: Ausgabe fürs Debuging
}


//****************************** Eigene Funktionen ******************************


//Daten aus dem EEPROM lesen
void LoadParameter() {
  EEPROM.begin(sizeof(Parameter));
  EEPROM.get(0, Parameter);
  EEPROM.end();
}



//Daten im EEPROM speichern
bool SaveParameter() {
  bool RetVal = LOW;
  EEPROM.begin(sizeof(Parameter));
  EEPROM.put(0, Parameter);
  RetVal = EEPROM.commit();
  EEPROM.end();
  return RetVal;
}



//Daten im EEPROm löschen
void ClearParameter() {
  int count = 0;
  EEPROM.begin(sizeof(Parameter));
  for (count = 0; count < sizeof(Parameter); count++) {
    EEPROM.write(count, 0);
  }
  EEPROM.end();
}



//Adressbereich zurücksetzen
void ResetArray(int ArraySize, const int Addr[][2], unsigned int* Array) {

  int c1 = 0;
  int c2 = 0;

  //Array durchlaufen
  for (c1 = 0; c1 <= ArraySize - 1; c1++) {

    //Bereich auf Null setzen
    for (c2 = Addr[c1][0]; c2 <= Addr[c1][1]; c2++) {
      Array[c2] = 0;
    }
  }
}



//Adressbereich beschreiben
void WriteArray(int Pointer, const int Addr[][2], unsigned int* Array, int Value) {

  int c1 = 0;

  //Array-Bereich beschreiben
  for (c1 = Addr[Pointer][0]; c1 <= Addr[Pointer][1]; c1++) {
    Array[c1] = Value;
  }
}



//Webserver
void HandlerWebpage() {
  Webserver.send(HTTPRequest, "text/html", LoginHTML);
}



//Webserver
void HandlerNotFound() {
  Webserver.send(HTTPNotFound, "text/html", NotFoundHTML);
}



//Webserver
void HandlerSave() {

  //Eingaben vom Webserver übernehmen
  Parameter.TimeDisplayOff = GetArgumentToInt("displayoff");
  Parameter.TimeDisplayOn = GetArgumentToInt("displayon");
  GetArgument("ssid").toCharArray(Parameter.Ssid, 40);
  GetArgument("password").toCharArray(Parameter.Password, 40);

  //Nachtmodus
  if (GetArgument("enablenightmode") == "on") {
    Parameter.EnableNightMode = 1;
  } else {
    Parameter.EnableNightMode = 0;
  }


  //OstWestModus Dreiviertel
  if (GetArgument("Clockmode1") == "1") {
    EastWest = 1;
    Parameter.EastWest = EastWest;

    ResetArray(NumberOfMinute, MinuteAddr, newletter);
    ResetArray(NumberOfSpecialChar, ToPastAddr, newletter);
    ResetArray(NumberOfHours, HourAddr, newletter);
    WriteArray(4, MinuteAddr, newletter, 1);

  } else {
    EastWest = 0;
    Parameter.EastWest = EastWest;
  }


  //Neue Parameter im EEPROM
  Parameter.LoginSaved = 1;

  //Parameter im EEPROM speichern
  SaveParameter();

  //Neue Seite anzeigen
  Webserver.send(HTTPPost, "text/html", SavedHTML);
}



//Webserver
void HandlerRefresh() {

  //HTML übernehmen
  String RefreshHTML = LoginHTML;

  //Eingaben vom Webserver übernehmen
  Parameter.RGBValueRed = GetArgumentToInt("rgbred");
  Parameter.RGBValueGreen = GetArgumentToInt("rgbgreen");
  Parameter.RGBValueBlue = GetArgumentToInt("rgbblue");

  //Farbwerte berechnen
  newredminus = Parameter.RGBValueRed;
  newgreenminus = Parameter.RGBValueGreen;
  newblueminus = Parameter.RGBValueBlue;
  fadevaluered = FadeValueRed(Parameter.RGBValueRed, Parameter.RGBValueGreen, Parameter.RGBValueBlue);
  fadevaluegreen = FadeValueGreen(Parameter.RGBValueRed, Parameter.RGBValueGreen, Parameter.RGBValueBlue);
  fadevalueblue = FadeValueBlue(Parameter.RGBValueRed, Parameter.RGBValueGreen, Parameter.RGBValueBlue);

  //Farbwerte um x Nachkommastellen verschieben
  for (int count = 0; count <= 10; count++) {
    if (fadevaluered < 1.0 && fadevaluegreen < 1.0 && fadevalueblue < 1.0) {
      break;
    }

    fadevaluered = fadevaluered / 10.0;
    fadevaluegreen = fadevaluegreen / 10.0;
    fadevalueblue = fadevalueblue / 10.0;
  }

  //Neue Farbwerte zuweisen
  for (i = ersteLED; i <= letzteLED; i++) {
    if (showletter[i] == 1) {
      pixels.setPixelColor(i, pixels.Color(Parameter.RGBValueRed, Parameter.RGBValueGreen, Parameter.RGBValueBlue));
    }
  }

  //LED's aktualisieren
  pixels.show();


  //HTML aktualisieren
  RefreshHTML.replace("%%RED%%", String(Parameter.RGBValueRed, DEC));
  RefreshHTML.replace("%%GREEN%%", String(Parameter.RGBValueGreen, DEC));
  RefreshHTML.replace("%%BLUE%%", String(Parameter.RGBValueBlue, DEC));

  //Neue Seite anzeigen
  Webserver.send(HTTPPost, "text/html", RefreshHTML);
}



//Webserver Argument auslesen (toINT)
int GetArgumentToInt(String ID) {

  int Value = 0;

  if (Webserver.hasArg(ID)) {
    Value = Webserver.arg(ID).toInt();
  }

  return Value;
}



//Webserver Argument auslesen
String GetArgument(String ID) {

  String Value = "0";

  if (Webserver.hasArg(ID)) {
    Value = Webserver.arg(ID);
  }

  return Value;
}


//Funktion UDP-Daten empfangen
void UdpDataReceive() {
  unsigned int PaketSize = 0;
  PaketSize = UDP.parsePacket();
  if (PaketSize != 0) {
    if (PaketSize == NtpPaketSize) {
      UDP.read(NtpPaketBuffer, NtpPaketSize);
      newsecsSince1900 = GetNtpTimestamp(NtpPaketBuffer);
    } else if (PaketSize == UserInterfacePaketSize) {
      UDP.read(UserInterfacePaketBuffer, UserInterfacePaketSize);
      UserInterface(UserInterfacePaketBuffer);
    } else {
      //Nothing to do here
    }
  }
}



//Funktion Nutzerdaten aus UDP-Daten auslesen
int UserInterfaceGetValue(byte Data[], int PointerStart, int PointerEnd) {
  int Value = 0;
  Value = Data[PointerStart];
  Value |= Data[PointerEnd] << 8;
  return Value;
}



//Funktion Handshake
bool HandshakeOk(byte Data[]) {
  bool HandshakeOk = HIGH;
  int HandshakeOkCount = 0;
  for (HandshakeOkCount = 2; HandshakeOkCount < 12; HandshakeOkCount++) {
    if (Data[HandshakeOkCount] != 0) {
      HandshakeOk = LOW;
    }
  }
  return HandshakeOk;
}



//Funktion Nutzerdaten übergeben
void UserInterface(byte UserInterfaceData[]) {

  //ID-Nummer
  UdpId = UserInterfaceGetValue(UserInterfaceData, 0, 1);

  switch (UdpId) {

    case UdpIdHandshakeRequest:
      if (HandshakeOk(UserInterfaceData)) {
        UserInterfaceData[0] = UdpIdHandshakeAnswer;
        UserInterfaceData[1] = 0;
        UserInterfaceData[2] = byte(Parameter.RGBValueRed);
        UserInterfaceData[3] = byte(Parameter.RGBValueRed >> 8);
        UserInterfaceData[4] = byte(Parameter.RGBValueGreen);
        UserInterfaceData[5] = byte(Parameter.RGBValueGreen >> 8);
        UserInterfaceData[6] = byte(Parameter.RGBValueBlue);
        UserInterfaceData[7] = byte(Parameter.RGBValueBlue >> 8);
        UserInterfaceData[8] = Parameter.EnableNightMode;
        UserInterfaceData[9] = Parameter.TimeDisplayOff;
        UserInterfaceData[10] = Parameter.TimeDisplayOn;
        UserInterfaceData[11] = 0;
        UserInterfaceIP = UDP.remoteIP();
        UDP.beginPacket(UserInterfaceIP, localPort);
        UDP.write(UserInterfaceData, 12);
        UDP.endPacket();
      }
      break;

    case UdpIdRgb:
      Parameter.RGBValueRed = UserInterfaceGetValue(UserInterfaceData, 2, 3);
      Parameter.RGBValueGreen = UserInterfaceGetValue(UserInterfaceData, 4, 5);
      Parameter.RGBValueBlue = UserInterfaceGetValue(UserInterfaceData, 6, 7);

      //Farbwerte berechnen
      newredminus = Parameter.RGBValueRed;
      newgreenminus = Parameter.RGBValueGreen;
      newblueminus = Parameter.RGBValueBlue;
      fadevaluered = FadeValueRed(Parameter.RGBValueRed, Parameter.RGBValueGreen, Parameter.RGBValueBlue);
      fadevaluegreen = FadeValueGreen(Parameter.RGBValueRed, Parameter.RGBValueGreen, Parameter.RGBValueBlue);
      fadevalueblue = FadeValueBlue(Parameter.RGBValueRed, Parameter.RGBValueGreen, Parameter.RGBValueBlue);

      //Parameter im EEPROM speichern
      SaveParameter();

      //Farbwerte um x Nachkommastellen verschieben
      for (i = 0; i <= 10; i++) {

        if (fadevaluered < 1.0 && fadevaluegreen < 1.0 && fadevalueblue < 1.0) {
          break;
        }

        fadevaluered = fadevaluered / 10.0;
        fadevaluegreen = fadevaluegreen / 10.0;
        fadevalueblue = fadevalueblue / 10.0;
      }

      //Neue Farbwerte zuweisen
      for (i = ersteLED; i <= letzteLED; i++) {
        if (showletter[i] == 1) {
          pixels.setPixelColor(i, pixels.Color(Parameter.RGBValueRed, Parameter.RGBValueGreen, Parameter.RGBValueBlue));
        }
      }

      //LED's aktualisieren
      pixels.show();

      break;

    case UdpIdTime:
      UdpHour = UserInterfaceGetValue(UserInterfaceData, 2, 3);
      UdpMinute = UserInterfaceGetValue(UserInterfaceData, 4, 5);
      UdpSecond = UserInterfaceGetValue(UserInterfaceData, 6, 7);
      if (NoWifi == HIGH) {
        utcaktstunde = UdpHour;
        utcaktminute = UdpMinute;
        utcaktsekunde = UdpSecond;
        utcx = 0;
      }
      break;

    case UdpIdDate:
      UdpDay = UserInterfaceGetValue(UserInterfaceData, 2, 3);
      UdpMonth = UserInterfaceGetValue(UserInterfaceData, 4, 5);
      UdpYear = UserInterfaceGetValue(UserInterfaceData, 6, 7);
      break;

    case UdpIdDisplay:
      Parameter.EnableNightMode = UserInterfaceGetValue(UserInterfaceData, 2, 3);
      Parameter.TimeDisplayOn = UserInterfaceGetValue(UserInterfaceData, 4, 5);
      Parameter.TimeDisplayOff = UserInterfaceGetValue(UserInterfaceData, 6, 7);
      setDisplayOff = LOW;
      SaveParameter();
      break;
  }
}



//Funktion Dimmwerte Rot berechnen
float FadeValueRed(int Red, int Green, int Blue) {
  float Value = 1.0;

  if (Red == 0) {
    Value = 0.0;
    return Value;
  }

  //Ab hier hat Rot einen Wert

  //Drei Werte vorhanden
  if (Green > 0 && Blue > 0) {
    if (Green <= Red && Green <= Blue) {
      Value = (float)Red / Green;
    }
    if (Blue <= Red && Blue <= Green) {
      Value = (float)Red / Blue;
    }
    return Value;
  }

  //Ab hier haben entweder Grün oder Blau eine Null oder Beide

  //Rot und Grün mit Werten; Blau muss Null sein
  if (Green > 0 && Green < Red) {
    Value = (float)Red / Green;
    return Value;
  }

  //Ab hier hat Grün eine Null und Blau fraglich

  //Rot und Blau
  if (Blue > 0 && Blue < Red) {
    Value = (float)Red / Blue;
    return Value;
  }

  //Ab hier nur Rot vorhanden oder Rot ist kleinster Wert
  return Value;
}



//Funktion Dimmwerte Grün berechnen
float FadeValueGreen(int Red, int Green, int Blue) {
  float Value = 1.0;

  if (Green == 0) {
    Value = 0.0;
    return Value;
  }

  //Ab hier hat Grün einen Wert

  //Drei Werte vorhanden
  if (Red > 0 && Blue > 0) {
    if (Red <= Green && Red <= Blue) {
      Value = (float)Green / Red;
    }
    if (Blue <= Green && Blue <= Red) {
      Value = (float)Green / Blue;
    }
    return Value;
  }

  //Ab hier haben entweder Rot oder Blau eine Null oder Beide

  //Grün und Rot mit Werten; Blau muss Null sein
  if (Red > 0 && Red < Green) {
    Value = (float)Green / Red;
    return Value;
  }

  //Ab hier hat Rot eine Null und Blau fraglich

  //Grün und Blau
  if (Blue > 0 && Blue < Green) {
    Value = (float)Green / Blue;
    return Value;
  }

  //Ab hier nur Grün vorhanden oder Grün ist kleinster Wert
  return Value;
}



//Funktion Dimmwerte Blau berechnen
float FadeValueBlue(int Red, int Green, int Blue) {
  float Value = 1.0;

  if (Blue == 0) {
    Value = 0.0;
    return Value;
  }

  //Ab hier hat Blau einen Wert

  //Drei Werte vorhanden
  if (Red > 0 && Green > 0) {
    if (Red <= Blue && Red <= Green) {
      Value = (float)Blue / Red;
    }
    if (Green <= Blue && Green <= Red) {
      Value = (float)Blue / Green;
    }
    return Value;
  }

  //Ab hier haben entweder Rot oder Grün eine Null oder Beide

  //Blau und Rot mit Werten; Grün muss Null sein
  if (Red > 0 && Red < Blue) {
    Value = (float)Blue / Red;
    return Value;
  }

  //Ab hier hat Rot eine Null und Grün fraglich

  //Blau und Grün
  if (Green > 0 && Green < Blue) {
    Value = (float)Blue / Green;
    return Value;
  }

  //Ab hier nur Blau vorhanden oder Blau ist kleinster Wert
  return Value;
}



//Funktion NTP-Daten abrufen
void SendNtpRequest() {

  //Verbindung zu einem NTP-Server aus dem Pool aufbauen
  WiFi.hostByName(NtpServerName, TimeServerIP);

  //NTP-Anfrage an den Server stellen
  SendNtpPaket(TimeServerIP);

  delay(50);
}



//Funktion NTP-Zeitstempel auslesen
long GetNtpTimestamp(byte NtpData[]) {

  unsigned long NtpTimestamp = 0;

  //Der Zeitstempel befindet sich im Byte 40, 41, 42, 43
  //Zeitstempel aus den vier Bytes auslesen => Ergebnis: Sekunden seit dem 1. Januar 1900, 00:00:00 Uhr
  NtpTimestamp = word(NtpData[40], NtpData[41]) << 16 | word(NtpData[42], NtpData[43]);
  return NtpTimestamp;
}



//Funktion zur Anfrage eines NTP-Paketes
void SendNtpPaket(IPAddress& address) {
  memset(NtpPaketBuffer, 0, NtpPaketSize);

  //Erstes Byte
  //Leap Indicator: Bit 0, 1 => 11 => 3 => Schaltsekunde wird nicht synchronisiert
  //Version: Bit Bit 2, 3, 4 => 000 => 0 => Version xy?
  //Mode: Bit 5, 6, 7 => 111 => 7 => für den Privaten Gebrauch
  NtpPaketBuffer[0] = 0b11100011;

  //Zweites Byte
  //Stratum: Bit 0 - 7 => 00000000 => 0 => nicht angegeben
  NtpPaketBuffer[1] = 0;

  //Drittes Byte
  //Poll: Bit 0 - 7 => 00000110 => 6 => Abfrageintervall
  NtpPaketBuffer[2] = 6;

  //Viertes Byte
  //Precision: Bit 0 - 7 => 0xEC => Taktgenauigkeit
  NtpPaketBuffer[3] = 0xEC;

  //Die nächsten 8 Byte haben nur für Servernaachrichten eine Bedeutung
  //8 Bytes => 0

  NtpPaketBuffer[12] = 49;
  NtpPaketBuffer[13] = 0x4E;
  NtpPaketBuffer[14] = 49;
  NtpPaketBuffer[15] = 52;

  UDP.beginPacket(address, 123);
  UDP.write(NtpPaketBuffer, NtpPaketSize);
  UDP.endPacket();
}



//Funktion zur Berechnung des Wochentages
//Montag = 1, Dienstag = 2, Mittwoch = 3, Donnerstag = 4, Freitag = 5, Samstag = 6, Sonntag = 7
unsigned int wochenTag_calc(unsigned long tage1900) {

  //Der 1. Januar 1900 war ein Montag
  unsigned int ergebnis = 1;

  int for_i = 0;

  for (for_i = 0; for_i < tage1900; for_i++) {

    if (ergebnis < 7) {
      ergebnis = ergebnis + 1;
    }

    else {
      ergebnis = 1;
    }
  }

  return ergebnis;
}



//Funktion zur Berechnung der Jahreszahl
unsigned int datumJahr_calc(unsigned long sec1900) {

  //NTP beginnt am 1. Januar 1900
  unsigned int ergebnis = 1900;
  unsigned int tageimjahr = 0;
  unsigned int tage = 0;
  unsigned int tage1900 = 0;

  int for_i = 0;
  bool schaltjahr = LOW;

  tage1900 = sec1900 / 86400;

  for (for_i = 0; for_i < tage1900; for_i++) {

    schaltjahr = schaltJahrJN(ergebnis);

    if (schaltjahr) {
      tageimjahr = 366;
    }

    else {
      tageimjahr = 365;
    }

    tage++;

    if (tage >= tageimjahr) {
      ergebnis++;
      tage = 0;
    }
  }

  return ergebnis;
}



//Funktion zur Erkennung ob ein Schaltjahr vorliegt
bool schaltJahrJN(unsigned int jahr) {

  bool ergebnis_jn = LOW;

  //Erkennung von Schaltjahren
  if ((jahr % 4) == 0) {

    ergebnis_jn = HIGH;

    if ((jahr % 100) == 0) {

      ergebnis_jn = LOW;

      if ((jahr % 400) == 0) {

        ergebnis_jn = HIGH;
      }
    }
  }

  else {
    ergebnis_jn = LOW;
  }

  return ergebnis_jn;
}



//Funktion zur Berechnung des Monates
int welcherMonat(int tg_nr, bool schaltjn) {

  //Monatsanfänge
  int monatMin[13] = { 0, 1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };
  //Monatsenden
  int monatMax[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

  int datum = 0;

  int y = 0;

  //Berechnung des Anfang und Ende jeden Montas im Schaltjahr
  if (schaltjn == HIGH) {

    for (y = 3; y < 13; y++) {
      monatMin[y] = monatMin[y] + 1;
    }

    for (y = 2; y < 13; y++) {
      monatMax[y] = monatMax[y] + 1;
    }
  }

  //Monat Januar
  if (tg_nr >= monatMin[1] && tg_nr <= monatMax[1]) {
    datum = 1;
  }

  //Monat Februar
  if (tg_nr >= monatMin[2] && tg_nr <= monatMax[2]) {
    datum = 2;
  }

  //Monat März
  if (tg_nr >= monatMin[3] && tg_nr <= monatMax[3]) {
    datum = 3;
  }

  //Monat April
  if (tg_nr >= monatMin[4] && tg_nr <= monatMax[4]) {
    datum = 4;
  }

  //Monat Mai
  if (tg_nr >= monatMin[5] && tg_nr <= monatMax[5]) {
    datum = 5;
  }

  //Monat Juni
  if (tg_nr >= monatMin[6] && tg_nr <= monatMax[6]) {
    datum = 6;
  }

  //Monat Juli
  if (tg_nr >= monatMin[7] && tg_nr <= monatMax[7]) {
    datum = 7;
  }

  //Monat August
  if (tg_nr >= monatMin[8] && tg_nr <= monatMax[8]) {
    datum = 8;
  }

  //Monat September
  if (tg_nr >= monatMin[9] && tg_nr <= monatMax[9]) {
    datum = 9;
  }

  //Monat Oktober
  if (tg_nr >= monatMin[10] && tg_nr <= monatMax[10]) {
    datum = 10;
  }

  //Monat November
  if (tg_nr >= monatMin[11] && tg_nr <= monatMax[11]) {
    datum = 11;
  }

  //Monat Dezember
  if (tg_nr >= monatMin[12] && tg_nr <= monatMax[12]) {
    datum = 12;
  }

  return datum;
}