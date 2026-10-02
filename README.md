# Smart Hospital IoT System 🏥

An ESP32-based Smart Hospital IoT system designed for real-time patient monitoring, environmental monitoring, medication dosage control, smart bed management, adaptive sampling, and fault-tolerant operation using FreeRTOS and MQTT.

## 📌 Project Overview

The **Smart Hospital IoT System** integrates multiple healthcare and environmental monitoring functions into a single IoT platform.

The system uses an **ESP32** with **FreeRTOS** to handle multiple tasks concurrently, including sensor monitoring, environmental monitoring, MQTT communication, alert handling, and OLED display updates.

Data is transmitted using **Wi-Fi and MQTT** to **Adafruit IO**, where separate dashboards can be used by medical staff and facility management.

## ✨ Features

* Real-time patient health monitoring
* Environmental condition monitoring
* Role-based dashboards
* MQTT-based cloud communication
* FreeRTOS multitasking
* Inter-task communication using queues, semaphores, and mutexes
* Watchdog timer for system reliability
* Smart medication dosage adjustment
* Medication safety levels and alerts
* Intelligent bed elevation control
* Adaptive monitoring/sampling rate
* Offline detection and local data buffering
* OLED-based real-time status display
* LED and buzzer alerts
* Fault-tolerant communication and recovery

## 🏗️ System Architecture

The system consists of the following major components:

### 1. ESP32 Controller

The ESP32 acts as the central controller and runs multiple FreeRTOS tasks concurrently.

Major tasks include:

* Sensor Reading
* Environment Monitoring
* MQTT Communication
* Alert Handling
* OLED Display Update

Queues, semaphores, and mutexes are used for safe communication and synchronization between tasks.

### 2. Patient Monitoring

The system monitors important patient parameters such as:

* Heart Rate
* SpO₂
* Body Temperature
* Blood Pressure

The monitored values can be displayed locally and published to the cloud for remote monitoring.

### 3. Environmental Monitoring

The facility monitoring section tracks:

* Room Temperature
* Oxygen Level
* Air Quality Index (AQI)

This information is intended for the facility management dashboard.

### 4. Medication Dosage Control

A dashboard-controlled dosage value allows medication dosage to be adjusted between **0–100 mg/hr**.

The system classifies dosage into three safety levels:

| Dosage      | Status   |
| ----------- | -------- |
| 0–50 mg/hr  | Normal   |
| 51–80 mg/hr | Warning  |
| >80 mg/hr   | Critical |

When the dosage reaches the critical range, the system activates an alert using the LED and buzzer and displays the medication status on the OLED.

### 5. Smart Bed Elevation

The system supports remote bed-angle control from **0° to 90°**.

Preset operating modes include:

* Sleep – 10°
* Breathing – 45°
* Emergency – 90°

The bed-control logic can also adjust the bed position according to patient conditions.

### 6. Dynamic Sampling

The monitoring frequency can be controlled through a dashboard.

The sampling interval can be adjusted between:

**5 seconds – 60 seconds**

When abnormal patient conditions are detected, the system can automatically switch to a higher-frequency sampling interval.

### 7. Offline Detection

The system monitors its communication status and identifies:

* **ONLINE**
* **DEGRADED**
* **OFFLINE**

During offline operation, the system can display:

> LOGGING OFFLINE

Sensor data can be temporarily stored locally and synchronized when connectivity is restored.

## 🔄 System Workflow

```text
Sensors / Control Inputs
          ↓
        ESP32
          ↓
     FreeRTOS Tasks
          ↓
   Data Processing
          ↓
     Alert Handling
          ↓
      MQTT / Wi-Fi
          ↓
      Adafruit IO
          ↓
 ┌───────────────────────┐
 │ Medical Staff         │
 │ Facility Management   │
 │ Medication Control    │
 │ Bed Control           │
 └───────────────────────┘
```

## ☁️ MQTT & Adafruit IO

The ESP32 communicates with **Adafruit IO** using MQTT.

The project uses feeds for parameters including:

* Body Temperature
* Heart Rate
* SpO₂
* Blood Pressure
* Room Temperature
* Oxygen Level
* AQI
* Alerts
* Medication Dosage

The dashboards allow monitoring and remote control of different parts of the system.

## 🖥️ Dashboards

### Medical Staff Dashboard

Displays patient-related information:

* Heart Rate
* SpO₂
* Body Temperature
* Blood Pressure
* Patient alerts

### Facility Management Dashboard

Displays environmental information:

* Room Temperature
* Oxygen Level
* AQI
* Environmental alerts

### Medication Control

Provides:

* Dosage adjustment
* Safety status
* Critical dosage alerts

### Bed Control

Provides:

* Bed-angle adjustment
* Preset positions
* Remote control

## ⚙️ Hardware

