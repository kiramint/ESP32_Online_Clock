#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <string>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

using namespace std;

String buffer;
String weatherNow;
bool globalStatus = true;
bool ledStatus = true; // onboard led running status

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WiFiUDP ntpUdp;
NTPClient ntp(ntpUdp, "ntp.tencent.com", 8 * 3600, 60000);

String weatherApi(int location)
{
    // API:http://restapi.amap.com/v3/weather/weatherInfo?key=a61964f43fcf2d7cb4a902f284db2811&extensions=base&output=JSON&city=410200
    string url = "http://restapi.amap.com/v3/weather/weatherInfo?key=a61964f43fcf2d7cb4a902f284db2811&extensions=base&output=JSON&city=" + to_string(location);
    Serial.print("Weather API:");
    Serial.println(url.c_str());
    HTTPClient client;
    client.begin(url.c_str());
    int httpResult = 0;
    httpResult = client.GET();
    String payload;
    if (httpResult == HTTP_CODE_OK) {
        payload = client.getString();
        Serial.println("\n================================GET================================");
        Serial.print(payload);
        Serial.println("\n================================GET================================");
    }
    else {
        Serial.println("\n================================FAILD================================");
    }
    String testData = "{\"status\":\"1\",\"count\":\"1\",\"info\":\"OK\",\"infocode\":\"10000\",\"lives\":[{\"province\":\"河南\",\"city\":\"开封市\",\"adcode\":\"410200\",\"weather\":\"阴\",\"temperature\":\"19\",\"winddirection\":\"南\",\"windpower\":\"≤3\",\"humidity\":\"81\",\"reporttime\":\"2023-06-04 09:30:28\",\"temperature_float\":\"19.0\",\"humidity_float\":\"81.0\"}]}";
    DynamicJsonDocument weatherRaw(4096);
    deserializeJson(weatherRaw, payload);
    JsonObject weather = weatherRaw["lives"][0];
    String temperature = weather["temperature"];
    String humidity = weather["humidity"];
    String weatherOutput = "Weather: " + temperature + "C " + humidity + "%";
    Serial.println(weatherOutput);
    client.end();
    return weatherOutput;
}

void setup()
{
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);

    Serial.begin(115200);
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.write("Screen init faild!\n");
        globalStatus = false;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.cp437(true);
    display.setTextSize(2);
    display.write("Booting...");
    display.display();
    Serial.println("Booting...");

    WiFi.begin("Kira Dandelion", "sakirawifi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(200);
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.write("Wifi init faild!\n");
        globalStatus = false;
    }
    else {
        Serial.write("Wifi Connected!\n");
    }
    if (globalStatus) {
        delay(3000); // wait for wifi connection
        ntp.begin();
        ntp.update();
        weatherNow = weatherApi(410200);
    }
}

void loop()
{
    int now = ntp.getSeconds();
    int y;
    int Lheight = 10;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.cp437(true);

    display.setTextSize(1); // Status
    y = 0;
    display.setCursor(12, y);
    if (WiFi.status() != WL_CONNECTED) {
        display.write("Wifi disconnected!\n");
    }
    else {
        display.write("Network Connected!\n");
    }

    display.setTextSize(2); // Time
    buffer = ntp.getFormattedTime();
    y += Lheight + 2;
    display.setCursor(14, y);
    display.write(buffer.c_str());
    display.setTextSize(1);

    y += 2 * Lheight;
    display.setCursor(16, y);
    display.write(weatherNow.c_str()); // Weather

    display.setTextSize(1); // IP
    String ipaddr = WiFi.localIP().toString();
    ipaddr = "IP:" + ipaddr;
    y += Lheight;
    display.setCursor(12, y);
    display.write(ipaddr.c_str());

    y += Lheight;
    display.setCursor(30, y);
    display.write("@Kira Mint~\n"); // Kirara

    // Refresh
    bool ifUpdate = false;
    while (true) {
        if (ntp.getSeconds() == 0 && ntp.getMinutes() == 0 && !ifUpdate) { // refresh per hour
            ntp.update();
            weatherNow = weatherApi(410200);
            Serial.println("\nData Updated!");
            ifUpdate = true;
        }
        if (ntp.getSeconds() != now) {
            display.display();
            if (ledStatus) {
                digitalWrite(2, HIGH);
                ledStatus = false;
            }
            else {
                digitalWrite(2, LOW);
                ledStatus = true;
            }
            break;
        }
    }
}
