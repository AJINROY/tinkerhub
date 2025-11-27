##Seat Monitoring System (ESP8266 + Ultrasonic + DHT11 + Buzzer)

This project is a smart seat monitoring system built using an ESP8266, HC-SR04 ultrasonic sensor, DHT11 temperature & humidity sensor, and a buzzer.
It detects whether someone is sitting on a seat, measures environmental conditions, and alerts the user if they sit for too long. 
And we can also find which seats are filled and how long they have been taken etc.
The ESP8266 also hosts a live web dashboard, where real-time data such as distance, temperature, humidity, uptime, and seat status are displayed.

##Features

Real-time seat occupancy detection using ultrasonic sensor
Temperature & humidity monitoring with DHT11
Auto-trigger buzzer alert if the user sits for more than a set time
Clean Web Dashboard (auto-updates every 2 seconds)

Shows:

Seat status (Occupied / Empty)
Distance reading
Live temperature and humidity
Device uptime
Fully runs on ESP8266 — no external server needed

##How It Works

Ultrasonic sensor measures distance to detect whether a person is sitting.
DHT11 reads temperature & humidity.
ESP8266WebServer hosts a web application that shows the live data.
If the person sits for more than a preset time, the buzzer alerts them.
Web UI auto-refreshes using JavaScript to fetch /data from ESP8266.

##Hardware Used

ESP8266 (ESP-12E module / NodeMCU type board)
H-SR04 Ultrasonic Sensor
DHT11 Temperature & Humidity Sensor
Active Buzzer
Jumper wires
USB cable for programming

##Accessing the Dashboard

Once the ESP8266 connects to WiFi, it prints its IP address in Serial Monitor.
Open any browser and visit the local host id generated from the execution of the program.
The dashboard will automatically start showing data.

##Future Improvements (optional)

Add live “Minutes Sitting” counter
Add buzzer status indicator on UI
Store logs using SPIFFS / SD card
Make mobile-friendly theme
Add MQTT or Firebase for cloud monitoring
