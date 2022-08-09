/*
 * This program is free software: you can redistribute it and/or modify it under the terms of 
 * the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, 
 * or (at your option) any later version. This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
 * 
 * Author: Instagram(@nopalfi)
 */

//  Tambahkan Semua Library
#include <ArduinoHttpClient.h>
#include <EEPROM.h>
#include <WiFiEspAT.h>
#include <ArduinoJson.h>
#include <Arial14.h>
#include <Arial_black_16.h>
#include <Arial_Black_16_ISO_8859_1.h>
#include <SPI.h>
#include <DMD2.h>
#include <SystemFont5x7.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// Buat DMDFrame bernama dmd dengan (lebar, tinggi) panel
SoftDMD dmd(3,1);

// Buat object TextBox dmd bernama box dengan parameter (DMDFrame &dmd, int left, int top, int width, int height)
DMD_TextBox box(dmd, 0, 0, 32, 16);

// Buat object tiap sensor DHT dengan parameter (pin, tipesensor) tipe sensor = [DHT11, DHT21, DHT22]
DHT_Unified dht1(31, DHT21); // Coklat
DHT_Unified dht2(35, DHT21); // Merah
DHT_Unified dht3(39, DHT21); // Orange

// Ubah untuk masing-masing pin di arduino
const int pompa = 43; // Hitam/Pompa
const int kipas = 47; // Putih/Kipas
const int blower = 51; // Abu-Abu/Blower

String sapi_birahi, sapi_persalinan;                  // Global variabel ketika mencoba mendapatkan nama-nama sapi

double min_temp, min_hum, max_temp, max_hum;          // Global variabel config yang didapat dari EEPROM

/*
 * lokasi server bisa berupa IP Address atau hostname tujuan [192.168.0.0, google.com]
 * Jangan tambahkan protocol (http://, https://)
 * Jangan tambahkan absolute path (192.168.0.0/sebuah_script.php)
 * serverAddress ini mengarah ke lokasi webserver untuk menjalankan PHP script sebagai
 * logic aplikasi dan komunikasi antar database dan arduino (via JSON)
 */
char serverAddress[] = "192.168.0.170"; 
int port = 80;

// Arduino mega + ESP32 menggunakan Koneksi Wifi daripada Ethernet
WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);
int status = WL_IDLE_STATUS;

double temp1, hum1, temp2, hum2, temp3, hum3; // Global variabel didapat dari DHT_Unified_sensor
String str;                                   // Global variabel str digunakan untuk menyimpan data nama-sapi yang akan dipassing ke sebuah method
void setup() {
  Serial.begin(115200);                       // Mega bisa dimulai diberbagai baud rate [9600, 115200, dll] tapi standart nya 115200
  while (!Serial);                            // Tunggu Serial tersedia sebelum lanjut
  Serial3.begin(115200);                      // Komunikasi antar ESP32 dimulai dengan baud rate 115200

  WiFi.init(Serial3);                         // Inisiasi object WiFi agar dapat komunikasi dengan ESP32

  if (WiFi.status() == WL_NO_MODULE) {        // Khusus Arduino mega + ESP, agar dapat menggunakan fitur wifi, jangan lupa ON kan Switch (1, 2, 3, 4)
    while (true);
  }

  /*
   * Pastikan WiFi memutuskan koneksi yang ada, aktifkan persistent agar wifi terus terhubung
   * ke koneksi sebelumnya dengan cepat
   * dan pastikan router sudah reserve static ip
   * dan konfigurasi ip address sesuai kebutuhan
   */
  WiFi.disconnect();
  WiFi.setPersistent();
  WiFi.endAP();
  IPAddress ip(192,168,0,200);
  IPAddress gw(192,168,0,1);
  IPAddress nm(255, 255, 255, 0);
  WiFi.config(ip, gw, gw, nm);
  // ---------------------------------------//

  /*
   * Silakan ubah konfigurasi SSID, dan Password WiFi sesuai kebutuhan
   * WiFi.begin("SSID","PASSWORD");
   */
  Serial.println("Connecting...");
  WiFi.begin("Kandang", "arutmin2022");
  while (WiFi.status() != WL_CONNECTED) {
    delay(20);
    Serial.print('.');
  }
  Serial.println("Connected to Wi-Fi");

  dmd.setBrightness(255);                 // Set kecerahan DMD (0-255)
  dmd.selectFont(Arial_Black_16);         // Set font sesuai kebutuhan
  dht1.begin();
  dht2.begin();
  dht3.begin();
  // Kosongkan variabel untuk nama-nama sapi
  sapi_birahi="";
  sapi_persalinan="";
  // Setiap memulai arduino mega, pastikan semua relay dimatikan
  digitalWrite(kipas, LOW);
  digitalWrite(blower, LOW);
  digitalWrite(pompa, LOW);

  /*
   * Saat ini, DMD2 diketahui terjadi konflik dengan HTTPClient dan WiFiClient
   * oleh karena itu gunakan method begin untuk mengaktifkan DMD dan method end untuk mengnonaktifakan DMD
   * Aktifkan DMD jika ingin menampilkan teks ke DMD
   * dan Nonaktifkan DMD jika ingin fetch data melalui HTTPClient atau WiFiClient
   */
  dmd.begin();
//  introText();
  delay(1000);
  dmd.end();
}

