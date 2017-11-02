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

//static const char *ssl_fingerprint = "4A 77 02 67 01 76 4E BE DE 3C 38 BF 1E 9B F3 65 A7 CB AD 2F 7D ED 21 9E 97 B5 B7 57 AB 31 96 18";
//static const char *ssl_fingerprint = "EF 9D 44 BA 1A 91 4C 42 06 B1 6A 25 71 26 58 61 BA DA FA B9";;
//static const char *ssl_fingerprint = "D6:55:19:B1:D9:BB:07:D6:DC:D9:6C:41:B5:25:26:7E:5B:33:46:64";
static const char *coinrank_base_url = "https://api.coinmarketcap.com/v1/ticker/";

static const char *blockchain_base_url = "http://blockchain.info/ticker";
static const char *ssl_fingerprint = NULL;

typedef struct {
  char *id;
  char *ticker;
  float price_usd;
} coin_info;

#define COIN_COUNT 3

coin_info coins[] {
  { "bitcoin", " btc", 0.0 },
  { "ethereum", " eth", 0.0 },
  { "bitcoin-cash", " bch", 0.0 }
};

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

void wifi_connect(const char *ssid, const char *username, const char *password) {
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
  }
}

const struct WiFiNet *wifi_scan() {
  Serial.printf("scanning wifi... %d known networks, ", sizeof(known_networks) / sizeof(struct WiFiNet));
  const struct WiFiNet * best_ap = NULL;
  int best_signal = -100;

   // WiFi.scanNetworks will return the number of networks found
  int networks = WiFi.scanNetworks();
  if (networks != 0) {
    Serial.printf("%d networks found.\n", networks);

    for (int i = 0; i < networks; ++i) {
      //Serial.printf("%d: %s, Ch:%d (%ddBm) %s\n", i + 1, WiFi.SSID(i).c_str(), 
      //  WiFi.channel(i), WiFi.RSSI(i), 
      //  WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");

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
    Serial.printf("Using [%s, %ddBm]\n", best_ap->ssid, best_signal); 
  } else {
    Serial.println("Did not find acceptable network");
  }
  return best_ap;
}

bool wifi_verify() {
  Serial.print("Verify connectivity... ");
  if (WiFi.status() != WL_CONNECTED) {
    const struct WiFiNet *ap = wifi_scan();
    if (ap != NULL) {
      Serial.print("connecting ");
      wifi_connect(ap->ssid, ap->username, ap->password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");  
      }
    }
  }

  // Now we are connected
  Serial.print(" connected IP=");
  Serial.println(WiFi.localIP());
  return true;
}

String ticker_url(const char *coin_id) {
  // coinmarketcap.com
  //String url = (String(coinrank_base_url) + coin_id) + "/";
  // blockchain.info
  String url = String(blockchain_base_url);
  return url;
}

float price_from_json(String &payload) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);
  if (root.success()) {
    // blockchain.info
    const char *price_usd_string = root["USD"]["last"];
    // coinmarketcap.com
    //const char *price_usd_string = root[0]["price_usd"];
    float price_usd = atof(price_usd_string);
    if (price_usd > 0.0) {
      return price_usd;
    }
  }

  return -1.0;  // Failure
}

void setup_last_price() {
  EEPROM.begin(sizeof(float) * COIN_COUNT);
}

float read_last_price(int index) {
  float last_price = 0.0;
  int address = sizeof(float) * index;
  return EEPROM.get(address, last_price);
}

void write_last_price(int index, float price_usd) {
  int address = sizeof(float) * index;
  EEPROM.put(address, price_usd);
  EEPROM.commit();
}

float get_price_usd(const char *coin_id) {
  float price_usd = -1.0;
  HTTPClient http;

  String url = ticker_url(coin_id);
  Serial.print(url);  

  http.useHTTP10(true);
  http.begin(url);
  int httpCode = http.GET();
  Serial.printf(" ==> %d; ", httpCode);  
  if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      price_usd = price_from_json(payload);
      if (price_usd <= 0.0) {
        Serial.printf("Price parsing error\n");  
      }
  } else {
      Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return price_usd;
}

void show_ticker(const char *ticker) {  
  if (strlen(ticker) >= 4) {
    display7.writeDigitRaw(0, char_encode(ticker[0]));
    display7.writeDigitRaw(1, char_encode(ticker[1]));
    display7.writeDigitRaw(3, char_encode(ticker[2]));
    display7.writeDigitRaw(4, char_encode(ticker[3]));
    display7.writeDisplay();
  }
}

void show_value(float price_usd) {
  display7.print(round(price_usd));
  display7.writeDisplay();
}

void setup(){
  Serial.begin(115200);
  Serial.println("Startup...");

  // Setup the display.
  display7.begin(DISPLAY_ADDRESS);
  display7.setBrightness(5);

  setup_last_price();
  float last_price = read_last_price(0);
  show_value(last_price);
}

void loop() {
  // ensure we are connected. Blocks until we are.
  if (wifi_verify()) {
    int coin_index = 0;
    coin_info *coin = &coins[coin_index];
    show_ticker(coin->ticker);
    float price_usd = get_price_usd(coin->id);
    if (price_usd > 0.0) {    
      coin->price_usd = price_usd;
      show_value(price_usd);

      Serial.print("$");
      Serial.println(price_usd);

      // Save the last thing we got.
      write_last_price(coin_index, price_usd);
    }
  }

  delay(60000);
}
