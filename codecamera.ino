#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "FS.h"
#include "SD_MMC.h"

// === Replace with your Wi-Fi and Server IP ===
const char* ssid = "Orange-925A";
const char* password = "JJAH677BTRR";
const char* serverName = "http://192.168.1.124:5000/upload";

// === Take photo every 10 minutes (600000 ms) ===
unsigned long interval = 30000;
unsigned long previousMillis = 0;

// === Camera Configuration ===
void configCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // Init camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x", err);
    return;
  }
}

// === Save image to SD card ===
bool saveImageToSD(camera_fb_t *fb) {
  if (!SD_MMC.begin()) {
    Serial.println("Card Mount Failed");
    return false;
  }

  String path = "/image_" + String(millis()) + ".jpg";
  fs::FS &fs = SD_MMC;
  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
    return false;
  } else {
    file.write(fb->buf, fb->len);
    Serial.printf("Saved file to: %s\n", path.c_str());
  }
  file.close();
  return true;
}

// === Send image via HTTP POST ===
void sendImage(camera_fb_t *fb) {
  WiFiClient client;
  HTTPClient http;

  Serial.println("[INFO] Sending image...");
  http.begin(client, serverName);
  http.addHeader("Content-Type", "image/jpeg");

  int response = http.POST(fb->buf, fb->len);

  if (response > 0) {
    Serial.printf("[SUCCESS] HTTP Response code: %d\n", response);
    String responseBody = http.getString();
    Serial.println("[SERVER]: " + responseBody);
  } else {
    Serial.printf("[ERROR] %s\n", http.errorToString(response).c_str());
  }

  http.end();
}


// === Setup WiFi & Camera ===
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  configCamera();

  if (!SD_MMC.begin()) {
    Serial.println("SD Card Mount Failed");
  }
}

// === Main Loop: every 10 minutes take/send/save ===
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    saveImageToSD(fb);
    sendImage(fb);
    esp_camera_fb_return(fb);
  }
}
