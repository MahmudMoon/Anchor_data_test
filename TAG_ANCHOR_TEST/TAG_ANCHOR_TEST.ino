#include <PubSubClient.h>
#include <WiFi.h>
#include <map>
#include <Arduino.h>
#include <vector>
#include <algorithm> // for std::sort
#include <ArduinoJson.h>



#include <HardwareSerial.h>


HardwareSerial MySerial(2);

//const char* ssid = "DESKTOP-Q111QC8 5213";
//const char* password = "t344T5$7";
// const int LED_PIN = 2;
//  const char* ssid = "BJIT_ADMIN";
//  const char* password = "Bjit#@dmin";

 const char* ssid = "SoftBank_AP";
 const char* password = "softbank@#2021";

//const char* mqtt_server = "0.tcp.in.ngrok.io";
const char* mqtt_server ="103.197.206.61";

// const char* mqtt_username = "mqtt_username";
// const char* mqtt_password = "mqtt_password";
const char* mqtt_clientid = "CLIENT1213";
int count  = 0;
int wifi = 2;
int lastTime = 0;
int lastTag2 = 0;
int lastTag3 = 0;
int lastTag4 = 0;
int wifiRestart = 0;

String macAddr = "";
std::vector<String> liveDevices;  // Declare a dynamic array (vector)




// Callback function when a message arrives
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  //if(topic == "topic/liveDevices"){

  // Copy the payload into a string for parsing
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  // Deserialize the JSON message (assuming the array is sent as JSON)
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }
  
  // Assume the array is sent as: [1, 2, 3, 4, 5]
  JsonArray array = doc.as<JsonArray>();

  liveDevices.clear();
  // Parse the JSON array into the local array
  for (int i = 0; i < array.size(); i++) {
    liveDevices.push_back(array[i].as<String>());
  }
 // }
}

String extractData(String res);

WiFiClient espClient;
PubSubClient client(espClient);

void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");

        while (WiFi.status() != WL_CONNECTED) {
          digitalWrite(wifi, HIGH);
          delay(250);
          Serial.print(".");
          digitalWrite(wifi, LOW);
          delay(250);
          wifiRestart+=1;
          if(wifiRestart>=30){
            wifiRestart = 0;
            break;
          }
        }
    
        // Attempt to connect
        if (client.connect(String(WiFi.macAddress()).c_str())) {
            Serial.println("connected");
            digitalWrite(wifi, HIGH);
            client.subscribe("topic/liveDevices", 1);  // Subscribe with QoS 1 (or 0)
            client.publish("topic/liveDeviceStatus", "ask");

        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            digitalWrite(wifi, LOW);
            delay(2500);
            setup_wifi();
        }
    }
}

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);
    

    
    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(wifi, HIGH);
        delay(250);
        Serial.print(".");
        digitalWrite(wifi, LOW);
        delay(250);
        wifiRestart+=1;
        if(wifiRestart>=40){
          ESP.restart();
        }
    }
    
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    macAddr = WiFi.macAddress();
    Serial.print("MAC address: ");
    Serial.println(macAddr);
    // Light up the LED on pin 2 when connected
    digitalWrite(wifi, HIGH);
}



void setup() {
  Serial.begin(115200);

  MySerial.begin(115200, SERIAL_8N1, 16, 17);

  delay(1000);
  pinMode(wifi, OUTPUT);
  // pinMode(LED_PIN, OUTPUT); // Set pin 2 as output
  //digitalWrite(wifi, LOW);

  Serial.println("Sending AT command to IPS module...");
  Serial.println("setup done");

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1884 );
  client.setCallback(callback);

  reconnect();
  // Subscribe to a topic
  //client.subscribe("topic/test");
  delay(2000);
  
}

void loop() {
   if (!client.connected()) {
        reconnect();
    }
    client.loop();
    String data = "";
  
    // Iterate through live devices, even if there's only one
    for (int i = 0; i < liveDevices.size(); i++) {
        // Send the AT command to each device
        Serial.println("AT+ANCHOR_SEND=" + liveDevices[i] + ",4,Test");
        MySerial.println("AT+ANCHOR_SEND=" + liveDevices[i] + ",4,Test");
        delay(100);
    }
  
    // Check the serial input from MySerial
    Serial.println("OUTSIDE LOOP");
    while (MySerial.available()) {
        String response = MySerial.readString();
        
        // Extract data from the response
        data.concat(extractData(response));

        // Check for a successful response
        if (response.indexOf("OK") != -1) {
            Serial.println("Communication with IPS Module Successful");
        }

        // If valid data is received, publish it
        if (data != "" && !data.endsWith("#")) {
            Serial.println("Publishing data to MQTT...");
            client.publish("topic/test", data.c_str());
            count = 0;
            unsigned long currentTime = millis();  // Get current time in milliseconds
            unsigned long diff = currentTime - lastTime;
            Serial.print("Time since last publish: ");
            Serial.println(diff);
            lastTime = currentTime;
        }

        break;  // Exit the loop after processing the response
    }

    delay(1000);  // Delay before next iteration
}

String extractData(String input) {
    String myRes = "";
    const int maxValues = 10; // Maximum number of values to store
    int values[maxValues];
    String tags[maxValues];
    int valuesCount = 0;

    String delimiter = "cm";
    int startIndex = 0;
    int endIndex = 0;

    // Extract data based on the 'cm' delimiter
    while ((endIndex = input.indexOf(delimiter, startIndex)) != -1) {
        String part = input.substring(startIndex, endIndex);
        
        // Extract the tag
        int tagStartIndex = part.indexOf("Tag");
        int tagEndIndex = part.indexOf(',', tagStartIndex);
        String tag = part.substring(tagStartIndex, tagEndIndex);
        
        // Extract the number (distance)
        int numberStartIndex = part.lastIndexOf(',') + 1;
        String number = part.substring(numberStartIndex);
        
        // Store the tag and distance
        if (valuesCount < maxValues) {
            tags[valuesCount] = tag;
            values[valuesCount] = number.toInt();
            valuesCount++;
        }

        startIndex = endIndex + delimiter.length();
    }

    // Store tags and distances in a map for sorting
    std::map<String, int> myMap;
    for (int i = 0; i < valuesCount; i++) {
        myMap[tags[i]] = values[i];
    }

    // Sort the map by distance
    std::vector<std::pair<String, int>> vec;
    for (const auto& pair : myMap) {
        vec.push_back(pair);
    }

    std::sort(vec.begin(), vec.end(), [](const std::pair<String, int>& a, const std::pair<String, int>& b) {
        return a.second < b.second;
    });

    // Prepare the result string
    //myRes.concat(macAddr);

    if(macAddr == "D4:8A:FC:A4:E7:2C"){
      myRes.concat("Anchor1");
    }else if(macAddr == "D4:8A:FC:A5:01:E4"){
      myRes.concat("Anchor2");
    }else if(macAddr == "D4:8A:FC:A7:77:3C"){
      myRes.concat("Anchor3");
    }else if(macAddr == "D4:8A:FC:A8:9C:3C"){
      myRes.concat("Anchor4");
    }

    myRes.concat("#");

    for (const auto& pair : vec) {
        Serial.print(pair.first);
        Serial.print(": ");
        Serial.println(pair.second);
        myRes.concat(pair.first);
        myRes.concat(":");
        myRes.concat(pair.second);
        myRes.concat("\n");
    }

    return myRes;
}

