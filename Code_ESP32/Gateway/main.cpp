#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// const char* SSID = "1PHNAD";
// const char* PASS = "Hongbietmk";
// const char* SSID = "Thu Thuy";
// const char* PASS = "0918510237";
const char* SSID = "DoAnTotNghiep";
const char* PASS = "12345678";

// API endpoints (using local machine's IP)
const char* muTapUrl = "https://thanhdat.nbqtai.id.vn/giaodich/mu-tap/";
const char* tscDrcUrl = "https://thanhdat.nbqtai.id.vn/giaodich/tsc-drc/";
const char* muNuocUrl = "https://thanhdat.nbqtai.id.vn/giaodich/mu-nuoc/";

typedef struct {
  char uidStr[20];
  char khoiLuong[20];
  uint8_t loaiCan;
} struct_message;

struct_message incoming;
bool dataReceived = false;

int maxRetries = 3;

#define Led 27
bool ledState = true;      // Trạng thái LED
// Thời gian chớp led
unsigned long previousMillis = 0;
const long interval = 900;  // Khoảng thời gian giữa mỗi lần chớp led(ms)

// Callback khi nhận dữ liệu
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&incoming, incomingData, sizeof(incoming));
  
  dataReceived = true;
  Serial.println("Dữ liệu nhận được:");
  Serial.print("UID: ");
  Serial.println(incoming.uidStr);
  Serial.print("Khối lượng: ");
  Serial.println(incoming.khoiLuong);
  Serial.print("Loại cân: ");
  Serial.println(incoming.loaiCan);
}

void setup() {
  Serial.begin(9600);  // Changed to standard baud rate
  pinMode(Led, OUTPUT);
  digitalWrite(Led, HIGH);

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(SSID, PASS);
  Serial.print("Đang kết nối WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());

  int channel = WiFi.channel();
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  Serial.print("ESP-NOW set kênh: ");
  Serial.println(channel);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init lỗi!");
    while(true) delay(1000);
  }

  esp_now_register_recv_cb(onDataRecv);
  Serial.println("ESP-NOW khởi tạo thành công!");
}

int sendWithRetry(HTTPClient& http, const String& payload, const String& method, int maxRetries = 3) {
  int httpResponseCode = -1;
  int retryCount = 0;

  while (retryCount < maxRetries) {
    if (method == "POST") {
      httpResponseCode = http.POST(payload);
    } else if (method == "PUT") {
      httpResponseCode = http.PUT(payload);
    } else {
      Serial.println("Unsupported HTTP method: " + method);
      return -1;
    }

    if (httpResponseCode > 0) {
      break; // Thành công
    }

    Serial.println("Request failed (code " + String(httpResponseCode) + "), retrying (" + String(retryCount + 1) + "/" + String(maxRetries) + ")");
    retryCount++;
    delay(1000); // delay giữa các lần thử
  }

  return httpResponseCode;
}


void handleResponse(int httpResponseCode, HTTPClient& http) {
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("HTTP Response code: " + String(httpResponseCode));
    Serial.println("Response: " + response);
  } else {
    Serial.println("Error on sending request: " + String(httpResponseCode));
  }
}

void sendHttpRequest() {
  HTTPClient http;
  JsonDocument doc;
  String requestBody, method, url;

  http.addHeader("Content-Type", "application/json");

  if (incoming.loaiCan == 1) {
    url = muTapUrl;
    method = "POST";
    doc["RFID"] = String(incoming.uidStr);
    doc["KhoiLuongMuTap"] = atof(incoming.khoiLuong);

  } else if (incoming.loaiCan == 3) {
    url = tscDrcUrl;
    method = "PUT";
    doc["RFID"] = String(incoming.uidStr);
    doc["TSC"] = atof(incoming.khoiLuong);
    doc["DRC"] = atof(incoming.khoiLuong) * 0.9;

  } else if (incoming.loaiCan == 2) {
    url = muNuocUrl;
    method = "PUT";
    doc["RFID"] = String(incoming.uidStr);
    doc["KhoiLuongMuNuoc"] = atof(incoming.khoiLuong);

  } else {
    Serial.println("Loại cân không hợp lệ: " + String(incoming.loaiCan));
    return;
  }

  serializeJson(doc, requestBody);
  Serial.println("Sending " + method + " to " + url);
  Serial.println("Request body: " + requestBody);

  http.begin(url);
  int httpResponseCode = sendWithRetry(http, requestBody, method);
  handleResponse(httpResponseCode, http);
  http.end();
}


void loop() {
  unsigned long currentMillis = millis();
  if (dataReceived) {
    Serial.println("Processing received data...");
    sendHttpRequest();
    dataReceived = false;
    Serial.println("Waiting for next ESP-NOW message...");
  }
  delay(100); // Small delay to prevent busy waiting

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    ledState = !ledState;
    digitalWrite(Led, ledState);
  }
}