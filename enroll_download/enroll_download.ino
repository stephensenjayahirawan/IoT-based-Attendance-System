#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <ArduinoJson.h>

#include <Fingerprint.h>

const char* certificate  = "cf 98 74 d7 d8 8f 36 a6 08 6f e2 c7 4a 60 e7 90 2b db 88 4b";

Adafruit_PCD8544 display = Adafruit_PCD8544(D0, D1, D2);
SoftwareSerial mySerial(D4, D3);
ESP8266WiFiMulti WiFiMulti;
Fingerprint finger = Fingerprint(&mySerial);

//const char* ssid     = "Tenda_06F090";
//const char* password = "12345678";

//const char* ssid     = "it@bkaunpar";
//const char* password = "kantinqiu";

const char* ssid     = "OnePlus3t";
const char* password = "parahyangan";

String bytesReceived;

uint16_t id;

void setup()
{
  Serial.begin(9600);
  while (!Serial); 
  delay(100);
  Serial.println("Sistem pendaftaran Fingerprint");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  display.begin();
  display.setContrast(50);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.clearDisplay();
  display.println("Sistem pendaftaran Fingerprint");
  display.display();
  delay(2000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    display.clearDisplay();
    display.println("Connecting to WiFi");
    display.display();
    delay(500);
    Serial.print(".");
  }

  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Berhasil menemukan sensor");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    int status_fingerprint = finger.emptyDatabase();
    if (status_fingerprint == OK) {
      Serial.println("Berhasil menghapus seluruh template!");
    }
  } else {
    display.clearDisplay();
    display.println("Fingerprint Sensor tidak ditemukan.");
    display.display();
    delay(2000);
    Serial.println("Fingerprint Sensor tidak ditemukan.");
    while (1) {
      delay(1);
    }
  }
}

