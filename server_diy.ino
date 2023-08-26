//Code by Debajyoti Das

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FastLED.h>
#include <ArduinoJson.h>

#include <ESP8266mDNS.h>
#include <LittleFS.h>

#include <ElegantOTA.h>

#define NUM_LEDS 10  //number of leds
#define DATA_PIN 14  //data pin
#define SWT_PIN 5    //button pin

CRGB leds[NUM_LEDS];


//---------------------------------------------
void handleRoot();
void handleSetColor();
void handlebright();
void handletheme();
void savedelta();
void saveinfo();
void savetheme();
void savepal();
void handleget();
void handlegetcolor();
void handleambi();
void handleimg();
void handleback();
void handlefont();
void handledata();
void handletitle();
CRGB hexToCRGB(String hexColor);

//----------------------------------------------
DEFINE_GRADIENT_PALETTE(sunrise){
  0,45, 255, 250,
  40,45, 255, 250,
  70,255, 13, 13 ,
  90,255, 255, 13 ,
  110, 150, 150, 50,
  255, 200, 150, 150
};

DEFINE_GRADIENT_PALETTE(morning){
  0, 45, 252, 247,
  125,255,236,83,
  255, 255, 202, 13
};

DEFINE_GRADIENT_PALETTE(afternoon){
  0,250,250,255,
  40,255,255,0,
  120,255,255,0,
  255,255,255,30
};

DEFINE_GRADIENT_PALETTE(sunset){
  0,255, 0, 1,
  20,255, 20, 1,
  30, 255, 255, 10,
  100,255, 64, 1 ,
  200, 255, 200, 1,
  255, 255, 60, 1
};

DEFINE_GRADIENT_PALETTE(evening){
  0,0, 0, 255,
  20,0, 0, 255,
  50,255,28,1,
  120,144,64,0,
  200, 138, 15, 1,
  255,147,28,0
};

DEFINE_GRADIENT_PALETTE(night){
  0,20,20,254,
  120,20,20,20,
  255,20,20,20
};

DEFINE_GRADIENT_PALETTE(lowlight){
  0,3,3,3,
  50,3,3,3,
  100,3,3,3,
  150,3,3,3,
  255,3,3,3
};

DEFINE_GRADIENT_PALETTE(off){
  0, 0, 0, 0,
  255, 0, 0, 0
};


//----------------------------------------------------
//FLASH VARIABLES
String ssid;
String password;
static CRGB pal_color1;
static CRGB pal_color2;
static CRGB pal_color3;
static int defaultTheme;
static long seconds;
static int chng_speed;

//---------------------------------------------
ESP8266WebServer server(80);


static int brightness_ = 140;
static int themeId = defaultTheme,lasttheme=0;

CRGBPalette16 currentpalette = off;
CRGBPalette16 targetpalette;
static CRGB color1 = pal_color1;
static CRGB color2 = pal_color2;
static CRGB color3 = pal_color3;
int paletteindex = 1;


static unsigned long currentmillis = 0,lastmillis = 0,startsecs=0,tnow,diff,start=0,waitstartsecs=0;
int x,last=0,op;

bool wifiConnect = false;
//--------------------------------------------------

bool loadConfig() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  StaticJsonDocument<400> doc;
  auto error = deserializeJson(doc, configFile);
  if (error) {
    Serial.println("Failed to parse config file");
    return false;
  }

  ssid = String(doc["ssid"]);
  password = String(doc["password"]);
  color1 = CRGB(doc["c1r"], doc["c1g"], doc["c1b"]);
  color2 = CRGB(doc["c2r"], doc["c2g"], doc["c2b"]);
  color3 = CRGB(doc["c3r"], doc["c3g"], doc["c3b"]);
  themeId = doc["themeId"];
  defaultTheme = themeId;
  pal_color1 = color1;
  pal_color2 = color2;
  pal_color3 = color3;
  seconds = doc["seconds"];
  chng_speed=doc["chng"];

  return true;
}

