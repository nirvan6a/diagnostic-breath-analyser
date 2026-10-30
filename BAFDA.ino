#include <SPI.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED params:
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// Declaration for SSD1306 display connected using I2C
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Actuator params:
#define ACTUATOR_PIN 2
int Actuator[2] = {0, 0}; // format: 0/1 = off/on; 0 = initial value of unique-id

// SD card params:
#define SD_PIN 10
#define DATAFILE "data.csv" // macro only supports 8.3 character filename

// Sensor params:
#define HEAT_TIME 5
#define VOLT_SUPPLY 5.0 // volts
#define SENSOR_ACETONE "ACE"
#define SENSOR_NH3 "NH3"
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

void setupSD() {
  // Initialize SD card
  if (!SD.begin(SD_PIN)) {
    Serial.println("SD card initialization failed!");
    OLED("SD Failure", 1, 1);
    while (1); // Stop the program if SD initialization fails
  }
  Serial.println("SD card initialized.");

  // Initialise DHT
  dht.begin();
  Wire.begin();
}

void getAmbient() {
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  if (isnan(temp)) {
    Serial.println("Failed to getAmbient");
    return;
  }
  Serial.print("Temp: " + String(temp) + "°C");
  Serial.println(" / Humidity: " + String(humidity) + "%");
  delay(5000);
}

void writeFile(String text) {
  String headers = "";
  if (!SD.exists(DATAFILE)) {
    headers = "TestID,Acetone(PPM),Ammonia(PPM)\n"; // Header for CSV file
  }

  File myFile;
  myFile = SD.open(DATAFILE, FILE_WRITE);
  // Check if the file opened successfully
  if (myFile) {
    Serial.println("Writing to file...");
    myFile.println(headers + text);
    myFile.close(); // Close the file
    Serial.println("Data written successfully.");
  } else {
    Serial.println("Error opening data file");
  }
}
float generateID() {
  return millis();
}
void OLED(String text, int clear, int size) {
  if (clear == 1) {
    display.clearDisplay();
    display.setCursor(0,15);
  }
  display.setTextColor(WHITE);
  display.setTextSize(size);
  display.println(text);
  display.display();
}
void setupDisplay() {
  // initialize the OLED object
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  OLED("Boot-up complete", 1, 1);
}
void heatSensors() {
  //Sensor heating startup:
  int t_i = HEAT_TIME;
  while (t_i > 0) {
    delay(1000);
    OLED("Heating sensors\n", 1, 1);
    OLED(String(--t_i), 0, 2);
  }
}

// Define the Sensor class
class Sensor {
  public:
    String name;
    int pin;
    float RL;
    float R0;
    float m;
    float b;
    float ppm;
    float transform;

    // Constructor to initialize the sensor with pin and threshold
    Sensor(String name, int pin, float rl, float r0, float m, float b) {
      this->name = name;
      this->pin = pin;
      this->RL = rl;
      this->R0 = r0;
      this->m = m;
      this->b = b;
      this->ppm = 0.00;
      this->transform = 0.00;
    }

    // Function to read the sensor value from the analog pin
    void calibrate() {
      if (this->R0 > 0) return; // R0 calibration has already been done
  
      // find R0
      float R0 = 0.00;
      int sample = 100;
      int wait = 500;
      for (int x = 0; x < sample; x++) {
        R0 = R0 + this->getRs();
        OLED("Calibrating sensors\n", 1, 1);
        OLED(String(sample - x), 0, 2);
        delay(wait);
      }
      R0 = R0/sample;
      this->R0 = R0; // Setting R0 in Gas array

      String str = "Calibration done, R0 = " + String(R0);
      Serial.println(str);
    }
    float getRs() {
      // formula: Rs = (VOLT_SUPPLY * RL)/VOLT_OUT - RL
      //float sensorValue, voltage, Rs = 0.00;
      float sensorValue = 0;
      float voltage = 0;
      float Rs = 0;

      sensorValue = analogRead(this->pin);
      Serial.println(sensorValue);
      voltage = (sensorValue / 1023.0) * VOLT_SUPPLY;
      Serial.println(voltage);
      Rs = ((VOLT_SUPPLY * this->RL) / voltage) - this->RL;
      Serial.println(Rs);
      Rs = this->ambientCorrection(Rs);
      return Rs;
    }
    float ambientCorrection(float sensorResistance) {
      return sensorResistance;
    }
    void getPPM() {
      String transform = "";
      float Rs = 0.00;
      float ratio = 0.00;

      Rs = this->getRs();
      Serial.println(Rs);
      ratio = Rs / this->R0;
      Serial.println(ratio);
      this->ppm = pow(10, ((log10(ratio) - this->b) / this->m));
      Serial.println(this->ppm);

      if (this->name == SENSOR_ACETONE) {
        this->getGlucose();
        transform = " /Glucose = " + String(this->transform);
      } 

      String str = "[" + this->name + "] Rs = " + String(Rs) + " /RL = " + String(this->RL) + " /R0 = " + String(this->R0) + " /Ratio = " + String(ratio) + " /PPM = " + String(this->ppm) + transform;
      Serial.println(str);
    }
    void getGlucose() {
      const float slope = 1.5;
      const float intercept = 90.0;

      float glucose = slope * this->ppm + intercept;
      if (glucose <40) glucose = 40.00;
      if (glucose >400) glucose = 400.00;
      this->transform = glucose;
    }
};

// Sensor init format: {sensor, pin, RL in K-oh, R0 in K-oh, slope, y-intercept}
//float Acetone[] = {SENSOR_ACETONE, A2, 46.4, 412, -0.347708838975381, -0.108223116674343}; // format {sensor, pin, RL in K-oh, R0 in K-oh, slope, y-intercept}
//float NH3[] = {SENSOR_NH3, A1, 46, 28, -0.438340578701237, 0.269264245543036}; // format {sensor, pin, RL in K-oh, R0 in K-oh, slope, y-intercept}
Sensor Acetone(SENSOR_ACETONE, A2, 46.50, 412.00, -0.347708838975381, -0.108223116674343);
Sensor NH3(SENSOR_NH3, A1, 46.00, 28.00, -0.438340578701237, 0.269264245543036);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  pinMode(ACTUATOR_PIN, INPUT_PULLUP);
  setupDisplay();
  setupSD();
  heatSensors();
  Acetone.calibrate();
  NH3.calibrate();
  getAmbient();
  OLED("Ready to test", 1, 1);
}

void loop() {
  Acetone.getPPM();
  NH3.getPPM();
  pollActuator();
  delay(1000);
}


void pollActuator() {
  // Actuator params:
  String uid, acetoneppm, nh3ppm, glucose = "";
  int Actuator_state = 0;
  Actuator_state = digitalRead(ACTUATOR_PIN);
  if (Actuator_state == LOW) {
    if (Actuator[0] == 0) {
      Actuator[0] = 1;
      Actuator[1] = generateID();
    }
    uid = String(Actuator[1]);
    acetoneppm = String(Acetone.ppm);
    glucose = String(Acetone.transform);
    nh3ppm = String(NH3.ppm);
    OLED("PPM [" + uid + "]\n", 1, 1);
    OLED("CH3COCH3: " + acetoneppm + " Glucose: " + glucose, 0, 1);  // Display readings:
    OLED("NH3: " + nh3ppm, 0, 1);
    writeFile(uid+"," + acetoneppm + "," + nh3ppm);
  } else {
    if (Actuator[0] == 1) {
      Actuator[0] = 0;
      Actuator[1] = 0;
    }
  } 
}
