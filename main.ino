#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "freertos/semphr.h"

#include "esp_task_wdt.h"  

#define TEMP_SENSOR 14
// #define MOTION_DETECTOR 12
#define BP_SENSOR 34

#define DOSAGE_RED_LED 13

#define BUZZER_PIN_1 4   
#define BUZZER_PIN_2 16    
#define BUZZER_PIN_3 17
#define BUZZER_PIN_4 5

#define SW_TEMP 32
#define SW_HR 33
#define SW_SPO2 25
// #define SW_MOTION 26

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
#define OLED_ADDRESS 0x3C

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASS ""

#define IO_USERNAME  "Sanjanaaaaaa"
#define IO_KEY       "aio_FYjo12iwTFtueXghlt2AEuUmNJJI"

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883

#define TEMP_FEED         IO_USERNAME "/feeds/body-temperature-c"
#define HR_FEED           IO_USERNAME "/feeds/heart-rate-monitor"
// #define MOTION_FEED       IO_USERNAME "/feeds/motion-detector"
#define SPO2_FEED         IO_USERNAME "/feeds/spo2-percent"
#define BP_FEED           IO_USERNAME "/feeds/blood-pressure"
#define FLAG              IO_USERNAME "/feeds/flag"

#define ROOM_TEMP_FEED    IO_USERNAME "/feeds/room-temperature"
#define OXYGEN_FEED       IO_USERNAME "/feeds/oxygen-level"
#define AQI_FEED          IO_USERNAME "/feeds/aqi"
#define ENV_ALERT_FEED    IO_USERNAME "/feeds/environment-alert"

#define DOSAGE_FEED IO_USERNAME "/feeds/medication-dosage"



#define ALERT_NONE      0
#define ALERT_TEMP      1
#define ALERT_HR        2
#define ALERT_SPO2      3
// #define ALERT_MOTION    4
#define ALERT_BP        5


char medicalSummary[160];
char facilitySummary[160];
bool summaryReady = false;

int targetDose = 0;
int currentDose = 0;

enum DosageLevel {
  DOSAGE_NORMAL,
  DOSAGE_WARNING,
  DOSAGE_CRITICAL
};

DosageLevel dosageLevel = DOSAGE_NORMAL;

volatile bool dosageDisplayEvent = false;
volatile int dosageDisplayValue = 0;

WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT,
                          IO_USERNAME, IO_KEY);

Adafruit_MQTT_Publish tempFeed   = Adafruit_MQTT_Publish(&mqtt, TEMP_FEED);
Adafruit_MQTT_Publish hrFeed     = Adafruit_MQTT_Publish(&mqtt, HR_FEED);
Adafruit_MQTT_Publish spo2Feed   = Adafruit_MQTT_Publish(&mqtt, SPO2_FEED);
Adafruit_MQTT_Publish statusFeed = Adafruit_MQTT_Publish(&mqtt, FLAG);
// Adafruit_MQTT_Publish motiondetectorFeed = Adafruit_MQTT_Publish(&mqtt, MOTION_FEED);
Adafruit_MQTT_Publish bpFeed = Adafruit_MQTT_Publish(&mqtt, BP_FEED);

Adafruit_MQTT_Publish roomTempFeed = Adafruit_MQTT_Publish(&mqtt, ROOM_TEMP_FEED);
Adafruit_MQTT_Publish oxygenFeed = Adafruit_MQTT_Publish(&mqtt, OXYGEN_FEED);
Adafruit_MQTT_Publish aqiFeed = Adafruit_MQTT_Publish(&mqtt, AQI_FEED);
Adafruit_MQTT_Publish envAlertFeed = Adafruit_MQTT_Publish(&mqtt, ENV_ALERT_FEED);

Adafruit_MQTT_Subscribe dosageFeed = Adafruit_MQTT_Subscribe(&mqtt, DOSAGE_FEED);

OneWire oneWire(TEMP_SENSOR);
DallasTemperature sensors(&oneWire);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

QueueHandle_t hrQueue;
QueueHandle_t spo2Queue;
QueueHandle_t bpQueue;
QueueHandle_t tempQueue;
// QueueHandle_t motionQueue;

QueueHandle_t alertQueue;
QueueHandle_t statusQueue;
SemaphoreHandle_t oledMutex;
SemaphoreHandle_t sensorSemaphore;

QueueHandle_t roomTempQueue;
QueueHandle_t oxygenQueue;
QueueHandle_t aqiQueue;

// Aggregation variables
float hrSum = 0;
float spo2Sum = 0;
float tempSum = 0;

float roomTempSum = 0;
float oxygenSum = 0;
float aqiSum = 0;

