#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Fingerprint.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

//const char* ssid = "Tenda_06F090";
//const char* password = "12345678";

//const char* ssid = "it@bkaunpar";
//const char* password = "kantinqiu";

const char* ssid     = "OnePlus3t";
const char* password = "parahyangan";

//const char* ssid     = "pudim";
//const char* password = "dimaskanjeng2016";


const char* certificate  = "cf 98 74 d7 d8 8f 36 a6 08 6f e2 c7 4a 60 e7 90 2b db 88 4b";
String host = "http://stephen.parkboy.net/getFingerprint?id_Ruangan=3&counter=";
int getFingerprintID();
String url_jam = "http://stephen.parkboy.net/getFingerprint?jam_arduino=1";
Adafruit_PCD8544 display = Adafruit_PCD8544(D0, D1, D2);
unsigned long lama_menunggu = 0;
unsigned long waktu = 0;
SoftwareSerial mySerial(D4, D3);


String payload = "";
Fingerprint finger = Fingerprint(&mySerial);
void setup()

{

  Serial.begin(9600);
  while (!Serial);
  Serial.println("Fingerprint template extractor");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  WiFi.begin(ssid, password);
  display.begin();

  display.setContrast(50);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.clearDisplay();
  display.println("Sistem pencocokan Fingerprint");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.clearDisplay();
    display.println("Connecting to WiFi");
    display.display();
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1);
  }

  int status_fingerprint = finger.emptyDatabase();
  if (status_fingerprint == OK) {
    Serial.println("Database Empty!");
  }
  Serial.print("Request Link:");

  HTTPClient http;
  display.clearDisplay();
  display.println("Mengunduh template");
  display.display();

  http.begin(url_jam);
  int httpCode = http.GET();
  payload = http.getString();
  Serial.print("Response Code:");
  Serial.println(httpCode);

  Serial.print("Returned data from Server:");

  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
  DynamicJsonBuffer jsonBuffer(capacity);
  if (httpCode == 200)
  {
   download_fingerprint();
  }
  else
  {

    display.clearDisplay();
    display.println("Gagal berkomunikasi dengan Server.");
    display.display();
    Serial.println("Error in response");
  }


  Serial.println("\ndone.");
  finger.getTemplateCount();
  Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
//  Serial.println("Waiting for valid finger...");

  http.end();

}

void loop()
{
//  if ( (millis() - waktu) / 60000 < lama_menunggu / 60 ) {
    if (finger.templateCount == 0){
      display.clearDisplay();
      display.println("Tidak ada Fingerprint");
      display.display();
    }else{
      display.clearDisplay();
      display.println("Menunggu Fingerprint");
      display.display();
      getFingerprintID();
    }
   delay(10);
//  } else if ((millis() - waktu) / 60000 == lama_menunggu / 60) {
//    display.clearDisplay();
//    display.println("Tunggu sedang downlaod Fingerprint!");
//    display.display();
//    unsigned long waktu_mulai = millis();
//    download_fingerprint();
//
//    waktu = millis();
//    lama_menunggu = 3600 - ((waktu - waktu_mulai)/1000);
//
//  }
  
}

