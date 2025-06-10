#include <MFRC522.h>
#include <Adafruit_ST7735.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

// WiFi cấu hình
// constexpr char WIFI_SSID[] = "1PHNAD";
// constexpr char WIFI_SSID[] = "Thu Thuy"; 
constexpr char WIFI_SSID[] = "DoAnTotNghiep"; 

uint8_t slaveAddress[] = { 0x24, 0x6F, 0x28, 0xBE, 0x2A, 0x7C };

// MFRC522 cấu hình
#define RST_PIN 21
#define SS_PIN 15
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
const int blockName = 2;
String tenKH = "";

// TFT cấu hình
#define TFT_CS 5
#define TFT_RST 13
#define TFT_DC 25
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

// UART RS232
#define RXD2 16
#define TXD2 17

#define buzzer 27

// Thời gian hiển thị
const unsigned long clearDisplayTime = 20000;
unsigned long displayTime = 0;

// Cấu trúc dữ liệu truyền đi
typedef struct {
  char uidStr[20];
  char canTieuly[20];
  uint8_t loaiCan = 3;
} struct_message;

struct_message Data;

float canLan1, canLan2;
bool guiThanhCong = false;

// Callback gửi ESP-NOW
void OnSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
    guiThanhCong = (status == ESP_NOW_SEND_SUCCESS);
    Serial.printf("Gửi dữ liệu: %s\n", guiThanhCong ? "Thành công" : "Thất bại");
}

// Hàm quét WiFi để tìm đúng channel
int32_t getWiFiChannel(const char* ssid) {
  int n = WiFi.scanNetworks();
  if (n <= 0) return 0;
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == ssid) return WiFi.channel(i);
  }
  return 0;
}

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  int channel = getWiFiChannel(WIFI_SSID);
  if (channel <= 0) {
    Serial.println("Không tìm thấy SSID!");
    while (true) delay(1000);
  }
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init thất bại");
    while (true) delay(1000);
  }
  esp_now_register_send_cb(OnSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, slaveAddress, 6);
  peerInfo.channel = channel;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Thêm peer thất bại");
    while (true) delay(1000);
  }

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.fillScreen(ST77XX_BLACK);

  SPI.begin();
  mfrc522.PCD_Init();
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}

void docCan();
float docMotLanCan();
void hien_thi();
String readBlock(int blockNumber);

void Buzzer(){
  digitalWrite(buzzer, HIGH);
  delay(200);
  digitalWrite(buzzer, LOW);
}

void loop() {
  // Reset dữ liệu
  memset(&Data, 0, sizeof(Data));
  Data.loaiCan = 3;
  tenKH = "";
  canLan1 = 0;
  canLan2 = 0;
  guiThanhCong = false;
  int soLanThu = 0;
  int soLanThuToiDa = 5; 

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Buzzer();
    // Lấy UID
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    strncpy(Data.uidStr, uid.c_str(), sizeof(Data.uidStr) - 1);

    // Đọc tên khách hàng từ block
    tenKH = readBlock(blockName);
    hien_thi();
    
    // Dừng thẻ
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    // Đọc dữ liệu từ cân
    docCan();

    // Hiển thị lên màn hình
    displayTime = millis();
    hien_thi();

   // Gửi dữ liệu
    while (!guiThanhCong && soLanThu < soLanThuToiDa) {
      esp_err_t res = esp_now_send(slaveAddress, (uint8_t*)&Data, sizeof(Data));
      if (guiThanhCong) {
        Serial.println("Gửi thành công");
        break;
      } else {
        Serial.println("Gửi thất bại, thử lại...");
        delay(100); // Đợi một chút trước khi thử lại
        soLanThu++;
      }
    }
    if (!guiThanhCong) {    
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextSize(2);  
      tft.setCursor(2, 65);
      tft.setTextColor(ST77XX_RED);
      tft.print(F("ESP_NOW !"));   
    }
  }

  // Xoá màn hình sau thời gian
  if (displayTime && millis() - displayTime > clearDisplayTime) {
    tft.fillScreen(ST77XX_BLACK);
    displayTime = 0;
  }
}

void docCan() {
  Serial.println("Đang chờ cân lần 1...");
  canLan1 = docMotLanCan();
  Serial.print("Cân lần 1: "); Serial.println(canLan1);
  hien_thi();
  
  Serial.println("Đang chờ cân lần 2...");
  canLan2 = docMotLanCan();
  Serial.print("Cân lần 2: "); Serial.println(canLan2);

  // Tính chênh lệch
  float hieu = canLan2 / canLan1;
  String chenhLech = String(hieu, 2);
  strncpy(Data.canTieuly, chenhLech.c_str(), sizeof(Data.canTieuly) - 1);

  Serial.print("Hàm lượng: ");
  Serial.println(Data.canTieuly);
}

float docMotLanCan() {
  while (true) {
    if (Serial2.available()) {
      String raw = Serial2.readStringUntil('\n');
      raw.trim();

      if (raw.length() > 0) {
        int startIdx = -1;
        for (int i = 0; i < raw.length(); i++) {
          if (isDigit(raw[i]) || raw[i] == '-') {
            startIdx = i;
            break;
          }
        }

        if (startIdx != -1) {
          String numericPart = raw.substring(startIdx);

          String cleanWeight = "";
          for (int i = 0; i < numericPart.length(); i++) {
            char c = numericPart[i];
            if (isDigit(c) || c == '.' || c == '-') {
              cleanWeight += c;
            } else {
              break;
            }
          }

          return cleanWeight.toFloat();
        }
      }
    }
    delay(10);
  }
}

void hien_thi() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);

  tft.drawRect(0, 0, 160, 26, ST77XX_CYAN);
  tft.setCursor(2, 5);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(tenKH);

  tft.drawRect(0, 30, 160, 26, ST77XX_CYAN);
  tft.setCursor(1, 35);
  tft.setTextColor(ST77XX_YELLOW);
  tft.print(F("ID: "));
  tft.setTextColor(ST77XX_WHITE);
  tft.print(Data.uidStr);

  tft.drawRect(0, 60, 160, 65, ST77XX_CYAN);
  tft.setCursor(2, 63);
  tft.setTextColor(ST77XX_YELLOW);
  tft.print(F("KL1: "));
  tft.setTextColor(ST77XX_WHITE);
  tft.print(canLan1);
  tft.print(F(" g"));

  tft.setCursor(2, 83);
  tft.setTextColor(ST77XX_YELLOW);
  tft.print(F("KL2: "));
  tft.setTextColor(ST77XX_WHITE);
  tft.print(canLan2);
  tft.print(F(" g"));

  tft.setCursor(2, 103);
  tft.setTextColor(ST77XX_ORANGE);
  tft.print(F("TSC: "));  
  tft.print(Data.canTieuly);  
}

String readBlock(int blockNumber) {
  byte buffer[18];
  byte bufferSize = sizeof(buffer);

  int trailerBlock = (blockNumber / 4) * 4 + 3;

  byte status = mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A,
    trailerBlock,
    &key,
    &(mfrc522.uid)
  );
  if (status != MFRC522::STATUS_OK) {
    Serial.println("Xác thực thẻ thất bại.");
    return "";
  }

  status = mfrc522.MIFARE_Read(blockNumber, buffer, &bufferSize);
  if (status != MFRC522::STATUS_OK) {
    Serial.println("Đọc block thất bại.");
    return "";
  }

  String result = "";
  for (int i = 0; i < 16; i++) {
    if (buffer[i] == '\0') break;
    result += (char)buffer[i];
  }

  Serial.println("Đọc block thành công.");
  return result;
}