void loop() {
  // Area Perulangan
  // |------------------Baca Semua DHT Sensor dengan metode berikut-------------|
  sensors_event_t event1;
  dht1.temperature().getEvent(&event1);
  temp1 = event1.temperature;
  dht1.humidity().getEvent(&event1);
  hum1 = event1.relative_humidity;
  
  sensors_event_t event2;
  dht2.temperature().getEvent(&event2);
  temp2 = event2.temperature;
  dht2.humidity().getEvent(&event2);
  hum2 = event2.relative_humidity;
  
  sensors_event_t event3;
  dht3.temperature().getEvent(&event3);
  temp3 = event3.temperature;
  dht3.humidity().getEvent(&event3);
  hum3 = event3.relative_humidity;
  //|----------------------------------------------------------------------------|

  //|------------------Main Logic Starts here-----------------|
  // function checkConfig digunakan untuk mengambil data config dari EEPROM dan simpan ke global variabel
  checkConfig();        
  
  delay(1000);
  // function relayLogic digunakan untuk melakukan logic sesuai dengan temperature dan humidity yang didapat
  relayLogic();  
           
  // function sapiBirahi merupakan function yang akan mengambil data nama-nama sapi birahi dari database dan (return dalam bentuk string)
  sapi_birahi = sapiBirahi();     
  
  //sapi_birahi = "AAAAAAA-BBBBBBB-CCCCCC-DDDDDDDD-EEEEEE";   hanya digunakan untuk tes seberapa jauh DMD bisa print data
  delay(1000);
  // function sapiPersalinan merupakan function yang akan mengambil data nama-nama sapi persalinan dari database dan (return dalam bentuk string)
  sapi_persalinan = sapiPersalinan();   
    
  //sapi_persalinan = "AAAAAAA-BBBBBBB-CCCCCC-DDDDDDDD-EEEEEE"; hanya digunakan untuk tes seberapa jauh DMD bisa print data
  delay(1000);
  dmd.begin();
  // Menampilkan semua data dari DHT sensors
  tempMarquee();          
  delay(1000);
  // Teks berjalan dari kanan ke kiri menampilakan nama-nama sapi birahi
  birahiMarquee(sapi_birahi);   
  delay(1000);
  // Teks berjalan dari kanan ke kiri menampilakan nama-nama sapi persalinan
  persalinanMarquee(sapi_persalinan);   
  delay(1000);
  dmd.end();
  // Fungsi saveTemp digunakan untuk mengirim data ke server
  saveTemp();      
}

void introText() {
  const String intro = "ARUTMIN SMART SYSTEM";
  for (int i = 120; i >= -230; i--){
  dmd.drawString(i,1,intro);
  Serial.println(i);
  if (i == -84) i = -86; 
  if (i == -46) i = -48;
  if (i == -154) {
    dmd.clearScreen();
    i = -156;
  }
  delay(20);
  }
  dmd.clearScreen();
}

