#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

#define RST_PIN       9
#define SS_PIN        10
#define GREEN_LED     3
#define RED_LED       4
#define BUZZER_PIN    5
#define SERVO_PIN     6
#define RF_REMOTE_PIN 2

MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo doorServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Master card token array - Updated to match your actual Wokwi card UID!
const byte AUTHORIZED_UID[] = {0x01, 0x02, 0x03, 0x04}; 

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  Wire.begin();

  lcd.init();
  lcd.backlight();

  doorServo.attach(SERVO_PIN);
  doorServo.write(0); // Door initially locked secure

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RF_REMOTE_PIN, INPUT_PULLUP);

  digitalWrite(RED_LED, HIGH);
  displaySecuredMessage();

  // --- Professional Serial Welcome Banner ---
  Serial.println("=========================================");
  Serial.println("Welcome to DLD Final Project");
  Serial.println("Security door lock system");
  Serial.println("System Ready...");
  Serial.println("=========================================");
}

void loop() {
  // =========================================
  // PATH A: PATIENT REMOTE BUTTON
  // =========================================
  if (digitalRead(RF_REMOTE_PIN) == LOW) {
    grantAccess("Remote Signal");
    delay(500);
    return;
  }

  // =========================================
  // PATH B: RFID MODULE CHECK
  // =========================================
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Show scanned UID in the terminal
  Serial.print("Scanned UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Validate card against Master Key
  bool match = true;
  for (byte i = 0; i < 4; i++) {
    if (mfrc522.uid.uidByte[i] != AUTHORIZED_UID[i]) {
      match = false;
    }
  }

  // Access Decision Logic
  if (match) {
    grantAccess("Caregiver Card");
  } else {
    denyAccess();
  }

  mfrc522.PICC_HaltA();
}

void grantAccess(String source) {
  Serial.println("[➔] Access GRANTED via " + source);
  
  lcd.clear();
  lcd.print("ACCESS GRANTED");
  lcd.setCursor(0, 1);
  lcd.print(source);

  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);

  // High-Quality Double Confirmation Beep
  Serial.println(" [♪ BEEP ♪]");
  tone(BUZZER_PIN, 1500, 100); delay(150);
  Serial.println(" [♪ BEEP ♪]");
  tone(BUZZER_PIN, 1500, 100); delay(1000);

  // Unlatch Action
  lcd.clear();
  lcd.print("DOOR OPEN");
  doorServo.write(90);
  delay(5000); // Keep door open for bedridden person clearance time

  // Lock Relatch Action
  lcd.clear();
  lcd.print("DOOR LOCKED");
  doorServo.write(0);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  delay(1000);

  displaySecuredMessage();
}

void denyAccess() {
  Serial.println("[🗙] Access DENIED! System Locked Until Manual Reset.");
  
  lcd.clear();
  lcd.print("ALARM TRIGGERED");
  lcd.setCursor(0, 1);
  lcd.print("System Locked");

  // Infinite loop holding the system in Alarm state
  // It will only exit if the Patient Remote/Reset button is pressed!
  while(digitalRead(RF_REMOTE_PIN) == HIGH) { 
    // Alternate siren sound
    digitalWrite(RED_LED, HIGH);
    tone(BUZZER_PIN, 1200); 
    delay(250);

    digitalWrite(RED_LED, LOW);
    tone(BUZZER_PIN, 800);  
    delay(250);
  }

  // Once the button is physically pressed, it breaks the loop and resets
  noTone(BUZZER_PIN);
  digitalWrite(RED_LED, HIGH);
  Serial.println("[➔] Alarm Manually Reset via Button.");
  displaySecuredMessage();
  delay(1000); // Debounce delay
}
void displaySecuredMessage() {
  lcd.clear();
  lcd.print("System Secured");
  lcd.setCursor(0, 1);
  lcd.print("Scan Card");
}
