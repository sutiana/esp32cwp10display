/*
ARIS SUTIANA
AGUSTUS 2025
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h> // Kembali menggunakan library Arduino_JSON
#include "time.h"
#include <DMD32.h>
#include "fonts/Arial_Black_16.h"
#include "fonts/SystemFont5x7.h"

// -- PENGATURAN PENGGUNA (SILAKAN UBAH DI SINI) --
const char* ssid = "NAMA_WIFI";
const char* password = "PASSWORD_WIFI";

String openWeatherMapApiKey = "API_KEY";
String city = "KOTA_ANDA"; // Contoh: "Jakarta", "Surabaya"
String units = "metric"; // "metric" untuk Celcius, "imperial" untuk Fahrenheit

// Pengaturan Panel P10
#define DISPLAYS_ACROSS 4
#define DISPLAYS_DOWN 1
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);

// Pengaturan Waktu (NTP)
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600 * 7; // WIB adalah GMT+7
const int daylightOffset_sec = 0;

// -- VARIABEL GLOBAL --
String weatherDescription = "Memuat...";
String temperature = "--";
String humidity = "--";
String aqiStatus = "Memuat...";
String saranDinamis = "Jaga Kesehatan Selalu!"; // Variabel baru untuk saran
double lat, lon; // Untuk menyimpan koordinat kota

unsigned long lastWeatherUpdate = 0;

// Variabel untuk mengatur tampilan bergantian
int displayMode = 0; // 0: Jam, 1: Tgl, 2: Suhu/Lembab, 3: Cuaca, 4: AQI, 5: Saran
unsigned long lastDisplaySwitch = 0;

// Timer untuk me-refresh display
hw_timer_t * timer = NULL;

const char* namaHari[] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};
const char* namaBulan[] = {"Jan", "Feb", "Mar", "Apr", "Mei", "Jun", "Jul", "Ags", "Sep", "Okt", "Nov", "Des"};


void IRAM_ATTR triggerScan() {
  dmd.scanDisplayBySPI();
}

// Fungsi untuk koneksi WiFi
void setupWifi() {
  Serial.print("Menghubungkan ke WiFi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi terhubung!");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
}

// Fungsi untuk menerjemahkan kondisi cuaca
String translateWeatherDescription(String description) {
  description.toLowerCase();
  if (description.indexOf("clear") >= 0) return "LANGIT CERAH";
  if (description.indexOf("few clouds") >= 0) return "SEDIKIT BERAWAN";
  if (description.indexOf("scattered") >= 0) return "BERAWAN SEBAGIAN";
  if (description.indexOf("broken clouds") >= 0) return "BERAWAN";
  if (description.indexOf("overcast") >= 0) return "SANGAT BERAWAN";
  if (description.indexOf("shower rain") >= 0) return "HUJAN DERAS";
  if (description.indexOf("light rain") >= 0) return "GERIMIS";
  if (description.indexOf("rain") >= 0) return "HUJAN";
  if (description.indexOf("thunderstorm") >= 0) return "BADAI PETIR";
  if (description.indexOf("snow") >= 0) return "SALJU";
  if (description.indexOf("mist") >= 0) return "KABUT";
  if (description.indexOf("haze") >= 0) return "BERKABUT";
  if (description.indexOf("smoke") >= 0) return "ASAP";
  
  description.toUpperCase(); // Jika tidak ada terjemahan, tampilkan aslinya
  return description;
}

// Fungsi baru untuk membuat saran dinamis
void generateSaran() {
    if (aqiStatus == "TIDAK SEHAT" || aqiStatus == "BERBAHAYA") {
        saranDinamis = "Udara Buruk, Gunakan Masker Saat di Luar Ruangan!";
    } else if (weatherDescription.indexOf("HUJAN") >= 0 || weatherDescription.indexOf("BADAI") >= 0) {
        saranDinamis = "Cuaca Hujan, Jangan Lupa Bawa Payung atau Jas Hujan.";
    } else if (weatherDescription.indexOf("CERAH") >= 0) {
        saranDinamis = "Hari yang Cerah, Gunakan Tabir Surya Jika Beraktivitas di Luar.";
    } else {
        saranDinamis = "Jaga Kesehatan dan Tetap Semangat Menjalani Hari!";
    }
}

// Fungsi untuk mengambil data Kualitas Udara (AQI)
void getAqiData(double lat, double lon) {
  String serverPath = "http://api.openweathermap.org/data/2.5/air_pollution?lat=" + String(lat) + "&lon=" + String(lon) + "&appid=" + openWeatherMapApiKey;
  HTTPClient http;
  http.begin(serverPath.c_str());
  http.setTimeout(10000);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    JSONVar myObject = JSON.parse(payload);
    if (JSON.typeof(myObject) == "undefined") {
      aqiStatus = "Error JSON";
      return;
    }
    int aqi = (int)myObject["list"][0]["main"]["aqi"];
    switch (aqi) {
      case 1: aqiStatus = "BAIK"; break;
      case 2: aqiStatus = "SEDANG"; break;
      case 3: aqiStatus = "KURANG BAIK"; break;
      case 4: aqiStatus = "TIDAK SEHAT"; break;
      case 5: aqiStatus = "BERBAHAYA"; break;
      default: aqiStatus = "TIDAK DIKETAHUI";
    }
    Serial.print("Kualitas Udara (AQI): ");
    Serial.println(aqiStatus);
  } else {
    Serial.print("Gagal mengambil data AQI, Kode Error: ");
    Serial.println(httpResponseCode);
    aqiStatus = "Gagal Update";
  }
  http.end();
}

// Fungsi untuk mengambil data cuaca utama
void getWeatherData() {
  String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&units=" + units + "&appid=" + openWeatherMapApiKey;
  HTTPClient http;
  http.begin(serverPath.c_str());
  http.setTimeout(10000);
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    String payload = http.getString();
    JSONVar myObject = JSON.parse(payload);
    if (JSON.typeof(myObject) == "undefined") {
      weatherDescription = "Error JSON";
      return;
    }
    String descEng = (const char*)myObject["weather"][0]["description"];
    weatherDescription = translateWeatherDescription(descEng); // Terjemahkan
    temperature = String((double)myObject["main"]["temp"], 1);
    humidity = String((int)myObject["main"]["humidity"]);
    lat = (double)myObject["coord"]["lat"];
    lon = (double)myObject["coord"]["lon"];
    
    Serial.println("--- Update Cuaca ---");
    Serial.println("Suhu: " + temperature + " C");
    Serial.println("Kelembaban: " + humidity + " %");
    Serial.println("Kondisi: " + weatherDescription);
    Serial.println("--------------------");
    
    getAqiData(lat, lon);
    generateSaran(); // Panggil fungsi untuk update saran
  } else {
    Serial.print("Gagal mengambil data cuaca, Kode Error: ");
    Serial.println(httpResponseCode);
    weatherDescription = "Gagal Update";
  }
  http.end();
}


void setup() {
  Serial.begin(115200);
  dmd.clearScreen(true);
  dmd.selectFont(SystemFont5x7);
  dmd.drawString(2, 4, "Connecting WiFi...", 18, GRAPHICS_NORMAL);
  
  setupWifi();

  uint8_t cpuClock = ESP.getCpuFreqMHz();
  timer = timerBegin(0, cpuClock, true);
  timerAttachInterrupt(timer, &triggerScan, true);
  timerAlarmWrite(timer, 300, true);
  timerAlarmEnable(timer);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  dmd.clearScreen(true);
  dmd.drawString(2, 4, "Fetching data...", 16, GRAPHICS_NORMAL);
  getWeatherData();
  lastWeatherUpdate = millis();
  lastDisplaySwitch = millis();
}

void loop() {
  // Flag untuk memastikan animasi hanya berjalan sekali per mode
  static bool entryAnimationDone[6] = {true, true, true, true, true, true};

  // Update cuaca setiap 15 menit (900000 ms)
  if (millis() - lastWeatherUpdate > 900000) {
    getWeatherData();
    lastWeatherUpdate = millis();
  }

  // Ganti tampilan setiap 8 detik (8000 ms)
  if (millis() - lastDisplaySwitch > 8000) {
    displayMode++;
    if (displayMode > 5) { // Ada 6 mode sekarang (0-5)
      displayMode = 0;
    }
    // Reset semua flag animasi saat mode berganti
    for(int i=0; i<6; i++) entryAnimationDone[i] = false;
    lastDisplaySwitch = millis();
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    dmd.clearScreen(true);
    char timeString[9];
    char dateString[30];
    
    String tempStr;
    String humStr;
    String weatherStr;
    String aqiStr;

    switch (displayMode) {
      case 0: // Tampilkan Jam
        if (!entryAnimationDone[0]) {
            // Animasi muncul dari bawah ke atas
            for (int y_pos = 16; y_pos >= 0; y_pos--) {
                dmd.clearScreen(true);
                dmd.selectFont(Arial_Black_16);
                dmd.drawString(4, y_pos, "TEI", 3, GRAPHICS_NORMAL);
                getLocalTime(&timeinfo);
                sprintf(timeString, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
                dmd.drawString(55, y_pos, timeString, 8, GRAPHICS_NORMAL);
                delay(25);
            }
            entryAnimationDone[0] = true;
        }

        // Tampilkan jam secara normal
        dmd.selectFont(Arial_Black_16);
        dmd.drawString(4, 0, "TEI", 3, GRAPHICS_NORMAL);
        sprintf(timeString, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        dmd.drawString(55, 0, timeString, 8, GRAPHICS_NORMAL);
        break;

      case 1: // Tampilkan Tanggal
        // [FIX] Animasi menghilang ke bawah dari mode sebelumnya (jam)
        if (!entryAnimationDone[1]) {
            for (int y_offset = 1; y_offset < 17; y_offset++) {
                dmd.clearScreen(true);
                dmd.selectFont(Arial_Black_16);
                dmd.drawString(4, y_offset, "TEI", 3, GRAPHICS_NORMAL);
                getLocalTime(&timeinfo);
                sprintf(timeString, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
                dmd.drawString(55, y_offset, timeString, 8, GRAPHICS_NORMAL);
                delay(25);
            }
            entryAnimationDone[1] = true;
        }

        dmd.selectFont(SystemFont5x7);
        sprintf(dateString, "%s, %d %s %d", 
                namaHari[timeinfo.tm_wday], 
                timeinfo.tm_mday, 
                namaBulan[timeinfo.tm_mon], 
                timeinfo.tm_year + 1900);
        
        dmd.drawMarquee(dateString, strlen(dateString), (32 * DISPLAYS_ACROSS) - 1, 4);
        for (int i = 0; i < strlen(dateString) * 6 + 128; i++) {
            if (millis() - lastDisplaySwitch > 8000) break;
            dmd.stepMarquee(-1, 0);
            delay(35);
        }
        break;

      case 2: // Tampilkan Suhu & Kelembaban
        if (!entryAnimationDone[2]) {
            // Animasi muncul dari bawah ke atas
            tempStr = "Suhu      : " + temperature + " C";
            humStr = "Kelembaban: " + humidity + "%";
            for (int y_pos = 16; y_pos >= 0; y_pos--) {
                dmd.clearScreen(true);
                dmd.selectFont(SystemFont5x7);
                dmd.drawString(2, y_pos, tempStr.c_str(), tempStr.length(), GRAPHICS_NORMAL);
                dmd.drawString(2, y_pos + 8, humStr.c_str(), humStr.length(), GRAPHICS_NORMAL);
                delay(25);
            }
            entryAnimationDone[2] = true;
        }
        
        // Tahan tampilan di posisi akhir
        dmd.selectFont(SystemFont5x7);
        tempStr = "Suhu      : " + temperature + " C";
        humStr = "Kelembaban: " + humidity + "%";
        dmd.drawString(2, 0, tempStr.c_str(), tempStr.length(), GRAPHICS_NORMAL);
        dmd.drawString(2, 8, humStr.c_str(), humStr.length(), GRAPHICS_NORMAL);
        break;

      case 3: // HANYA Tampilkan Kondisi Cuaca
        // [FIX] Animasi menghilang ke bawah dari mode sebelumnya (suhu)
        if (!entryAnimationDone[3]) {
            tempStr = "Suhu      : " + temperature + " C";
            humStr = "Kelembaban: " + humidity + "%";
            for (int y_offset = 1; y_offset < 17; y_offset++) {
                dmd.clearScreen(true);
                dmd.selectFont(SystemFont5x7);
                dmd.drawString(2, y_offset, tempStr.c_str(), tempStr.length(), GRAPHICS_NORMAL);
                dmd.drawString(2, y_offset + 8, humStr.c_str(), humStr.length(), GRAPHICS_NORMAL);
                delay(25);
            }
            entryAnimationDone[3] = true;
        }

        dmd.selectFont(SystemFont5x7);
        weatherStr = "Cuaca Saat Ini: " + weatherDescription;
        dmd.drawMarquee(weatherStr.c_str(), weatherStr.length(), (32 * DISPLAYS_ACROSS) - 1, 4);
        for (int i = 0; i < weatherStr.length() * 6 + 128; i++) {
            if (millis() - lastDisplaySwitch > 8000) break;
            dmd.stepMarquee(-1, 0);
            delay(35);
        }
        break;
        
      case 4: // HANYA Tampilkan Kualitas Udara
        dmd.selectFont(SystemFont5x7);
        aqiStr = "Kondisi Udara " + aqiStatus;
        dmd.drawMarquee(aqiStr.c_str(), aqiStr.length(), (32 * DISPLAYS_ACROSS) - 1, 4);
        for (int i = 0; i < aqiStr.length() * 6 + 128; i++) {
            if (millis() - lastDisplaySwitch > 8000) break;
            dmd.stepMarquee(-1, 0);
            delay(35);
        }
        break;
        
      case 5: // Tampilkan Saran Dinamis
        dmd.selectFont(SystemFont5x7);
        dmd.drawMarquee(saranDinamis.c_str(), saranDinamis.length(), (32 * DISPLAYS_ACROSS) - 1, 4);
        for (int i = 0; i < saranDinamis.length() * 6 + 128; i++) {
            if (millis() - lastDisplaySwitch > 10000) break;
            dmd.stepMarquee(-1, 0);
            delay(35);
        }
        break;
    }
  }
  
  delay(50); 
}