void tempMarquee() {
  dmd.selectFont(SystemFont5x7);
  for (int i = 0; i < 5; i++) {
    dmd.drawString(0, 0, "   T1 :"+ (String) temp1+"C");
    dmd.drawString(0, 8, "   rH1:"+ (String) hum1+"%");
    delay(1000); 
  }
  dmd.clearScreen();
  for (int i = 0; i < 5; i++) {
    dmd.drawString(0, 0, "   T2 : "+ (String) temp2+"C");
    dmd.drawString(0, 8, "   rH2: "+ (String) hum2+"%");
    delay(1000); 
  }
  dmd.clearScreen();
  for (int i = 0; i < 5; i++) {
    dmd.drawString(0, 0, "   T3 : "+ (String) temp3+"C");
    dmd.drawString(0, 8, "   rH3: "+ (String) hum3+"%");
    delay(1000); 
  }
  dmd.clearScreen();
}

void birahiMarquee(String birahi) {
  Serial.println("BirahiMarquee");
  dmd.selectFont(Arial_Black_16);
  String str;
  str += "Cek Birahi:";
  str += birahi;
  int panjangstr = str.length();
  for (int i = 100; i >= (0 - panjangstr) * 8; i--) {
    if (i == -41) {
      i = -43;
      dmd.clearScreen();
    }
    if (i == -86) i = -88;
    if (i == -142) i = -144;
    if (i == -206) i = -208;
    dmd.drawString(i, 1, str);  
    delay(50);
  }
  dmd.clearScreen();
}

void persalinanMarquee(String persalinan) {
  Serial.println("PersalinanMarquee");
  dmd.selectFont(Arial_Black_16);
  String str;
  str += "Cek Kelahiran:";
  str += persalinan;
  int panjangstr = str.length();
  for (int i = 100; i >= (0 - panjangstr) * 8; i--) {
    if (i == -41) {
      i = -43;
      dmd.clearScreen();
    }
    dmd.drawString(i, 1, str);  
    delay(50);
  }
  dmd.clearScreen();
}

String sapiBirahi() {
  str = "";
  StaticJsonDocument<100> json;
  Serial.println("making GET request");
  client.get("/sapi_birahi.php");
  client.sendHeader("Content-Type","application-json");
  client.setHttpResponseTimeout(5);

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  Serial.println("Status code: "+statusCode);
    Serial.println(response);
    if (statusCode != 200) {
      response = sapi_birahi;
    }
    client.flush();
  DeserializationError error = deserializeJson(json, response);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return response;
  }
  if (json.isNull()) {
    str = "";
  } else {
    for (JsonObject item : json.as<JsonArray>()) {
      char* nama_sapi = item["nama_sapi"];
      str += (String) nama_sapi;
      str += "-";
    }
    int lngth = str.length();
    str[lngth - 1] = '\0';
  }
  return str;
}

String sapiPersalinan() {
  
  StaticJsonDocument<100> json;
  Serial.println("Sending GET Request...");
  client.get("/sapi_persalinan.php");
  client.sendHeader("Content-Type","application-json");
  client.setHttpResponseTimeout(5);

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  Serial.println("Status code: "+statusCode);
  Serial.println(response);
  if (statusCode != 200) {
      response = sapi_persalinan;
  }
  client.flush();
  DeserializationError error = deserializeJson(json, response);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return str;
  }
  if (json.isNull()) {
    str = "";
  } else {
    for (JsonObject item : json.as<JsonArray>()) {
      char* nama_sapi = item["nama_sapi"];
      str += (String) nama_sapi;
      str += "-";
    }
    int lngth = str.length();
    str[lngth - 1] = '\0';
  }
  return str;
}

