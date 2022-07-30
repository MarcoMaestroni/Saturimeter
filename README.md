# Saturimeter

# Intro

<img align="right" src="https://user-images.githubusercontent.com/109110970/181921962-a00ab53e-bbf7-4d57-b084-4475070f08d0.jpg" width="150" height="150">

This is a project I realized during my Master Thesis in Biomedical Engineering at the Politecnico Di Milano in 2020.<br>
The project has been uploaded for demonstration purposes only.
<br>
# Index

- [Overview](#overview)
- [Hardware](#hardware)
- [Features](#features)
- [Pictures](#pictures)

# Overview
The aim was to to design and develop a system able to perform a measurement of:

- heart rate
- oxygen saturation (SpO2) 
- temperature 

using an external sensor (photodetector) mounted on a breakout board and to display these values on an external display and on a Kivy-based GUI.

<p align="center" width="100%">
  <img align="center" src="https://user-images.githubusercontent.com/109110970/181920550-d1e9b101-2362-4e9b-8c3a-d683b35cd5b0.png" width="700">
</p>

# Hardware

- [Microcontroller CY8C58LP](https://www.infineon.com/dgdl/Infineon-PSoC_5LP_CY8C58LP_Family_Datasheet_Programmable_System-on-Chip_(PSoC_)-DataSheet-v15_00-EN.pdf?fileId=8ac78c8c7d0d8da4017d0ec547013ab9)
- [Sensor MAX30101](https://www.maximintegrated.com/en/products/interface/signal-integrity/MAX30101.html)
- [Display SSD1306](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)
- [Level Shifter](https://www.sparkfun.com/products/12009)

# Features

- SpO2, Heart Rate and Temperature real-time reading and showing on both the display and the GUI interface
- Configuration of reading settings via serial menu or GUI interface
- Change temperature units via serial menu or GUI interface
- Warning of connection errors via on board LED

# Pictures
<p align="center" width="100%">
  <img align="center" src="https://user-images.githubusercontent.com/109110970/181924716-e7bc256a-4f4b-4986-9f82-a48e18452989.jpg" width="30%">
  <img align="center" src="https://user-images.githubusercontent.com/109110970/181924717-dc732fa6-bd93-4386-b92d-dc40b17afe44.jpg" width="30%">
  <img align="center" src="https://user-images.githubusercontent.com/109110970/181924718-ef8e03a8-3a30-4aea-a90a-a9bb49bf583e.jpg" width="30%">
</p>

<p align="center" width="100%">
  <img align="center" src="https://user-images.githubusercontent.com/109110970/181924719-12fe971d-3f38-4284-ba0c-78d06b984b81.png" width="45%">
  <img align="center" src="https://user-images.githubusercontent.com/109110970/181924720-40579d59-bb45-4c57-b5b8-6432ca90c79d.png" width="45%">
</p>

<p align="center" width="100%">
  <img align="center" src="https://user-images.githubusercontent.com/109110970/181924721-5511fbea-3eb4-4ce7-836e-1bb462186e2a.jpg" width="45%">
  <img align="center" src="https://user-images.githubusercontent.com/109110970/181924723-d5acd6f0-3d40-4255-970f-3cebcaa220d0.jpg" width="45%">
</p>
