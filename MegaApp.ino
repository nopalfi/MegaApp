#include <EEPROM.h>

// Tambahkan semua library
#include <TroykaDHT.h>

#include <ArduinoJson.h>

#include <ArduinoHttpClient.h>

#include <WiFiEspAT.h>



// Buat 2 buah sensor objek
DHT dht1(2, DHT21);
DHT dht2(3, DHT21);
// Untuk menyimpan data sensor
String temperature1, temperature2, humadity1, humadity2;

char serverAddress[] = "192.168.14.107";  // server address
int port = 80;

// Inisiasi client wifi dan client http untuk akses internet
WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);
int status = WL_IDLE_STATUS;


void setup() {
  // Untuk parsing json objek
  StaticJsonDocument<200> doc;
  // Serial = mega2560, Serial3 = ESP8266EX
  Serial.begin(115200);
  while (!Serial);

  Serial3.begin(115200);

  // inisiasi dimulai jika esp tersedia
  WiFi.init(Serial3);

  // Jangan lupa mengubah switch menjadi 1,2,3,4 = ON
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  // Ubah SSID wifi dan password
  WiFi.begin("sudo pacman -Syu", "miromiro");
  Serial.println("Waiting for connection to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }
  Serial.println();

  IPAddress ip = WiFi.localIP();
  Serial.println();
  Serial.println("Connected to WiFi network.");
  int Max_rtem = EEPROM.read(0);
  int Max_rhum = EEPROM.read(1);
  int Min_rtem = EEPROM.read(2);
  int Min_rhum = EEPROM.read(3);
  int jeda = EEPROM.read(4);
  double jeda1 = 60000;
  double jeda2 = jeda * jeda1;

  Serial.println(Max_rtem);
  Serial.println(Max_rhum);
  Serial.println(Min_rtem);
  Serial.println(Min_rhum);
  Serial.println(jeda);
  Serial.println(jeda2);
  Serial.println("Baca EEprom");
  int eepromAll = (int) EEPROM.read(0) * (int) EEPROM.read(1) * (int) EEPROM.read(2) * (int) EEPROM.read(3) * (int) EEPROM.read(4);
  if (eepromAll == 0) {
    StaticJsonDocument<200> json;
    Serial.println("making GET request");
    client.get("/config.php");
    
    // read the status code and body of the response
    int statusCode = client.responseStatusCode();
    String response = client.responseBody();
    Serial.print("Status code: ");
    Serial.println(statusCode);
    Serial.print("Response: ");
    Serial.println(response);
    DeserializationError error = deserializeJson(json, response);
  
    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
      
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
    Serial.println("EEpom Dirubah");
    Serial.print("Nilai EEpom =  ");
    Serial.println(eepromAll);
  }
}

void loop() {
  int Max_rtem = EEPROM.read(0);
  int Max_rhum = EEPROM.read(1);
  int Min_rtem = EEPROM.read(2);
  int Min_rhum = EEPROM.read(3);
  int jeda = EEPROM.read(4);
  double jeda1 = 60000;
  double jd = jeda * jeda1;
  Serial.print(Max_rtem);
  Serial.print(Max_rhum);
  Serial.print(Min_rtem);
  Serial.println(Min_rhum);
  
  temperature1 = "";
  temperature2 = "";
  humadity1 = "";
  humadity2 = "";
//  int delayValue = EEPROM.read(5);
  
  dht1.read();
  dht2.read();
  switch (dht1.getState()) {
    case DHT_OK:
      temperature1 = (String) dht1.getTemperatureC();
      humadity1 = (String) dht1.getHumidity();
      break;
    case DHT_ERROR_CHECKSUM:
      temperature1 = "Checksum error";
      humadity1 = "Checksum error";
      break;
    case DHT_ERROR_TIMEOUT:
      temperature1 = "Time out error";
      humadity1 = "Time out error";
      break;
    case DHT_ERROR_NO_REPLY:
      temperature1 = "Sensor not connected";
      humadity1 = "Sensor not connected";
      break;
  }
  switch (dht2.getState()) {
    case DHT_OK:
      temperature2 = (String) dht2.getTemperatureC();
      humadity2 = (String) dht2.getHumidity();
      break;
    case DHT_ERROR_CHECKSUM:
      temperature2 = "Checksum error";
      humadity2 = "Checksum error";
      break;
    case DHT_ERROR_TIMEOUT:
      temperature2 = "Time out error";
      humadity2 = "Time out error";
      break;
    case DHT_ERROR_NO_REPLY:
      temperature2 = "Sensor not connected";
      humadity2 = "Sensor not connected";
      break;
  }

      char url[] = "192.168.14.107";
      int port = 80;
      HttpClient http = HttpClient(client, url, port);    //Declare object of class HTTPClient
      
      String postDatas = postDataSensor(temperature1, temperature2, humadity1, humadity2);
      
      Serial.println("making POST request");
      String contentType = "application/x-www-form-urlencoded";

      http.post("/", contentType, postDatas);
      // read the status code and body of the response
          int statusCode = http.responseStatusCode();
          String response = http.responseBody();

          Serial.print("Status code: ");
          Serial.println(statusCode);
          Serial.print("Response: ");
          Serial.println(response);
          http.stop();
          Serial.println (jeda1);
          Serial.println (jd);
          cekstatus();
  delay(jd);
  
}

void eepromCheck() {
  if (EEPROM.length() == 0) {
    } else {
        for (int i = 0; i < EEPROM.length(); i++) {
          EEPROM.write(i, 0);
        }
    }
}


String postDataSensor(String temp1, String temp2, String hum1, String hum2) {
      String postData;
      //Post Data
      postData = "humidity1=";
      postData += humadity1;
      postData += "&temperature1=";
      postData += temperature1;
      postData += "&humidity2=";
      postData += humadity2;
      postData += "&temperature2=";
      postData += temperature2;  
      return postData;
}

void cekstatus(){
  StaticJsonDocument<200> json;
  Serial.println("making GET request");
  client.get("/config.php");

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
  DeserializationError error = deserializeJson(json, response);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  int statusConfig = (int) json["status"];

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
    char url[] = "192.168.14.107";
    int port = 80;
    HttpClient http = HttpClient(client, url, port);    //Declare object of class HTTPClient
      
    
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
