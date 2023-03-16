#include "U8glib.h"
#include <stdlib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MAX31865.h>

#define ONE_WIRE_BUS 2
#define CALDO A5 //realy hot port
#define FREDDO A4  //relay cold port
#define RANGE 0.5

//value fot the termal couple
#define RREF      430.0
#define RNOMINAL  100.0

Adafruit_MAX31865 thermo = Adafruit_MAX31865(6, 5, 7, 8); //set library var fo the thermo
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

U8GLIB_SSD1306_128X64 u8g(13, 11, 12, 9, 10);   // SPI communication: SCL = 13, SDA = 11, RES = 10, DC = 9, CS = 8 for the oled display

int potentiometer_value = 190;    // value from the potentiometer
char buffer[20];                  // generic buffer used for general purpose
int string_width;     

float pixel_x = 0;     // x pos for pixel
float pixel_y = 0;     // y pos for pixel
float line_x = 0;      // x pos for line end
float line_y = 0;      // y pos for line end
float text_x = 0;      // x pos for text
float text_y = 0;      // y pos for text

int center_x = 64;     // x center of the knob 
int center_y = 108;    // y center of the knob (outside of the screen)
int radius_pixel = 92; // radius for pixel tickmarks
int radius_line = 87;  // radius for line end
int radius_text = 75;  // radius for text

int angle;             // angle for the individual tickmarks
int tick_value;        // numeric value for the individual tickmarks

byte precalculated_x_radius_pixel[180]; 
byte precalculated_y_radius_pixel[180]; 

float tempGoal = 19;      // default temperature goal

//some variables for coutning time
const unsigned long deltaTime = 1 * 1 * 1000UL;
const unsigned long deltaTime_2 = 1 * 2 * 1000UL;
static unsigned long lastSampleTime = 0 - deltaTime;  
static unsigned long lastSampleTime_2 = 0 - deltaTime_2; 

boolean buttonStatus = true; //swith on thermo on mode
boolean buttonStatus1 = false; //thermo on/off
boolean firstRead = true;
boolean ac = false; //if true, or caldo or freddo is on, if false nothing
float temp = 0;

//relay controll variables
boolean hot = false;
boolean cold = false;
boolean bloccaAC = false; 

//univr logo
const uint8_t upir_logo[] U8G_PROGMEM = {
  B00010010,  B10010100, B10101010,
  B00010010,  B11010000, B10101100,
  B00010010,  B10110100, B10101000,
  B00001100,  B10010100, B11001000
};

//fire logo
const uint8_t hot_logo[] U8G_PROGMEM = {
  B00010000,  
  B00011000,  
  B01100100,  
  B01000010,
  B01100010,  
  B01100010,
  B00111100
};

//cold logo
const uint8_t cold_logo[] U8G_PROGMEM = {
  B00101000,  
  B00010000,  
  B10010010,  
  B01111100,
  B10010010,  
  B00010000,
  B00101000
};


//off
const uint8_t off_logo[] U8G_PROGMEM = {
 B00010000,  
  B01010100,  
  B10010010,  
  B10000010,
  B10000010,
  B10000010,
  B01111100, 
};


