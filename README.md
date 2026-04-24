Wireless Sensor Dashboard - Java Application

Requirements:
- Java 17 or newer recommended

Files:
- DashboardApp.java        Main entry point
- DashboardFrame.java      Swing GUI dashboard
- SensorData.java          Sensor data model
- MessageParser.java       Parses incoming protocol messages
- TcpSensorServer.java     TCP communication handler
- UdpSensorServer.java     UDP communication handler

Build:
javac *.java

Run:
java DashboardApp

Notes:
- Default port is 5000
- Set USE_TCP = true in DashboardApp.java for TCP mode
- Set USE_TCP = false for UDP mode
- Ensure Pico and PC are on the same WiFi network
