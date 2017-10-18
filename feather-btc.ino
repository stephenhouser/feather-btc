/* Feather BTC - BitCoin USD Display on Adafruit HUZZAH ESP8266 Feather
 *
 */
#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

extern "C" {
  #include "user_interface.h"
  #include "wpa2_enterprise.h"
}

template <class T> int EEPROM_writeAnything(int ee, const T& value) {
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++) {
      if (EEPROM.read(ee) != *p) {
        EEPROM.write(ee, *p);  // Only write the data if it is different to what's there
      }
      ee++;
      p++; 
    }
    EEPROM.commit();
    return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value) {
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++) {
          *p++ = EEPROM.read(ee++);
    }
    return i;
}

struct WiFiNet {
  const char *ssid;
  const char *username;
  const char *password;
};

#define DISPLAY_ADDRESS   0x70
Adafruit_7segment display7 = Adafruit_7segment();
float last_btc = 0.0;

static const uint8_t numbertable[] = {
  0x3F, /* 0 */
  0x06, /* 1 */
  0x5B, /* 2 */
  0x4F, /* 3 */
  0x66, /* 4 */
  0x6D, /* 5 */
  0x7D, /* 6 */
  0x07, /* 7 */
  0x7F, /* 8 */
  0x6F, /* 9 */
};

uint8_t digit_encode(int digit) {
  return numbertable[digit % 10];
}

uint8_t char_encode(char c) {
  switch (c) {
    case '-': return 0x40;
    case '.': return 0x80;
    case 'b': return 0x7C;
    case 'c': return 0x58;
    case 't': return 0x78;
    case ' ':
    default: return 0;
  }
}

uint8_t digits[5];
char dot_animation[] = {B00000, B10000, B01000, B00010, B00001, 
                        B11011, B00001, B00010, B01000, B10000};
int dot_animation_step = 0;

#include "credentials.h"
/* PREDEFINED_NETWORKS is a #define in credentials.h
 * #define PREDEFINED_NETWORKS { {"SSID", "username", "password"}, ...}
 */
const struct WiFiNet known_networks[] = PREDEFINED_NETWORKS;

const struct WiFiNet *find_ssid(const char *ssid) {
  int known_networks_count = sizeof(known_networks) / sizeof(struct WiFiNet);  
  //Serial.printf("%d known networks\n", known_networks_count);
  for (int i = 0; i < known_networks_count; i++) {
    if (!strcmp(known_networks[i].ssid, ssid)) {
      return &known_networks[i];
    }
  }
  
  return NULL;
}

void connect(const char *ssid, const char *username, const char *password) {
  if (username == NULL) {
    WiFi.begin(ssid, password);
  } else {
    // WPA2 configuration
    struct station_config wifi_config;

    wifi_set_opmode(STATION_MODE);
    memset(&wifi_config, 0, sizeof(wifi_config));
  
    strcpy(reinterpret_cast<char*>(wifi_config.ssid), ssid);
    wifi_station_set_config(&wifi_config);
    
    wifi_station_clear_cert_key();
    wifi_station_clear_enterprise_ca_cert();

    wifi_station_set_wpa2_enterprise_auth(1);
    wifi_station_set_enterprise_identity((uint8*)username, strlen(username));
    wifi_station_set_enterprise_username((uint8*)username, strlen(username));
    wifi_station_set_enterprise_password((uint8*)password, strlen(password));

    WiFi.begin();
    //wifi_station_connect();
  }
}

const struct WiFiNet *wifi_scan() {
  Serial.printf("scan start... %d known networks, ", sizeof(known_networks) / sizeof(struct WiFiNet));
  const struct WiFiNet * best_ap = NULL;
  int best_signal = -100;

   // WiFi.scanNetworks will return the number of networks found
  int networks = WiFi.scanNetworks();
  if (networks != 0) {
    Serial.printf("%d networks found.\n", networks);

    for (int i = 0; i < networks; ++i) {
      Serial.printf("%d: %s, Ch:%d (%ddBm) %s\n", i + 1, WiFi.SSID(i).c_str(), 
        WiFi.channel(i), WiFi.RSSI(i), 
        WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");

        if (WiFi.RSSI(i) > best_signal) {
          const struct WiFiNet *net = find_ssid(WiFi.SSID(i).c_str());
          if (net != NULL) {
            best_ap = net;
            best_signal = WiFi.RSSI(i);
          }
        }
    }
  }

  if (best_ap) {
    Serial.printf("Using [%s, %ddBm]", best_ap->ssid, best_signal); 
  } else {
    Serial.println("Did not find acceptable network");
  }
  return best_ap;
}

void setup(){
  Serial.begin(115200);
  Serial.println("Startup...");

  EEPROM.begin(sizeof(float));
  EEPROM_readAnything(0, last_btc);

  // Setup the display.
  display7.begin(DISPLAY_ADDRESS);
  display7.setBrightness(5);
  display7.print((int)last_btc);
  display7.writeDisplay();
}

void loop() {
  Serial.print("B");
  if (WiFi.status() != WL_CONNECTED) {
    const struct WiFiNet *ap = wifi_scan();
    if (ap != NULL) {
      connect(ap->ssid, ap->username, ap->password);
      Serial.print("-");
      while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");  
      }
    }
  }

  // Now we are connected
  Serial.print(" IP=");
  Serial.print(WiFi.localIP());
  Serial.print(" -");
  

  //for (int d = 0; d < 5; d++) {
  //    display7.writeDigitRaw(d, char_encode('-'));
  //}
  display7.writeDigitRaw(0, char_encode(' '));
  display7.writeDigitRaw(1, char_encode('b'));
  display7.writeDigitRaw(3, char_encode('t'));
  display7.writeDigitRaw(4, char_encode('c'));
  display7.writeDisplay();

  HTTPClient http;
  http.useHTTP10(true);
  http.begin("http://blockchain.info/ticker");
  Serial.print("-");
  int httpCode = http.GET();
  Serial.printf("S=%d ", httpCode);
  if(httpCode > 0) {
      //Serial.printf("HTTP GET => %d\n", httpCode);
      if(httpCode == HTTP_CODE_OK) {
          DynamicJsonBuffer jsonBuffer;
          String payload = http.getString();
          //Serial.println(payload);
          //Serial.flush();
          Serial.printf("=%dB", payload.length());
          JsonObject& root = jsonBuffer.parseObject(payload);
          if (!root.success()) {
            Serial.println("JSON parse failed");
          } else {
            Serial.print("-");
            const char *last = root["USD"]["last"];
            Serial.print("-");
            float usd = atof(last);
            Serial.print("-");
            if (usd != 0.0) {
              Serial.print("$");
              last_btc = usd;
              EEPROM_writeAnything(0, last_btc);
              Serial.print("$");              
            }
          }
      }
  } else {
      Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();

  Serial.printf(" BTC=%g ", last_btc);
  display7.print(round(last_btc));
  display7.writeDisplay();
  Serial.println("E");

  delay(60000);
}
