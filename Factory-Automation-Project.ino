#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <AutoConnect.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Arduino.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <LittleFS.h>

ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server;
AutoConnect portal(server);
AutoConnectConfig config;

String MacID = "", KonumID = "0";
String param1 = "", param2 = "", SoapValue = "";
String SoapAction = "İLGİLİ BİLGİLER";
String SoapAction2 = "İLGİLİ BİLGİLER";
String SoapAction3 = "İLGİLİ BİLGİLER";
String path = "webservices/procostsoap.asmx";
String xmlStartTagDivider = "İLGİLİ BİLGİLER", xmlFinishTagDivider = "İLGİLİ BİLGİLER";
String xmlStartTagDivider2 = "İLGİLİ BİLGİLER", xmlFinishTagDivider2 = "İLGİLİ BİLGİLER";
String xmlStartTagDivider3 = "<LoginResult>", "İLGİLİ BİLGİLER" "</LoginResult>";
String BaglantiBilgisi = "";
String UzantiBilgisi = "";
String periodBilgisi = "";
String token = "";            // Gelen token'ı burada saklayacağız
String usernameBilgisi = "";  // Kullanıcı adı
String passwordBilgisi = "";  // Şifre
unsigned long lastTokenCheckTime = 0;
const unsigned long tokenCheckInterval = 2 * 60 * 1000;  // 4 saat milisaniye cinsinden
bool tokenCheckedOnce = false;                           // İlk token kontrolü yapıldı mı?

float SicaklikVal = 0, OncekiSicaklikVal = 0, NemVal = 0;
String paramSicaklik = "", paramNem = "";
int CountDelay, Miktar = 0, Fire = 0;

const int ButonArttir = D4;
const int ButonFireArttir = D5;
volatile unsigned long lastDebounceTimeArttir = 0;
volatile unsigned long lastDebounceTimeFireArttir = 0;
volatile bool MiktarChanged = false;
volatile bool FireChanged = false;
unsigned long lastTempHumiditySendTime = 0;   // Son gönderim zamanı
unsigned long sendInterval = 30 * 60 * 1000;  // 30 dakika milisaniye cinsinden
unsigned long lastSendTime = 0;               // Global olarak tanımla
unsigned long lastMiktarSendTime = 0;         // Miktar gönderim zamanı
unsigned long lastFireSendTime = 0;           // Fire gönderim zamanı

int buttonPressCountMiktar = 0;  // Miktar butonuna basılma sayısı
int buttonPressCountFire = 0;    // Fire butonuna basılma sayısı

int totalMiktar = 0;  // 30 dakika boyunca biriken Miktar
int totalFire = 0;    // 30 dakika boyunca biriken Fire

float totalTemperature = 0;     // Sıcaklık toplamı
bool tempSent = false;          // Sıcaklık verisinin gönderilip gönderilmediği
float lastTemperatureSent = 0;  // Son gönderilen sıcaklık değeri

bool check = false;

#define DHTPIN D7
#define DHTTYPE DHT11
DHT_Unified dht(DHTPIN, DHTTYPE);

float RoundToHalf(float x) {
  return 0.5 * round(2.0 * x);
}

const char AUX_EEPROM_IO[] PROGMEM = R"(
[
  {
    "title": "EEPROM",
    "uri": "/",
    "menu": true,
    "element": [
      {
        "name": "Username",
        "type": "ACInput",
        "label": "Username",
        "global": true
      },
      {
        "name": "Password",
        "type": "ACInput",
        "label": "Password",
        "global": true
      },
      {
        "name": "Baglanti",
        "type": "ACInput",
        "label": "Baglanti",
        "global": true
      },
      {
        "name": "Uzanti",
        "type": "ACInput",
        "label": "Uzanti",
        "global": true
      },
      {
        "name": "period",
        "type": "ACRadio",
        "label": "Gönderim Seçeneği",
        "value": [
           "1",
           "2"
        ],
        "global": true
      },
      {
        "name": "write",
        "type": "ACSubmit",
        "value": "WRITE",
        "uri": "/eeprom"
      }
    ]
  },
 {
    "title": "EEPROM",
    "uri": "/eeprom",
    "menu": false,
    "element": [
      {
        "name": "Username",
        "type": "ACText",
        "format": "Username: %s",
        "posterior": "br",
        "global": true
      },
      {
        "name": "Password",
        "type": "ACText",
        "format": "Password: %s",
        "posterior": "br",
        "global": true
      },
      {
        "name": "Baglanti",
        "type": "ACText",
        "format": "Baglanti: %s",
        "posterior": "br",
        "global": true
      },
      {
        "name": "Uzanti",
        "type": "ACText",
        "format": "Uzanti: %s",
        "posterior": "br",
        "global": true
      },
      {
        "name": "period",
        "type": "ACText",
        "format": "Gönderim Seçeneği: %s",
        "posterior": "br",
        "global": true
      }
    ]
  }
]
)";


