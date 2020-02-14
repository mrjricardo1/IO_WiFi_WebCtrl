/*
  WiFi Web Server - Lighting Control and Timer Application

 A simple web server to control state of GPIOs via the web.
 This sketch will print the IP address of your WiFi Shield (once connected)
 to the Serial monitor. From there, you can open that address in a web browser
 to turn on and off the output. It also features a push button to trigger a
 timer to keep the output activated for 60 minutes and then off for the rest
 of the day so if for example the output is activated at 9pm on a day then it 
 will keep being activated at 9pm for an hour every day.

 This example is written for a network using WPA encryption. For
 WEP or WPA, change the WiFi.begin() call accordingly.

 Based on initial code from Tom Igoe which was created on 25 Nov 2012.

 It has been modified by RM on May 2019.
 */
#include <SPI.h>
#include <WiFi101.h>

///////please enter SSID and Password here
char ssid[] = "Your_SSID_here";           // your network SSID (name)
char pass[] = "Your_WiFi_Password_here";  // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                         // your network key Index number (needed only for WEP)

// Flags
int pb_release_flag = 0;
int auto_pb_press_flag = 0;

// Pin definition
int auto_push_button = 10;    // Pin 10 for auto-mode push button
int gnd_output = 11;          // Pin 11 to use as GND
int red_led = 12;             // Pin 12 for Red LED
int green_led = 13;           // Pin 13 for Green LED

// General Purpose Variables
int status = WL_IDLE_STATUS;
WiFiServer server(80);
unsigned long currentMillis;
unsigned long previousMillis = 0;
const long interval = 1000;     // 1000ms for 1second
int ledState = LOW;
unsigned long TimeON = 3600;    // 3600s for 60minutes or 1 hour
unsigned long TimeOFF = 86400;  // 86400s for 1440minutes or 24 hours - 3600s of ON and the rest of the time OFF
unsigned long TimeCounter = 0;
String output9State = "off";
String Timer9State = "off";
String header;
int debou_state = 0;            // Debounce state register
String sw_version = "01.00.06, 30/05/19";

// Fixed IP address
IPAddress ip(192, 168, 0, 100);

/////////////////////////
//    Setup 
/////////////////////////
void setup() {
  Serial.begin(9600);                     // initialize serial communication
  pinMode(9, OUTPUT);                     // set the LED pin mode
  pinMode(auto_push_button,INPUT_PULLUP); // push-button input
  pinMode(gnd_output, OUTPUT);            // Output set as GND
  digitalWrite(gnd_output, LOW);          // Set line to LOW
  pinMode(red_led, OUTPUT);               // set the LED pin mode
  pinMode(green_led, OUTPUT);             // set the LED pin mode
  digitalWrite(red_led, LOW);             // Set both LEDs to LOW
  digitalWrite(green_led, LOW);           

  // delay to see sw print information on serial monitor
  delay(2000);

  // Print sw information
  Serial.print("SW version: " + sw_version);
  Serial.print("Created by RM - 2019");
  
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true);       // don't continue
  }

  // Configuring Fixed IP to the WiFi module
  WiFi.config(ip);

  // attempt to connect to WiFi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();                           // start the web server on port 80
  printWiFiStatus();                        // you're connected now, so print out the status
}


