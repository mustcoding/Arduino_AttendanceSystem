#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> 
#include <time.h>
#include <WebSocketsServer.h>

#define SS_PIN 5
#define RST_PIN 22
#define BUZZER_PIN 2 // Define the pin for the buzzer

MFRC522 mfrc522(SS_PIN, RST_PIN);

const char* ssid = "MAHMOR"; // Change to your WiFi SSID
const char* password = "12345678"; // Change to your WiFi password
const char* sendId = "http://10.131.74.57:8000/retrieve-rfid-id";
const char* searchById = "http://10.131.74.57:8000/searchByRfid";
const char* getStudentSessionId = "http://10.131.74.57:8000/get-id-by-studentId";
const char* checkAttendance = "http://10.131.74.57:8000/checkAttendance-by-time";
const char* recordAttendanceUrl = "http://10.131.74.57:8000/recordAttendance";
const char* sendStudentId = "http://10.131.74.57:8000/toDisplay";

WebSocketsServer webSocket = WebSocketsServer(81);

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Approximate your card to the reader...");
  Serial.println();
  connectToWiFi(); // Connect to WiFi
  initializeTime(); // Initialize and synchronize time
  pinMode(BUZZER_PIN, OUTPUT); // Set the buzzer pin as output
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  Serial.print("UID tag :");
  String content= "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();

  triggerBuzzer();

  sendRFIDData(content);
  delay(1000);
}

void triggerBuzzer() {
  digitalWrite(BUZZER_PIN, HIGH); // Turn on the buzzer
  delay(200); // Keep the buzzer on for 200 ms
  digitalWrite(BUZZER_PIN, LOW); // Turn off the buzzer
}

void sendRFIDData(String data) {
  WiFiClient wifi;
  HTTPClient client; // Initialize HTTPClient

  String rfid = data;
  rfid.toUpperCase(); // Convert to uppercase
  
  client.begin(sendId); // Set the server URL
  
  client.addHeader("Content-Type", "application/json"); // Set content type
  
  String payload = "{\"number\":\"" + rfid + "\"}"; // Create the payload
  
  int httpResponseCode = client.POST(payload); // Send the POST request with payload
  
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = client.getString(); // Get the response
    Serial.println("Response: ");
    Serial.println(response);

    // Parse the JSON response
    DynamicJsonDocument doc(1024); // Create a JSON document
    deserializeJson(doc, response); // Deserialize the JSON response
    
    // Extract the ID
    int id = doc["rfid"][0]["id"]; // Assuming "id" is a long integer
    
    if (id == 0){
      client.end();
    } else {
      searchByRfid(id);
    }
    // Print the ID
    Serial.print("ID: ");
    Serial.println(id);
    client.end(); // End the connection

  } else {
    Serial.println("Error on sending POST request.");
  }
}

void searchByRfid(int data) {
  WiFiClient wifi;
  HTTPClient client; // Initialize HTTPClient

  int rfid = data;

  client.begin(searchById); // Set the server URL
  
  client.addHeader("Content-Type", "application/json"); // Set content type
  
  // Convert the integer RFID to a string
  String rfidString = String(rfid);
  
  // Construct the JSON payload with the RFID number as a string
  String payload = "{\"rfidNumber\":" + rfidString + "}";
  
  int httpResponseCode = client.POST(payload); // Send the POST request with payload
  
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = client.getString(); // Get the response
    Serial.println("Response: ");
    Serial.println(response);

    // Parse the JSON response
    DynamicJsonDocument doc(1024); // Create a JSON document
    deserializeJson(doc, response); // Deserialize the JSON response
    
    // Extract the ID
    int id = doc["students"][0]["id"]; // Assuming "id" is a long integer
    String username = doc["students"][0]["parent_guardian"]["username"];
    String name = doc["students"][0]["name"];
    
    // Print the ID
    Serial.print("Student ID: ");
    Serial.println(id);
    client.end(); // End the connection

    studentStudySession(id, username, name);

  } else {
    Serial.println("Error on sending POST request.");
  }
}

void studentStudySession(int data, String username, String name) {
  WiFiClient wifi;
  HTTPClient client; // Initialize HTTPClient

  int student_id = data;
 

  client.begin(getStudentSessionId); // Set the server URL
  
  client.addHeader("Content-Type", "application/json"); // Set content type
  
  // Convert the integer RFID to a string
  String studentID = String(student_id);
  
  // Construct the JSON payload with the RFID number as a string
  String payload = "{\"student_id\":" + studentID + "}";
  
  int httpResponseCode = client.POST(payload); // Send the POST request with payload
  
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = client.getString(); // Get the response
    Serial.println("Response: ");
    Serial.println(response);

    // Parse the JSON response
    DynamicJsonDocument doc(1024); // Create a JSON document
    deserializeJson(doc, response); // Deserialize the JSON response
    
    // Extract the ID
    int student_study_session_id = doc["study"][0]["id"]; // Assuming "id" is a long integer
    
    // Print the ID
    Serial.print("Student Study Session ID: ");
    Serial.println(student_study_session_id);
    client.end(); // End the connection

    attendanceTimetable(student_study_session_id, student_id, username, name);

  } else {
    Serial.println("Error on sending POST request.");
  }
}