The project is implemented using an **ESP32 DevKit** with components including:

* ESP32 DevKit C
* DS18B20 temperature sensor
* PIR motion sensor
* Slide potentiometer
* DIP switches
* OLED SSD1306 display
* LEDs
* Buzzers
* Breadboard and resistors

## 🔌 Pin Configuration

| Component                | ESP32 Pin |
| ------------------------ | --------: |
| DS18B20 Data             |   GPIO 14 |
| PIR Motion Sensor        |   GPIO 12 |
| Dosage Potentiometer     |   GPIO 34 |
| OLED SDA                 |   GPIO 21 |
| OLED SCL                 |   GPIO 22 |
| Buzzer 1                 |    GPIO 4 |
| Buzzer 2                 |   GPIO 16 |
| Buzzer 3                 |   GPIO 17 |
| Buzzer 4                 |    GPIO 5 |
| Dosage LED               |   GPIO 13 |
| DIP Switch – Motion      |   GPIO 26 |
| DIP Switch – SpO₂        |   GPIO 25 |
| DIP Switch – Heart Rate  |   GPIO 33 |
| DIP Switch – Temperature |   GPIO 32 |

## 🧵 FreeRTOS Implementation

FreeRTOS is used to allow different operations to execute concurrently.

The system uses:

* Tasks
* Queues
* Semaphores
* Mutexes
* Task delays
* Watchdog Timer

This prevents sensor monitoring, communication, alerts, and display updates from blocking each other.

## 🚨 Alert System

The system provides visual and audio alerts for abnormal conditions.

### Alert Outputs

* LEDs
* Buzzers
* OLED notifications
* Cloud/dashboard alerts

Critical medication dosage conditions trigger an immediate local alert.

## 🛡️ Fault Tolerance

The system is designed to continue operating during communication failures.

When Wi-Fi or MQTT connectivity is unavailable:

1. The system detects the failure.
2. The system enters the appropriate offline/degraded state.
3. The OLED displays the communication status.
4. Sensor readings can be buffered locally.
5. Data synchronization resumes after connectivity is restored.

## 🛠️ Technologies Used

* **ESP32**
* **C/C++**
* **Arduino Framework**
* **FreeRTOS**
* **MQTT**
* **Adafruit IO**
* **Wokwi**
* **OLED SSD1306**
* **DS18B20**
* **Wi-Fi**
* **Embedded Systems / IoT**

## 📚 Libraries

The project uses the following libraries:

* Adafruit GFX Library
* OneWire
* DallasTemperature
* Adafruit SSD1306
* Adafruit MQTT Library

## 📁 Project Structure

```text
smart-hospital-iot/
│
├── main.ino
├── diagram.json
├── libraries.txt
├── README.md
│
└── docs/
    ├── project-report.pdf
    ├── architecture-diagram.png
    ├── workflow-diagram.png
    └── dashboard-screenshots/
```

## ▶️ Running the Project

### Using Wokwi

1. Open the Wokwi project.
2. Start the simulation.
3. Configure the required Adafruit IO credentials.
4. Run the ESP32 simulation.
5. Change sensor inputs using the Wokwi controls.
6. Monitor the OLED and alert outputs.
7. View the corresponding data on the Adafruit IO dashboards.

### Wokwi Project

[Open the Wokwi Simulation](https://wokwi.com/projects/476756177830197249)

## 🎯 Internship Tasks Covered

This project integrates the six major internship tasks:

| Task   | Feature                               |
| ------ | ------------------------------------- |
| Task 1 | Multi-Sensor Expansion using FreeRTOS |
| Task 2 | Dual-Role IoT Monitoring System       |
| Task 3 | Smart Medication Dosage Adjustment    |
| Task 4 | Intelligent Remote Bed Elevation      |
| Task 5 | Smart Dynamic Sampling Rate           |
| Task 6 | Advanced Offline Detection            |

The internship specification requires the final submission to be a **single integrated Wokwi project** containing the implemented functionality, along with the GitHub repository, Wokwi link, report, diagrams, dashboard screenshots, and demonstration video.

## 📊 Project Benefits

The proposed system provides:

* Centralized hospital monitoring
* Real-time patient and environmental data
* Remote monitoring through dashboards
* Automated alerts
* Concurrent task execution
* Remote medication and bed control
* Adaptive monitoring
* Offline fault tolerance
* Improved system reliability

## 👩‍💻 Project

**Project:** Smart Hospital IoT System
**Platform:** ESP32 + Wokwi
**Communication:** Wi-Fi / MQTT
**Cloud Platform:** Adafruit IO
**Operating Model:** FreeRTOS-based multitasking

---

**Smart Hospital IoT System — Real-Time Monitoring, Control and Fault-Tolerant Healthcare Automation**