void loop() {

  /////////////////////////////////////////////
  // Handle auto_pb press and timer function
  /////////////////////////////////////////////
  // Switch case to handle push-button press debouncing
  switch (debou_state) {
    case 0:
      if(digitalRead(auto_push_button) == 0) {
        currentMillis = millis();
        previousMillis = currentMillis;
        debou_state++;
      }
    break;

    case 1:
      currentMillis = millis();
      // 100ms debounce to confirm the push button has been pressed
      if (currentMillis - previousMillis >= 100) {
        if(digitalRead(auto_push_button) == 0) {
          pb_release_flag = 1;
          debou_state++;
        }
        else {
          debou_state = 0;
        }
      }
    break;

    case 2:
      if(digitalRead(auto_push_button) == 1 && pb_release_flag == 1) {
        pb_release_flag = 0;
        previousMillis = 0;
        // Toggle "auto_pb_press_flag"
        if(auto_pb_press_flag == 0) {
          // Switch the MAIN output ON - Timer Starts
          digitalWrite(9, HIGH);
          auto_pb_press_flag = 1;
        }
        else if(auto_pb_press_flag == 1) {
          // Switch the MAIN output OFF - Timer fucntion is stopped
          digitalWrite(9, LOW);
          auto_pb_press_flag = 0;
        }
        delay(100);
        debou_state++;
      }
    break;

    case 3:
      debou_state = 0;
    break;
  }

  // RM: push button press handling as it was before - without debouncing
  /*
  if(digitalRead(auto_push_button) == 0) {
    pb_release_flag = 1;
  }
  else if(digitalRead(auto_push_button) == 1 && pb_release_flag == 1) {
    pb_release_flag = 0;
    // Toggle "auto_pb_press_flag"
    if(auto_pb_press_flag == 0) {
      // Switch the MAIN output ON - Timer Starts
      digitalWrite(9, HIGH);
      auto_pb_press_flag = 1;
    }
    else if(auto_pb_press_flag == 1) {
      // Switch the MAIN output OFF - Timer fucntion is stopped
      digitalWrite(9, LOW);
      auto_pb_press_flag = 0;
    }
  }
  */

  // Button has been pressed and released - the timer action can begin
  if(auto_pb_press_flag == 1) {
    // Load string
    Timer9State = "on";
    // The timer function is running
    currentMillis = millis();

    // Check if the millis counter has reached the interval
    if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      
      // if the LED is off turn it on and vice-versa
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }
  
      // Blink Green LED once a second when the timer function is running
      digitalWrite(green_led, ledState);
      digitalWrite(red_led, LOW);

      // Increase every second
      TimeCounter++;
    }

    // Check if it has reached the TimeOn counter
    if(TimeCounter == TimeON) {
      // Switch the MAIN output OFF
      digitalWrite(9, LOW);
    }
    else if(TimeCounter == TimeOFF) {
      // Switch the MAIN output ON - The cycle stats again
      digitalWrite(9, HIGH);
      // Set counter to zero
      TimeCounter = 0;
    }
  }
  else if(auto_pb_press_flag == 0) {
    // The timer function has stopped running
    // Load string
    Timer9State = "off";
    TimeCounter = 0;
    currentMillis = 0;
    previousMillis = 0;
    digitalWrite(green_led, LOW);
    digitalWrite(red_led, LOW);
  }
  /////////////////////////////////////////////


  /////////////////////////////////////////////
  // Handle the Webpage
  /////////////////////////////////////////////
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    header = "";  // RM added for testing
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected

      // Switch red LED On to indicate that there is a client connected
      digitalWrite(green_led, LOW);
      digitalWrite(red_led, HIGH);
      
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;                        // capture data
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // Checking if the header is valid
            // dXNlcjpwYXNz = 'user:pass' (user:pass) base64 encode
            // Finding the right credential string, then loads web page
            if(header.indexOf("NXdhcmt3b3J0aDplbW1hbHVjYXM=") >= 0) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
  
              // Display the HTML web page
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              client.println("<title>RM Control</title>");
              // CSS to style the on/off buttons
              // Feel free to change the background-color and font-size attributes to fit your preferences
              client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
              client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
              client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              client.println(".button2 {background-color: #77878A;}</style></head>");
              
              // Web Page Heading
              client.println("<body><h1>Garden Lighting Control</h1><br>");
              
              client.println("<body><h2>Deck Lighting</h2><br>");
              // Display current state, and ON/OFF buttons for GPIO 9
              client.println("<p>Current State: " + output9State + "</p>");
              // Display if the timer on GPIO 9 is running
              client.println("<p>Timer State: " + Timer9State + "</p>");
              
              // If the output9State is off, it displays the ON button       
              if (output9State=="off") {
                client.println("<p><a href=\"/9/on\"><button class=\"button\">On</button></a></p>");
              } else {
                client.println("<p><a href=\"/9/off\"><button class=\"button button2\">Off</button></a></p>");
              }
              client.println("</body></html>");
  
              // The HTTP response ends with another blank line:
              client.println();
              // break out of the while loop:
              break;
            }
            else {
              // Wrong user or password, so HTTP request fails... 
              client.println("HTTP/1.1 401 Unauthorized");
              client.println("WWW-Authenticate: Basic realm=\"Secure\"");
              client.println("Content-Type: text/html");
              client.println();
              client.println("<html>Authentication failed</html>");
            }
          }
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /9/on" or "GET /9/off":
        if (currentLine.endsWith("GET /9/on")) {
          output9State = "on";
          digitalWrite(9, HIGH);               // GET /9/on turns the LED on
        }
        if (currentLine.endsWith("GET /9/off")) {
          output9State = "off";
          digitalWrite(9, LOW);                // GET /9/off turns the LED off
        }
      }
    }
    // clear header variable
    header = "";
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
    // Switch red LED off
    digitalWrite(red_led, LOW);
    delay(1000);
    client.stop();
    delay(1000);
  }
  /////////////////////////////////////////////
  
}

void printWiFiStatus() {
  // print network detials:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print your subnet mask:
  IPAddress subnet = WiFi.subnetMask();
  Serial.print("Subnet Mask: ");
  Serial.println(subnet);

  // print your gateway address:
  IPAddress gateway = WiFi.gatewayIP();
  Serial.print("Gateway IP: ");
  Serial.println(gateway);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page open a browser to http://");
  Serial.println(ip);
}
