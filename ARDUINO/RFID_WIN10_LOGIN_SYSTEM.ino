#include <AnalogPin.h>
#include <Keyboard.h>
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>
#include <Ultrasonic.h>
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>

#define LED 13

AnalogPin Apin(A0);
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;
MFRC522 mfrc522(SS, 5);
MFRC522::MIFARE_Key key;
SoftwareSerial EEBlue(11, 10);
Ultrasonic ultrasonic(8, 7, 68000UL);

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire, 13, 400000UL, 100000UL);

char str[32] = "";
char card1[32] = "EB1CEFFC";
String readid, password = "";
byte readCard[4];
byte distance, i, lastx, x = 0;
byte isLogged = 1;
long passwordExpireInterval = 3600000;
unsigned long lastPasswordCheck, lastPotChange, start = 0;

void setup() {
   display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
   u8g2_for_adafruit_gfx.begin(display);
   display.clearDisplay();
   display.display();
   pinMode(LED, OUTPUT);
   pinMode(11, INPUT);
   pinMode(10, OUTPUT);
   digitalWrite(LED, HIGH);
   Serial.begin(57600);
   SPI.begin();
   mfrc522.PCD_Init();
   Keyboard.begin();
   EEBlue.begin(9600);
}
void setPassword() {
   if (EEBlue.available() > 0) {
      password = EEBlue.readStringUntil('\n');
   }
}
void resetPassword() {
   if ((millis() - lastPasswordCheck) >= passwordExpireInterval) {
      password = "";
      lastPasswordCheck = millis();
   }
}
void loginCommand() {
   Keyboard.press(KEY_LEFT_CTRL);
   Keyboard.press('a');
   Keyboard.releaseAll();
   delay(400);
   Keyboard.press(KEY_ESC);
   Keyboard.release(KEY_ESC);
   Keyboard.print(password);
   Keyboard.releaseAll();
   delay(100);
   Keyboard.press(KEY_RETURN);
   Keyboard.release(KEY_RETURN);
   isLogged = 1;
   digitalWrite(LED, HIGH);
   message();
}
void winLock() {
   Keyboard.press(KEY_LEFT_GUI);
   Keyboard.press('l');
   Keyboard.releaseAll();
   isLogged = 0;
   digitalWrite(LED, LOW);
   message();
}
void message() {
   display.clearDisplay();
   u8g2_for_adafruit_gfx.setFont(u8g2_font_doomalpha04_tr);
   if (isLogged == 0) {
      u8g2_for_adafruit_gfx.drawStr(0, 32, "LOCKED");
   } else {
      u8g2_for_adafruit_gfx.drawStr(0, 32, "UNLOCKED");
   }
   display.display();
   delay(1000);
}
String arrayToString(byte array[], unsigned int len, char buffer[]) {
   for (unsigned int i = 0; i < len; i++) {
      byte nib1 = (array[i] >> 4) & 0x0F;
      byte nib2 = (array[i] >> 0) & 0x0F;
      buffer[i * 2 + 0] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA;
      buffer[i * 2 + 1] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA;
   }
   buffer[len * 2] = '\0';
   return buffer;
}
void getID() {
   if (!mfrc522.PICC_IsNewCardPresent()) {
      return;
   }
   if (!mfrc522.PICC_ReadCardSerial()) {
      return;
   }
   for (int i = 0; i < mfrc522.uid.size; i++) {
      readCard[i] = mfrc522.uid.uidByte[i];
   }
   mfrc522.PICC_HaltA();
}
void deleteCardInfo() {
   for (int i = 0; i < mfrc522.uid.size; i++) {
      readCard[i] = "";
   }
   readid = "";
}
byte readDistance() {
   return ultrasonic.read(CM);
}

void countDown() {
   if (start == 0 || i == 0) {
      start = millis();
      i = 11;
   }
   long last;
   while (i > 0 && distance > x) {
      last = millis();
      distance = readDistance();
      Apin.setNoiseThreshold(4);
      x = map(Apin.read(), 0, 1023, 30, 200);
      if (last - start >= 1500) {
         display.clearDisplay();
         u8g2_for_adafruit_gfx.setFont(u8g2_font_doomalpha04_tr);
         u8g2_for_adafruit_gfx.drawStr(0, 32, "LOCKING IN");
         u8g2_for_adafruit_gfx.setCursor(70, 32);
         u8g2_for_adafruit_gfx.print(i - 1);
         (i > 10) ? u8g2_for_adafruit_gfx.drawStr(85, 32, "sec") : u8g2_for_adafruit_gfx.drawStr(80, 32, "sec");
         display.display();
         i--;
         if (i == 0) {
            winLock();
            distance = 0;
            start = 0;
         }
         start += 1000;
         last = millis();
      }
   }
   distance = 0;
   start = 0;
   i = 0;
}
void loop() {
   Apin.setNoiseThreshold(5);
   x = map(Apin.read(), 0, 1023, 30, 200);
   distance = readDistance();
   if (distance > x && isLogged == 1) {
      countDown();
   } else {
      if (lastx == x && millis() - lastPotChange > 2000) {
         display.clearDisplay();
         u8g2_for_adafruit_gfx.setFont(u8g2_font_doomalpha04_tr);
         u8g2_for_adafruit_gfx.drawStr(0, 32, "STATUS OK");
      } else if (lastx != x) {
         display.clearDisplay();
         u8g2_for_adafruit_gfx.setFont(u8g2_font_open_iconic_embedded_2x_t);
         u8g2_for_adafruit_gfx.drawGlyph(0, 32, 0x0050);
         u8g2_for_adafruit_gfx.setFont(u8g2_font_doomalpha04_tr);
         u8g2_for_adafruit_gfx.setCursor(30, 31);
         u8g2_for_adafruit_gfx.print(x);
         if (x > 99) {
            u8g2_for_adafruit_gfx.drawStr(55, 32, "CM");
         } else {
            u8g2_for_adafruit_gfx.drawStr(50, 32, "CM");
         }
         lastx = x;
         lastPotChange = millis();
      }
      display.display();
   }
   getID();
   readid = arrayToString(readCard, 4, str);
   setPassword();
   resetPassword();
   if (password != "" && password.length() > 8 && readid == card1 && isLogged == 0) {
      loginCommand();
   } else if (readid == card1 && isLogged == 1) {
      winLock();
   }
   deleteCardInfo();
}
