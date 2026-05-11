# FarmGuard-AIoT-TeamMeshack-James-Mandila
# FarmGuard-AIoT

## Smart Irrigation and Microclimate Monitoring System

FarmGuard-AIoT is a secure, intelligent and connected Embedded Systems & Internet of Things (AIoT) project developed for **BCT 2308 – Embedded Systems & IoT**.

The system is designed to monitor environmental conditions in farms, greenhouses and smart agriculture environments while performing automated irrigation, edge anomaly detection, cloud telemetry, dashboard visualization and secure remote control.

---

# Project Objectives

The project aims to:

* Monitor environmental conditions in real time
* Automate irrigation safely
* Provide live IoT telemetry
* Support dashboard visualization and alerts
* Implement edge intelligence and anomaly detection
* Demonstrate reliability and fault handling
* Secure device communication and remote commands
* Simulate a deployable AIoT smart farming product

---

# System Features

## Embedded System Features

* ESP32-based firmware
* Non-blocking embedded programming
* GPIO, ADC and sensor interfacing
* Relay-controlled pump automation
* OLED local display
* Manual override button

## IoT Features

* WiFi connectivity
* MQTT publish/subscribe communication
* Structured JSON telemetry
* MQTT topic hierarchy
* Dashboard integration
* Remote command execution

## Reliability Features

* WiFi reconnection logic
* MQTT reconnection logic
* Watchdog concepts
* Safe actuator timeout
* Fault detection and safe mode
* Sensor plausibility checks

## Edge Intelligence Features

* Rule-based anomaly detection
* Heat stress detection
* Dry stress detection
* Water risk detection
* Sensor fault detection

## Security Features

* Unique device identity
* MQTT authentication
* Command authorization token
* Hidden configuration files
* Firmware versioning
* Threat modelling

---

# System Architecture

```text
Real Environment
       ↓
[Sensors]
       ↓
[ESP32 Smart Edge Node]
       ↓
[Local Decision Making]
       ↓
[Relay + Pump Control]
       ↓
[WiFi + MQTT]
       ↓
[MQTT Broker / Node-RED]
       ↓
[Dashboard + Alerts]
       ↓
[Remote User]
```

---

# Hardware Components

| Component               | Purpose                          |
| ----------------------- | -------------------------------- |
| ESP32 Development Board | Main embedded controller         |
| DHT22 Sensor            | Temperature and humidity sensing |
| Soil Moisture Sensor    | Soil monitoring                  |
| Ultrasonic Sensor       | Water level monitoring           |
| Relay Module            | Pump switching                   |
| Mini Water Pump         | Irrigation                       |
| OLED Display            | Local interface                  |
| Push Button             | Manual override                  |
| Breadboard + Wires      | Circuit connections              |
| External Power Supply   | Pump power                       |

---

# Software Stack

| Layer                  | Technology         |
| ---------------------- | ------------------ |
| Firmware               | Arduino IDE        |
| Controller             | ESP32              |
| Communication Protocol | MQTT               |
| Broker                 | Mosquitto          |
| Middleware             | Node-RED           |
| Dashboard              | Node-RED Dashboard |
| Version Control        | GitHub             |
| Multitasking           | FreeRTOS           |

---

# Folder Structure

```text
FarmGuard-AIoT/
│
├── firmware/
├── dashboard/
├── diagrams/
├── docs/
├── screenshots/
├── test_logs/
├── README.md
```

---

# MQTT Topic Structure

```text
farmguard/team03/telemetry
farmguard/team03/status
farmguard/team03/alerts
farmguard/team03/command/pump
farmguard/team03/command/config
farmguard/team03/ota/status
```

---

# Example Telemetry Payload

```json
{
  "device_id": "FG-AIOT-TEAM03",
  "schema_version": "1.0",
  "timestamp": "2026-05-05T10:30:00Z",
  "temperature_c": 27.5,
  "humidity_percent": 68,
  "soil_moisture_percent": 34,
  "water_level_percent": 75,
  "pump_status": "OFF",
  "edge_state": "NORMAL",
  "firmware_version": "1.0.0"
}
```