/*
  In this page you can set the temperature goal with the potentiometer
*/
void secondPage(void)
{
  u8g.firstPage();                // required for u8g library
  do {                   
    u8g.setColorIndex(1);          // set color to white
    u8g.setFont(u8g_font_6x10r);   // set smaller font for tickmarks   
 
    // calculate tickmarks
    for (int i=-48; i<=48; i=i+3) {                                // only try to calculate tickmarks that would end up be displayed
      angle = i + ((potentiometer_value*3)/10) % 3;                // final angle for the tickmark
      tick_value = round((potentiometer_value/10.0) + angle/3.0);  // get number value for each tickmark
      pixel_x = precalculated_x_radius_pixel[angle+90];              // get x value from lookup table
      pixel_y = precalculated_y_radius_pixel[angle+90];              // get y value from lookup table

      if (pixel_x > 0 && pixel_x < 128 && pixel_y > 0 && pixel_y < 64) {  // only draw inside of the screen

        if(tick_value >= 0 && tick_value <= 40) {  // only draw tickmarks between values 0-100%, could be removed when using rotary controller

          if (tick_value % 10 == 0) {                                // draw big tickmark == lines + text
            line_x =  sin(radians(angle)) * radius_line + center_x;  // calculate x pos for the line end
            line_y = -cos(radians(angle)) * radius_line + center_y;  // calculate y pos for the line end
            u8g.drawLine(pixel_x, pixel_y, line_x, line_y);          // draw the line

            text_x =  sin(radians(angle)) * radius_text + center_x;  // calculate x pos for the text
            text_y = -cos(radians(angle)) * radius_text + center_y;  // calculate y pos for the text 
            itoa(tick_value, buffer, 10);                            // convert integer to string
            string_width = u8g.getStrWidth(buffer);                  // get string width
            u8g.drawStr(text_x - string_width/2, text_y, buffer);    // draw text - tickmark value
            
          } 
          else {                                                     // draw small tickmark == pixel tickmark
            u8g.drawPixel(pixel_x, pixel_y);                         // draw a single pixel
          }      
        }
      }
    }
    Serial.println(analogRead(A0));
    u8g.setFont(u8g_font_8x13);                                     // set bigger font
    dtostrf(potentiometer_value/10.0, 1, 1, buffer);                // float to string, -- value, min. width, digits after decimal, buffer to store
    tempGoal = potentiometer_value/10.0;
    buffer[4] = '°';
    buffer[5] = 'C';
    string_width = u8g.getStrWidth(buffer);                         // calculate string width

    u8g.setColorIndex(1);                                           // set color to white
    u8g.drawRBox(64-(string_width+4)/2, 0, string_width+4, 11, 2);  // draw background rounded rectangle
    u8g.drawTriangle( 64-3, 11,   64+4, 11,   64, 15);              // draw small arrow below the rectangle
    u8g.setColorIndex(0);                                           // set color to black 
    u8g.drawStr(64-string_width/2, 10, buffer);                     // draw the value on top of the display

    u8g.setColorIndex(1);
    u8g.drawBitmapP(104, 1, 3, 4, upir_logo);  

  } while ( u8g.nextPage() );    // required for u8g library

  potentiometer_value = map(analogRead(A0), 0, 1014, 0, 400);       // read the potentiometer value, remap it to 0-1000

}


/*
  It controlls the relay flag about cooling/heating
*/
void tempHandler(void){

  if(temp>(potentiometer_value/10.0)+RANGE){ //turn on cooling mode
        digitalWrite(FREDDO, HIGH);
        digitalWrite(CALDO, LOW);
        ac = true;
        cold = true;
        hot = false;
      }
  else if(temp<(potentiometer_value/10.0)-RANGE){ //turn on heatig mode
        digitalWrite(FREDDO, LOW);
        digitalWrite(CALDO, HIGH);
        ac = true;
        hot = true;
        cold = false;
    }
}



/*
  This function creates the string to be uploaded to the cloud and sends it to the esp (whose work is to permorm the upload)
*/
void sendData(void){
  unsigned long now = millis();
  if(lastSampleTime_2 > now){   //if millis resetted (c.a. 5 days)
    lastSampleTime_2 = 0;
  }
  if (now - lastSampleTime_2 >= deltaTime_2){ //it reads the temperature after first boot and then after every 5 minutes
      lastSampleTime_2 += deltaTime_2;

    String apiKey = "EKL504G6U3M10XJK";
    String postStr = apiKey;
    postStr +="&field5=";  //actual temperature
    postStr += String(temp);
    postStr += "&field2="; //temperatura goal
    dtostrf(potentiometer_value/10.0, 1, 2, buffer);
    postStr += buffer;
    postStr += "&field3="; //heating temperature flag
    if(hot)
      postStr += String(1);
    else
      postStr += String(0);
    postStr += "&field4="; //cooling temperature flag
    if(cold)
      postStr += String(1);
    else
      postStr += String(0);
    postStr += "\r\n\r\n\r\n\r\n";
    Serial.print(postStr); //sending string to esp via rx/tx
  }
  
}

