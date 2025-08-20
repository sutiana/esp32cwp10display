# Papan Informasi Cuaca & Jam Digital dengan ESP32 dan Panel P10

Proyek ini adalah sebuah papan informasi multifungsi berbasis **ESP32** yang menampilkan data secara *real-time* pada **4 buah panel LED Matriks P10**. Papan informasi ini mengambil data waktu dari server NTP dan data cuaca terkini dari API OpenWeatherMap, menjadikannya sebuah jam digital dan stasiun cuaca mini yang cerdas dan dinamis.

## Fitur Utama

* **Jam Digital Akurat**: Menampilkan jam, menit, dan detik yang disinkronkan secara otomatis dengan server waktu internet (NTP).
* **Tanggal Lengkap**: Menampilkan hari, tanggal, bulan, dan tahun dalam format Bahasa Indonesia.
* **Informasi Cuaca Lengkap**:
    * Suhu saat ini (°C).
    * Kelembaban udara (%).
    * Deskripsi kondisi cuaca (misalnya "Langit Cerah", "Hujan Ringan", dll.) dalam Bahasa Indonesia.
* **Kualitas Udara (AQI)**: Menampilkan status kualitas udara (misalnya "Baik", "Tidak Sehat", "Berbahaya").
* **Saran Dinamis**: Memberikan rekomendasi teks berjalan yang relevan berdasarkan kondisi cuaca dan kualitas udara (misalnya "Gunakan Masker" atau "Bawa Payung").
* **Tampilan Dinamis**: Informasi ditampilkan secara bergantian dengan efek transisi animasi yang mulus (geser ke atas/bawah dan teks berjalan).
* **Konfigurasi Mudah**: Pengaturan WiFi, kota, dan API Key dapat dengan mudah diubah di bagian atas kode.

## Kebutuhan Perangkat Keras

1.  **ESP32 DEVKIT V1**: Mikrokontroler utama.
2.  **Panel LED Matriks P10**: 4 buah (atau sesuai kebutuhan), tipe HUB12.
3.  **Power Supply 5V**: Minimal 3A, sangat disarankan menggunakan 5A untuk 4 panel agar stabil.
4.  **Kabel Jumper**: Secukupnya untuk menghubungkan ESP32 ke panel P10.
5.  **Kapasitor (Opsional tapi disarankan)**: 1000µF untuk menstabilkan catu daya ESP32.

## Kebutuhan Perangkat Lunak

1.  **Arduino IDE**: Pastikan Anda sudah menginstal [dukungan untuk board ESP32](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-v2/).
2.  **Pustaka (Library) Arduino**:
    * `DMD32`: Untuk mengontrol panel P10.
    * `Arduino_JSON`: Untuk mem-parsing data dari API.

    Anda bisa menginstalnya melalui **Library Manager** di Arduino IDE.

## Penyiapan & Instalasi

### 1. Koneksi Perangkat Keras (Wiring)

Hubungkan pin dari ESP32 ke **konektor input** pada panel P10 pertama.

| ESP32 Pin | Panel P10 (HUB12) | Keterangan     |
| :-------- | :---------------- | :------------- |
| **GND** | GND               | Ground         |
| **GPIO 4** | OE                | Output Enable  |
| **GPIO 16** | B                 | Address B      |
| **GPIO 17** | A                 | Address A      |
| **GPIO 5** | SCLK / LAT        | Latch / Strobe |
| **GPIO 18** | CLK               | Clock          |
| **GPIO 23** | R1                | Data           |

**Penting**: Berikan daya **5V eksternal** yang kuat langsung ke panel P10. Jangan mengambil daya dari pin 5V ESP32 karena tidak akan cukup dan dapat merusak board.

### 2. Dapatkan API Key OpenWeatherMap

1.  Buka situs [OpenWeatherMap](https://openweathermap.org/) dan buat akun gratis.
2.  Setelah masuk, navigasi ke halaman **"API keys"**.
3.  Salin *key* yang tersedia.

### 3. Konfigurasi Kode

Buka file `.ino` di Arduino IDE dan ubah variabel di bagian **"PENGATURAN PENGGUNA"**:

```cpp
// -- PENGATURAN PENGGUNA (SILAKAN UBAH DI SINI) --
const char* ssid = "NAMA_WIFI_ANDA";
const char* password = "PASSWORD_WIFI_ANDA";

String openWeatherMapApiKey = "API_KEY_ANDA_DARI_OPENWEATHERMAP";
String city = "Bekasi"; // Ganti dengan nama kota Anda
```

### 4. Upload ke ESP32

* Pilih board **"DOIT ESP32 DEVKIT V1"** (atau yang sesuai).
* Pilih Port COM yang benar.
* Klik **Upload**.

## Kontribusi

Merasa ada yang bisa ditingkatkan? Silakan buat *fork* dari repositori ini, lakukan perubahan, dan ajukan *pull request*. Semua kontribusi sangat kami hargai!

## Lisensi

Proyek ini dilisensikan di bawah **MIT License**. Lihat file `LICENSE` untuk detail lebih lanjut.