---

# Example Alert Payload

```json
{
  "device_id": "FG-AIOT-TEAM03",
  "timestamp": "2026-05-05T11:45:00Z",
  "alert_type": "LOW_SOIL_MOISTURE",
  "severity": "HIGH",
  "value": 27,
  "threshold": 30,
  "message": "Soil moisture below threshold. Irrigation recommended."
}
```

---

# Edge Intelligence Logic

The ESP32 performs local edge anomaly detection.

Example:

```text
IF temperature > 35°C
AND humidity < 40%
THEN edge_state = HEAT_STRESS
```

Additional edge states:

| Edge State   | Meaning                         |
| ------------ | ------------------------------- |
| NORMAL       | Conditions acceptable           |
| DRY_STRESS   | Soil moisture critically low    |
| HEAT_STRESS  | High temperature + low humidity |
| WATER_RISK   | Water tank low                  |
| SENSOR_FAULT | Invalid readings detected       |

---

# Security Measures

Implemented security mechanisms include:

* MQTT username and password authentication
* Unique device identity
* Command authorization tokens
* Safe actuator timeout protection
* Hidden secrets/configuration files
* Firmware versioning
* Threat modelling

---

# Reliability Features

The system is designed to handle faults safely.

Implemented reliability mechanisms:

* Automatic WiFi reconnection
* MQTT broker reconnection
* Sensor plausibility validation
* Safe actuator fallback behaviour
* Fault alerts
* Watchdog concepts
* Non-blocking firmware design

---

# Dashboard Features

The dashboard provides:

* Live sensor readings
* Historical charts
* Pump status monitoring
* Device online/offline status
* Alert visualization
* Remote command interface
* Edge intelligence visualization

---

# Testing Areas

The project includes testing for:

* Sensor accuracy
* Pump response
* Dashboard latency
* Network reliability
* MQTT reconnection
* Alert correctness
* Security validation
* Edge intelligence behaviour
* Fault tolerance

---

# Future Improvements

Possible future enhancements:

* TinyML-based classification
* OTA firmware updates
* Multi-device deployment
* Solar-powered operation
* Mobile application integration
* Cloud database analytics
* Predictive irrigation scheduling

---

# Academic Context

This project was developed as part of:

**BCT 2308 – Embedded Systems & Internet of Things (IoT)**

The project demonstrates:

* Embedded systems engineering
* IoT communication systems
* Edge computing concepts
* Reliability engineering
* Cybersecurity awareness
* AIoT product engineering

---

# Team Information

| Role                        | Responsibility                         |
| --------------------------- | -------------------------------------- |
| Embedded Firmware Lead      | Sensor reading, control logic and RTOS |
| IoT Connectivity Lead       | MQTT, WiFi and telemetry               |
| Dashboard Lead              | Node-RED dashboard and alerts          |
| Security & Reliability Lead | Threat modelling and fault handling    |
| Documentation Lead          | Reports, diagrams and presentation     |

---

# Repository Contents

This repository contains:

* ESP32 firmware
* MQTT communication logic
* Dashboard configuration
* Architecture diagrams
* Test evidence
* Threat model
* Technical documentation
* Screenshots and logs

---

# Demo Capabilities

The final system demonstrates:

* Real-time environmental monitoring
* Automated irrigation
* Secure remote control
* Dashboard telemetry
* Alert generation
* Fault handling
* Edge intelligence
* Reliable operation under network failure

---

# Conclusion

FarmGuard-AIoT is designed as a complete AIoT smart agriculture prototype that combines embedded systems, IoT communication, automation, reliability engineering, cybersecurity and edge intelligence into a unified cyber-physical system.

The project demonstrates how intelligent edge devices can improve agricultural monitoring, irrigation efficiency and decision support in real-world environments.
