#include <Arduino.h>
#include <ESP8266WiFi.h>

extern "C" {
	#include "user_interface.h"
	#include "wpa2_enterprise.h"
  }
    
struct WiFiNet {
	const char *ssid;
	const char *username;
	const char *password;
  };
  
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

  //configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  //Serial.println("done.");
  return true;
}