void attendanceTimetable(int student_study_session_id, int student_id, String username, String name) {
  WiFiClient wifi;
  HTTPClient client; // Initialize HTTPClient

  client.begin(checkAttendance); // Set the server URL
  
  client.addHeader("Content-Type", "application/json"); // Set content type

  int httpResponseCode = client.GET(); // Send the GET request

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = client.getString(); // Get the response
    Serial.println("Response: ");
    Serial.println(response);

    // Check if the response code is 404
    if (httpResponseCode == 404) {
      Serial.println("Error: Timetable not found.");
      client.end(); // End the connection
      return; // Stop further execution
    }

    // Parse the JSON response
    DynamicJsonDocument doc(1024); // Create a JSON document
    deserializeJson(doc, response); // Deserialize the JSON response
    
    // Extract the ID
    int attendance_timetable_id = doc["timetable"]["id"]; // Assuming "id" is a long integer
    
    // Print the ID
    Serial.print("Attendance TimeTable ID: ");
    Serial.println(attendance_timetable_id);
    client.end(); // End the connection

    recordAttendance(student_study_session_id, student_id, attendance_timetable_id, username, name);

  } else {
    Serial.print("Error on sending GET request. HTTP Response code: ");
    Serial.println(httpResponseCode);
  }
}

void recordAttendance(int student_study_session_id, int student_id, int attendance_timetable_id, String username, String name) {
  WiFiClient wifi;
  HTTPClient client; // Initialize HTTPClient

  int checkpoint = 2;
  int attendance_status = 1;

  // Get the current date and time
  String date_time_in = getCurrentDateTime();
  String date_time_out = "";
  String platform = "RFID";
  Serial.print("Current date_time_in: ");
  Serial.println(date_time_in);
  client.begin(recordAttendanceUrl); // Set the server URL
  
  client.addHeader("Content-Type", "application/json"); // Set content type
  
  // Convert the integer RFID to a string
  String studentID = String(student_id);
  
  // Construct the JSON payload with the RFID number as a string
  // Construct the JSON payload with all the variables
  String payload = "{\"student_study_session_id\":" + String(student_study_session_id) +
                   ",\"student_id\":" + studentID +
                   ",\"attendance_timetable_id\":" + String(attendance_timetable_id) +
                   ",\"checkpoint_id\":" + String(checkpoint) +
                   ",\"is_attend\":" + String(attendance_status) +
                   ",\"date_time_in\":\"" + date_time_in + "\"" +
                   ",\"platform\":\"" + platform + "\"" +
                   ",\"date_time_out\":\"" + date_time_out + "\"}";

  int httpResponseCode = client.POST(payload); // Send the POST request with payload
  
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = client.getString(); // Get the response
    Serial.println("Response: ");
    Serial.println(response);

    // Parse the JSON response
    DynamicJsonDocument doc(1024); // Create a JSON document
    deserializeJson(doc, response); // Deserialize the JSON response
    
    // Extract the ID
    int registration_complete = doc["attendance"]["id"]; // Assuming "id" is a long integer
    
    // Print the ID
    Serial.print("Attendance Successfully being recorded with ID: ");
    Serial.println(registration_complete);

    displayStudentInfo(student_id, registration_complete);

    if (registration_complete>0){
      sendNotification(username, name);
    }

    

    client.end(); // End the connection

  } else {
    Serial.println("Error on sending POST request.");
  }
}

void sendNotification(String username, String name) {
    WiFiClient wifi;
    HTTPClient client;

    String serverPath = "https://onesignal.com/api/v1/notifications";
    client.begin(serverPath);

    client.addHeader("Content-Type", "application/json");
    client.addHeader("accept", "application/json");
    client.addHeader("Authorization", "Basic OGMxODYyZmMtODllYS00MjYzLThkMzctODQzMTIyZDllZTkw");

    String message = name + " Was ATTEND TO SCHOOL";
    // Construct the JSON payload
    String payload = "{\"app_id\": \"78f2e497-f306-4a96-9f47-c0ae59591675\","
                     "\"include_external_user_ids\": [\"" + username + "\"],"
                     "\"headings\": {\"en\": \"SMAJU Attendance System\"},"
                     "\"contents\": {\"en\": \""+message+"\"}}";

    int httpResponseCode = client.POST(payload); // Send the POST request with payload

    if (httpResponseCode > 0) {
        Serial.print("Notification sent, HTTP Response code: ");
        Serial.println(httpResponseCode);
        String response = client.getString(); // Get the response
        Serial.println("Response: ");
        Serial.println(response);
    } else {
        Serial.println("Error on sending POST request for notification.");
    }

    client.end(); // End the connection
}

void displayStudentInfo(int student_id, int registration_complete) {
  WiFiClient wifi;
  HTTPClient client; 

  // Implement the code to send student information to the web page
  // This could be via a WebSocket, HTTP, or other methods
  Serial.print("Student Id: ");
  Serial.println(student_id);

  client.begin(sendStudentId);
  client.addHeader("Content-Type", "application/json");

  // Convert the student_id to a String
  String payload = "{\"student_id\":\"" + String(student_id) + "\", \"attendance_id\":\"" + String(registration_complete) + "\"}";

  int httpResponseCode = client.POST(payload);
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = client.getString(); // Get the response
    Serial.println("Response: ");
    Serial.println(response);
  } else {
    Serial.println("Error on sending POST request.");
  }

  client.end(); // End the connection
}

String getCurrentDateTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "";
  }
  // Format the date and time
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%m/%d/%y %H:%M:%S", &timeinfo);
  
  return String(buffer); // Return the formatted date and time as a string
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password); // Connect to WiFi network using SSID and password
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  
  Serial.println("Connected to WiFi");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP()); // Print IP address assigned to your ESP32
  initializeTime(); // Initialize and synchronize time
}

void initializeTime() {
  // Timezone offset for Kuala Lumpur (UTC +8:00)
  const long gmtOffset_sec = 8 * 3600;
  const int daylightOffset_sec = 0; // No daylight saving time in Kuala Lumpur
  
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for time synchronization");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
  Serial.println("Time synchronized.");
}