void saveUserConfigToFS(const String& username, const String& password, const String& Baglanti, const String& Uzanti, const String& period) {
  File file = LittleFS.open("/config.txt", "w");
  if (!file) {
    Serial.println("Konfigürasyon dosyası açılamadı!");
    return;
  }
  file.println(username);
  file.println(password);
  file.println(Baglanti);
  file.println(Uzanti);
  file.println(period);

  file.close();
  Serial.println("Veriler LittleFS'ye kaydedildi.");
}

bool loadUserConfigFromFS(String& username, String& password, String& Baglanti, String& Uzanti, String& period) {
  File file = LittleFS.open("/config.txt", "r");
  if (!file) {
    Serial.println("Konfigürasyon dosyası açılamadı!");
    return false;
  }

  // Verilerin doğru şekilde okunup okunmadığını kontrol edin
  username = file.readStringUntil('\n');
  password = file.readStringUntil('\n');
  Baglanti = file.readStringUntil('\n');
  Uzanti = file.readStringUntil('\n');
  periodBilgisi = file.readStringUntil('\n');

  username.trim();
  password.trim();
  Baglanti.trim();
  Uzanti.trim();
  periodBilgisi.trim();

  file.close();
  return true;
}

String onEEPROM(AutoConnectAux& page, PageArgument& args) {
  String username, password, Baglanti, Uzanti, period;

  // Verileri LittleFS'den okuma
  if (!loadUserConfigFromFS(username, password, Baglanti, Uzanti, period)) {
    page["Username"].value = "";  // LittleFS dosyası yoksa boş bırak
    page["Password"].value = "";
    page["Baglanti"].value = "";
    page["Uzanti"].value = "";
    page["period"].value = "";  // Varsayılan değer
  } else {
    page["Username"].value = username;
    page["Password"].value = password;
    page["Baglanti"].value = Baglanti;
    page["Uzanti"].value = Uzanti;
    page["period"].value = period;
  }

  return String();
}

String onEEPROMWrite(AutoConnectAux& page, PageArgument& args) {
  String username = page["Username"].value;
  String password = page["Password"].value;
  String Baglanti = page["Baglanti"].value;
  String Uzanti = page["Uzanti"].value;
  String period = page["period"].value;

  // Eğer değer geçerliyse kaydedin
  saveUserConfigToFS(username, password, Baglanti, Uzanti, period);

  Serial.println("LittleFS'ye yazılan veriler:");
  Serial.println("Username: " + username);
  Serial.println("Password: " + password);
  Serial.println("Baglanti: " + Baglanti);
  Serial.println("Uzanti: " + Uzanti);


  usernameBilgisi = username;
  passwordBilgisi = password;
  BaglantiBilgisi = Baglanti;
  UzantiBilgisi = Uzanti;
  return String();
}

void saveWiFiCredentials(String ssid, String wifi_password) {
  File file = LittleFS.open("/wifi_config.txt", "w");
  if (!file) {
    Serial.println("WiFi konfigürasyon dosyası açılamadı!");
    return;
  }
  file.println(ssid);
  file.println(wifi_password);
  file.close();
  Serial.println("WiFi bilgileri LittleFS'ye kaydedildi.");
}

void loadWiFiCredentials(char* ssid, char* wifi_password) {
  File file = LittleFS.open("/wifi_config.txt", "r");
  if (!file) {
    Serial.println("WiFi konfigürasyon dosyası açılamadı!");
    return;
  }
  String ssidStr = file.readStringUntil('\n');
  String wifi_passwordStr = file.readStringUntil('\n');

  ssidStr.trim();
  wifi_passwordStr.trim();

  ssidStr.toCharArray(ssid, 32);
  wifi_passwordStr.toCharArray(wifi_password, 32);

  file.close();
  Serial.println("WiFi bilgileri LittleFS'den yüklendi.");
}