void saveConfig() {
  StaticJsonDocument<400> doc;
  doc["ssid"] = ssid;
  doc["password"] = password;

  doc["c1r"] = pal_color1.r;
  doc["c1g"] = pal_color1.g;
  doc["c1b"] = pal_color1.b;

  doc["c2r"] = pal_color2.r;
  doc["c2g"] = pal_color2.g;
  doc["c2b"] = pal_color2.b;

  doc["c3r"] = pal_color3.r;
  doc["c3g"] = pal_color3.g;
  doc["c3b"] = pal_color3.b;

  doc["themeId"] = defaultTheme;
  doc["seconds"] = seconds;
  doc["chng"]=chng_speed;

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }

  serializeJson(doc, configFile);
}
//------------------------------------------------------

static IPAddress local_ip = IPAddress(192, 168, 0, 181);
static IPAddress gateway = IPAddress(192, 168, 0, 1);
static IPAddress subnet = IPAddress(255, 255, 255, 0);
const char* ap_ssid = "Ambi";

bool connectWifi() {
  int count = 0;
  WiFi.mode(WIFI_STA);
  WiFi.config(local_ip, gateway, subnet);
  WiFi.begin(ssid, password);
  while ((WiFi.status() != WL_CONNECTED) && count < 6) {
    delay(1000);
    Serial.println("Connecting to Wifi...");
    count++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
    return true;
  } else
    return false;
}

bool startHotspot() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ap_ssid);
  Serial.println("Hotspot activated");
  return true;
}

void pwrBut(){
  x=digitalRead(SWT_PIN);
  if(!last && x){
    start=millis();
    op=0;
  }else if(last && x){
    tnow=millis();
    diff=tnow-start;
    if(!op && diff>=50){
      op=1;
    }
    else if(op==1 && diff>=700){
      op=2;
      if(themeId!=0)
      themeId=(themeId>=13)?1:themeId+1;
    }else if(op==2 && diff>=5000){
      for(int i=0;i<NUM_LEDS;i++)
        leds[i]=CRGB::Red;
      FastLED.show();
      ESP.restart();
    }
  }else if(last && !x){
    if(op==1){
      if(!themeId){
        if(lasttheme==14)
          themeId=1;
        else
          themeId=lasttheme;
      }
      else{
        lasttheme=themeId;
        themeId=0;
      }
    }
  }
  last=x;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Mounting FS...");

  pinMode(SWT_PIN,INPUT);

  //FILESYSTEM
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  if (!loadConfig()) {
    Serial.println("Failed to load config");
  } else {
    Serial.println("Config loaded");
  }

  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness_);

  //HANDLE WIFI
  if (!connectWifi()){
    startHotspot();
    leds[0]=CRGB::Red;
  }
  else {
    wifiConnect = true;
    Serial.println(WiFi.localIP());
    leds[0]=CRGB::Green;
  }
  if (MDNS.begin("ambi"))
    Serial.println("MDNS started");
  MDNS.addService("http", "tcp", 80);
  FastLED.show();


  
//HANDLE API
  server.on("/set_color", handleSetColor);
  server.on("/set_bright", handlebright);
  server.on("/theme", handletheme);
  server.on("/savecycle", savedelta);
  server.on("/savewifi", saveinfo);
  server.on("/savetheme", savetheme);
  server.on("/savepal", savepal);
  server.on("/", handleRoot);
  server.on("/get", handleget);
  server.on("/getcolor", handlegetcolor);
  server.on("/chngspeed",change_speed);

  server.on("/data",handledata);
  server.on("/ambi.png",handleambi);
  server.on("/img.png",handleimg);
  server.on("/back.webp",handleback);
  server.on("/font.otf",handlefont);
  server.on("/title.ico",handletitle);
  
  ElegantOTA.begin(&server);
  server.begin();

  delay(2000);
}

bool once=true;

