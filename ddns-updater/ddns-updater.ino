#include <WiFiManager.h>
#include <WebServer.h>
#include <Preferences.h>
#include <EasyDDNS.h>

// defines
#define AP_NAME "ddns_updater"
#define AP_TIMEOUT 300
#define WEBPAGE_BUFFER_SIZE 2000
#define GPIO_BUTTON 0
#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP  600        // update dns every 10min

// non-volatile memory
static Preferences prefs;

// wifi manager
WiFiManager wifiManager;

// webserver
static boolean webserver_active = false;
static WebServer  webserver(80);
static String choice;
static String domain;
static String user;
static String password;

static void httpHandleRoot(void)
{
  static char webpage_buffer[WEBPAGE_BUFFER_SIZE];
  int a;
  boolean args_changed = false;

  for (a = 0; a < webserver.args(); a++)
  {
    if (webserver.argName(a).equals("choice") == true)
    {
      choice = webserver.arg(a);
      prefs.putString("choice", choice);
      args_changed = true;
    }
    if (webserver.argName(a).equals("domain") == true)
    {
      domain = webserver.arg(a);
      prefs.putString("domain", domain);
      args_changed = true;
    }
    if (webserver.argName(a).equals("user") == true)
    {
      user = webserver.arg(a);
      prefs.putString("user", user);
      args_changed = true;
    }
    if (webserver.argName(a).equals("password") == true)
    {
      password = webserver.arg(a);
      prefs.putString("password", password);
      args_changed = true;
    }
  }

  if (args_changed == true)
  {
    Serial.print("Choice: "); Serial.println(choice);
    Serial.print("Domain: "); Serial.println(domain);
    Serial.print("User: "); Serial.println(user);
    Serial.print("Password: "); Serial.println(password);
  }

  snprintf(webpage_buffer, WEBPAGE_BUFFER_SIZE,
      "<html>\
        <body>\
          <h1>DDNS Updater</h1>\
          <form>\
            <table>\
              <tr>\
                <td width=\"100\" valign=\"top\">Choice:</td>\
                <td width=\"400\" valign=\"top\"><input type=\"text\" name=\"choice\" size=\"30\" value=\"%s\"></td>\
              </tr>\
              <tr>\
                <td width=\"100\" valign=\"top\">Domain:</td>\
                <td width=\"400\" valign=\"top\"><input type=\"text\" name=\"domain\" size=\"30\" value=\"%s\"></td>\
              </tr>\
              <tr>\
                <td width=\"100\" valign=\"top\">User:</td>\
                <td width=\"400\" valign=\"top\"><input type=\"text\" name=\"user\" size=\"30\" value=\"%s\"></td>\
              </tr>\
              <tr>\
                <td width=\"100\" valign=\"top\">Password:</td>\
                <td width=\"400\" valign=\"top\"><input type=\"text\" name=\"password\" size=\"30\" value=\"%s\"></td>\
              </tr>\
            </table>\        
            <input type=\"submit\" value=\"Save\">\
          </form>\
          <p>Click the \"Save\" button and the data will be stored in persistent memory.</p>\
        </body>\
      </html>", choice.c_str(), domain.c_str(), user.c_str(), password.c_str());

  webserver.send(200, "text/html", webpage_buffer);
}

void setup(void) 
{    
  Serial.begin(115200);       

  pinMode(GPIO_BUTTON, INPUT);
  Serial.println("Keep IO0-button pressed to start in webserver mode.");
  
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

  // load settings from NVM
  prefs.begin("nvs", false);
  choice = prefs.getString("choice", "");
  Serial.print("Choice: "); Serial.println(choice);
  domain = prefs.getString("domain", "");
  Serial.print("Domain: "); Serial.println(domain);
  user = prefs.getString("user", "");
  Serial.print("User: "); Serial.println(user);
  password = prefs.getString("password", "");
  Serial.print("Password: "); Serial.println(password);

  // check if IO0 Button is pressed
  if (HIGH == digitalRead(GPIO_BUTTON))
  {
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  
    EasyDDNS.service(choice);
    EasyDDNS.client(domain, user, password);

    EasyDDNS.onUpdate([&](const char* oldIP, const char* newIP)
    {
      Serial.printf("WAN IP: %s\n", newIP);  
      Serial.printf("Going to deep sleep now and waking up again in %d seconds.\n", TIME_TO_SLEEP);
      delay(1000);
      Serial.flush();   
      esp_deep_sleep_start();
    });
  }
  else
  {
    webserver_active = true;
    Serial.println("Web server enabled.");
    webserver.on("/", httpHandleRoot);
    webserver.begin();
  }
}

void loop(void) 
{
  if (webserver_active == true)
  {
    webserver.handleClient();
  }
  else
  {
    EasyDDNS.update(0);
  }
}  
