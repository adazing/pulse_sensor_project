//display
#define USE_ARDUINO_INTERRUPTS true    // Set-up low-level interrupts for most acurate BPM math
#include <PulseSensorPlayground.h>     // Includes the PulseSensorPlayground Library
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define USE_ARDUINO_INTERRUPTS true
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include<Wire.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//dht sensor
#include <DHT.h>
#define DHTPIN  7    //Arduino Digital pin connected to Dout of DHT11
#define DHTTYPE DHT11   //Type of DHT Sensor (DHT11)
DHT dht(DHTPIN, DHTTYPE);
const long eventTime_DHT = 3000; //Update Time in mili Second
unsigned long previousTime_DHT = 0;


//pulse sensor
const int PulseWire = A0;       // 'S' Signal pin connected to A0
const int LED13 = 13;          // The on-board Arduino LED
int Threshold = 800;           // Determine which Signal to "count as a beat" and which to ignore                        
PulseSensorPlayground pulseSensor;  // Creates an object


//BPM moving average
int bpm_add;
int bpm_length=5;
int bpm_val;
int bpm_values[5];


//Body Temperature moving average
int bt_add;
int bt_length=5;
int bt_val;
int bt_values[5];


//temperature moving average
int HI_add;
int HI_length=5;
int HI_val;
int HI_values[5];


//thermistor
int ThermistorPin = A1;
int Vo;
float R1 = 10000;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

//button
const int buttonPin=2;
int buttonState = 0;

//buzzer
const int buzzer = 9; //buzzer to arduino pin 9

//warning
int recently_warned=0;

//false alarm
int recently_false=0;


void setup() {


  //setup  
  Serial.begin(9600); //start serial
  pinMode(PulseWire, INPUT);// pulse sensor - A0
  pinMode(buttonPin, INPUT); // button - 2
  pinMode(buzzer, OUTPUT); // buzzer - 9


  //dht sensor configuration
  dht.begin();

  Serial.println("Haha1");
  
  // display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  //display configuration
  // if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
  //   // Serial.println("blah");
  //   Serial.println(F("SSD1306 allocation failed"));
  //   for(;;); // Don't proceed, loop forever
  // }
  Serial.println("Haha");

  display.clearDisplay();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("HEAT SAVER"));
  display.display();


  // PulseSensor configuration
  pulseSensor.analogInput(PulseWire);
  pulseSensor.blinkOnPulse(LED13);       // Blink on-board LED with heartbeat
  pulseSensor.setThreshold(Threshold);
  // Double-check the "pulseSensor" object was created and began seeing a signal
  if (pulseSensor.begin()) {
    Serial.println("PulseSensor object created!");
  }


  //configure BPM moving average
  for (int i = 0; i < bpm_length; i++){
    bpm_values[i]=80;
  }


  //configure body temperature moving average
  for (int i = 0; i < bt_length; i++){
    bt_values[i]=98.6;
  }


  //configure air temperature moving average
  for (int i = 0; i < HI_length; i++){
    HI_values[i]=98.6;
  }
}


float HeatIndex(float RH, float T){


  RH /= 100;
  //formula for calculating Heat index
  float HI = -42.379 + 2.04901523*T + 10.14333127*RH - 0.22475541*T*RH - 0.00683783*T*T - 0.05481717*RH*RH + 0.00122874*T*T*RH + 0.00085282*T*RH*RH - 0.00000199*T*T*RH*RH;
//formula for calculating Heat index if Humidity < 13% and temp. is
//between 80 degree Fahrenheit and 112 degree Fahrenheit.
  if(RH < 13 && T >= 80 && T < 112)
  {
    float Adj1 = ((13-RH/4))*sqrt((17-fabs(T-95.0))/17);
    HI = HI + Adj1;
   }
//formula for calculating Heat index if Humidity > 13% and temp. is
//between 80 degree Fahrenheit and 87 degree Fahrenheit.
  if(RH > 85 && (T < 87 && T > 80))
  {
    float Adj2 = ((RH-85)/10)*((87-T)/5);
    HI = HI + Adj2;
  }
//If Heat index is below then 80 Fahrenheit
  if(HI < 80)
  {
    HI = 0.5*(T + 61.0 + ((T-68.0)*1.2) + (RH*0.094));
  }
  return HI;
}