void loop() {
  if (wifiConnect && WiFi.status() != WL_CONNECTED)
    ESP.restart();
  
  MDNS.update();
  server.handleClient();

  pwrBut();
  nblendPaletteTowardPalette(currentpalette, targetpalette, chng_speed);
  if(themeId!=0)fill_palette(leds, NUM_LEDS, 0, 255 / NUM_LEDS, currentpalette, brightness_, LINEARBLEND);
  FastLED.show();

  if(themeId!=14 && once==false) once=true;
  switch (themeId) {
    case 0:
      targetpalette = off;
      fadeToBlackBy(leds, NUM_LEDS, 1);
      break;
    case 1:
      targetpalette = sunrise;
      break;
    case 2:
      targetpalette = morning;
      break;
    case 3:
      targetpalette = afternoon;
      break;
    case 4:
      targetpalette = sunset;
      break;
    case 5:
      targetpalette = evening;
      break;
    case 6:
      targetpalette = night;
      break;
    case 7:
      targetpalette = lowlight;
      break;
    case 8:
      targetpalette = CRGBPalette16(color1);
      break;
    case 9:
      targetpalette = CRGBPalette16(color1, color2);
      break;
    case 10:
      targetpalette = CRGBPalette16(color1, color2, color3);
      break;

    case 11:
      currentmillis = millis();
      switch (paletteindex) {
        case 1:
          targetpalette = sunrise;
          break;
        case 2:
          targetpalette = morning;
          break;
        case 3:
          targetpalette = afternoon;
          break;
        case 4:
          targetpalette = sunset;
          break;
        case 5:
          targetpalette = evening;
          break;
        case 6:
          targetpalette = night;
          break;
        case 7:
          targetpalette = lowlight;
          break;
        default:
          paletteindex = 1;
          break;
      }
      if (currentmillis - lastmillis >= seconds * 1000) {
        if (paletteindex++ > 7)
          paletteindex = 1;
        lastmillis = currentmillis;
      }
      break;

    case 12:
      currentmillis = millis();
      switch (paletteindex) {
        case 1:
          targetpalette = CRGBPalette16(color1);
          break;
        case 2:
          targetpalette = CRGBPalette16(color2);
          break;
        case 3:
          targetpalette = CRGBPalette16(color3);
          break;
        default:
          paletteindex = 1;
          break;
      }
      if (currentmillis - lastmillis >= seconds * 1000) {
        if (paletteindex++ > 3)
          paletteindex = 1;
        lastmillis = currentmillis;
      }
      break;

    case 13:
      currentmillis = millis();
      switch (paletteindex) {
        case 1:
          targetpalette = CRGBPalette16(color1, color2, color3);
          break;
        case 2:
          targetpalette = CRGBPalette16(color3, color1, color2);
          break;
        case 3:
          targetpalette = CRGBPalette16(color2, color3, color1);
          break;
        default:
          paletteindex = 1;
          break;
      }

      if (currentmillis - lastmillis >= seconds * 1000) {
        if (paletteindex++ > 3)
          paletteindex = 1;
        lastmillis = currentmillis;
      }
      break;

    case 14:
      if(startsecs>=0 && startsecs <5){
        targetpalette=off;
      }else if(startsecs>=5 && startsecs <9){
        targetpalette=sunrise;
      }else if(startsecs>=9 && startsecs <13){
        targetpalette=morning;
      }else if(startsecs>=13 && startsecs <16){
        targetpalette=afternoon;
      }else if(startsecs>=16 && startsecs <18){
        targetpalette=sunset;
      }else if(startsecs>=18 && startsecs <20){
        targetpalette=evening;
      }else{
        targetpalette=night;
      }

      if(once){lastmillis=millis();once=false;}
      currentmillis = millis();
      if (currentmillis - lastmillis >= waitstartsecs*60*1000) {
        startsecs++;
        waitstartsecs=60;
        Serial.println(startsecs);
        Serial.println(waitstartsecs);

        if(startsecs==24)
          startsecs=0;
        lastmillis = currentmillis;
      }
      break;
  }
}

//--------------------------------------------------------------------
//HANDLE ROUTES
void handledata(){
  File jsonFile = LittleFS.open("/config.json", "r");
  if (!jsonFile) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(jsonFile, "application/json");
  jsonFile.close();
}

void handleambi(){
  File ambifile = LittleFS.open("/ambi.png", "r");
  if (!ambifile) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.streamFile(ambifile, "image/png");
  ambifile.close();
}

void handleimg(){
  File imgfile = LittleFS.open("/img.png", "r");
  if (!imgfile) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.streamFile(imgfile, "image/png");
  imgfile.close();
}

