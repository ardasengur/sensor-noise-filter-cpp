# Sensor Data Noise Filter (Moving Average) 🚀

Bu proje, robotik ve gömülü sistemlerde mesafe sensörlerinden (Ultrasonik, LiDAR vb.) gelen gürültülü verileri yumuşatmak için yazılmış bir C++ uygulamasıdır.

## Neden Gerekli?
Motor sürücüleri ve PID kontrolcüleri, anlık sensör sıçramalarına (spike) sert tepkiler verir. Bu durum motorlarda titremeye (jitter) ve aşırı akım çekimine yol açar. Bu algoritma, donanımsal gürültüleri filtreleyerek sisteme kararlı bir veri akışı sağlar.

## Kullanılan C++ Konseptleri
* **Dinamik Bellek Yönetimi:** Çalışma zamanında `new` ve `delete` kullanılarak veri setine uygun bellek tahsisi.
* **Pointer Aritmetiği:** Dizilerin kopyalanması yerine bellek adresleri üzerinden yüksek performanslı veri manipülasyonu.
* **Struct Yapıları:** Zaman damgası (timestamp), ham veri ve filtrelenmiş verinin tek bir veri bloğunda paketlenmesi.

## Nasıl Çalıştırılır?
Kodu derlemek ve çalıştırmak için:
```bash
g++ main.cpp -o filter
./filter
