This project is an ESP8266-based IoT application.

The system collects temperature and humidity data using a DHT11 sensor, and tracks two values with buttons:

Miktar (Quantity, e.g., number of produced items)

Fire (Defect, e.g., number of defective/rejected items)

Main Features
** Wi-Fi Connection Management

If the device cannot connect to a known Wi-Fi network, it creates its own configuration portal using the AutoConnect library.

** Settings Storage with LittleFS

Wi-Fi credentials, user login information, and server connection details are stored in the ESP8266's internal file system.

** Temperature and Humidity Measurement with DHT11 Sensor

** Tracking Quantity and Defect Counts via Buttons

** Sending Data to a SOAP Web Service

The collected temperature, humidity, and quantity data are sent to a specified web service via HTTP POST requests formatted in XML.

Operation Modes
If sending type 1 (Real-time Sending) is selected:
➔ Data is sent immediately when a button is pressed or a temperature change is detected.

If sending type 2 (Send Every 30 Minutes) is selected:
➔ Data is averaged over 30 minutes and then sent.

If the Wi-Fi connection is lost:
➔ The device switches to Access Point (AP) Mode and allows the user to reconnect.