void loop() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Heat Saver"));
  display.display();
  if (recently_warned != 0){
    recently_warned-=1;
  }
  if (recently_false != 0){
    recently_false-=1;
  }

  //find moving average BPM
  bpm_add=0;
  for(int x = 0; x < bpm_length; x++){
    bpm_add+=bpm_values[x];
  }
  int BPM = float(bpm_add/bpm_length);
  Serial.print("BPM: ");
  Serial.println(BPM);

  display.setCursor(20, 20);
  display.println(BPM);
  display.display();

  //find next BPM for moving average  
  bpm_val= pulseSensor.getBeatsPerMinute();      // Calculates BPM
  for (int y = bpm_length-1; y>0; y--){// ex: 1 2 3 //1 2 2 //1 1 2
    bpm_values[y]=bpm_values[y-1];
  }
  //set the first number of values to the new val
  bpm_values[0]=bpm_val;



  // noTone(buzzer);     // Stop sound...


  //read Thermistor
  Vo = analogRead(ThermistorPin);
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 273.15;
  T = (T * 9.0)/ 5.0 + 32.0 + 6.78;
  Serial.println(" F");
  //find moving average body temperature
  bt_add=0;
  for(int x = 0; x < bt_length; x++){
    bt_add+=bt_values[x];
  }
  int BT = float(bt_add/bt_length);
  Serial.print("Body Temperature: ");
  Serial.println(BT);
  //find next body temperature for moving average  
  bt_val= BT;
  for (int y = bt_length-1; y>0; y--){// ex: 1 2 3 //1 2 2 //1 1 2
    bt_values[y]=bt_values[y-1];
  }
  //set the first number of values to the new val
  bt_values[0]=bt_val;


  //read button
  buttonState = digitalRead(buttonPin);
  Serial.print("Button ");
  Serial.println(buttonState);


  //read DHT sensor
  float RH = dht.readHumidity(); // getting the value of relative humidity
  float t = dht.readTemperature(); // getting the value of temp. in Celsius
  float T = dht.readTemperature(true); // getting the value of temp. in Fahrenheit
  Serial.print("Air Temperature: ");
  Serial.print(T);
  Serial.println(" F");
  Serial.print("Relative Humidity: ");
  Serial.print(RH);
  Serial.println(" %");
  float HI = HeatIndex(RH, T);


  //find moving average air Heat Index
  HI_add=0;
  for(int x = 0; x < HI_length; x++){
    HI_add+=HI_values[x];
  }
  int airHI = float(HI_add/HI_length);
  Serial.print("Heat Index: ");
  Serial.println(airHI);
  //find next heat index for moving average  
  HI_val= airHI;
  for (int y = HI_length-1; y>0; y--){// ex: 1 2 3 //1 2 2 //1 1 2
    HI_values[y]=HI_values[y-1];
  }
  //set the first number of values to the new val
  HI_values[0]=HI_val;


  //warning?
  if ((HI>103 || BPM>100 || BT>100) && recently_warned==0 ){
    recently_warned = 360;
    for(int x=0; x<5; x++){
      tone(buzzer, 200);
      delay(500);
      noTone(buzzer);
      delay(500);
    }
    while (buttonState==LOW){
      buttonState = digitalRead(buttonPin);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(F("Caution!"));
      display.display();
    }
    delay(1000);
    buttonState=LOW;
    while (buttonState==LOW){
      noTone(buzzer);
      buttonState = digitalRead(buttonPin);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(F("Rest in a cool/shaded area."));
      display.display();
    }
    delay(1000);
    buttonState=LOW;
    while (buttonState==LOW){
      buttonState = digitalRead(buttonPin);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(F("Remove unneeded clothing."));
      display.display();
    }
    delay(1000);
    buttonState=LOW;
    while (buttonState==LOW){
      buttonState = digitalRead(buttonPin);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(F("Put wet cloths on body or take cool bath."));
      display.display();
    }
    delay(1000);
    buttonState=LOW;
    while (buttonState==LOW){
      buttonState = digitalRead(buttonPin);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(F("Sip water."));
      display.display();
    }
    delay(1000);
    buttonState=LOW;
    while (buttonState==LOW){
      buttonState = digitalRead(buttonPin);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(F("Maybe call your guardian."));
      display.display();
    }
  }


  //heatstroke detected
  // Serial.println(-0.01692608*HI-0.0476577*t+0.033539348*RH+0.03877972*float(add/length)-2.5337396);
  if(0.12117384*HI-0.00442989*BPM+0.46277294*BT-29.234274>0.0){
    display.setTextSize(1.5); // Draw 2X-scale text
    delay(1000);
    buttonState=LOW;
    tone(buzzer, 1000);
    while (buttonState==LOW){
      buttonState = digitalRead(buttonPin);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(F("Heat stroke detected! Click once to continue or click and hold to cancel."));
      display.display();
    }
    delay(1000);
    buttonState = digitalRead(buttonPin);
    if (buttonState){
      noTone(buzzer);
      recently_false=360;
    }else{
    buttonState=LOW;
    while (buttonState==LOW){
      buttonState = digitalRead(buttonPin);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(F("Call 911 (Medical Emergency)!"));
      display.display();
    }
    delay(1000);
    buttonState=LOW;
    while (buttonState==LOW){
      buttonState = digitalRead(buttonPin);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(F("Move the person to a cooler place."));
      display.display();
    }
    delay(1000);
    buttonState=LOW;
    while (buttonState==LOW){
      buttonState = digitalRead(buttonPin);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(F("Help lower the person's temperature with cool cloths or a cool bath."));
      display.display();
    }
    delay(1000);
    buttonState=LOW;
    while (buttonState==LOW){
      buttonState = digitalRead(buttonPin);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(F("Do not give the person anything to drink."));
      display.display();
    }
    }
  }


  delay(500);
}
