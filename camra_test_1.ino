#include <WiFi.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "DHT.h"
#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <base64.h>
#include "esp_camera.h"

// Replace with your network credentials
const char* ssid = "CELIK2";
const char* password = "CelikCelikAilesi";

// NTP settings
const char* ntpServer = "0.tr.pool.ntp.org";
const int timeZone = 0;

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, timeZone, 0);

// DHT11 sensor
/*
#define DHT_PIN 16
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);


unsigned long lastRefreshTime = 0;
const unsigned long refreshInterval = 60 * 60 * 1000; // 1 hour in milliseconds
*/
void setup() {

  Serial.begin(115200);
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  // Initialize SD card
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;


  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  camera_fb_t * fb = NULL;


  pinMode(13, INPUT_PULLUP); //--> This is done to resolve an "error" in 1-bit mode when SD_MMC.begin("/sdcard", true).

  Serial.println("Start accessing SD Card 1-bit mode");
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD Card Mount Failed");
    return;
  }
  Serial.println("Started accessing SD Card 1-bit mode successfully");

  pinMode(13, INPUT_PULLDOWN);
  //******
  //----------------------------------------

  //----------------------------------------Checking SD card type
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD Card attached");
    return;
  }
  //----------------------------------------

  //----------------------------------------Turning on the LED Flash on the ESP32 Cam Board
  // The line of code to turn on the LED Flash is placed here, because the next line of code is the process of taking photos or pictures by the ESP32 Cam.
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  delay(1000);
  //----------------------------------------
  esp_camera_fb_return(fb);

  // Take Picture with Camera
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  delay(300);

  esp_camera_fb_return(fb);
  //----------------------------------------


  Serial.println("SD card OK");
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());

  // Connect to NTP server
  timeClient.begin();
  timeClient.update();

  // Initialize DHT sensor
  //dht.begin();

  // Start the web server
  server.on("/", handleRoot);
  server.begin();

  Serial.println("Web server started");
  delay(200);

}

void loop() {
  Serial.println("ooo");
  server.handleClient();
  Serial.println("oo");
  // Update time from NTP server
  timeClient.update();
  Serial.println("o");

}
void handleRoot() {
  Serial.println("0");
  camera_fb_t * fb = NULL;
  Serial.println("1");

  // Take Picture with Camera
  fb = esp_camera_fb_get();
  Serial.println("2");
  if (!fb) {
    Serial.println("Camera capture failed, reboot");
    server.send(500, "text/plain", "Camera capture failed");
    ESP.restart();
  }
  Serial.println("3");

  // Generate file name using current time
  String formattedTime = timeClient.getFormattedTime();
  formattedTime.replace(":", "x");
  String fileName = "/I" + formattedTime + ".jpg";
  String fileTxt  =  "/I" + formattedTime + ".txt";

  Serial.println("4");
  File jGG = SD_MMC.open(fileName.c_str(), FILE_WRITE);
  if (!jGG) {
    Serial.println("Failed to open file in writing mode");
    Serial.println(jGG);
    server.send(500, "text/plain", "Failed to save photo");
    esp_camera_fb_return(fb); // Deal with camera_fb_t memory leak
    return;
  }
  Serial.println("5");
  jGG.write(fb->buf, fb->len); // Write photo data to the file
  jGG.close();
  Serial.println("6");
  ////--------

  File jTT = SD_MMC.open(fileTxt.c_str(), FILE_WRITE); // Corrected variable name

  Serial.println("7");
  if (!jTT) {
    Serial.println("Failed to open file in writing mode");
    server.send(500, "text/plain", "Failed to save txt");
    esp_camera_fb_return(fb); // Deal with camera_fb_t memory leak
    return;
  }
  Serial.println("8");
  String encrypt = base64::encode(fb->buf, fb->len);
  jTT.println(encrypt); // Write photo data to the file
  jTT.close();

  ////---------
  Serial.println("9");
  // Read temperature and humidity from DHT sensor
//  float temperature = dht.readTemperature();
//  float humidity = dht.readHumidity();
  Serial.println("10");
  // Generate the HTML response with the image URL and sensor data
  String htmlTemplate = R"(
    <!DOCTYPE html>
    <html>
    <head>
    <title>Cool Website</title>
    <style>
    body {
      background: linear-gradient(to right, #7FC8A9, #A16B47);
    }
    </style>
    </head>
    <body>
        <h2>B64</h2>
        <p></p>
    </body>
    </html>
)";
  Serial.println("11");
  String htmlContent = htmlTemplate; // Corrected variable name
  Serial.println("12");
  String content = jTT.readString(); // Read the content of the file
  Serial.println("12");
  htmlContent.replace("%s", content); // Replace the placeholder with the content
  Serial.println("13");
  server.send(200, "text/html", htmlContent); // No need for String()
  Serial.println("14");
  fb->len = 0; // Reset the length of the camera_fb_t buffer to avoid memory leak
  Serial.println("15");
  esp_camera_fb_return(fb);
  Serial.println("16");
  delay(3000);
  Serial.println("PASSSS!______");
}
