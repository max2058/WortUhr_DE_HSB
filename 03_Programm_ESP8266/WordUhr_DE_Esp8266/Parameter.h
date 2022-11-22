
#ifndef Parameter_h
#define Parameter_h

typedef struct {

  int LoginSaved = 0;
  char Ssid[40] = "";
  char Password[40] = "";
  int ConfigSaved = 0;
  int RGBValueRed = 0;
  int RGBValueBlue = 0;
  int RGBValueGreen = 0;
  int EastWest = 0;
  int EnableNightMode = 0;
  int TimeDisplayOff = 0;
  int TimeDisplayOn = 0;
  
} Parameter_t;

#endif
