#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

const char* ssid = "oplus_co_apqcxe";
const char* password = "0929713756";

LiquidCrystal_I2C lcd(0x27, 16, 2);
int smokeSensorPin = A0;
int buzzerPin = 0;
int led1Pin = 2;
int led2Pin = 14;

// ตัวแปรสำหรับควบคุม buzzer
unsigned long previousMillisBuzzer = 0;
const long buzzerInterval = 200;  // ปรับเป็น 0.2 วินาทีตามที่ต้องการ
bool buzzerState = HIGH;  // เพิ่มตัวแปรเก็บสถานะ buzzer

// ตัวแปรสำหรับควบคุมการส่ง Telegram
unsigned long previousMillisTelegram = 0;
const long telegramInterval = 30000;

const char* telegramBotToken = "7893293432:AAG8IaryNTHPf7XPNHtYsLKFWUtZk6NzWc8";
const String chatID = "-1002255245792";

WiFiClientSecure client;
UniversalTelegramBot bot(telegramBotToken, client);

bool wifiConnected = false;
bool alertSent = false;
bool isAlertActive = false;  // เพิ่มตัวแปรเก็บสถานะการแจ้งเตือน

void sendTelegramMessage(String message) {
  if (wifiConnected && client.connect("api.telegram.org", 443)) {
    if (!bot.sendMessage(chatID, message, "")) {
      Serial.println("ส่งข้อความไป Telegram ล้มเหลว!");
    } else {
      Serial.println("ส่งข้อความไป Telegram สำเร็จ!");
    }
    client.stop();
  } else {
    Serial.println("ข้ามการส่งข้อความ (ไม่มีอินเทอร์เน็ต)");
  }
}

void connectWiFi() {
  Serial.print("กำลังเชื่อมต่อ WiFi...");
  WiFi.begin(ssid, password);

  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 20) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(retryCount % 16, 1);
    lcd.print(".");
    retryCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi เชื่อมต่อเรียบร้อย!");
    wifiConnected = true;
    digitalWrite(led1Pin, LOW);
    digitalWrite(led2Pin, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected ");
    lcd.setCursor(0, 1);
    lcd.print("Ready to Work  ");

    for (int i = 0; i < 3; i++) {
      digitalWrite(buzzerPin, LOW);
      delay(200);
      digitalWrite(buzzerPin, HIGH);
      delay(200);
    }
  } else {
    Serial.println("\nWiFi ไม่สามารถเชื่อมต่อได้");
    wifiConnected = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed   ");
  }
  delay(2000);
  lcd.clear();
}

void setup() {
  lcd.begin();
  lcd.backlight();
  pinMode(buzzerPin, OUTPUT);
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);  // ปิด buzzer เริ่มต้น (HIGH = ปิด)
  digitalWrite(led1Pin, HIGH);
  digitalWrite(led2Pin, LOW);
  Serial.begin(115200);

  lcd.setCursor(0, 0);
  lcd.print(" SYSTEM LOADING ");
  for (int i = 0; i < 16; i++) {
    lcd.setCursor(i, 1);
    lcd.print(".");
    delay(250);
  }
  delay(500);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(" SYSTEM STARTED ");
  delay(2000);
  lcd.clear();

  connectWiFi();

  if (wifiConnected) {
    client.setInsecure();
    Serial.println("Client secure connection ready.");
  }
}

void loop() {
  unsigned long currentMillis = millis();
  int smokeValue = analogRead(smokeSensorPin);

  // แสดงค่าควันบนจอ LCD ตามปกติเมื่อไม่มีการแจ้งเตือน
  if (!isAlertActive) {
    lcd.setCursor(0, 0);
    lcd.print("Smoke and Gas    ");
    lcd.setCursor(0, 1);
    lcd.print("Level:  ");
    lcd.print(smokeValue);
    lcd.print("    ");
  }
  
  Serial.println(smokeValue);

  // ตรวจสอบการเชื่อมต่อ WiFi
  if (WiFi.status() != WL_CONNECTED && wifiConnected) {
    wifiConnected = false;
    digitalWrite(led1Pin, HIGH);
    digitalWrite(led2Pin, LOW);
  } else if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
    wifiConnected = true;
    digitalWrite(led1Pin, LOW);
    digitalWrite(led2Pin, HIGH);
  }

  // หากไม่มีการเชื่อมต่อ WiFi ให้พยายามเชื่อมต่อใหม่
  if (!wifiConnected) {
    lcd.setCursor(0, 0);
    lcd.print("WiFi Disconnected");
    lcd.setCursor(0, 1);
    lcd.print("Retrying...");
    connectWiFi();
  }

  // ตรวจสอบระดับแก๊ส
  if (smokeValue > 800) {
    // เริ่มสถานะแจ้งเตือน
    isAlertActive = true;
    
    // แสดงข้อความแจ้งเตือนบนจอ LCD
    lcd.setCursor(0, 0);
    lcd.print(" ** WARNING! ** ");
    lcd.setCursor(0, 1);
    lcd.print("Gas Level High! ");

    // ควบคุม buzzer ให้ดังเป็นจังหวะทุก 0.2 วินาที
    if (currentMillis - previousMillisBuzzer >= buzzerInterval) {
      previousMillisBuzzer = currentMillis;
      buzzerState = !buzzerState;  // สลับสถานะ buzzer
      digitalWrite(buzzerPin, buzzerState);
    }

    // ส่งข้อความแจ้งเตือนทาง Telegram ทุกๆ 30 วินาที
    if (!alertSent || (currentMillis - previousMillisTelegram >= telegramInterval)) {
      previousMillisTelegram = currentMillis;
      sendTelegramMessage("⚠️ พบควัน/แก๊สรั่วไหล กรุณาตรวจสอบด่วน!");
      alertSent = true;
    }
  } else {
    // จบสถานะแจ้งเตือน
    if (isAlertActive) {
      isAlertActive = false;
      digitalWrite(buzzerPin, HIGH);  // ปิด buzzer
      alertSent = false;
    }
  }

  delay(50);  // ลดเวลา delay เพื่อให้การตอบสนองเร็วขึ้น
}