volatile bool buttonPressedArttir = false;
volatile bool buttonPressedFireArttir = false;

void IRAM_ATTR handleButtonPressArttir() {
  unsigned long currentTime = millis();
  if (currentTime - lastDebounceTimeArttir > 5000) {
    Miktar++;
    MiktarChanged = true;
    Serial.print("Butona basılma sayısı: ");
    Serial.println(Miktar);
    lastDebounceTimeArttir = currentTime;
  }
}

void IRAM_ATTR handleButtonPressFireArttir() {
  unsigned long currentTime2 = millis();
  if (currentTime2 - lastDebounceTimeArttir > 5000) {
    Fire++;
    FireChanged = true;
    Serial.print("Fire Butona basılma sayısı: ");
    Serial.println(Fire);
    lastDebounceTimeArttir = currentTime2;
  }
}

void sendSOAPRequest(int Miktar, int Fire) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    String SoapGPIO = "İLGİLİ BİLGİLER"

    if (client.connect(BaglantiBilgisi, 80)) {
      Serial.println("Bağlantı başarılı, veri gönderiliyor...");
      client.println("POST " + UzantiBilgisi + path + " HTTP/1.1");
      client.print("Host: ");
      client.println(BaglantiBilgisi);
      client.println("Content-Type: text/xml; charset=utf-8");
      client.print("Content-Length: ");
      client.println(SoapGPIO.length());
      client.print("SOAPAction: ");
      client.println(SoapAction2);
      client.println("");
      client.println(SoapGPIO);
      client.println("");

      while (client.connected()) {
        if (client.available()) {
          String responseLine = client.readStringUntil('\n');
          Serial.println("HTTP Durum Kodu: " + responseLine);

          String resp = client.readString();
          int firstDividerIndex = resp.indexOf(xmlStartTagDivider2) + xmlStartTagDivider2.length();
          int secondDividerIndex = resp.indexOf(xmlFinishTagDivider2);
          if (firstDividerIndex > -1 && secondDividerIndex > -1) {
            String lastStr = resp.substring(firstDividerIndex, secondDividerIndex);
            Serial.println("Servis Yaniti: " + lastStr);
          } else {
            Serial.println("Yanıt içeriği bulunamadı veya hatalı");
          }
          break;
        }
        yield();
      }
      client.stop();
    } else {
      Serial.println("Bağlantı başarısız!!!");
    }
  } else {
    Serial.println("WiFi Bağlantısı Yok!");
  }
}


void sendSOAPOrtam(float Sicaklik, float Nem) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    String SoapValue = "İLGİLİ BİLGİLER"
    if (client.connect(BaglantiBilgisi, 80)) {
      client.println("POST " + UzantiBilgisi + path + " HTTP/1.1");
      client.print("Host: ");
      client.println(BaglantiBilgisi);
      client.println("Content-Type: text/xml; charset=utf-8");
      client.print("Content-Length: ");
      client.println(SoapValue.length());
      client.print("SOAPAction: ");
      client.println(SoapAction);
      client.println("");
      client.println(SoapValue);
      client.println("");

      while (client.connected()) {
        if (client.available()) {
          String responseLine = client.readStringUntil('\n');
          Serial.println("HTTP Durum Kodu: " + responseLine);

          String resp = client.readString();
          int firstDividerIndex = resp.indexOf(xmlStartTagDivider) + xmlStartTagDivider.length();
          int secondDividerIndex = resp.indexOf(xmlFinishTagDivider);
          if (firstDividerIndex > -1 && secondDividerIndex > -1) {
            String lastStr = resp.substring(firstDividerIndex, secondDividerIndex);
            Serial.println("Servis Yaniti: " + lastStr);
          } else {
            Serial.println("Yanıt içeriği bulunamadı veya hatalı");
          }
          break;
        }
        //yield();  // Diğer işlemleri yapabilmesi için
      }
      client.stop();
    } else {
      Serial.println("Bağlantı başarısız!!!");
    }
  }
}

