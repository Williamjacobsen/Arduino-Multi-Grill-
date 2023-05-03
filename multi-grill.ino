#include <HX711_Load_Cell.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int timeAndTareButtonPin = 2;
const int switchScreenBtnPin = 3;
const int HX711_dout = 4;
const int HX711_sck = 5;

HX711_Load_Cell LoadCell(HX711_dout, HX711_sck);
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

void setup() {
  Serial.begin(57600);
  Serial.println("\nStarting...");

  pinMode(timeAndTareButtonPin, INPUT);
  pinMode(switchScreenBtnPin, INPUT);

  lcd.init();
  lcd.backlight();

  LoadCell.begin(128);
  unsigned long stabilizingtime = 5000;
  LoadCell.start(stabilizingtime, true);
  LoadCell.setBetterTare();
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check HX711 wiring and pin designations");
    exit(0);
  }
  LoadCell.setCalFactor(21.5); // default 1.0
  Serial.println("Startup done...");
  while (!LoadCell.update());
  //calibrate();
}

void calibrate()
{
  Serial.println("Now, place your known mass on the loadcell.");
  Serial.println("Then send the weight of this mass (i.e. 100.0) from serial monitor.");
  
  float known_mass = 0;
  while (known_mass == 0) {
    LoadCell.update();
    if (Serial.available() > 0)
    {
      known_mass = Serial.parseFloat();
    }
  }
  
  LoadCell.refreshDataSet(); 
  float calibration_value = LoadCell.getNewCalibration(known_mass);
  Serial.print("calibration_value: ");
  Serial.println(calibration_value);

  Serial.println("Calibration done...\n");
}

const int serialPrintInterval = 0;
bool newDataReady;
float time = millis();
float weight_value = 0.00;
float tmp_weight_value;
float peak_value = 0.00;
String screen = "weight";
float _timer = 0.00;
bool timerRunning = false;
float prevTimerAnalogValue = 0.00;

/*
 * A0 - Potentiometer - Max: 1019 - Min: 5
 * A4 - I2C: SDA
 * A5 - 12C: SCL
 * Pin 2 - Blue Button - Time: Start/Stop - Weight: Tare
 * Pin 3 - Black Button - Switch Screen
*/

void loop() {
  _timer = analogRead(A0) - 5;
  bool timeAndTareButtonBtn = digitalRead(timeAndTareButtonPin);
  bool switchScreenBtn = digitalRead(switchScreenBtnPin);

  if (switchScreenBtn && time + 500 < millis())
  {
    if (screen == "weight")
    {
      screen = "timer";
    }
    else
    {
      screen = "weight";
    }
    time = millis();
    Serial.println("New screen: " + screen);
  }

  if (timeAndTareButtonBtn && screen == "weight" && time + 500 < millis())
  {
    LoadCell.setBetterTare();
    weight_value = 0.00;
    peak_value = 0.00;

    lcd.setCursor(2, 1);
    lcd.print(unitConverter(round(weight_value)) + "      ");
    Serial.println(unitConverter(round(weight_value)));
    delay(10);
    time = millis();
  }
  else if (timeAndTareButtonBtn && screen == "timer" && time + 500 < millis())
  {
    timerRunning = !timerRunning;
    time = millis();
    Serial.println("Timer Status: " + String(timerRunning));
  }
  
  delay(1);

  if (screen == "weight")
  {
    weightScreen(); 
  }
  else
  {
    timerScreen();
  }

  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.setBetterTare();
    else if (inByte == 'r') calibrate();
  }
}

void weightScreen()
{
  lcd.setCursor(2, 0);
  lcd.print("Vægt:");
  lcd.setCursor(2, 1);
  lcd.print(unitConverter(round(weight_value)) + "       ");
        
  newDataReady = false;
  if (LoadCell.update()) newDataReady = true;

  if (newDataReady) 
  {
    if (millis() > time + serialPrintInterval) 
    { 
      tmp_weight_value = LoadCell.getData();
      if (tmp_weight_value + 4 < weight_value || tmp_weight_value - 4 > weight_value)
      {
        weight_value = LoadCell.getData();
        Serial.print("Load_cell output val: ");
        Serial.println(unitConverter(round(weight_value)));
        time = millis(); 
        if (peak_value < weight_value)
        {
          peak_value = weight_value;
        }
      }
      else if (weight_value < (peak_value/25) && weight_value > -(peak_value/25))
      {
        LoadCell.setBetterTare();
        weight_value = 0.00;
        peak_value = 0.00;

        lcd.setCursor(2, 1);
        lcd.print(unitConverter(round(weight_value)) + "       ");
        Serial.println(unitConverter(round(weight_value)));
      }
    } 
  }
}

