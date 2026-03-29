// Replace/ Fill-in information from your Blynk Template here

#define BLYNK_TEMPLATE_ID "TMPLyIoKDvhn"
#define BLYNK_DEVICE_NAME "DIS"

#define BLYNK_FIRMWARE_VERSION "0.1.0"
#define BLYNK_PRINT Serial
#include "BlynkEdgent.h"


//ultrasonic
#define echoPin 32
#define trigPin 33

long duration;
int distance;

long tankDepth= 25;

void ultrasonic()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2; //formula to calculate the distance for ultrasonic sensor
  double level = tankDepth- distance;
  Serial.print("Level: ");
  Serial.println(level);
  Blynk.virtualWrite(V0, level);
  delay(500);
}
//ph sensor
#include "DFRobot_ESP_PH.h"
#include "EEPROM.h"

DFRobot_ESP_PH ph;
#define ESPADC 4096.0   //the esp Analog Digital Convertion value
#define ESPVOLTAGE 3300 //the esp voltage supply value
#define PH_PIN 34    //the esp gpio data pin number



void PH_Value()

{
  float voltage, phValue, temperature = 25;
  static unsigned long timepoint = millis();
  if (millis() - timepoint > 1000U) //time interval: 1s
  {
    timepoint = millis();
    //voltage = rawPinValue / esp32ADC * esp32Vin
    voltage = analogRead(PH_PIN) / ESPADC * ESPVOLTAGE; // read the voltage
    Serial.print("voltage:");
    Serial.println(voltage, 4);
    
    //temperature = readTemperature();  // read your temperature sensor to execute temperature compensation
    Serial.print("temperature:");
    Serial.print(temperature, 1);
    Serial.println("^C");

    phValue = ph.readPH(voltage, temperature); // convert voltage to pH with temperature compensation
    Serial.print("pH:");
    Serial.println(phValue, 4);
    Blynk.virtualWrite(V1, phValue);
  }
  ph.calibration(voltage, temperature); // calibration process by Serail CMD
  
}
//TDS sensor
#define TdsSensorPin 35
#define VREF 5.0 // analog reference voltage(Volt) of the ADC
#define SCOUNT 30 // sum of sample point

int analogBuffer[SCOUNT]; // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25;


void TDS()
{
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) //every 40 milliseconds,read the analog value from the ADC
  {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin); //read the analog value and store into the buffer
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U)
  {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0); //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
    float compensationVolatge = averageVoltage / compensationCoefficient; //temperature compensation
    tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; //convert voltage value to tds value
    //Serial.print("voltage:");
    //Serial.print(averageVoltage,2);
    //Serial.print("V ");
    Serial.print("TDS Value:");
    Serial.print(tdsValue, 0);
    Serial.println("ppm");
    Blynk.virtualWrite(V2, tdsValue);

  }
}
int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;

}

//led
int RelayPin1 = 23;
int RelayPin2 = 22;
int RelayPin3 = 21;
int RelayPin4 = 19;

BLYNK_WRITE(V3) { //Button Widget is writing to pin V0
  int pinValue = param.asInt();
  digitalWrite(RelayPin1, pinValue);
}

BLYNK_WRITE(V4) { //Button Widget is writing to pin V1
  int pinValue = param.asInt();
  digitalWrite(RelayPin2, pinValue);
}
BLYNK_WRITE(V5) { //Button Widget is writing to pin V2
  int pinValue = param.asInt();
  digitalWrite(RelayPin3, pinValue);
}
BLYNK_WRITE(V6) { //Button Widget is writing to pin V3
  int pinValue = param.asInt();
  digitalWrite(RelayPin4, pinValue);
}



void setup()
{
  Serial.begin(115200);
  EEPROM.begin(32);//needed to permit storage of calibration value in eeprom
  ph.begin();
  //Serial.begin(115200);
  //pinMode(34, INPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(TdsSensorPin, INPUT);

  //led
  pinMode(RelayPin1, OUTPUT);
  pinMode(RelayPin2, OUTPUT);
  pinMode(RelayPin3, OUTPUT);
  pinMode(RelayPin4, OUTPUT);

  //TURN OFF all Relays on Start
  digitalWrite(RelayPin1, HIGH);
  digitalWrite(RelayPin2, HIGH);
  digitalWrite(RelayPin3, HIGH);
  digitalWrite(RelayPin4, HIGH);


  BlynkEdgent.begin();
  delay(2000);
}

void loop()
{
  BlynkEdgent.run();
  ultrasonic();
  PH_Value();
  TDS();

}
