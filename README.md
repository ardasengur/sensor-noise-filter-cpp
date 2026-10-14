# Sensor Data Noise Filter (Moving Average)

Bu proje, robotik ve gömülü sistemlerde mesafe sensörlerinden (Ultrasonik, LiDAR vb.) gelen gürültülü verileri yumuşatmak için yazılmış bir C++ uygulamasıdır, hareketli ortalama kullanarak ham verideki düzensizliği dengelemeyi amaçlıyorum. Spike durumunda bir sonraki veriler ile doğrulama yapmasını sağlayarak yanıltıcı sensör verisinin önüne geçilerek en doğru sonuç kullanılacak.

## Neden Gerekli?
Motor sürücüleri ve PID kontrolcüleri, anlık sensör sıçramalarına (spike) sert tepkiler verir. Bu durum motorlarda titremeye (jitter) ve aşırı akım çekimine yol açar. Bu algoritma, donanımsal gürültüleri filtreleyerek sisteme kararlı bir veri akışı sağlar.

## Kullanılan C++ Konseptleri
* **Dinamik Bellek Yönetimi:** Çalışma zamanında `new` ve `delete` kullanılarak veri setine uygun bellek tahsisi.
* **Pointer Aritmetiği:** Dizilerin kopyalanması yerine bellek adresleri üzerinden yüksek performanslı veri manipülasyonu.
* **Struct Yapıları:** Zaman damgası (timestamp), ham veri ve filtrelenmiş verinin tek bir veri bloğunda paketlenmesi.


## Örnek Veri (1 saniyelik)
Zaman(ms)       Ham             Filtreli        Durum
__________________________________________________________
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
320             499.99          500.74
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

## Nasıl Çalıştırılır?
Kodu derlemek ve çalıştırmak için:
```bash
g++ main.cpp -o filter
./filter
