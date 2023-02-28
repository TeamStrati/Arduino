#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme; // I2C

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

float hum_score, gas_score;
float gas_reference = 250000;
float hum_reference = 40;
int   getgasreference_count = 0;

int greenLedPin = 12;
int redLedPin = 13;

void setup() {
  Serial.begin(9600);
  Serial.println("Connecting");
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("Das Display konnte nicht gefunden werden!");
  }else{
    Serial.println("Das Display wurde verbunden!");
  }
  display.display();
  delay(1000);
  display.clearDisplay();
  if (!bme.begin()) {
    Serial.println("Der Sensor BME680 konnte nicht gefunden werden!");
    while (1);
  }else{
    Serial.println("Der Sensor wurde verbunden!");
  }

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  pinMode(greenLedPin, OUTPUT);
  Serial.println("Die Grüne LED wurde verbunden.");
  pinMode(redLedPin, OUTPUT);
  Serial.println("Die Rote LED wurde verbunden.");
}

void loop() {
  delay(5000);
  if (! bme.performReading()) {
    Serial.println("Failed to perform reading :(");
   return;
  }
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  
  //Temperature
  String temp = "Temp.: ";
  temp += String(bme.temperature, 2);
  temp += "C";
  Serial.println(temp);
  display.println(temp);

  //Humidity
  display.setCursor(0, 8);
  String hum = "Hum.: ";
  hum += String(bme.humidity, 2);
  hum += "%";
  display.println(hum);
  Serial.println(hum);

  //Gas
  String gas = "Gas: ";
  gas += String(bme.gas_resistance / 1000.0, 2);
  gas += " KOhms";
  display.setCursor(0, 16);
  display.println(gas);
  Serial.println(gas);

  //Air Quality
  float airQualityFloat = getAirQualityFloat();
  String airQuality = CalculateIAQ(airQualityFloat)
  display.setCursor(0, 24);
  display.println(airQuality);
  Serial.println(airQuality);

  //For Serial Plotter
  //Serial.print("AirQualityIndex: ");
  //Serial.println(airQualityFloat);

  if(airQuality = "Good" or  "Average"){
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(redLedPin, LOW);
  }else{
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, HIGH);
  }
  display.display();
  display.clearDisplay();
}


float getAirQualityFloat() {
  float current_humidity = bme.readHumidity();
  if (current_humidity >= 38 && current_humidity <= 42)
    hum_score = 0.25*100; // Humidity +/-5% around optimum 
  else
  { //sub-optimal
    if (current_humidity < 38) 
      hum_score = 0.25/hum_reference*current_humidity*100;
    else
    {
      hum_score = ((-0.25/(100-hum_reference)*current_humidity)+0.416666)*100;
    }
  }
  
  //Calculate gas contribution to IAQ index
  float gas_lower_limit = 5000;   // Bad air quality limit
  float gas_upper_limit = 50000;  // Good air quality limit 
  if (gas_reference > gas_upper_limit) gas_reference = gas_upper_limit; 
  if (gas_reference < gas_lower_limit) gas_reference = gas_lower_limit;
  gas_score = (0.75/(gas_upper_limit-gas_lower_limit)*gas_reference -(gas_lower_limit*(0.75/(gas_upper_limit-gas_lower_limit))))*100;
  float air_quality_score = hum_score + gas_score;
  if ((getgasreference_count++)%10==0) GetGasReference(); 
return (100-air_quality_score) * 5;
}

void GetGasReference(){
  int readings = 10;
  for (int i = 1; i <= readings; i++){
    gas_reference += bme.readGas();
  }
  gas_reference = gas_reference / readings;
}

String CalculateIAQ(float score) {
  String IAQ_text = "Air quality is ";
  score = (100 - score) * 5;
  if      (score >= 301)                  IAQ_text += "Very bad";
  else if (score >= 201 && score <= 300 ) IAQ_text += "Worse";
  else if (score >= 176 && score <= 200 ) IAQ_text += "Bad";
  else if (score >= 151 && score <= 175 ) IAQ_text += "Little bad";
  else if (score >=  51 && score <= 150 ) IAQ_text += "Average";
  else if (score >=  0 && score <=  50 ) IAQ_text += "Good";
  return IAQ_text;
}