void handleback(){
  File file = LittleFS.open("/back.webp", "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.streamFile(file, "image/webp");
  file.close();
}

void handlefont(){
  File file = LittleFS.open("/font.otf", "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.streamFile(file, "font/otf");
  file.close();
}

void handletitle(){
  File file = LittleFS.open("/title.ico", "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.streamFile(file, "image/x-icon");
  file.close();
}

void handleRoot() {
  // server.send(200, "text/html", root);
  File file = LittleFS.open("/root.html", "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.streamFile(file, "text/html");
  file.close();
}

void sendOK(String msg) {
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["status"] = "OK";
  jsonDoc["message"] = msg;
  String jsonResponse;
  serializeJson(jsonDoc, jsonResponse);
  server.send(200, "application/json", jsonResponse);
}

void sendFail() {
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["status"] = "Failed";
  String jsonResponse;
  serializeJson(jsonDoc, jsonResponse);
  server.send(200, "application/json", jsonResponse);
}

//-------------------------------------------------------------

void change_speed(){
  String chng = server.arg("chng");
  chng_speed = chng.toInt();
  saveConfig();
  sendOK(chng);
}

void handleget() {

  StaticJsonDocument<200> jsonDoc;
  jsonDoc["bright"] = brightness_;
  jsonDoc["spd"]=chng_speed;
  jsonDoc["theme"] = themeId;
  jsonDoc["default"] = defaultTheme;
  jsonDoc["cycle"]=seconds;
  jsonDoc["ssid"]=ssid;
  String jsonResponse;
  serializeJson(jsonDoc, jsonResponse);
  server.send(200, "application/json", jsonResponse);
}

void handlegetcolor() {
  StaticJsonDocument<200> crgb;
  crgb["c1r"] = color1.r;
  crgb["c1g"] = color1.g;
  crgb["c1b"] = color1.b;
  crgb["c2r"] = color2.r;
  crgb["c2g"] = color2.g;
  crgb["c2b"] = color2.b;
  crgb["c3r"] = color3.r;
  crgb["c3g"] = color3.g;
  crgb["c3b"] = color3.b;
  String rgbString;
  serializeJson(crgb, rgbString);
  server.send(200, "application/json", rgbString);
}

void saveinfo() {
  password = server.arg("pass");
  ssid = server.arg("ssid");
  saveConfig();
  sendOK("saved");
  ESP.restart();
}

void savedelta() {
  String delta = server.arg("delta");
  seconds = delta.toInt();
  sendOK(delta);
  saveConfig();
}

void savetheme() {
  String themeid = server.arg("deftheme");
  defaultTheme = themeid.toInt();
  sendOK(themeid);
  saveConfig();
}

void savepal() {
  pal_color1 = color1;
  pal_color2 = color2;
  pal_color3 = color3;
  sendOK("pallete saved");
  saveConfig();
}

void handletheme() {
  String themeid = server.arg("themeid");
  String ss=server.arg("ss");
  startsecs=ss.toInt()/60;
  waitstartsecs=60-(ss.toInt()%60);
  themeId = themeid.toInt();
  if(themeid.toInt()!=0)
    lasttheme=themeId;
  
  sendOK(themeid);
}

void handleSetColor() {
  String hexColorCode = server.arg("color");
  int id = server.arg("id").toInt();
  CRGB temp;
  switch (id) {
    case 1:
      color1 = hexToCRGB(hexColorCode);
      break;
    case 2:
      color2 = hexToCRGB(hexColorCode);
      break;
    case 3:
      color3 = hexToCRGB(hexColorCode);
      break;
    default:
      sendFail();
      return;
  }
  sendOK(hexColorCode);
}

void handlebright() {
  String brightness = server.arg("bright");
  int b = brightness.toInt();
  FastLED.setBrightness(b);
  FastLED.show();
  brightness_ = b;
  sendOK(brightness);
}

//---------------------------------------------------------------
CRGB hexToCRGB(String hexColor) {
  long hexValue = strtol(hexColor.c_str(), NULL, 16);
  byte red = (hexValue >> 16) & 0xFF;
  byte green = (hexValue >> 8) & 0xFF;
  byte blue = hexValue & 0xFF;
  return CRGB(red, green, blue);
}
