# Sensor Data Noise Filter (Real-Time Adaptive Moving Average)

![C++](https://img.shields.io/badge/Language-C%2B%2B11%2B-00599C?style=flat-square&logo=c%2B%2B)
![Architecture](https://img.shields.io/badge/Architecture-O(1)%20Ring%20Buffer%20%26%20FSM-orange?style=flat-square)
![Domain](https://img.shields.io/badge/Domain-Embedded%20%26%20Robotics-success?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-blue?style=flat-square)

Bu proje, robotik ve gömülü sistemlerde mesafe sensörlerinden (Ultrasonik, LiDAR, ToF vb.) gelen gürültülü verileri **gerçek zamanlı (real-time streaming)** olarak yumuşatmak ve anlık hatalı sıçramaları (spike) ayıklamak için geliştirilmiş, nesne yönelimli (OOP) bir **C++ sinyal filtreleme** kütüphanesidir.

Klasik Hareketli Ortalama (Moving Average) filtrelerinden farklı olarak **Durum Makinesi Tabanlı Sıçrama Doğrulama (State-Machine Spike Validation)** ve **$O(1)$ Dairesel Tampon (Ring Buffer)** mimarisine sahiptir. Gelecekteki veri dizisine ihtiyaç duymadan verileri tek tek (`update(rawReading)`) işler: Ani bir sıçrama algılandığında veriyi şüpheli olarak beklemeye alır ve bir sonraki okumayla doğrular. Böylece tek seferlik sensör gürültülerini tamamen yok ederken, gerçek fiziksel mesafe değişimlerine (Step Change) anında adapte olur.

---

## Neden Gerekli?
Motor sürücüleri ve PID kontrolcüleri, anlık sensör sıçramalarına (spike) sert tepkiler verir. Bu durum motorlarda titremeye (jitter), mekanik aşınmaya ve aşırı akım çekimine yol açar. Öte yandan, standart bir Moving Average filtresi gerçek bir engel belirdiğinde eski verilerin ortalamasını almaya devam ettiği için tepki gecikmesi (lag) yaratır.

Bu algoritma gömülü sistemlerdeki üç temel sorunu aynı anda çözer:
1. **Gerçek Zamanlı Sıçrama Koruması (State Machine):** Şüpheli okuma geldiğinde motora/kontrolcüye son güvenli ortalamayı vermeye devam eder, bir sonraki adımda sıçrama yalanlanırsa gürültüyü pencereye hiç sokmadan siler.
2. **Dinamik Pencere Sıfırlama:** Kalıcı bir seviye değişimi doğrulandığında eski ortalamayı tamamen unutarak (`resetBuffer()`) yeni seviyeden sıfırdan başlar.
3. **$O(1)$ Zaman ve Bellek Karmaşıklığı (Ring Buffer):** Geçmişteki tüm verileri bellekte tutmak yerine yalnızca pencere boyutu kadar (`windowSize`) dairesel bir dizi ve kayan toplam (`runningSum`) tutar. Mikrodenetleyicilerde (Arduino, STM32, ESP32) RAM şişmesine veya döngü gecikmesine yol açmaz.

---

## Algoritma Mantığı (Durum Makinesi Karar Mekanizması)

Filtre, `STATE_NORMAL` ve `STATE_SUSPECT` olmak üzere iki durumlu bir sonlu durum makinesi (FSM) olarak çalışır. Mevcut okuma ile son geçerli ortalama arasındaki fark eşik değerini (`threshold = 20.0`) aştığında `STATE_SUSPECT` moduna geçilir:

| Durum / Senaryo | Gerçek Zamanlı Veri Akışı | Filtrenin Kararı ve Aksiyonu (`update()`) |
| :--- | :--- | :--- |
| **Normal Seyir** | `498.49` $\rightarrow$ `498.65` | Fark eşik değerin altında. Veri doğrudan **Ring Buffer**'a eklenir ve kayan toplam ($O(1)$) ile yeni ortalama döndürülür. |
| **Şüpheli Veri Yakalama** | `498.65` $\rightarrow$ **`551.32`** | Ani sıçrama! `551.32` gözaltına alınır (`STATE_SUSPECT`). Doğrulama gelene kadar sisteme son güvenli ortalama (**`498.57`**) döndürülür. |
| **Arka Arkaya Kararsız Spike** | `551.32` $\rightarrow$ **`444.51`** $\rightarrow$ `501.99` | İki ardışık zıt spike (`551` ve `444`) boyunca güvenli ortalama (`498.57`) korunur; `501.99` geldiğinde her iki spike da reddedilir ve normal akışa dönülür. |
| **Gerçek Seviye Değişimi** | `504.53` $\rightarrow$ **`556.68`** $\rightarrow$ **`562.62`** | `556.68` beklendikten hemen sonra gelen `562.62` yeni seviyeyi doğrular. **Ring Buffer sıfırlanır (eski MA unutulur)** ve iki yeni verinin ortalaması (**`559.65`**) ile yeni seviyeden devam edilir. |

---

## Mimari ve Kullanılan C++ Konseptleri
- **Nesne Yönelimli Tasarım (OOP) & Kapsülleme:** Filtre durumunun, tampon belleğin ve parametrelerin `AdaptiveMovingAverage` sınıfı içinde kapsüllenmesi.
- **Ring Buffer (Dairesel Tampon) & Kayan Toplam:** Modulo aritmetiği (`head = (head + 1) % windowSize`) ve `runningSum` ile her adımda döngü kurmadan $O(1)$ sürede ortalama hesabı.
- **Sonlu Durum Makinesi (Finite State Machine - FSM):** `enum State` ve `enum FilterStatus` yapılarıyla nedensel (causal), gerçek zamanlı karar yönetimi.
- **RAII & Dinamik Bellek Yönetimi:** Constructor (`AdaptiveMovingAverage`) içinde yalnızca pencere boyutu kadar bellek tahsisi (`new[]`) ve Destructor (`~AdaptiveMovingAverage`) içinde güvenli bellek temizliği (`delete[]`).

---

## Gerçek Sensörlerle Entegrasyon Rehberi (Nasıl Kullanılır?)

`AdaptiveMovingAverage` sınıfı herhangi bir veri dizisine bağımlı değildir; sensörden okuduğunuz ham değeri `filter.update(hamVeri)` metoduna göndermeniz yeterlidir.

### 1. Arduino / ESP32 / STM32 (Örn: HC-SR04 Ultrasonik veya VL53L1X ToF Sensörü)
Mikrodenetleyici projelerinde `AdaptiveMovingAverage` sınıfını kodunuza dahil ettikten sonra `loop()` içinde doğrudan kullanabilirsiniz:

```cpp
// Pencere Boyutu: 5 örnek, Sıçrama Eşiği (Threshold): 20.0 mm (veya cm)
AdaptiveMovingAverage distanceFilter(5, 20.0f);

void setup() {
    Serial.begin(115200);
    // Sensör başlatma kodları (Örn: sensor.init();)
}

void loop() {
    // 1. Sensörden anlık ham mesafeyi oku
    float rawDistance = readUltrasonicSensor(); // veya sensor.readRangeContinuousMillimeters();

    // 2. Ham veriyi filtreye gönder ve temizlenmiş çıktıyı anında al
    float cleanDistance = distanceFilter.update(rawDistance);

    // 3. Temiz veriyi motor sürücüye veya PID kontrolcüye gönder
    pidController.compute(cleanDistance);

    delay(10); // 100 Hz (10 ms) örnekleme periyodu
}
```

### 2. Standart C++ / ROS (Robot Operating System) / Linux Gömülü Sistemler
Sensör verilerinin bir `while` döngüsü veya callback fonksiyonu ile geldiği sistemlerde:

```cpp
// Nesneyi oluştur (Pencere: 5, Eşik: 20.0 birim)
AdaptiveMovingAverage lidarFilter(5, 20.0f);

// Sensörden her yeni paket geldiğinde çalışan fonksiyon / döngü:
void onSensorDataReceived(float rawMeasurement) {
    float filteredValue = lidarFilter.update(rawMeasurement);

    // İsteğe bağlı: Filtrenin o anki kararını kontrol et
    if (lidarFilter.getStatus() == STATUS_NEW_LEVEL) {
        std::cout << "Sensör önünde yeni bir engel/seviye doğrulandı: " << filteredValue << std::endl;
    }
}
```

> **Parametre Ayarı İpucu (`windowSize` ve `threshold`):**
> - **`windowSize` (Örn: `5` - `10`):** Küçük değerler daha hızlı tepki verir, büyük değerler sinyali daha pürüzsüz yapar.
> - **`threshold` (Örn: `20.0f`):** Sensörünüzün normal dip gürültüsünün (standart sapmasının) yaklaşık **4-5 katı** olarak seçilmelidir. Örneğin sensörünüz durduğu yerde `±3 cm` oynuyorsa, `threshold` değerini `15.0f` veya `20.0f` yapmak; normal dalgalanmaların ortalamaya girmesini, `20 cm` üzeri ani sıçramaların ise doğrulama mekanizmasına takılmasını sağlar.

---

## Örnek Çıktı Analizi (Kritik Kırılma Anları)

Aşağıdaki gerçek zamanlı simülasyon kesitinde;
- **20. ve 30. ms**'de üst üste gelen iki ters yönlü spike'ın (`551.32` ve `444.51`) nasıl bekletilip **40. ms**'de tamamen reddedildiği,
- **400. ms** ve **750. ms**'deki kalıcı seviye değişimlerinin ise sadece 1 örnek (10 ms) doğrulama sonrasında eski ortalama unutularak nasıl yakalandığı görülmektedir:

```text
Zaman(ms)       Ham             Filtreli        Durum (Real-Time Karar)
----------------------------------------------------------------------------------
0               498.49          498.49
10              498.65          498.57
20              551.32          498.57          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
30              444.51          498.57          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
40              501.99          499.71          <-- [SPIKE SILINDI] Onceki supheli veri reddedildi!
50              500.48          499.90
...
240             500.79          500.55
250             442.68          500.55          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
260             498.45          500.62          <-- [SPIKE SILINDI] Onceki supheli veri reddedildi!
...
390             504.53          499.78
400             556.68          499.78          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
410             562.62          559.65          <-- [YENI SEVIYE] Dogrulandi! Eski MA unutuldu!
420             560.57          559.95
...
740             558.24          559.81
750             616.52          559.81          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
760             621.23          618.87          <-- [YENI SEVIYE] Dogrulandi! Eski MA unutuldu!
770             658.42          618.87          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
780             623.24          620.33          <-- [SPIKE SILINDI] Onceki supheli veri reddedildi!
```

<details>
<summary><b>📄 1 Saniyelik Tüm Simülasyon Verisini Görüntüle (0 - 1000 ms)</b></summary>

```text
Zaman(ms)       Ham             Filtreli        Durum (Real-Time Karar)
----------------------------------------------------------------------------------
0               498.49          498.49
10              498.65          498.57
20              551.32          498.57          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
30              444.51          498.57          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
40              501.99          499.71          <-- [SPIKE SILINDI] Onceki supheli veri reddedildi!
50              500.48          499.90
60              495.41          499.00
70              500.34          499.37
80              502.99          500.24
90              501.89          500.22
100             502.92          500.71
110             495.78          500.78
120             500.36          500.79
130             500.03          500.20
140             498.27          499.47
150             496.40          498.17
160             500.89          499.19
170             501.13          499.35
180             504.99          500.34
190             496.85          500.05
200             498.12          500.40
210             501.12          500.44
220             500.98          500.41
230             501.75          499.76
240             500.79          500.55
250             442.68          500.55          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
260             498.45          500.62          <-- [SPIKE SILINDI] Onceki supheli veri reddedildi!
270             503.54          501.10
280             498.21          500.55
290             499.33          500.06
300             498.58          499.62
310             501.08          500.15
320             503.63          500.17
330             504.70          501.47
340             501.28          501.86
350             496.03          501.35
360             500.49          501.23
370             498.30          500.16
380             499.57          499.13
390             504.53          499.78
400             556.68          499.78          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
410             562.62          559.65          <-- [YENI SEVIYE] Dogrulandi! Eski MA unutuldu!
420             560.57          559.95
430             562.04          560.48
440             561.14          560.61
450             558.21          560.92
460             563.82          561.16
470             559.74          560.99
480             562.80          561.14
490             561.34          561.18
500             561.28          561.80
510             556.64          560.36
520             562.18          560.85
530             562.72          560.83
540             561.94          560.95
550             558.58          560.41
560             563.16          561.72
570             558.95          561.07
580             559.65          560.46
590             632.78          560.46          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
600             560.89          560.25          <-- [SPIKE SILINDI] Onceki supheli veri reddedildi!
610             563.21          561.17
620             559.62          560.46
630             606.53          560.46          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
640             557.92          560.26          <-- [SPIKE SILINDI] Onceki supheli veri reddedildi!
650             564.09          561.15
660             560.38          561.05
670             521.15          561.05          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
680             561.62          560.73          <-- [SPIKE SILINDI] Onceki supheli veri reddedildi!
690             611.23          560.73          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
700             562.26          561.25          <-- [SPIKE SILINDI] Onceki supheli veri reddedildi!
710             559.07          561.48
720             560.29          560.72
730             559.19          560.48
740             558.24          559.81
750             616.52          559.81          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
760             621.23          618.87          <-- [YENI SEVIYE] Dogrulandi! Eski MA unutuldu!
770             658.42          618.87          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
780             623.24          620.33          <-- [SPIKE SILINDI] Onceki supheli veri reddedildi!
790             619.36          620.09
800             619.59          619.99
810             617.59          620.20
820             621.15          620.18
830             692.84          620.18          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
840             616.33          618.80          <-- [SPIKE SILINDI] Onceki supheli veri reddedildi!
850             681.56          618.80          <-- [BEKLEMEDE] Supheli veri! Dogrulama bekleniyor...
860             618.30          618.59          <-- [SPIKE SILINDI] Onceki supheli veri reddedildi!
870             617.38          618.15
880             619.35          618.50
890             624.56          619.18
900             625.09          620.93
910             618.54          620.98
920             620.83          621.67
930             620.40          621.88
940             619.08          620.79
950             616.51          619.07
960             620.33          619.43
970             618.60          618.98
980             618.90          618.68
990             620.38          618.94
1000            621.50          619.94
```
</details>

---

## Simülasyonu Çalıştırma

Projeyi klonlayın ve `g++` derleyicisi ile simülasyonu derleyip çalıştırın:

```bash
git clone [https://github.com/ardasengur/sensor-noise-filter-cpp.git](https://github.com/ardasengur/sensor-noise-filter-cpp.git)
cd sensor-noise-filter-cpp
g++ -std=c++11 main.cpp -o filter
./filter
```