void download_fingerprint() {
  int status_fingerprint = finger.emptyDatabase();
  if (status_fingerprint == OK) {
    Serial.println("Database Empty!");
  }
  HTTPClient http;
  uint8_t id = 0;
  display.clearDisplay();
  display.println("Mengunduh template");
  display.display();
  int j = 0;
  int repetisi = 1000;
  while ( j < repetisi) {

    http.begin(host + j);
    int httpCode = http.GET();
    payload = http.getString();
    Serial.print("Response Code:");
    Serial.println(httpCode);

    Serial.print("Returned data from Server:");

    const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
    DynamicJsonBuffer jsonBuffer(capacity);
    if (httpCode == 200)
    {
      JsonObject& respon_json = jsonBuffer.parseObject(payload);
      repetisi = respon_json["jumlah_fingerprint"].as<int>();
      if (!respon_json.success()) {
        Serial.println(F("Parsing failed!"));
        return;
      }
      Serial.println(F("Response:"));
      Serial.println(respon_json["jumlah_fingerprint"].as<int>());

      Serial.print(j);
      Serial.println("------------------------------------");
      Serial.print("Attempting to load #"); Serial.println(id);

      mySerial.flush();
      uint8_t status_fingerprint = 0;
      mySerial.flush();
      bool check = false;
      if(repetisi==0){
        return;
      }
      while (check == false) {
        status_fingerprint = finger.uploadTemplate();
        if (status_fingerprint == OK) {
          Serial.println("Writing Template!");
          for (int i = 0; i < respon_json["FINGERPRINT"][0]["length"].as<int>() - 1; i++) {
            byte a = byte(respon_json["FINGERPRINT"][0]["template"][i].as<int>());
            mySerial.write(a);
            yield();
          }
          mySerial.flush();
          check = true;
        } else if (status_fingerprint == PACKETRECIEVEERR) {
          Serial.println("Communication error");
        } else if (status_fingerprint == BADLOCATION) {
          Serial.println("Could not store in that location");
        } else if (status_fingerprint == FLASHERR) {
          Serial.println("Error writing to flash");
        } else {
          Serial.println("Unknown error");
        }
        yield();
      }
      check = false;

      while ( check == false) {

        mySerial.flush();
        status_fingerprint = finger.storeModel(j);
        if (status_fingerprint == OK) {
          Serial.println("Stored!");
          check = true;
        } else if (status_fingerprint == PACKETRECIEVEERR) {
          Serial.println("Communication error");
        } else if (status_fingerprint == BADLOCATION) {
          Serial.println("Could not store in that location");
        } else if (status_fingerprint == FLASHERR) {
          Serial.println("Error writing to flash");
        } else {
          Serial.println("Unknown error");
        }
        yield();
      }
      j++;
    }
    else
    {

      display.clearDisplay();
      display.println("Gagal berkomunikasi dengan Server.");
      display.display();
      Serial.println("Error in response");
    }


  }
  Serial.println("\ndone.");
  finger.getTemplateCount();
  Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  Serial.println("Waiting for valid finger...");
  Serial.println();

  http.end();
}

int getFingerprintID() {

  uint8_t status_fingerprint = finger.getImage();
  if (status_fingerprint != OK)  return -1;

  status_fingerprint = finger.image2Tz();
  if (status_fingerprint != OK)  return -1;

  status_fingerprint = finger.fingerFastSearch();
  if (status_fingerprint != OK) {

    display.clearDisplay();
    display.println("Tidak menemukan Fingerprint yang cocok");
    display.display();
    delay(1000);
    return -1;
  }

  display.clearDisplay();
  display.println("Menemukan mencocokan Fingerprint");
  display.display();
  delay(1500);
  WiFi.begin(ssid, password);
  Serial.print(WiFi.status() );
  Serial.println(WL_CONNECTED);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(WiFi.status() );
    delay(500);
    Serial.print(".");
  }
  payload = "";
  HTTPClient http;
  String urlAbsen =  "https://www.stephen.parkboy.net/absen?id_Ruangan=3&ID=" + String(finger.fingerID);
  Serial.println(urlAbsen);
  Serial.println(finger.fingerID);
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

  client->setFingerprint(certificate);
  HTTPClient https;
  https.begin(*client, urlAbsen );
  int httpCode = https.GET();
  payload = https.getString();

  Serial.print("Response Code:");
  Serial.println(httpCode);
  if(httpCode !=200){
    display.clearDisplay();
    display.println("Gagal melakukan komunikasi dengan web");
    display.display();
    delay(1500);
    return -1;
  }
  Serial.print("Returned data from Server:");
  Serial.println(payload);

  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& respon_json = jsonBuffer.parseObject(payload);

  if (!respon_json.success()) {
    display.clearDisplay();
    display.println("Gagal mendapatkan JSON");
    display.display();
    delay(1500);
    Serial.println(F("Parsing failed!"));
  }
  display.clearDisplay();
  display.println(respon_json["message"].as<String>());
  display.display();
  delay(1500);
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}
