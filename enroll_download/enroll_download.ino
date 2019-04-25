
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClientSecureBearSSL.h>

const char* fingerprint  = "cf 98 74 d7 d8 8f 36 a6 08 6f e2 c7 4a 60 e7 90 2b db 88 4b";
#include <Adafruit_Fingerprint.h>

SoftwareSerial mySerial(4,5);

ESP8266WiFiMulti WiFiMulti;
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

const char* ssid     = "it@bkaunpar";
const char* password = "kantinqiu";

uint16_t id;

void setup()  
{
  Serial.begin(9600);
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit Fingerprint sensor enrollment");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("it@bkaunpar", "kantinqiu");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // set the data rate for the sensor serial port
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
    
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
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

void loop()                     // run over and over again
{
  Serial.println("Ready to enroll a fingerprint!");
  Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
  id = readnumber();
  if (id == 0) {// ID #0 not allowed, try again!
     return;
  }
  Serial.print("Enrolling ID #");
  Serial.println(id);
  
  while (!  getFingerprintEnroll() );
}

uint8_t getFingerprintEnroll() {

  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  downloadFingerprintTemplate();
   Serial.println("------------------------------------");
 Serial.print("Attempting to load #"); Serial.println(id);
  // OK success!

  Serial.print("Attempting to get #"); Serial.println(id);
   p = finger.getModel();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.print("Template "); Serial.print(id); Serial.println(" transferring:");
      break;
   default:
      Serial.print("Unknown error "); Serial.println(p);
      return p;
  }

  byte xbytesReceived[650]; // 2 data packets
  memset(xbytesReceived, 0xff, 650);
  uint32_t starttime = 0;
  starttime = millis();
  int total_bytes = 0;
  while ( (millis() - starttime) < 5000) {
      if (mySerial.available()) {
          xbytesReceived[total_bytes++] = mySerial.read();
          
      }
      yield();
  }
  Serial.print(total_bytes); Serial.println(" bytes read.");
  Serial.println("Decoding packet...");
  String bytesReceived;
  for ( int i = 0; i < total_bytes;i++){
    bytesReceived+= xbytesReceived[i];
    bytesReceived +=",";
    yield();
  }
  Serial.println(bytesReceived);
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    client->setFingerprint(fingerprint);

    HTTPClient https;
    
    Serial.print("[HTTPS] begin...\n");
      String url = "https://stephen.parkboy.net/storeFingerprint?FINGERPRINT=";
      Serial.print(url);
    
        if (https.begin(*client, url+bytesReceived )) {  // HTTPS
  
        Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        int httpCode = https.GET();
  
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
  
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            Serial.println(payload);
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
  
        
    }else {
      Serial.printf("[HTTPS] Unable to connect\n");
    
    }
    https.end(); 
  }
  
 
  Serial.println();
//  for (int i = 0; i < total_bytes; ++i) {
//      Serial.print(bytesReceived[i]);
//      yield();
//  }
//  if (client.connect(host, port)) {
//    Serial.println("transfering to web server");
//    client.print("GET /getFingerprint?NPM=123&FINGERPRINT=");
//    for (int i = 0; i < total_bytes; ++i) {
//      client.print("0x");
//      printHex(bytesReceived[i], 2);
//      if (i != total_bytes - 1){
//          client.print(","); 
//      }
//      yield();
//     }
//    client.println(" HTTP/1.1");
//    client.println("Host: stephen.parkboy.net");
//    client.println("Connection: close");
//    client.println();
//    
//  client.stop();
//  }
  p = FINGERPRINT_PACKETRECIEVEERR;
  Serial.println("\ndone.");
   return  p;
}

uint8_t downloadFingerprintTemplate()
{


}



char printHex(int num, int precision) {
    char tmp[16];
    char format[128];
 
    sprintf(format, "%%.%dX", precision);
 
    sprintf(tmp, format, num);
    Serial.print(tmp);
}