/*
  Main page with actual temperature and temperature goal, and ac status (heat/cooling/off)
*/
void mainPage(void)
{

  
  if(true){
    unsigned long now = millis();
    if(lastSampleTime > now){   //if millis resetted (c.a. 5 days)
      lastSampleTime = 0;
      firstRead = true;
    }
    if (now - lastSampleTime >= deltaTime || firstRead){ //it reads the temperature after first boot and then after every 5 minutes
      lastSampleTime += deltaTime;

      uint16_t rtd = thermo.readRTD();
      temp = thermo.temperature(RNOMINAL, RREF);
 
      if(firstRead){
         firstRead = false;
      }
      
      //this check aims to turn on the rely only when the actual temperature is not in the range of the the goal
      if((temp>=(potentiometer_value/10.0)-RANGE && temp<=(potentiometer_value/10.0)+RANGE) || !buttonStatus1){
        bloccaAC = true;
      }
      else{
        bloccaAC = false;
      }

      
      if(ac && buttonStatus1){
        if((temp>=(potentiometer_value/10.0)+0.25 && hot == true)  || (temp<=(potentiometer_value/10.0)-0.25 && cold == true)){ //if the temperature is in the goal, set flag about it
          digitalWrite(FREDDO, LOW);
          digitalWrite(CALDO, LOW);
          ac = false;
          hot = false;
          cold = false;
        }
        else
          tempHandler(); 
      }
      else if(!bloccaAC){
        tempHandler();
      }
    }
  }

  u8g.firstPage();         
  do {              //lets print al the info for the main thermo page mode on

    buffer[5] = ' ';
    u8g.setFont(u8g_font_8x13B); //change font
    dtostrf(potentiometer_value/10.0, 1, 2, buffer);  // float to string, -- value, min. width, digits after decimal, buffer to store
    
    if(potentiometer_value/10.0<10){ 
      buffer[3] = '°';
      buffer[4] = ' ';
    }
    else
      buffer[4] = '°';

    u8g.drawStr(75, 32, buffer); //print temperature goal
    dtostrf(temp, 1, 2, buffer); 
    buffer[5] = '°';
    u8g.drawStr(10, 32, buffer);  //print actual temperature
    
    u8g.drawLine(65, 15, 65, 50);
    u8g.drawLine(55, 38, 75, 38);
    u8g.setFont(u8g_font_6x13);  //change font
    u8g.drawStr(20, 50, "Temp");
    u8g.drawStr(83, 50, "Goal");
    u8g.drawBitmapP(104, 2, 3, 4, upir_logo);
    
    if(cold) //show cooling logo
      u8g.drawBitmapP(1, 1, 1, 7, cold_logo);
    else if(hot) //show heating logo
      u8g.drawBitmapP(0, 1, 1, 7, hot_logo);

    if(!buttonStatus1 && !cold && !hot)
      offPage(); //thermo mode off
  } while ( u8g.nextPage() ); 

}

/*
If thermo mode off, it will be show this page, only the actual temperature
*/
void offPage(void){

  u8g.firstPage();   
  do {      
    buffer[5] = ' ';
    u8g.setFont(u8g_font_fub14);  //change font  to one bigger
    dtostrf(temp, 1, 2, buffer);
    buffer[5] = '°';
    buffer[6] = 'C';
    string_width = u8g.getStrWidth(buffer); 
    u8g.drawStr(64-string_width/2, 40, buffer); //center and print actual temperature
    u8g.drawBitmapP(104, 3, 3, 4, upir_logo);
    u8g.drawBitmapP(1, 1, 1, 7, off_logo);  //print off logo, thats 'cause we're in thermo off mode
  } while ( u8g.nextPage() );

  buffer[6] = '\0';
}


boolean tasto;
boolean prev_tasto;

/*
  This function read all the avaiable input button wired to the arduino
*/
void readButtons(void){
 
  tasto = digitalRead(4);

  if(!prev_tasto && tasto) //rising edge
    prev_tasto = tasto;

  if(prev_tasto && !tasto){ //falling edge
    prev_tasto = tasto;
    buttonStatus = !buttonStatus;
  }

  if(digitalRead ( 2 ) == 1){ //read on/off button
    if(buttonStatus1 == false){
      firstRead = true;
      hot = false;
      cold = false;
    }
    buttonStatus1 = true;
    

  }
  else{
    buttonStatus1 = false;
    cold = false;
    hot = false;
    digitalWrite(FREDDO, LOW);
    digitalWrite(CALDO, LOW);
  }

}

void setup() {
  
  pinMode ( 2 , INPUT_PULLUP ); //thermo mode on/off
  pinMode ( 4 , INPUT_PULLUP ); //swith between thermo on pages
  Serial.begin(9600);
  u8g.setColorIndex(1);          // set color to white

  for (int i = 0; i < 180; i++){    // pre-calculate x and y positions into the look-up tables
     precalculated_x_radius_pixel[i] =  sin(radians(i-90)) * radius_pixel + center_x; 
     precalculated_y_radius_pixel[i] = -cos(radians(i-90)) * radius_pixel + center_y;      
  }

  thermo.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
  
}


void loop() {

  readButtons(); //read button status

  if(buttonStatus1 == false)
     firstRead = true;
  
  if ( buttonStatus == true || (buttonStatus1 == false && buttonStatus == false))
  {
    mainPage();
  }
  else{
    secondPage();
  }

  sendData(); //this function creates the string to be uploaded to the cloud and sends it to the esp (whose work is to permorm the upload)

}