int sampleCount = 0;

unsigned long lastAggregationTime = 0;
#define AGGREGATION_INTERVAL 30000   // 30 seconds

void spo2Task(void *param) {
  int spo2;

  while (1) {

    if(!digitalRead(SW_SPO2)){
      spo2 = random(60, 100);
    }
    else 
      spo2 = 0;

    xQueueOverwrite(spo2Queue, &spo2);
    xSemaphoreGive(sensorSemaphore);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void bloodPressureTask(void *param) {
  while (1) {
    int bpADC = analogRead(BP_SENSOR);
    int bp = map(bpADC, 0, 4095, 90, 180);
    xQueueOverwrite(bpQueue, &bp);
    xSemaphoreGive(sensorSemaphore);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
  
}

void heartRateTask (void *param) {
  int heartRate;
  while (1) {
    if(!digitalRead(SW_HR)){
      heartRate = random(45, 200);
    }
    else heartRate = 0;
    xQueueOverwrite(hrQueue, &heartRate);
    xSemaphoreGive(sensorSemaphore);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
  
}

void temperatureTask(void *param){
    float temperature;
    while (1){
        if (!digitalRead(SW_TEMP)){
            sensors.requestTemperatures();
            temperature = sensors.getTempCByIndex(0);
        }
        else{
            temperature = 0;
        }
        xQueueOverwrite(tempQueue, &temperature);
        xSemaphoreGive(sensorSemaphore);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// void motionTask(void *param){
//     int motion;
//     while (1)
//     {
//         if (!digitalRead(SW_MOTION))
//             motion = digitalRead(MOTION_DETECTOR);
//         else
//             motion = 0;
//         xQueueOverwrite(motionQueue, &motion);
//         xSemaphoreGive(sensorSemaphore);
//         vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }

void roomTemperatureTask(void *param) {
  float roomTemperature;

  while (1) {
    roomTemperature = random(200, 351) / 10.0;

    xQueueOverwrite(roomTempQueue, &roomTemperature);
    xSemaphoreGive(sensorSemaphore);

    Serial.print("Room Temperature: ");
    Serial.println(roomTemperature);

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void oxygenLevelTask(void *param) {
  float oxygenLevel;

  while (1) {
    oxygenLevel = random(180, 231) / 10.0;

    xQueueOverwrite(oxygenQueue, &oxygenLevel);
    xSemaphoreGive(sensorSemaphore);

    Serial.print("Oxygen Level: ");
    Serial.println(oxygenLevel);

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void aqiTask(void *param) {
  int aqi;

  while (1) {
    aqi = random(0, 201);

    xQueueOverwrite(aqiQueue, &aqi);
    xSemaphoreGive(sensorSemaphore);

    Serial.print("AQI: ");
    Serial.println(aqi);

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void dosageTask(void *param) {

  bool doseChanging = false;

  while (1) {

    if (currentDose < targetDose) {
      currentDose++;
      doseChanging = true;

      Serial.print("Dosage: ");
      Serial.print(currentDose);
      Serial.println(" mg/hr");
    }

    else if (currentDose > targetDose) {
      currentDose--;
      doseChanging = true;

      Serial.print("Dosage: ");
      Serial.print(currentDose);
      Serial.println(" mg/hr");
    }

    else if (doseChanging) {
      Serial.print("Target Dose Reached: ");
      Serial.print(currentDose);
      Serial.println(" mg/hr");

      dosageDisplayValue = currentDose;
      dosageDisplayEvent = true;

      doseChanging = false;
    }

    // Determine safety level
    if (currentDose <= 50) {
      dosageLevel = DOSAGE_NORMAL;
    }
    else if (currentDose <= 80) {
      dosageLevel = DOSAGE_WARNING;
    }
    else if (currentDose > 80) {
      dosageLevel = DOSAGE_CRITICAL;
    }

    if (dosageLevel == DOSAGE_CRITICAL && currentDose != targetDose) {

      // Serial.println("Dosage Critical");
      digitalWrite(DOSAGE_RED_LED, HIGH);
      digitalWrite(BUZZER_PIN_4, HIGH);

      vTaskDelay(pdMS_TO_TICKS(1000));

      digitalWrite(DOSAGE_RED_LED, LOW);
      digitalWrite(BUZZER_PIN_4, LOW);

    }
    else {
      digitalWrite(DOSAGE_RED_LED, LOW);
      digitalWrite(BUZZER_PIN_4, LOW);
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void displayTask(void *param)
{
    float temperature;
    int heartRate;
    int spo2;
    int bp;
    // int motion;

    bool showingDosage = false;
    unsigned long dosageScreenStart = 0;

    while (1)
    {
        xQueuePeek(tempQueue, &temperature, 0);
        xQueuePeek(hrQueue, &heartRate, 0);
        xQueuePeek(spo2Queue, &spo2, 0);
        xQueuePeek(bpQueue, &bp, 0);
        // xQueuePeek(motionQueue, &motion, 0);

        // New dosage has reached its target
        if (dosageDisplayEvent)
        {
            dosageDisplayEvent = false;
            showingDosage = true;
            dosageScreenStart = millis();
        }

        // Keep dosage screen visible for 3 seconds
        if (showingDosage && millis() - dosageScreenStart >= 3000)
        {
            showingDosage = false;
        }

        if (xSemaphoreTake(oledMutex, pdMS_TO_TICKS(100)) == pdTRUE){

            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);

            if (showingDosage)
            {         
                display.setCursor(0, 0);
                display.print("MEDICATION");

                display.setCursor(0, 18);
                display.print("Dosage: ");
                display.print(dosageDisplayValue);
                display.print(" mg/hr");

                display.setCursor(0, 36);
                display.print("Status:");

                display.setCursor(0, 52);

                if (dosageDisplayValue <= 50)
                {
                    display.print("NORMAL");
                }
                else if (dosageDisplayValue <= 80)
                {
                    display.print("WARNING");
                }
                else
                {
                    display.print("CRITICAL");
                }
            }
            else
            {
                display.setCursor(0, 0);
                display.print("PATIENT VITALS");

                display.setCursor(0, 16);
                display.print("Temp: ");
                display.print(temperature, 1);
                display.print(" C");

                display.setCursor(0, 30);
                display.print("HR: ");
                display.print(heartRate);
                display.print(" BPM");

                display.setCursor(0, 44);
                display.print("SpO2: ");
                display.print(spo2);
                display.print("%");

                display.setCursor(0, 58);
                display.print("BP: ");
                display.print(bp);
            }

            display.display();

            xSemaphoreGive(oledMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void mqttTask(void *param)
{
    float temperature;
    float roomTemperature;
    float oxygenLevel;

    int heartRate;
    int spo2;
    int bp;
    // int motion;
    int aqi;

    char envAlert[60];

    char statusMsg[40];

    esp_task_wdt_add(NULL);

    while (1) {
      esp_task_wdt_reset();
      if (xSemaphoreTake(sensorSemaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
          xQueuePeek(tempQueue, &temperature, 0);
          xQueuePeek(hrQueue, &heartRate, 0);
          xQueuePeek(spo2Queue, &spo2, 0);
          xQueuePeek(bpQueue, &bp, 0);
          // xQueuePeek(motionQueue, &motion, 0);
          xQueuePeek(roomTempQueue, &roomTemperature, 0);
          xQueuePeek(oxygenQueue, &oxygenLevel, 0);
          xQueuePeek(aqiQueue, &aqi, 0);
          xQueuePeek(statusQueue, statusMsg, 0);
      }

      hrSum += heartRate;
      spo2Sum += spo2;
      tempSum += temperature;

      roomTempSum += roomTemperature;
      oxygenSum += oxygenLevel;
      aqiSum += aqi;

      sampleCount++;

      if (roomTemperature < 18.0 || roomTemperature > 30.0) {
            snprintf(envAlert, sizeof(envAlert),
              "Room Temperature Alert: %.1f C", roomTemperature);
      }
      else if (oxygenLevel < 19.5 || oxygenLevel > 23.5) {
            snprintf(envAlert, sizeof(envAlert),
              "Oxygen Level Alert: %.1f %%", oxygenLevel);
      }
      else if (aqi > 100) {
            snprintf(envAlert, sizeof(envAlert),
              "Poor Air Quality: AQI %d", aqi);
      }
      else {
             strcpy(envAlert, "Environment Normal");
      }
    
      if (!mqtt.connected()){
          Serial.println("Connecting to MQTT...");
          while (mqtt.connect() != 0) {
              esp_task_wdt_reset();
              Serial.println("MQTT connection failed. Retrying...");
              vTaskDelay(pdMS_TO_TICKS(500));
          }
          Serial.println("MQTT Connected!");
          Serial.println("Medication Dosage subscription active.");
      }

      if (millis() - lastAggregationTime >= AGGREGATION_INTERVAL && sampleCount > 0) {

          float avgHR = hrSum / sampleCount;
          float avgSpO2 = spo2Sum / sampleCount;
          float avgTemp = tempSum / sampleCount;

          float avgRoomTemp = roomTempSum / sampleCount;
          float avgOxygen = oxygenSum / sampleCount;
          float avgAQI = aqiSum / sampleCount;

          snprintf(medicalSummary, sizeof(medicalSummary),
          "{\"alert\":\"%s\",\"avgHR\":%.1f,\"avgSpO2\":%.1f,\"avgTemp\":%.1f}",
          statusMsg, avgHR, avgSpO2, avgTemp);

          snprintf(facilitySummary, sizeof(facilitySummary),
            "{\"alert\":\"%s\",\"avgRoomTemp\":%.1f,\"avgOxygen\":%.1f,\"avgAQI\":%.1f}",
            envAlert, avgRoomTemp, avgOxygen, avgAQI);

          Serial.println("===== PERIODIC SUMMARY =====");

          Serial.println("Medical Summary:");
          Serial.print("Average HR: ");
          Serial.println(avgHR);

          Serial.print("Average SpO2: ");
          Serial.println(avgSpO2);

          Serial.print("Average Body Temperature: ");
          Serial.println(avgTemp);

          Serial.println("Facility Summary:");
          Serial.print("Average Room Temperature: ");
          Serial.println(avgRoomTemp);

          Serial.print("Average Oxygen: ");
          Serial.println(avgOxygen);

          Serial.print("Average AQI: ");
          Serial.println(avgAQI);

          Serial.println("============================");

          hrSum = 0;
          spo2Sum = 0;
          tempSum = 0;

          roomTempSum = 0;
          oxygenSum = 0;
          aqiSum = 0;

          sampleCount = 0;

          lastAggregationTime = millis();
          summaryReady = true;
      }

      // mqtt.processPackets(10);
      Adafruit_MQTT_Subscribe *subscription;

      subscription = mqtt.readSubscription(100);

      if (subscription == &dosageFeed) {

        targetDose = atoi((char *)dosageFeed.lastread);

        if (targetDose < 0)
          targetDose = 0;

        if (targetDose > 100)
          targetDose = 100;

        Serial.print("New Target Dose: ");
        Serial.print(targetDose);
        Serial.println(" mg/hr");
      }

      mqtt.ping();

      tempFeed.publish(temperature);
      hrFeed.publish((int32_t)heartRate);
      spo2Feed.publish((int32_t)spo2);
      bpFeed.publish((int32_t)bp);
      // motiondetectorFeed.publish((int32_t)motion);

      roomTempFeed.publish(roomTemperature);
      oxygenFeed.publish(oxygenLevel);
      aqiFeed.publish((int32_t)aqi);

      // Normal live alert feeds
      statusFeed.publish(statusMsg);
      envAlertFeed.publish(envAlert);

      // Publish aggregated summaries only every 30 seconds
      if (summaryReady) {

        Serial.println("===== AGGREGATED SUMMARIES =====");

        Serial.print("Medical Summary: ");
        Serial.println(medicalSummary);

        Serial.print("Facility Summary: ");
        Serial.println(facilitySummary);

        Serial.println("================================");

        summaryReady = false;
    }

      vTaskDelay(pdMS_TO_TICKS(20000));
  }
}

char statusMsg[40];

void processingTask(void *param) {

  esp_task_wdt_add(NULL);

  float temperature;
  int heartRate;
  int spo2;
  int bp;
  // int motion;
  char localStatus[40];

  while (1) {

    esp_task_wdt_reset();

    strcpy(localStatus, "OK");
    int alert = ALERT_NONE;

    xQueuePeek(tempQueue, &temperature, 0);
    xQueuePeek(hrQueue, &heartRate, 0);
    xQueuePeek(spo2Queue, &spo2, 0);
    xQueuePeek(bpQueue, &bp, 0);
    // xQueuePeek(motionQueue, &motion, 0);

    strcpy(statusMsg, "OK");
    
    if (temperature > 38.0) {
      strcpy(localStatus, "High Body Temperature");
      alert = ALERT_TEMP;
    }
    else if (heartRate != 0 && (heartRate < 60 || heartRate > 100)) {
      strcpy(localStatus, "Heart Rate Abnormal");
      alert = ALERT_HR;
    }
    else if (spo2 < 88) {
      strcpy(localStatus, "SpO2 Low");
      alert = ALERT_SPO2;
    }
    else if (bp > 150) {
      strcpy(localStatus, "Blood Pressure High");
      alert = ALERT_BP;
    }
    // else if (motion) {
    //   strcpy(localStatus, "Motion Detected");
    //   alert = ALERT_MOTION;
    // }

  if(temperature > 38){
      digitalWrite(BUZZER_PIN_1, HIGH);
      vTaskDelay(pdMS_TO_TICKS(50));
      digitalWrite(BUZZER_PIN_1, LOW);
      vTaskDelay(pdMS_TO_TICKS(50));
  }

  if((heartRate < 60 || heartRate > 100)){
      digitalWrite(BUZZER_PIN_2, HIGH);
      vTaskDelay(pdMS_TO_TICKS(50));
      digitalWrite(BUZZER_PIN_2, LOW);
      vTaskDelay(pdMS_TO_TICKS(50));
  }

  if(spo2 < 88){
      digitalWrite(BUZZER_PIN_3, HIGH);
      vTaskDelay(pdMS_TO_TICKS(50));
      digitalWrite(BUZZER_PIN_3, LOW);
      vTaskDelay(pdMS_TO_TICKS(50));
  }

  // if (motion){
  //     digitalWrite(BUZZER_PIN_4, HIGH);
  //     vTaskDelay(pdMS_TO_TICKS(50));
  //     digitalWrite(BUZZER_PIN_4, LOW);
  //     vTaskDelay(pdMS_TO_TICKS(50));
  // }

  if(bp > 150){
      digitalWrite(BUZZER_PIN_1, HIGH);
      vTaskDelay(pdMS_TO_TICKS(50));
      digitalWrite(BUZZER_PIN_1, LOW);
      vTaskDelay(pdMS_TO_TICKS(50));
  }

  xQueueOverwrite(alertQueue, &alert);
  xQueueOverwrite(statusQueue, localStatus);
  vTaskDelay(pdMS_TO_TICKS(5000));
  }
}



void setup() {

  Serial.begin(115200);

  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 30000,
    .idle_core_mask = 0,
    .trigger_panic = true
  };

  esp_task_wdt_reconfigure(&wdt_config);

  pinMode(BUZZER_PIN_1, OUTPUT);
  pinMode(BUZZER_PIN_2, OUTPUT);
  pinMode(BUZZER_PIN_3, OUTPUT);
  pinMode(BUZZER_PIN_4, OUTPUT);

  // pinMode(MOTION_DETECTOR, INPUT);

  pinMode(SW_TEMP, INPUT);
  pinMode(SW_HR, INPUT);
  pinMode(SW_SPO2, INPUT);
  // pinMode(SW_MOTION, INPUT);

  pinMode(DOSAGE_RED_LED, OUTPUT);
  digitalWrite(DOSAGE_RED_LED, LOW);

  sensors.begin();
  Wire.begin(21, 22);

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  mqtt.subscribe(&dosageFeed);

  hrQueue   = xQueueCreate(1, sizeof(int));
  spo2Queue = xQueueCreate(1, sizeof(int));
  bpQueue = xQueueCreate(1, sizeof(int));
  tempQueue = xQueueCreate(1, sizeof(float));
  // motionQueue = xQueueCreate(1, sizeof(int));

  alertQueue = xQueueCreate(1, sizeof(int));
  statusQueue = xQueueCreate(1, 40);
  oledMutex = xSemaphoreCreateMutex();
  sensorSemaphore = xSemaphoreCreateCounting(10, 0);
  roomTempQueue = xQueueCreate(1, sizeof(float));
  oxygenQueue = xQueueCreate(1, sizeof(float));
  aqiQueue = xQueueCreate(1, sizeof(int));

  float initialRoomTemp = 25.0;
  float initialOxygen = 21.0;
  int initialAQI = 30;

  xQueueOverwrite(roomTempQueue, &initialRoomTemp);
  xQueueOverwrite(oxygenQueue, &initialOxygen);
  xQueueOverwrite(aqiQueue, &initialAQI);

  char initialStatus[40] = "OK";
  xQueueOverwrite(statusQueue, initialStatus);

  xTaskCreatePinnedToCore(processingTask, "ProcessTask", 6144, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(bloodPressureTask, "BloodPressureTask", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(heartRateTask, "HeartRateTask", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(spo2Task, "SpO2Task", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(temperatureTask, "TemperatureTask", 2048, NULL, 2, NULL, 1);
  // xTaskCreatePinnedToCore(motionTask, "MotionTask", 2048, NULL, 2, NULL, 1);

  xTaskCreatePinnedToCore(displayTask, "DisplayTask", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(mqttTask, "MQTTTask", 4096, NULL, 2, NULL, 0);

  xTaskCreatePinnedToCore(roomTemperatureTask, "RoomTempTask", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(oxygenLevelTask, "OxygenTask", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(aqiTask, "AQITask", 2048, NULL, 2, NULL, 1);

  xTaskCreatePinnedToCore(dosageTask, "DosageTask", 2048, NULL, 2, NULL, 1);
}


void loop() {
  vTaskDelay(portMAX_DELAY);
}
