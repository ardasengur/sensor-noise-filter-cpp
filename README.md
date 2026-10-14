# Sensor Data Noise Filter (Adaptive Moving Average)

![C++](https://img.shields.io/badge/Language-C%2B%2B11%2B-00599C?style=flat-square&logo=c%2B%2B)
![Domain](https://img.shields.io/badge/Domain-Embedded%20%26%20Robotics-success?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-blue?style=flat-square)

Bu proje, robotik ve gömülü sistemlerde mesafe sensörlerinden (Ultrasonik, LiDAR, ToF vb.) gelen gürültülü verileri yumuşatmak ve anlık hatalı sıçramaları (spike) ayıklamak için geliştirilmiş bir **C++ sinyal filtreleme** uygulamasıdır. 

Klasik Hareketli Ortalama (Moving Average) filtrelerinden farklı olarak **İleri Bakışlı Doğrulama (Look-Ahead Spike Validation)** mekanizmasına sahiptir: Ani bir veri değişimi algılandığında hemen karar vermek yerine sonraki okumalarla bu değişimi doğrular. Böylece tek seferlik sensör gürültülerini tamamen yok ederken, sensörün önündeki gerçek fiziksel mesafe değişimlerine (Step Change) faz gecikmesi olmadan anında adapte olur.

---

## Neden Gerekli?
Motor sürücüleri ve PID kontrolcüleri, anlık sensör sıçramalarına (spike) sert tepkiler verir. Bu durum motorlarda titremeye (jitter), mekanik aşınmaya ve aşırı akım çekimine yol açar. Öte yandan, standart bir Moving Average filtresi gerçek bir engel belirdiğinde eski verilerin ortalamasını almaya devam ettiği için tepki gecikmesi (lag) yaratır. 

Bu algoritma iki sorunu aynı anda çözer:
1. **Tekil Gürültü Bastırma:** Anlık hatalı okumaları pencereye dahil etmeden siler.
2. **Dinamik Pencere Sıfırlama:** Kalıcı bir seviye değişimi doğrulandığında eski ortalamayı unutarak yeni seviyeden sıfırdan başlar.

---

## Algoritma Mantığı (Karar Mekanizması)

Filtre, mevcut sensör okuması ile son geçerli ortalama arasındaki fark belirlenen eşik değerini (`threshold = 20.0`) aştığında **Şüpheli Durum** moduna geçer ve bir sonraki veriyi inceler:

| Senaryo | Örnek Veri Akışı | Algoritmanın Kararı ve Aksiyonu |
| :--- | :--- | :--- |
| **Normal Seyir** | `500` $\rightarrow$ `503` $\rightarrow$ `502` | Eşik değerin altında. Standart kayan pencere (Moving Average) uygulanır. |
| **Tekil Gürültü (Spike)** | `501` $\rightarrow$ **`453`** $\rightarrow$ `497` | `453` değeri bir sonraki `497` verisiyle yalanlandı. **`453` silinir**, eski ortalama bozulmadan devam eder. |
| **Gerçek Seviye Değişimi** | `500` $\rightarrow$ **`561`** $\rightarrow$ **`560`** | `561` sıçraması bir sonraki `560` verisiyle doğrulandı. **Eski ortalama tamamen unutulur**, `561` noktasından yeni bir Moving Average başlatılır. |
| **Kararsız Geçiş** | `500` $\rightarrow$ **`600`** $\rightarrow$ **`550`** | Ara değerler netleşene kadar bekleyen döngü devam eder; sinyal kararlı bir seviyeye oturduğunda geçmiş adımlar güncellenir. |

---

## Kullanılan C++ Konseptleri
- **Dinamik Bellek Yönetimi:** Çalışma zamanında `new` ve `delete[]` kullanılarak veri setine uygun bellek tahsisi ve güvenli bellek temizliği.
- **Pointer Aritmetiği:** Dizilerin kopyalanması yerine bellek adresleri üzerinden yüksek performanslı veri manipülasyonu.
- **Struct Yapıları:** Zaman damgası (`timestamp`), ham veri (`FirstValue`) ve filtrelenmiş verinin (`FilteredValue`) tek bir veri bloğunda paketlenmesi.
- **Modern Rastgele Sayı Üretimi (`<random>`):** Mersenne Twister (`mt19937`) motoru, Gauss dip gürültüsü (`normal_distribution`) ve Bernoulli dağılımı ile gerçekçi sensör simülasyonu.

---

## Örnek Çıktı Analizi (Kritik Kırılma Anları)

Aşağıdaki kesitte algoritmanın **50. ms** ve **370. ms**'deki sahte sıçramaları nasıl yuttuğu, **400. ms** ve **750. ms**'deki gerçek mesafe değişimlerine ise gecikmesiz nasıl adapte olduğu görülmektedir:

```text
Zaman(ms)       Ham             Filtreli        Durum
-------------------------------------------------------------------------
0               499.85          499.85
...
40              499.98          501.85
50              453.71          501.85          <-- SPIKE SILINDI (Eski MA devam ediyor)
60              497.96          501.11
...
360             501.52          500.28
370             451.54          500.28          <-- SPIKE SILINDI (Eski MA devam ediyor)
380             499.38          500.07
390             498.37          500.80
400             561.87          561.87          <-- YENI SEVIYE ONAYLANDI (Eski MA unutuldu!)
410             560.92          561.39
420             560.14          560.98
430             485.82          560.98          <-- SPIKE SILINDI (Eski MA devam ediyor)
440             560.08          560.80
...
740             565.09          561.35
750             501.35          501.35          <-- YENI SEVIYE ONAYLANDI (Eski MA unutuldu!)
760             499.22          500.29
```

<details>
<summary><b>📄 1 Saniyelik Tüm Simülasyon Verisini Görüntüle (0 - 1000 ms)</b></summary>

```text
Zaman(ms)       Ham             Filtreli        Durum
-------------------------------------------------------------------------
0               499.85          499.85
10              503.66          501.75
20              503.27          502.26
30              502.50          502.32
40              499.98          501.85
50              453.71          501.85          <-- SPIKE SILINDI
60              497.96          501.11
70              499.89          500.44
80              506.98          501.33
90              499.76          501.29
100             497.23          500.36
110             495.43          499.86
120             499.10          499.70
130             498.88          498.08
140             495.84          497.30
150             499.35          497.72
160             498.08          498.25
170             500.68          498.56
180             494.87          497.76
190             498.01          498.20
200             496.51          497.63
210             497.65          497.54
220             502.46          497.90
230             499.65          498.86
240             501.61          499.58
250             499.34          500.14
260             499.34          500.48
270             499.40          499.87
280             500.33          500.00
290             501.97          500.08
300             501.40          500.49
310             500.02          500.63
320             500.74          500.74
330             500.74          500.82
340             494.71          499.37
350             504.47          499.99
360             501.52          500.28
370             451.54          500.28          <-- SPIKE SILINDI
380             499.38          500.07
390             498.37          500.80
400             561.87          561.87          <-- YENI SEVIYE ONAYLANDI
410             560.92          561.39
420             560.14          560.98
430             485.82          560.98          <-- SPIKE SILINDI
440             560.08          560.80
450             563.63          561.15
460             557.64          560.50
470             559.02          560.27
480             565.12          561.10
490             562.20          561.52
500             560.93          560.98
510             560.09          561.47
520             560.47          561.76
530             561.40          561.02
540             561.79          560.94
550             561.91          561.13
560             558.62          560.84
570             558.75          560.49
580             557.08          559.63
590             557.09          558.69
600             556.42          557.59
610             558.62          557.59
620             556.79          557.20
630             624.21          557.20          <-- SPIKE SILINDI
640             562.74          558.35
650             559.36          558.94
660             564.80          560.18
670             509.44          560.18          <-- SPIKE SILINDI
680             559.09          561.23
690             561.07          560.90
700             561.42          561.31
710             562.13          560.78
720             557.80          560.30
730             599.03          560.30          <-- SPIKE SILINDI
740             565.09          561.35
750             501.35          501.35          <-- YENI SEVIYE ONAYLANDI
760             499.22          500.29
770             502.02          500.86
780             497.61          500.05
790             501.46          500.33
800             498.15          499.69
810             500.63          499.97
820             501.29          499.83
830             501.92          500.69
840             493.14          499.03
850             499.19          499.23
860             503.17          499.74
870             502.26          499.94
880             501.11          499.78
890             429.18          499.78          <-- SPIKE SILINDI
900             499.23          501.11
910             501.07          500.69
920             501.07          500.45
930             498.80          499.99
940             492.76          498.59
950             500.95          498.93
960             495.58          497.83
970             496.06          496.83
980             499.57          496.98
990             498.10          498.05
1000            499.08          497.68
```
</details>

---

## Nasıl Çalıştırılır?

Projeyi klonlayın ve `g++` derleyicisi ile derleyip çalıştırın:

```bash
git clone [https://github.com/ardasengur/sensor-noise-filter-cpp.git](https://github.com/ardasengur/sensor-noise-filter-cpp.git)
cd sensor-noise-filter-cpp
g++ -std=c++11 main.cpp -o filter
./filter
```