#include <math.h>
#include <WiFi.h>
#include <FreeRTOS/FreeRTOS.h>

// Replace with your network credentials
const char* ssid     = "ESP32";
const char* password = "123456789";
const int beta = 1;
const int R1 = 10000; //resistance of R1

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Display variables to show GPIO current output state
String output5State = "off";
String output6State = "off";

// Assign output variables to GPIO pins
const int output5 = 5;
const int output6 = 6;
const int thermistorPin = 1; //1 is ADC1_0

double thermistor_temperature_C(double R, double R0) {
    const double T0 = 298.15;   // reference temp in Kelvin (25°C)
    const double B  = 2904.0;   // beta value

    double inv_T = (1.0 / T0) + (1.0 / B) * log(R / R0);
    double T_K = 1.0 / inv_T;

    double T_C = T_K - 273.15;

    return T_C;
}


void setup() {
	Serial.begin(115200);
    analogReadResolution(12); //set analog read resolution to 12 bits (min: 0, max = 2^12 - 1)
    analogSetAttenuation(ADC_11db); //full 3.3V range for analog readings

	// Initialize the digital output variables
	pinMode(output5, OUTPUT);
	pinMode(output6, OUTPUT);

	// Set outputs to LOW
	digitalWrite(output5, LOW);
	digitalWrite(output6, LOW);

	// Connect to Wi-Fi network with SSID and password
	Serial.print("Setting AP (Access Point)…");
    
    //max out antenna power
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
	
    // Remove the password parameter, if you want the AP (Access Point) to be open
	WiFi.softAP(ssid, password);

	IPAddress IP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(IP);

	server.begin();
}

void loop() {
	WiFiClient client = server.available();   // Listen for incoming clients

	if (client) {                             // If a new client connects,
		Serial.println("New Client.");        // print a message out in the serial port
		String currentLine = "";              // make a String to hold incoming data from the client
		while (client.connected()) {          // loop while the client's connected
			if (client.available()) {         // if there's bytes to read from the client,
				char c = client.read();       // read a byte, then
				Serial.write(c);              // print it out the serial monitor
				header += c;
				if (c == '\n') {              // if the byte is a newline character
					if (currentLine.length() == 0) {
						// HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
						// and a content-type so the client knows what's coming, then a blank line:
						client.println("HTTP/1.1 200 OK");
						client.println("Content-type:text/html");
						client.println("Connection: close");
						client.println();

                        int thermistor = analogRead(1); //read voltage of thermistor
                        double thermistorVoltage = (thermistor*3.3)/4095.0; //convert to voltage
                        double thermistorResistance = (thermistorVoltage*R1)/(3.3-thermistorVoltage); //convert to resistance
                        double temperature = thermistor_temperature_C(thermistorResistance, R1);
                        String displayTemp = String(temperature);

						// turns the GPIOs on and off
						if (header.indexOf("GET /5/on") >= 0) {
							Serial.println("GPIO 5 on");
							output5State = "on";
							digitalWrite(output5, HIGH);

						} else if (header.indexOf("GET /5/off") >= 0) {
							Serial.println("GPIO 5 off");
							output5State = "off";
							digitalWrite(output5, LOW);
						} else if (header.indexOf("GET /6/on") >= 0) {
							Serial.println("GPIO 6 on");
							output6State = "on";
							digitalWrite(output6, HIGH);
						} else if (header.indexOf("GET /6/off") >= 0) {
							Serial.println("GPIO 6 off");
							output6State = "off";
							digitalWrite(output6, LOW);
						}

						// Display the HTML web page
						client.println("<!DOCTYPE html><html>");
						client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
						client.println("<link rel=\"icon\" href=\"data:,\">");
						// CSS to style the on/off buttons
						client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
						client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
						client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
						client.println(".button2 {background-color: #555555;}</style></head>");

						// Web Page Heading
						client.println("<body><h1>ESP32 Web Server</h1>");

                        // display temperature here:
                        client.println("<p>Temperature: " + displayTemp + " °C</p>");

						// Display current state, and ON/OFF button for GPIO 5
						client.println("<p>GPIO 5 - State " + output5State + "</p>");
						if (output5State == "off") {
							client.println("<p><a href=\"/5/on\"><button class=\"button\">TURN ON</button></a></p>");
						} else {
							client.println("<p><a href=\"/5/off\"><button class=\"button button2\">TURN OFF</button></a></p>");
						}

						// Display current state, and ON/OFF button for GPIO 6
						client.println("<p>GPIO 6 - State " + output6State + "</p>");
						if (output6State == "off") {
							client.println("<p><a href=\"/6/on\"><button class=\"button\">TURN ON</button></a></p>");
						} else {
							client.println("<p><a href=\"/6/off\"><button class=\"button button2\">TURN OFF</button></a></p>");
						}

						client.println("</body></html>");
						client.println();
						break;
					} else {
						currentLine = "";
					}
				} else if (c != '\r') {
					currentLine += c;
				}
			}
		}
		// Clear the header variable
		header = "";
		// Close the connection
		client.stop();
		Serial.println("Client disconnected.");
		Serial.println("");
	}
}