#include <PubSubClient.h>
#include <WiFi.h>
#include <map>
#include <Arduino.h>
#include <vector>
#include <algorithm> // for std::sort
#include <ArduinoJson.h>



#include <HardwareSerial.h>


HardwareSerial MySerial(2);

const char* ssid = "DESKTOP-Q111QC8 5213";
const char* password = "t344T5$7";

// const char* ssid = "GUEST_AP";
// const char* password = "BJIT#Guest";

//const char* mqtt_server = "0.tcp.in.ngrok.io";
const char* mqtt_server ="103.197.206.61";

// const char* mqtt_username = "mqtt_username";
// const char* mqtt_password = "mqtt_password";
const char* mqtt_clientid = "CLIENT1213";
int count  = 0;
int wifi = 5;
int lastTime = 0;
int lastTag1 = 0;
int lastTag2 = 0;
int lastTag3 = 0;
int lastTag4 = 0;

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
            digitalWrite(wifi, HIGH);
            delay(2500);
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
    }
    
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    macAddr = WiFi.macAddress();
    Serial.print("MAC address: ");
    Serial.println(macAddr);
    

   // digitalWrite(wifi, HIGH);
}



void setup() {
  Serial.begin(115200);

  MySerial.begin(115200, SERIAL_8N1, 16, 17);

  delay(1000);
  pinMode(wifi, OUTPUT);
  digitalWrite(wifi, LOW);

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
  
  
  for (int i = 0; i < liveDevices.size(); i++) {

    // Serial.print("Element ");
    // Serial.print(i);
    // Serial.print(": ");
    // Serial.println(liveDevices[i]);

    Serial.println("AT+ANCHOR_SEND="+liveDevices[i]+",4,Test");

    MySerial.println("AT+ANCHOR_SEND="+liveDevices[i]+",4,Test");
    delay(100);
  
   
  }
  
  
  //  MySerial.println("AT+ANCHOR_SEND=Tag1,4,Test");
    // delay(200);

  //  MySerial.println("AT+ANCHOR_SEND=Tag2,4,Test");
  //   delay(100);

  

  Serial.println("OUTSIDE LOOP");

  while(MySerial.available()){
      String response = MySerial.readString();
      
      data.concat(extractData(response));
      //count+=1;

      if (response.indexOf("OK") != -1) {
        Serial.println("Communication with IPS Module Successful");
      }
      {
     Serial.println(data);
     Serial.println(count);
      if(data!=""){
        client.publish("topic/test", data.c_str());
        count=0;
        unsigned long currentTime = millis();  // Get current time in milliseconds
        unsigned long diff = currentTime - lastTime;
        Serial.print("Difference: ");
        Serial.println(diff);
        lastTime = currentTime;
      }
        break;
      }
    }

 delay(1000);

}


String extractData(String input){

String myRes = "";

const int maxValues = 10; // Maximum number of values to store
int values[maxValues];
String tags[maxValues];
int valuesCount = 0;

  String delimiter = "cm";
  int startIndex = 0;
  int endIndex = 0;


  while ((endIndex = input.indexOf(delimiter, startIndex)) != -1) {
    // Find the part before "cm"
    String part = input.substring(startIndex, endIndex);
    
    // Find the tag
    int tagStartIndex = part.indexOf("Tag");
    int tagEndIndex = part.indexOf(',', tagStartIndex);
    String tag = part.substring(tagStartIndex, tagEndIndex);
    
    // Find the last comma in this part to locate the number
    int numberStartIndex = part.lastIndexOf(',') + 1;
    // Extract the number
    String number = part.substring(numberStartIndex);
    
    // Store the tag and number in the arrays
    if (valuesCount < maxValues) {
      tags[valuesCount] = tag;
      values[valuesCount] = number.toInt();
      valuesCount++;
    }
    
    // Update startIndex to continue searching
    startIndex = endIndex + delimiter.length();
  }

  // Print the extracted values and tags
 // Serial.println("Extracted values and tags:");

  std::map<String, int> myMap;

  Serial.print("Tags and Values");
  for (int i = 0; i < valuesCount; i++) {  
    myMap.insert(std::make_pair(tags[i], values[i]));

  }


  // for (auto it = myMap.begin(); it != myMap.end(); ++it) {
  //   Serial.print("Key: ");
  //   Serial.print(it->first);
  //   Serial.print(", Value: ");
  //   Serial.println(it->second);
  // }

// Create a vector of pairs from the map
  std::vector<std::pair<String, int>> vec;
  for (const auto& pair : myMap) {
      vec.push_back(pair);
  }

     // Sort the vector based on the values (second part of pair)
  std::sort(vec.begin(), vec.end(),
      [](const std::pair<String, int>& a, const std::pair<String, int>& b) {
          return a.second < b.second;
      });


 // Serial.println("\nSorted map by values:");
  myRes.concat(macAddr);
  myRes.concat("#");

  if(vec.size() >= 2 ){
    for (const auto& pair : vec) {
      Serial.print(pair.first.c_str());
      Serial.print(": ");
     // Serial.println(pair.second);
      myRes.concat(pair.first.c_str());
      myRes.concat(":");
      myRes.concat(pair.second);
      myRes.concat("\n");
    }
  }

  return myRes;
}