void checkConfig() {
  Serial.println("Check config");
  Serial.println(EEPROM.read(0));
  Serial.println(EEPROM.read(1));
  Serial.println(EEPROM.read(2));
  Serial.println(EEPROM.read(3));
  Serial.println(EEPROM.read(4));

  min_hum = EEPROM.read(0);
  max_hum = EEPROM.read(1);
  min_temp = EEPROM.read(2);
  max_temp = EEPROM.read(3);
  int eepromAll = (int) EEPROM.read(0) * (int) EEPROM.read(1) * (int) EEPROM.read(2) * (int) EEPROM.read(3) * (int) EEPROM.read(4);
  if (eepromAll == 0) {
    StaticJsonDocument<200> json;
    Serial.println("config GET request");
    client.get("/config.php");
    client.sendHeader("Content-Type","application/json");
    client.setHttpResponseTimeout(10);
    
    // read the status code and body of the response
    int statusCode = client.responseStatusCode();
    String response = client.responseBody();
    Serial.println("Status code: "+statusCode);
    Serial.println("Response");
    Serial.println(response);
    if (statusCode != 200) {
      response = "";
    }
    client.flush();
    DeserializationError error = deserializeJson(json, response);
  
    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
      
    int statusConfig = (int) json["status"];
    Serial.println(statusConfig);

  if (statusConfig ==1){
    int statusConfig = (int) json["status"];
    double min_temp = (double) json["min_temp"];
    double min_hum = (double) json["min_hum"];
    double max_temp = (double) json["max_temp"];
    double max_hum = (double) json["max_hum"];
    double delayValue = (double) json["delay"];
  
    EEPROM.write(0, max_temp);
    EEPROM.write(1, max_hum);
    EEPROM.write(2, min_temp);
    EEPROM.write(3, min_hum);
    EEPROM.write(4, delayValue);
    
    HttpClient http = HttpClient(client, serverAddress, port);    //Declare object of class HTTPClient
      
    
    Serial.println("making POST request");
    String contentType = "application/x-www-form-urlencoded";

    http.post("/config.php", contentType, "status=0");
    // read the status code and body of the response
        int statusCode = http.responseStatusCode();
        String response = http.responseBody();

        Serial.print("Status code: ");
        Serial.println(statusCode);
        Serial.print("Response: ");
        Serial.println(response);
        http.stop();
        Serial.println("EEpom Sudah Dirubah");
  
  }
}
}
void saveTemp() {
  Serial.println("POST Request");
  HttpClient http = HttpClient(client, serverAddress, port);
  String postDatas = postDataSensor((String) temp1, (String) temp2, (String) temp3, (String) hum1, (String) hum2, (String) hum3);
  String contentType = "application/x-www-form-urlencoded";
  http.setHttpResponseTimeout(5);
  http.post("/suhu.php", contentType, postDatas);
  int statusCode = http.responseStatusCode();
  Serial.println("Status code: "+ (String) statusCode);
  String response = http.responseBody();
  http.stop();
  tempClear();
}

String postDataSensor(String temp1, String temp2, String temp3, String hum1, String hum2, String hum3) {
  String postData;
  postData = "humidity1=";
  postData += (String) hum1;
  postData += "&temperature1=";
  postData += (String) temp1;
  postData += "&humidity2=";
  postData += (String) hum2;
  postData += "&temperature2=";
  postData += (String) temp2;
  postData += "&humidity3=";
  postData += (String) hum3;
  postData += "&temperature3=";
  postData += (String) temp3;  
  return postData;
}

void tempClear() {
  temp1 = 0;
  temp2 = 0;
  temp3 = 0;
  hum1 = 0;
  hum2 = 0;
  hum3 = 0;
}

void relayLogic() {
  int A, B;
  int tA = (int) temp2;
  int tB = (int) temp3;
  int rHA = (int) hum2;
  int rHB = (int) hum3;

  if (tB > tA) {
    A = tB;
  } else {
    A = tA;
  }

  if (rHB > rHA) {
    B = rHB;
  } else {
    B = rHA;
  }

  Serial.print("A: ");
  Serial.println(A);
  Serial.print("B: ");
  Serial.println(B);

  if (A < min_temp) {
    Serial.println(A);
    Serial.println(min_temp);
    Serial.println("Kipas OFF");
    Serial.println("Blower OFF");
    Serial.println("Pompa OFF");
    digitalWrite(kipas, LOW);
    digitalWrite(blower, LOW);
    digitalWrite(pompa, LOW);
  } 
 if (A > max_temp && B > max_hum) {
  Serial.println(A);
    Serial.println(max_temp);
    Serial.println(B);
    Serial.println(max_hum);
    Serial.println("Kipas ON");
    Serial.println("Blower ON");
    Serial.println("Pompa OFF");
    digitalWrite(kipas, HIGH);
    digitalWrite(blower, HIGH);
    digitalWrite(pompa, LOW);
  } 
 if (A > max_temp && B < max_hum) {
  Serial.println(A);
    Serial.println(max_temp);
    Serial.println(B);
    Serial.println(max_hum);
    Serial.println("Kipas ON");
    Serial.println("Blower OFF");
    Serial.println("Pompa ON");
    digitalWrite(kipas, HIGH);
    digitalWrite(blower, LOW);
    digitalWrite(pompa, HIGH);
  }
}