float timerVal = 0.00;
float tmpTimer = 0.00;
float timerVals[] = { 0.00, 0.00, 0.00, 0.00, 0.00 };
int timerValsIdx = 0;
float prevTimerVal = 0.00;
bool timerValChanged = false;
float timerValShown = 0.00;

int minutes = 0;
int seconds = 0;
void updateTimer(float timerVal)
{
  String _t = String(timerVal);
  String tmpMinutes = "";
  String tmpSeconds = "";
  bool dotFound = false;
  for (int i = 0; i < _t.length(); i++)
  {
    if (_t[i] == '.')
    {
      dotFound = true;
    }
    else if (!dotFound)
    {
      tmpMinutes += _t[i]; 
    }
    else
    {
      tmpSeconds += _t[i]; 
    }
  }
  minutes = tmpMinutes.toInt();
  seconds = tmpSeconds.toInt();
  Serial.println(minutes);
  Serial.println(seconds);
  Serial.println("");
}

float timeConverter()
{
  if (seconds > 59)
  {
    minutes++;
    seconds -= 60;
  }
  String tmpSeconds;
  if (seconds < 10)
  {
    tmpSeconds = "0" + String(seconds);
  }
  else
  {
    tmpSeconds = String(seconds);
  }
  return (String(minutes) + "." + tmpSeconds).toFloat();
}

void timerScreen()
{
  tmpTimer = _timer / 60;
  
  timerVals[timerValsIdx] = tmpTimer;
  if (timerValsIdx == 4)
  {
    timerValsIdx = 0;
  }
  else
  {
    timerValsIdx++; 
  }

  if (timerVals[0] == timerVals[1] && timerVals[0] == timerVals[2] && timerVals[0] == timerVals[3] && timerVals[0] == timerVals[4])
  {
    if (prevTimerVal != timerVals[0])
    {
       timerValChanged = true;
    }
    else
    {
      timerValChanged = false;
    }
    timerVal = timerVals[0];
    prevTimerVal = timerVal;
  }
  //Serial.println(String(timerVals[0]) + " - " + String(timerVals[1]) + " - " + String(timerVals[2]) + " - " + String(timerVals[3]) + " - " + String(timerVals[4]));

  if (timerValChanged)
  {
    updateTimer(timerVal);
    String tmpSeconds;
    if (seconds < 10)
    {
      tmpSeconds = "0" + String(seconds);
    }
    else
    {
      tmpSeconds = String(seconds);
    }
    timerValShown = (String(minutes) + "." + tmpSeconds).toFloat();
    timerValChanged = false;
  }

  timerValShown = timeConverter();
  
  lcd.setCursor(2, 0);
  lcd.print("Timer:");
  lcd.setCursor(2, 1);
  lcd.print(String(timerValShown) + "          ");
  
  int i = 0;
  while (timerRunning)
  {
     if (timerValShown < 0.01)
     {
        timerRunning = false;
        return;
     }
    
     bool timeAndTareButtonBtn = digitalRead(timeAndTareButtonPin);
     if (timeAndTareButtonBtn && millis() > time + 500)
     {
        time = millis();
        timerRunning = false;
     }
     
     delay(1);
     i++;
     if (i == 1000)
     {
        updateTimer(timerValShown);
        if (seconds == 0)
        {
          minutes--; 
          seconds += 59;
        }
        else
        {
          seconds--;
        }

        String tmpSeconds;
        if (seconds < 10)
        {
          tmpSeconds = "0" + String(seconds);
        }
        else
        {
          tmpSeconds = String(seconds);
        }
        timerValShown = (String(minutes) + "." + tmpSeconds).toFloat();
        
        i = 0;
        lcd.setCursor(2, 1);
        lcd.print(String(timerValShown) + "          ");
        Serial.println(timerValShown);
     }
  }
}

String unitConverter(float weight_value)
{
  String tmp = "";
  if (weight_value > 999)
  {
    tmp = String(weight_value/1000) + " kg";
  }
  else
  {
    tmp = String(weight_value) + " g";
  }
  return tmp;
}
