#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "GlobalState.h"
#include "handler.h"
#include "menuHandlr.h"
#include "pages.h"
#include "oled_ui.h"

namespace {
uint8_t fb[1024]; bool ready=false; uint32_t lastDraw=0;
const uint8_t font[][5]={
{0,0,0,0,0},{0,0,95,0,0},{0,3,0,3,0},{20,127,20,127,20},{36,42,127,42,18},
{35,19,8,100,98},{54,73,85,34,80},{0,5,3,0,0},{0,28,34,65,0},{0,65,34,28,0},
{20,8,62,8,20},{8,8,62,8,8},{0,80,48,0,0},{8,8,8,8,8},{0,96,96,0,0},
{32,16,8,4,2},{62,81,73,69,62},{0,66,127,64,0},{66,97,81,73,70},{33,65,69,75,49},
{24,20,18,127,16},{39,69,69,69,57},{60,74,73,73,48},{1,113,9,5,3},{54,73,73,73,54},
{6,73,73,41,30},{0,54,54,0,0},{0,86,54,0,0},{8,20,34,65,0},{20,20,20,20,20},
{65,34,20,8,0},{2,1,81,9,6},{50,73,121,65,62},{126,17,17,17,126},{127,73,73,73,54},
{62,65,65,65,34},{127,65,65,34,28},{127,73,73,65,65},{127,9,9,1,1},{62,65,73,73,122},
{127,8,8,8,127},{0,65,127,65,0},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},
{127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},{62,65,81,33,94},
{127,9,25,41,70},{70,73,73,73,49},{1,1,127,1,1},{63,64,64,64,63},{31,32,64,32,31},
{127,32,24,32,127},{99,20,8,20,99},{3,4,120,4,3},{97,81,73,69,67}
};
void cmd(uint8_t c){Wire.beginTransmission(0x3C);Wire.write(0);Wire.write(c);Wire.endTransmission();}
void data(const uint8_t*p){Wire.beginTransmission(0x3C);Wire.write(0x40);Wire.write(p,128);Wire.endTransmission();}
void px(int x,int y){if(x>=0&&x<128&&y>=0&&y<64)fb[x+(y>>3)*128]|=1<<(y&7);}
void txt(int x,int y,const char*s){while(*s){uint8_t c=*s++;if(c>=32&&c<=90){const uint8_t*g=font[c-32];for(int a=0;a<5;a++)for(int b=0;b<7;b++)if(g[a]&(1<<b))px(x+a,y+b);}x+=6;}}
void box(int x,int y,int w,int h,bool active){for(int i=0;i<w;i++){px(x+i,y);px(x+i,y+h-1);}for(int i=0;i<h;i++){px(x,y+i);px(x+w-1,y+i);}if(active){px(x+2,y+2);px(x+3,y+3);}}
void num(int x,int y,int n){char b[10];snprintf(b,sizeof(b),"%d",n);txt(x,y,b);}
void flush(){for(uint8_t p=0;p<8;p++){cmd(0xB0+p);cmd(0);cmd(0x10);data(&fb[p*128]);}}
void oledInit(){const uint8_t a[]={0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0,0x40,0x8D,0x14,0x20,0,0xA1,0xC8,0xDA,0x12,0x81,0x7F,0xD9,0xF1,0xDB,0x40,0xA4,0xA6,0xAF};for(uint8_t c:a)cmd(c);}
void dashboard(){
 txt(2,1,"UART SLAVE");txt(74,1,"ACTIVE");txt(110,1,activeStation==STATION_MODE_SOLDER?"S":"H");
 box(1,15,62,47,activeStation==STATION_MODE_SOLDER);box(65,15,62,47,activeStation==STATION_MODE_HOTAIR);
 txt(6,18,"SOLDER");num(6,29,currentTemp);txt(30,29,"C");
 if(tipError)txt(6,43,"NO TIP");else if(sleeping)txt(6,43,"SLEEP");else if(boostMode)txt(6,43,"BOOST");else txt(6,43,pwmOut?"ON":"OFF");
 num(39,43,targetTemp);txt(53,43,"C");
 txt(70,18,"HOT AIR");num(70,29,airGetTemp());txt(94,29,"C");txt(70,43,airGetModeStr());num(70,53,airGetTargetTemp());txt(94,53,"C");
}
const char*pn(){switch(page){case PAGE_SET:return"SET";case PAGE_BOOST:return"BOOST";case PAGE_SLEEP:return"SLEEP";case PAGE_CAL:return"CAL";case PAGE_PID:return"PID";case PAGE_TIP:return"TIP";case PAGE_BUZZER:return"BUZZER";default:return"MENU";}}
void menu(){
 txt(2,1,"MENU");txt(42,1,pn());
 if(isStationMenu()){const char*n[]={"SOLDER","HOT AIR","SAVE","EXIT"};for(int i=0;i<4;i++){if(i==getStationItem())txt(2,13+i*12,">");txt(10,13+i*12,n[i]);}return;}
 if(page==PAGE_SET){const char*n[]={"STATION","TEMP","BOOST","SLEEP","CAL","PID","TIP","BUZZER","SAVE","EXIT"};for(int i=0;i<SET_COUNT&&i<4;i++){if(i==item)txt(2,13+i*12,">");txt(10,13+i*12,n[i]);}if(isEditingValue&&item==SET_TEMP){txt(58,49,"EDIT");num(82,49,activeStation==STATION_MODE_HOTAIR?airGetTargetTemp():targetTemp);}}
 else {txt(2,15,"ITEM");num(38,15,item);txt(2,29,isEditingValue?"EDIT":"SELECT");}
}
}
void initOledUI(){Wire.setSDA(PIN_OLED_SDA);Wire.setSCL(PIN_OLED_SCL);Wire.begin();oledInit();memset(fb,0,sizeof(fb));flush();ready=true;Serial.println(F("[OLED] UI ready"));}
void updateOledUI(){if(!ready||millis()-lastDraw<250)return;lastDraw=millis();memset(fb,0,sizeof(fb));if(inMenu)menu();else dashboard();flush();}