void sendSOAPToken() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    String SoapMessage = "İLGİLİ BİLGİLER"

    if (client.connect(BaglantiBilgisi, 80)) {
      client.println("POST " + UzantiBilgisi + path + " HTTP/1.1");
      client.print("Host: ");
      client.println(BaglantiBilgisi);
      client.println("Content-Type: text/xml; charset=utf-8");
      client.print("Content-Length: ");
      client.println(SoapMessage.length());
      client.print("SOAPAction: ");
      client.println(SoapAction3);
      client.println("");
      client.println(SoapMessage);
      client.println("");

      while (client.connected()) {
        if (client.available()) {
          String responseLine3 = client.readStringUntil('\n');
          Serial.println("HTTP Durum Kodu: " + responseLine3);

          String resp3 = client.readString();
          int firstDividerIndex = resp3.indexOf(xmlStartTagDivider3) + xmlStartTagDivider3.length();
          int secondDividerIndex = resp3.indexOf(xmlFinishTagDivider3);
          if (firstDividerIndex > -1 && secondDividerIndex > -1) {
            String lastStr3 = resp3.substring(firstDividerIndex, secondDividerIndex);
            Serial.println("Servis Yaniti: " + lastStr3);

            // Gelen yanıtı kontrol ediyoruz
            if (lastStr3.length() > 0) {
              String veri1, veri2, veri3, veri4;
              splitData(lastStr3, veri1, veri2, veri3, veri4);
              Serial.println("Veri 1: " + veri1);
              Serial.println("Veri 2: " + veri2);
              Serial.println("Veri 3: " + veri3);
              Serial.println("Veri 4: " + veri4);
            } else {
              Serial.println("TOKEN GEÇERSİZ");  // Geçersiz token ise mesaj yazdır
            }
          } else {
            Serial.println("Yanıt içeriği bulunamadı veya hatalı");
          }
          break;
        }
        yield();
      }
      client.stop();
    } else {
      Serial.println("Bağlantı başarısız!!!");
    }
  } else {
    Serial.println("WiFi Bağlantısı Yok!");
  }
}

void splitData(String data, String& veri1, String& veri2, String& veri3, String& veri4) {
  int firstDollar = data.indexOf('$');
  int secondDollar = data.indexOf('$', firstDollar + 1);
  int thirdDollar = data.indexOf('$', secondDollar + 1);

  // $ işaretleri ile verileri ayırma
  if (firstDollar != -1 && secondDollar != -1 && thirdDollar != -1) {
    veri1 = data.substring(0, firstDollar);
    veri2 = data.substring(firstDollar + 1, secondDollar);
    veri3 = data.substring(secondDollar + 1, thirdDollar);
    veri4 = data.substring(thirdDollar + 1);
  }
}

  void setup() {
  Serial.begin(115200);
  pinMode(ButonArttir, INPUT_PULLUP);
  pinMode(ButonFireArttir, INPUT_PULLUP);

  if (!LittleFS.begin()) {
    Serial.println("LittleFS başlatılamadı!");
    return;
  }
  Serial.println("LittleFS başlatıldı.");
  String username, password, Baglanti, Uzanti, period;
  if (loadUserConfigFromFS(username, password, Baglanti, Uzanti, period)) {
    Serial.print("Loaded period from LittleFS: ");
    Serial.println(period);  // Bu satırla period değerini kontrol edin
    BaglantiBilgisi = Baglanti;
    UzantiBilgisi = Uzanti;
    periodBilgisi = period;
  }

  // WiFi bilgilerini LittleFS'den yükleme
  char ssid[32] = { 0 };
  char wifi_password[32] = { 0 };
  loadWiFiCredentials(ssid, wifi_password);  // Daha önce saklanan WiFi bilgilerini yükle

  // SSID ve şifreyi seri monitöre yazdırma
  Serial.print("Kaydedilen SSID: ");
  Serial.println(ssid);
  Serial.print("Kaydedilen WiFi Şifresi: ");
  Serial.println(wifi_password);

  // Eğer SSID ve şifre yüklendiyse, bağlanmayı dene
  if (strlen(ssid) > 0 && strlen(wifi_password) > 0) {
    WiFiMulti.addAP(ssid, wifi_password);  // Saklanan WiFi bilgilerini kullanarak bağlanmayı dener
    Serial.println("LittleFS'den WiFi bilgileri yüklendi:");
    Serial.println("SSID: " + String(ssid));
  }

  // WiFi'ye bağlanmaya çalış
  WiFi.begin(ssid, wifi_password);
  int wifiTimeout = 10;  // 10 saniye timeout
  while (WiFi.status() != WL_CONNECTED && wifiTimeout > 0) {
    delay(1000);
    Serial.print(".");
    wifiTimeout--; 
  }
  portal.load(FPSTR(AUX_EEPROM_IO));
  portal.on("/", onEEPROM);             // EEPROM menüsüne yönlendirme
  portal.on("/eeprom", onEEPROMWrite);  // EEPROM yazma işlemi
  MacID = WiFi.macAddress();
  if (WiFi.status() == WL_CONNECTED) {
    // WiFi'ye bağlanıldığında SSID ve şifreyi kaydedin
    saveWiFiCredentials(WiFi.SSID().c_str(), WiFi.psk().c_str());
    Serial.println("WiFi bilgileri kaydedildi.");
  } else {
    Serial.println("Starting AP mode.");
    config.autoReconnect = true;
    config.apid = "ESP8266_AutoConnect";
    config.psk = "12345678";
    config.hostName = "esp8266";
    config.immediateStart = true;
    config.menuItems = AC_MENUITEM_CONFIGNEW | AC_MENUITEM_OPENSSIDS;

    portal.config(config);
    portal.begin();

    if (WiFi.status() == WL_CONNECTED) {
      // WiFi'ye bağlandığında, SSID ve şifreyi kaydet
      saveWiFiCredentials(WiFi.SSID().c_str(), WiFi.psk().c_str());
      Serial.println("WiFi bilgileri kaydedildi.");
    }
  }

  sendSOAPToken();                // İlk token'i al
  lastTokenCheckTime = millis();  // Zamanlayıcı başlatılıyor
  tokenCheckedOnce = true;        // İlk token alındı
  delay(500);
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);
}


