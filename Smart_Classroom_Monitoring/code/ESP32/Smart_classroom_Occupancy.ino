#include <WiFi.h>
#include <ThingSpeak.h>
#include <Preferences.h>

// =====================================================
// IR SENSOR PINS
// =====================================================
#define IR1_PIN 5
#define IR2_PIN 18

// =====================================================
// CLEAR BUTTON
// GPIO 23 -> PUSH BUTTON -> GND
// =====================================================
#define CLEAR_BUTTON_PIN 23

// =====================================================
// WIFI
// =====================================================
const char* ssid = "WRITE_YOUR_WIFI_NAME";
const char* password = "WRITE_YOUR_WIFI_PASSWORD";

// =====================================================
// THINGSPEAK
// =====================================================
unsigned long channelID = PASTE_YOUR_ChannelID;
const char* writeAPIKey = "PASTE_YOUR_writeAPIKey";

WiFiClient client;

// =====================================================
// STORED COUNTERS
// =====================================================
int presentInside = 0;
int totalEntered = 0;
int totalExited = 0;

// =====================================================
// ESP32 NON-VOLATILE STORAGE
// =====================================================
Preferences preferences;

// =====================================================
// THINGSPEAK TIMER
// =====================================================
unsigned long lastUpload = 0;
const unsigned long uploadInterval = 15000;

// =====================================================
// CLEAR BUTTON
// =====================================================
bool lastButtonState = HIGH;
unsigned long lastButtonTime = 0;


// =====================================================
// SAVE DATA TO ESP32 MEMORY
// =====================================================
void saveData() {

  preferences.begin("classroom", false);

  preferences.putInt("present", presentInside);
  preferences.putInt("entered", totalEntered);
  preferences.putInt("exited", totalExited);

  preferences.end();
}


// =====================================================
// LOAD DATA FROM ESP32 MEMORY
// =====================================================
void loadData() {

  preferences.begin("classroom", true);

  presentInside = preferences.getInt("present", 0);
  totalEntered = preferences.getInt("entered", 0);
  totalExited = preferences.getInt("exited", 0);

  preferences.end();

  Serial.println();
  Serial.println("==============================");
  Serial.println(" SAVED DATA RESTORED");
  Serial.println("==============================");

  Serial.print("Present Inside : ");
  Serial.println(presentInside);

  Serial.print("Total Entered  : ");
  Serial.println(totalEntered);

  Serial.print("Total Exited   : ");
  Serial.println(totalExited);

  Serial.println("==============================");
}


// =====================================================
// CLEAR ALL COUNTERS
// =====================================================
void clearCounters() {

  presentInside = 0;
  totalEntered = 0;
  totalExited = 0;

  // Save zero values permanently
  saveData();

  Serial.println();
  Serial.println("==============================");
  Serial.println("     COUNTERS CLEARED");
  Serial.println("==============================");
  Serial.println("Present Inside : 0");
  Serial.println("Total Entered  : 0");
  Serial.println("Total Exited   : 0");
  Serial.println("==============================");
}


// =====================================================
// CHECK CLEAR BUTTON
// =====================================================
void checkClearButton() {

  bool buttonState = digitalRead(CLEAR_BUTTON_PIN);

  // Detect button press
  if (buttonState == LOW && lastButtonState == HIGH) {

    // Simple debounce
    if (millis() - lastButtonTime > 300) {

      clearCounters();

      lastButtonTime = millis();
    }
  }

  lastButtonState = buttonState;
}


// =====================================================
// CONNECT TO WIFI
// =====================================================
void connectWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("Connecting to WiFi");

  WiFi.begin(ssid, password);

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 10000) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("WiFi Connected!");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

  } else {

    Serial.println("WiFi connection failed.");
    Serial.println("Counting will continue locally.");
  }
}


