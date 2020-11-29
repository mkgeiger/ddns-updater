#include <WiFiManager.h>
#include <EasyDDNS.h>

// defines
#define AP_NAME "ddns_updater"
#define AP_TIMEOUT 300
#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP  600        // 10min

// wifi manager
WiFiManager wifiManager;

void setup(void) 
{    
  Serial.begin(115200);       
    
  // set custom ip for portal
  wifiManager.setAPStaticIPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
  wifiManager.setTimeout(AP_TIMEOUT); // Timeout until config portal is turned off
  if (!wifiManager.autoConnect(AP_NAME))
  {
    Serial.printf("Failed to connect and hit timeout\n");
    delay(3000);
    //reset and try again
    ESP.restart();
  }

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  
  EasyDDNS.service("freemyip");
  EasyDDNS.client("domainname.freemyip.com", "token");

  EasyDDNS.onUpdate([&](const char* oldIP, const char* newIP)
  {
    Serial.printf("WAN IP: %s\n", newIP);  
    Serial.println("Going to deep sleep now.");
    delay(1000);
    Serial.flush(); 
    
    esp_deep_sleep_start();
  });
}

void loop(void) 
{
    EasyDDNS.update(0);
}  