uint16_t readnumber(void) {
  uint16_t num = 0;

  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void loop()  
{
  Serial.println("Menunggu Fingerprint!");
  display.clearDisplay();
  display.println("Menunggu Fingerprint");
  display.display();
  bytesReceived = "";
  id = 1;
  Serial.print("Enrolling ID #");
  Serial.println(id);

  while (!  getFingerprintEnroll() );
}

uint8_t getFingerprintEnroll() {

  int p = -1;
  while (p != OK) {
    p = finger.getImage();
    switch (p) {
      case OK:
        Serial.println("Berhasil mengambil Gambar");
        break;
      case NOFINGER:
        break;
      case PACKETRECIEVEERR:
        Serial.println("Komunikasi terganggu");
        break;
      case IMAGEFAIL:
        Serial.println("Gagal menyimpan dalam img buffer!");
        break;
      default:
        Serial.println("ERROR");
        break;
    }
  }


  p = finger.image2Tz(1);
  switch (p) {
    case OK:
      Serial.println("Berhasil mengubah gambar");
      break;
    case IMAGEMESS:
      Serial.println("Gambar tidak valid");
      return p;
    case PACKETRECIEVEERR:
      Serial.println("Komunikasi terganggu");
      return p;
    case FEATUREFAIL:
      Serial.println("Gambar tidak valid");
      return p;
    case INVALIDIMAGE:
      Serial.println("Gambar tidak valid");
      return p;
    default:
      Serial.println("ERROR");
      return p;
  }

  delay(2000);
  p = 0;
  display.clearDisplay();
  display.println("Tempelkan Fingerprint yang sama kembali!");
  display.display();
  while (p != NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Tempelkan Fingerprint yang sama kembali!");
  while (p != OK) {
    p = finger.getImage();
    switch (p) {
      case OK:
        Serial.println("Berhasil mengambil Gambar");
        break;
      case NOFINGER:
        break;
      case PACKETRECIEVEERR:
        Serial.println("Komunikasi terganggu");
        break;
      case IMAGEFAIL:
        Serial.println("Gagal mengambil gambar");
        break;
      default:
        Serial.println("ERROR");
        break;
    }
  }


  p = finger.image2Tz(2);
  switch (p) {
    case OK:
      Serial.println("Berhasil mengubah gambar");
      break;
    case IMAGEMESS:
      Serial.println("Gambar terlalu berantakan");
      return p;
    case PACKETRECIEVEERR:
      Serial.println("Komunikasi terganggu");
      return p;
    case FEATUREFAIL:
      Serial.println("Gambar tidak valid");
      return p;
    case INVALIDIMAGE:
      Serial.println("Gambar tidak valid");
      return p;
    default:
      Serial.println("ERROR");
      return p;
  }

  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == OK) {
    display.clearDisplay();
    display.println("Fingerprint cocok");
    display.display();

    Serial.println("Fingerprint cocok");
  } else if (p == PACKETRECIEVEERR) {
    Serial.println("Komunikasi terganggu");
    return p;
  } else if (p == ENROLLMISMATCH) {
    display.clearDisplay();
    display.println("Fingerprint tidak cocok. Silahkan ulangi");
    display.display();
    delay(2000);
    Serial.println("Fingerprint tidak cocok. Silahkan ulangi");
    return p;
  } else {
    Serial.println("ERROR");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == OK) {
    Serial.println("Berhasil menyimpan!");
  } else if (p == PACKETRECIEVEERR) {
    Serial.println("Komunikasi terganggu");
    return p;
  } else if (p == BADLOCATION) {
    Serial.println("tidak dapat menyimpan pada lokasi tersebut");
    return p;
  } else if (p == FLASHERR) {
    Serial.println("Gagal menyimpan");
    return p;
  } else {
    Serial.println("ERROR");
    return p;
  }


  Serial.print("Mengambil Template"); Serial.println(id);
  p = finger.getModel();
  switch (p) {
    case OK:
      Serial.print("Template "); Serial.print(id); Serial.println(" transferring:");
      break;
    default:
      Serial.print("ERROR "); Serial.println(p);
      display.clearDisplay();
      display.println("ERROR");
      display.display();
      delay(2000);
      return p;
  }

  byte xbytesReceived[650]; // 2 data packets
  memset(xbytesReceived, 0xff, 650);
  uint32_t starttime = 0;
  starttime = millis();
  int total_bytes = 0;
  display.clearDisplay();
  display.println("Mengambil Fingerprint dari Sensor");
  display.display();
  while ( (millis() - starttime) < 10000) {
    if (mySerial.available()) {
      xbytesReceived[total_bytes++] = mySerial.read();
    }
    yield();
  }
  Serial.print(total_bytes); Serial.println(" bytes.");
  for ( int i = 0; i < total_bytes; i++) {
    bytesReceived += xbytesReceived[i];
    bytesReceived += ",";
    yield();
  }
  Serial.println(bytesReceived);
  //  display.clearDisplay();
  //  display.println("Mengirimkan data ke web Server");
  //  display.display();
  if ((WiFi.status() == WL_CONNECTED)) {

    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(certificate);

    HTTPClient https;

    Serial.print("[HTTPS] begin...\n");
    String url = "https://stephen.parkboy.net/storeFingerprint?FINGERPRINT=";
    Serial.print(url);

    if (https.begin(*client, url + bytesReceived )) { // HTTPS

      Serial.print("[HTTPS] GET...\n");
      int httpCode = https.GET();

      if (httpCode > 0) {
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {

          String payload = https.getString();
          const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
          DynamicJsonBuffer jsonBuffer(capacity);
          JsonObject& respon_json = jsonBuffer.parseObject(payload);
          if (!respon_json.success()) {
            Serial.println(F("Parsing failed!"));
            display.clearDisplay();
            display.println("Sukes berkomunikasi dengan web Server");
            display.display();
            delay(2000);
          } else {
            display.clearDisplay();
            display.println(respon_json["message"].as<String>());
            display.display();
            delay(2000);
          }
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
    } else {
      display.clearDisplay();
      display.println("Tidak dapat berkomunikasi dengan server.");
      display.display();
      delay(2000);
      Serial.printf("Tidak dapat berkomnukasi dengan server\n");
    }
    delay(15000);
    https.end();
  }

  Serial.println();

  p = PACKETRECIEVEERR;
  Serial.println("\ndone.");
  return  p;
}