// =====================================================
// SETUP
// =====================================================
void setup() {

  Serial.begin(115200);

  // IR sensors
  pinMode(IR1_PIN, INPUT);
  pinMode(IR2_PIN, INPUT);

  // Clear button
  pinMode(CLEAR_BUTTON_PIN, INPUT_PULLUP);

  // Restore previously saved values
  loadData();

  // Connect WiFi
  connectWiFi();

  // Start ThingSpeak
  ThingSpeak.begin(client);

  Serial.println();
  Serial.println("==============================");
  Serial.println(" SMART CLASSROOM MONITOR");
  Serial.println("==============================");

  Serial.print("Present Inside : ");
  Serial.println(presentInside);

  Serial.print("Total Entered  : ");
  Serial.println(totalEntered);

  Serial.print("Total Exited   : ");
  Serial.println(totalExited);

  Serial.println("==============================");
}


// =====================================================
// LOOP
// =====================================================
void loop() {

  // ---------------------------------------------------
  // CLEAR BUTTON
  // ---------------------------------------------------
  checkClearButton();


  // ---------------------------------------------------
  // WIFI
  // ---------------------------------------------------
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }


  // ===================================================
  // IR1 -> IR2 = ENTER
  // ===================================================

  if (digitalRead(IR1_PIN) == LOW) {

    unsigned long startTime = millis();

    while (millis() - startTime < 2000) {

      if (digitalRead(IR2_PIN) == LOW) {

        totalEntered++;
        presentInside++;

        // Save immediately
        saveData();

        Serial.println();
        Serial.println(">>> STUDENT ENTERED <<<");

        Serial.print("Present Inside : ");
        Serial.println(presentInside);

        Serial.print("Total Entered  : ");
        Serial.println(totalEntered);

        Serial.print("Total Exited   : ");
        Serial.println(totalExited);

        // Wait until both sensors become clear
        while (digitalRead(IR1_PIN) == LOW ||
               digitalRead(IR2_PIN) == LOW) {

          delay(50);
        }

        delay(500);

        return;
      }
    }
  }


  // ===================================================
  // IR2 -> IR1 = EXIT
  // ===================================================

  if (digitalRead(IR2_PIN) == LOW) {

    // Don't allow negative occupancy
    if (presentInside <= 0) {

      Serial.println();
      Serial.println(">>> EXIT IGNORED - ROOM EMPTY <<<");

      // Wait until sensors become clear
      while (digitalRead(IR1_PIN) == LOW ||
             digitalRead(IR2_PIN) == LOW) {

        delay(50);
      }

      delay(500);

      return;
    }


    unsigned long startTime = millis();

    while (millis() - startTime < 2000) {

      if (digitalRead(IR1_PIN) == LOW) {

        totalExited++;
        presentInside--;

        // Save immediately
        saveData();

        Serial.println();
        Serial.println(">>> STUDENT EXITED <<<");

        Serial.print("Present Inside : ");
        Serial.println(presentInside);

        Serial.print("Total Entered  : ");
        Serial.println(totalEntered);

        Serial.print("Total Exited   : ");
        Serial.println(totalExited);

        // Wait until both sensors become clear
        while (digitalRead(IR1_PIN) == LOW ||
               digitalRead(IR2_PIN) == LOW) {

          delay(50);
        }

        delay(500);

        return;
      }
    }
  }


  // ===================================================
  // THINGSPEAK UPDATE
  // ===================================================

  if (millis() - lastUpload >= uploadInterval) {

    if (WiFi.status() == WL_CONNECTED) {

      // Field 1
      ThingSpeak.setField(1, presentInside);

      // Field 2
      ThingSpeak.setField(2, totalEntered);

      // Field 3
      ThingSpeak.setField(3, totalExited);

      int response =
        ThingSpeak.writeFields(channelID, writeAPIKey);

      if (response == 200) {

        Serial.println();
        Serial.println("ThingSpeak Update Successful");

        Serial.print("Present : ");
        Serial.println(presentInside);

        Serial.print("Entered : ");
        Serial.println(totalEntered);

        Serial.print("Exited  : ");
        Serial.println(totalExited);

      } else {

        Serial.print("ThingSpeak Error Code: ");
        Serial.println(response);
      }

      lastUpload = millis();
    }
  }
}