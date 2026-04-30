# STM32 AI Fault Tolerant Control Gateway

[![STM32](https://img.shields.io/badge/Platform-STM32H745-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32h745-747.html)
[![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS-green.svg)](https://www.freertos.org/)
[![AI](https://img.shields.io/badge/AI-TinyML-orange.svg)](https://www.st.com/en/embedded-software/x-cube-ai.html)

A high-performance, dual-core **predictive maintenance gateway** designed for industrial reliability. This project leverages the **Cortex-M7 and Cortex-M4** cores of the STM32H745 to implement real-time anomaly detection and fault-tolerant motor control.

---

## 🚀 Project Overview
This project implements an intelligent gateway for industrial IoT and motor control. By utilizing a dual-core architecture, the system ensures a strict separation between heavy computational tasks (AI inference) and time-critical safety operations (motor control).

### Key Technical Features
*   **AI-Based Diagnostics:** Real-time anomaly detection using an **Autoencoder** model deployed via **X-CUBE-AI**.
*   **High-Fidelity Sensing:** Synchronized **ADC-DMA** pipeline for Current Signature Analysis (CSA) and **MPU6050** vibration profiling via I2C.
*   **Fault-Tolerant Actuation:** Proactive motor protection using the **DRV8833** driver, ensuring a failsafe state within **50ms** of anomaly detection.
*   **Robust RTOS Architecture:** Multi-threaded environment powered by **FreeRTOS** with dedicated tasks for sensing, inference, and diagnostics.

---

## 🛠 Hardware Setup
The system is prototyped in a dedicated home laboratory environment.

*   **MCU:** STM32H745ZIQ (Dual-Core Cortex-M7 @ 480MHz, M4 @ 240MHz)
*   **Motor Driver:** DRV8833 Dual H-Bridge
*   **Sensors:** 
    *   **MPU6050** (6-Axis IMU for vibration profiling)
    *   **ACS712** (Hall-effect current sensor for CSA)
    *   **DHT11 & LDR** (Environmental monitoring)
*   **User Interface:** I2C 20x4 LCD Display & UART Debug Interface

> **Note:** For a detailed view of the prototyping environment, refer to the project documentation.

---

## 🏗 System Architecture
The firmware is designed with a clear separation of concerns between the two cores:

### Software Layers (FreeRTOS Tasks)
1.  **Task 1 (Sensing):** High-speed acquisition via DMA-backed ADC and I2C drivers.
2.  **Task 2 (AI Inference):** Signal processing and TinyML model execution on the M7 core.
3.  **Task 3 (Control Loop):** Deterministic PID and PWM generation on the M4 core.
4.  **Task 4 (Diagnostics):** System heartbeat monitoring and failsafe signal triggering.

---

## 📊 Quality Attributes (Utility Tree)
The project prioritizes the following quality metrics to ensure industrial-grade reliability:
*   **Functional Correctness:** Reliable current signature analysis and deterministic control loops.
*   **Robustness & Safety:** Memory integrity via hardware-backed CRC and automated failsafe protocols.
*   **Scalability:** Modular HAL/LL driver separation to allow additional sensor integration.

---

## 📁 Repository Structure
```plaintext
├── Firmware/         # STM32CubeIDE Project Files (M7 & M4)
├── Drivers/          # Custom drivers for MPU6050, DRV8833, ACS712
├── Models/           # TinyML Autoencoder models (.tflite, C-code)
├── Docs/             # PlantUML diagrams and system documentation
└── Tools/            # Python scripts for data visualization


---
## 👨‍💻 Developer
**Mehmet Alperen**  
*Software Engineer & PhD Student at Istanbul Technical University (ITU)*
---
