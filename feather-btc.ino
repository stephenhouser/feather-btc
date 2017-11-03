/* Feather BTC - BitCoin USD Display on Adafruit HUZZAH ESP8266 Feather
 *
 */
//#define DEBUG_SSL
//#define DEBUGV

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

extern bool wifi_verify();  

#define TICKER_COINMARKETCAP
//static const char *ssl_fingerprint = "4A 77 02 67 01 76 4E BE DE 3C 38 BF 1E 9B F3 65 A7 CB AD 2F 7D ED 21 9E 97 B5 B7 57 AB 31 96 18";
//static const char *ssl_fingerprint = "D6:55:19:B1:D9:BB:07:D6:DC:D9:6C:41:B5:25:26:7E:5B:33:46:64";
#if defined(TICKER_COINMARKETCAP)
static const char *base_url = "https://api.coinmarketcap.com/v1/ticker/";
static const char *ssl_fingerprint = "FC 73 E0 2A 02 8F BD 49 8B E8 59 5E 00 B5 30 BF C8 79 5C E7";;
#else
static const char *base_url = "http://blockchain.info/ticker";
#endif

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

static const uint8_t alphatable[] = {
  0x77, /* A */
  0x7c, /* b */
  0x58, /* c */
  0x5e, /* d */
  0x79, /* E */
  0x71, /* F */
  0x3D, /* G */
  0x74, /* h  */
  0x10, /* i */
  0x0e, /* j */
  0x70, /* k - avoid */
  0x38, /* L */
  0x37, /* m - avoid */
  0x54, /* n */
  0x5c, /* o */
  0x73, /* P */
  0x67, /* q =>like 9 */
  0x50, /* r */
  0x6d, /* S =>like 5 */
  0x78, /* t */
  0x3e, /* U */
  0x62, /* v => like a superscript u. */
  0x7e, /* w - avoid */
  0x76, /* x => avoid, like H */
  0x6e, /* y */
  0x5b /* z => like 2 */
};

uint8_t digit_encode(int digit) {
  return numbertable[digit % 10];
}

uint8_t char_encode(char c) {
  if (isdigit(c)) {
    return numbertable[c - '0'];
  } 
  
  if (isalpha(c)) {
    return alphatable[tolower(c) - 'a'];
  }
  
  switch (c) {
    case '-': return 0x40;
    case '.': return 0x80;
    case ' ': return 0x00;
  }

  return 0x00;
}

uint8_t digits[5];
char dot_animation[] = {B00000, B10000, B01000, B00010, B00001, 
                        B11011, B00001, B00010, B01000, B10000};
int dot_animation_step = 0;

String ticker_url(const char *coin_id) {
  #if defined(TICKER_COINMARKETCAP)
    // coinmarketcap.com
    String url = (String(base_url) + coin_id) + "/";
  #else
    // blockchain.info
    String url = String(base_url);
  #endif
  return url;
}

float price_from_blockchan_json(String &payload) {
  const size_t bufferSize = 22*JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(22) + 1660;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(payload);
  if (root.success()) {
    // blockchain.info
    const char *price_usd_string = root["USD"]["last"];
    float price_usd = atof(price_usd_string);
    if (price_usd > 0.0) {
      return price_usd;
    }
  }

return -1.0;  // Failure
}

float price_from_coinmarketcap_json(String &payload) {
  const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(14) + 310;  
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonArray& root = jsonBuffer.parseArray(payload);
  if (root.success()) {
    // coinmarketcap.com
    const char *price_usd_string = root[0]["price_usd"];
    float price_usd = atof(price_usd_string);
    if (price_usd > 0.0) {
      return price_usd;
    }
  }

return -1.0;  // Failure
}

float price_from_json(String &payload) {
  #if defined(TICKER_COINMARKETCAP)
    return price_from_coinmarketcap_json(payload);
  #else
  return price_from_blockchain_json(payload);
  #endif
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
  #if defined(TICKER_COINMARKETCAP)
    http.begin(url, ssl_fingerprint);
  #else
    http.begin(url, ssl_fingerprint);
  #endif
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
  //Serial.setDebugOutput(true);
  Serial.println("Startup...");

  // Setup the display.
  display7.begin(DISPLAY_ADDRESS);
  display7.setBrightness(5);

  setup_last_price();
  float last_price = read_last_price(0);
  show_value(last_price);
}

int coin_index = 0;

void loop() {
  // ensure we are connected. Blocks until we are.
  if (wifi_verify()) {
    coin_info *coin = &coins[coin_index];
    show_ticker(coin->ticker);
    float price_usd = get_price_usd(coin->id);
    if (price_usd > 0.0) {    
      coin->price_usd = price_usd;
      show_value(price_usd);

      Serial.print("$");
      Serial.println(price_usd);

      // Save the last thing we got.
      //write_last_price(coin_index, price_usd);
      // Increment coin index to show next coin's current price
      coin_index = ++coin_index % COIN_COUNT;
    }
  }

  delay(60000 / COIN_COUNT);
}