void loop() {
  portal.handleClient();
  unsigned long currentMillis = millis();  // Geçen süreyi kontrol ediyoruz
  // Her 4 saatte bir token kontrolü yap
  /*if (WiFi.status() == WL_CONNECTED && tokenCheckedOnce && (currentMillis - lastTokenCheckTime >= tokenCheckInterval)) {
    sendSOAPToken();
    lastTokenCheckTime = currentMillis;  // Zamanlayıcıyı sıfırla
  }*/
  if (WiFi.status() == WL_CONNECTED) {
  sensors_event_t tempEvent, humidityEvent;
  float currentTemperature = SicaklikVal;  // Mevcut sıcaklık değeri
  float currentHumidity = NemVal;          // Mevcut Nem değeri
  int currentMiktar = Miktar;              // Mevcut Miktar değeri
  int currentFire = Fire;
  sensors_event_t event;

  dht.temperature().getEvent(&tempEvent);
  dht.humidity().getEvent(&humidityEvent);
  // SSID ve şifreyi seri monitöre yazdırma
  Serial.print("Kaydedilen SSID: ");
  Serial.println(ssid);
  Serial.print("Kaydedilen WiFi Şifresi: ");
  Serial.println(wifi_password);
  if (isnan(tempEvent.temperature)) {
    Serial.println("Sıcaklık sensöründen veri alınamadı!");
  } else {
    SicaklikVal = tempEvent.temperature;
    paramSicaklik = String(SicaklikVal);  // Sıcaklık değerini string'e dönüştürüyoruz
    paramSicaklik.replace(".", ",");      // Nokta yerine virgül kullanıyoruz
  }

  if (isnan(humidityEvent.relative_humidity)) {
    Serial.println("Nem sensöründen veri alınamadı!");
  } else {
    NemVal = humidityEvent.relative_humidity;
    paramNem = String(NemVal);   // Nem değerini string'e dönüştürüyoruz
    paramNem.replace(".", ",");  // Nokta yerine virgül kullanıyoruz
  }

  WiFiClient client;

  delay(1000);
  Serial.println("");
  Serial.print("Miktar: ");
  Serial.println(Miktar);
  Serial.print("Fire: ");
  Serial.println(Fire);
  Serial.print("MacID: ");
  Serial.println(MacID);
  Serial.print("Sıcaklık: ");
  Serial.println(SicaklikVal);
  Serial.print("Nem: ");
  Serial.println(NemVal);
  // SSID ve şifreyi seri monitöre yazdırma
  Serial.print("Kaydedilen SSID: ");
  Serial.println(ssid);
  Serial.print("Kaydedilen WiFi Şifresi: ");
  Serial.println(wifi_password);
  delay(1000);

  if (periodBilgisi == "1") {
    Serial.println("Anlık Gönderim Seçeneği Aktif Edildi (1)");
    attachInterrupt(digitalPinToInterrupt(ButonArttir), handleButtonPressArttir, FALLING);
    attachInterrupt(digitalPinToInterrupt(ButonFireArttir), handleButtonPressFireArttir, FALLING);
    // Miktar veya Fire değiştiyse SOAP isteği gönder
    if (MiktarChanged || FireChanged) {
      sendSOAPRequest(Miktar, Fire);
      MiktarChanged = false;
      FireChanged = false;
      Miktar = 0;
      Fire = 0;
      Serial.println("Miktar veya Fire değişti, SOAP isteği gönderildi.");
    } else {
      Serial.println("Butona basılmadı.");
    }

    // Sicaklik değiştiyse SOAP isteği gönder
    if (SicaklikVal != OncekiSicaklikVal) {
      sendSOAPOrtam(SicaklikVal, NemVal);
      OncekiSicaklikVal = SicaklikVal;
      Serial.println("Sıcaklık değişti, SOAP isteği gönderildi.");
    } else {
      Serial.println("Sıcaklık değeri değişmedi.");
    }
    // Bilgileri yazdır
    Serial.println("");
    Serial.print("Miktar: ");
    Serial.println(Miktar);
    Serial.print("Fire: ");
    Serial.println(Fire);
    Serial.print("MacID: ");
    Serial.println(MacID);
    Serial.print("Sıcaklık: ");
    Serial.println(SicaklikVal);
    Serial.print("Nem: ");
    Serial.println(NemVal);
    delay(1000);
  }


  else if (periodBilgisi == "2") {
    Serial.println("30 Dakikalık seçim aktif edildi (2)");
    attachInterrupt(digitalPinToInterrupt(ButonArttir), handleButtonPressArttir, FALLING);
    attachInterrupt(digitalPinToInterrupt(ButonFireArttir), handleButtonPressFireArttir, FALLING);
    unsigned long currentMillis = millis();
    if (currentMillis - lastFireSendTime >= sendInterval) {
      sendSOAPOrtam(SicaklikVal, NemVal);
    }

    // 30 dakikada Miktar için ortalama gönderimi
    if (currentMillis - lastMiktarSendTime >= sendInterval) {
      if (buttonPressCountMiktar > 0) {
        float avgMiktar = (float)totalMiktar / buttonPressCountMiktar;  // Miktar ortalaması
        sendSOAPRequest(avgMiktar, Fire);
      } else {
        sendSOAPRequest(Miktar, Fire);  // Butona hiç basılmadıysa mevcut Miktar'ı gönder
      }
      lastMiktarSendTime = currentMillis;
      buttonPressCountMiktar = 0;  // Sayaçları sıfırla
      totalMiktar = 0;
    }

    // 30 dakikada Fire için ortalama gönderimi
    if (currentMillis - lastFireSendTime >= sendInterval) {
      if (buttonPressCountFire > 0) {
        float avgFire = (float)totalFire / buttonPressCountFire;  // Fire ortalaması
        sendSOAPRequest(Miktar, avgFire);
      } else {
        sendSOAPRequest(Miktar, Fire);  // Butona hiç basılmadıysa mevcut Fire'ı gönder
      }

      lastFireSendTime = currentMillis;
      buttonPressCountFire = 0;  // Sayaçları sıfırla
      totalFire = 0;
    }
    delay(10);
    Serial.println("");
    Serial.print("Miktar: ");
    Serial.println(Miktar);
    Serial.print("Fire: ");
    Serial.println(Fire);
    Serial.print("MacID: ");
    Serial.println(MacID);
    Serial.print("Sıcaklık: ");
    Serial.println(SicaklikVal);
    Serial.print("Nem: ");
    Serial.println(NemVal);
    delay(1000);
  }

  else {
    Serial.println("Performing actions for period 3");
    Serial.println("Wifi Bağlantısı Yok");
    Serial.println("Sistem Yeniden Başlatılıyor");
    delay(15000);
    Serial.println("Starting AP mode.");
    config.boundaryOffset = 0;
    config.autoReconnect = true;
    config.apid = "ESP8266_AutoConnect";
    config.psk = "12345678";
    config.hostName = "esp8266";
    config.immediateStart = true;
    config.menuItems = AC_MENUITEM_CONFIGNEW | AC_MENUITEM_OPENSSIDS;
    portal.config(config);
    portal.begin();
  }
}
  //Serial.print("Bağlanılan Adres:");
  //Serial.println(BaglantiBilgisi + UzantiBilgisi + path);
}