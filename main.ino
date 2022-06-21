//esp32 için software serial kütüphanesi yüklendi...
#include <SoftwareSerial.h>

// arduinonun bağlanacağı ağın adı şifresi bağlanılacak 
// port bilgisi gibi veriler seçiliyor...
String agAdi = "Ozelag12";
String agSifresi = "btu12345";

String ip = "";
String port = "80";

// esp için 10 ve 11. pinler seçiliyor.
int rxPin = 10;
int txPin = 11;

SoftwareSerial esp(rxPin, txPin);

/*
  Servo kütüphanesi servo motor kontrolü
  "Adafruit_Fingerprint.h" kütüphanesi parmak izi sensörü için import edilmiştir.
   "keypad.h" kütüphanesi ise tarafımızca yazılmıştır ve kodları açıklanacaktır.
*/
#include <Servo.h>
#include <Adafruit_Fingerprint.h>
#include "keypad.h"

// parmak izi sensörü de software serial yapısı kullanmaktadır.
SoftwareSerial mySerial(2, 3);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// keypad için şifremiz "1234"
String pass = "";
String dPass = "1234";

Servo myservo;

bool alarm = false;
int alarm_time = 0;
int timeout_sure = 5000;

// işlemler tamamalandığında buzzerdan ses çalan fonksiyon..
void onay_ses() {
  // tone fonksiyonu ile 12. pinden 1000 hz tonunda 50 ms süreli ses çalınmıştır. 
  tone(12, 1000, 50);
  delay(100);
  tone(12, 1000, 50);
  delay(100);
  tone(12, 1000, 50);
}
// arduinonun ilk çalışma anında kurulumların yapıldığı fonksiyondur.
void setup() {
  //arduino ile bilgisayar arası seri haberleşme sağlanmıştır.
  Serial.begin(9600);
  Serial.println("KOZMOS ALARM SYSTEMS");
  // esp32 modülü kurulumu yapılıyor.
  wifi_setup();
  //kurulum tamamlanınca 3 adet beep sesi duyulacak.
  onay_ses();

  //keypad kurulum işlemi yapılıyor.
  keypad_initialize();
  //parmakizi sensörü kurulumu yapılıyor.
  parmakizi_init();
  //servo 9. pinde çalıştırılacak
  myservo.attach(9);
  //12. pin buzzer için OUTPUT olarak ayarlanıyor.
  pinMode(12, OUTPUT);
  //setup işlemi bitince onay sesi alınıyor.
  onay_ses();
  // NOT: analog pinler için pinmode işlemine ihtiyaç olmamaktadır.
}

// loop fonksiyonu setup işlemlerinden sonra sürekli olarak çağrılacaktır. 
// Tüm sürekli işlemler burada gerçekleşmeli
void loop() {
  // keypad okuma işlemi gerçekleşir.
  get_keypad();
  // parmak izi sensörü okunuyor.
  fingerit();
  // alarm loop fonksionu içerisinde çalınıyor. 
  // Eğer alarm değişkeni true ise alarm çalacaktır.
  if (alarm && (millis()) % 1000 < 20)tone(12, 1000, 1000);
  // alarmın bulunmadığı zamanlarda sensörler okunacaktır
  if (!alarm) {
    int r2 = analogRead(A2);
    // su sensörü okunuyor
    if (analogRead(A0) > 200) {
      // Eğer su terspiti olursa alarm çalacak, 
      // ekrana su algılandı yazılacak ve server'a bilgi gönderilecektir.
      // diğer sensörler için de aynı şey geçerlidir.
      tone(12, 1000, 5000);
      alarm = true;
      Serial.println("Su algılandı");
      // server'a veri yazan fonksiyon wifi_get()
      wifi_get(ip, "/?m=water");
    }
    // yangın sensörü okunuyor
    if (analogRead(A1) < 200) {
      tone(12, 1000, 5000);
      alarm = true;
      Serial.println("Yangin algılandı");
      wifi_get(ip, "/?m=fire");
    }
    // Lazer sensörü okunuyor
    if (r2 > 100) {
      if (analogRead(A2) > 100) {
        tone(12, 1000, 5000);
        alarm = true;
        Serial.println("Giriş algilandi!");
        wifi_get(ip, "/?m=fire");
      }
    }
  }
}

// klavyenin okunma fonksiyonudur.
void get_keypad() {
  // basılan harf okunur
  String key = keypad_read();
  // tuşa basılmadıysa "" cevabı döner o yüzdn kontrol sağlıyoruz.
  if (key != "")
  // Aşağıdaki if Tuşa bir kere basıldığında 
  // birden çok kez yazılmasını engellemektedir.
    if (key != last_key)
    {
      // basılan tuş # ise yazılan şifre kontrol edilir.
      if (key == "#") {
        if (!dPass.compareTo(pass)) {
          Serial.println("şifreler eşleşti!");
          pass = "";
          // şifre eşleştiyse alarm çalıyo mu diye bakılır.
          // eğer alarm çalıyosa kapatılır
          // çalmıyosa kapı açma işlemi yapılır.
          if (alarm) {
            alarm = false;
            onay_ses();
            Serial.println("Alarm kapatıldı!");
            //esp_send("msg=alarm_kapatildi");
          } else {
            onay_ses();
            open_door();
          }
          // server'a bilgi verilir.
          wifi_get(ip, "/?m=valid");
        } else {
          // şifreler eşleşmezse bunun da server'a bildirim işlemi yapılır.
          Serial.println("Yanlış şifre girildi!");
          //esp_send("msg=yanlis%20sifre%20girisi");
          wifi_get(ip, "/?m=unvalid");
          //şifrelerin tekrar girilmesi için sıfırlanması gerekir.
          pass = "";
        }
      } else if (key == "*") {
        // asterix karakteri yanlış yazılan şifreyi silmek için kullanılır.
        pass = "";
      } else {
        // geri kalan tuışlar şifreye eklenir.
        pass += key;
        Serial.println(pass);
      }
    }
  last_key = key;
}

// kapı açma işleminden sorumlu fonksiyon
void open_door() {
  Serial.println("kapi açılıyor!");
  // kapı açılırken servo 180 dereceden 0 dereceye çevirilir.
  for (int pos = 180; pos >= 0; pos -= 1) {
    myservo.write(pos);
    delay(20);
  }
  delay(1000);
  Serial.println("kapi kapanıyor!");
  // kapı kapanırken servo 0 dereceden 180 dereceye çevirilir.
  for (int pos = 0; pos <= 180; pos += 1) {
    myservo.write(pos);
    delay(20);
  }
  delay(50);
}

// parmak izi modülünün başlatılmasından sorumlu fonksiyon
void parmakizi_init() {
  // parmak izi sensörü 57600 baud rate ile başlatılır.
  finger.begin(57600);
  delay(5);
  // parmak izi modülünün çalışıp çalışmadığı kontrol edilir
  // çalışmıyorsa fonksiyondan çıkılır.
  if (finger.verifyPassword()) {
    Serial.print("FINGER: FOUND ");
  } else {
    Serial.println("FINGER: DIDNT FOUND :(");
    return;
  }

  Serial.print(F("PARAMS "));
  finger.getParameters();
  // parmak izi modülünde kayıtlı parmak izleri bulunur
  finger.getTemplateCount();

  // parmak izi sayısı ekranda yazdırılır.
  if (finger.templateCount == 0) Serial.println("NO DATA");
  else {
    Serial.print("CONTAINS "); Serial.print(finger.templateCount); Serial.println(" TEMPLATES");
  }
}

void fingerit() {
  // parmak izi sensöründen veri alınır.
  int finger = getFingerprintIDez();
  // alınan değer -1 ise parmak izi algılanmamıştır.
  if (finger != -1) {
    // parmak izi algılanırsa alarm kapatılır veya kapı açılır.
    Serial.print("ID: "); Serial.println(finger);
    if (alarm) {
      alarm = false;
      onay_ses();
      Serial.println("Alarm kapatıldı!");
    } else {
      onay_ses(); open_door();
    }
    // giriş olduğu bilgisi server'a bildirilir.
    wifi_get(ip, "/?m=valid");
  }
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  // parmak izi resm çekilir.
  uint8_t p = finger.getImage();
  // FINGERPRINT_OK değeri alındığı sürece fonksiyon devam eder
  // devam etmediği takdirde hata vardır ve -1 değeri döndürülür.
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // fonksiyon buraya kadar dönmediyse parmak izi bulunmuştur.
  // parmak izinin id si yazdırılır ve döndürülür.
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}

// wifi kurulum işlemleri gerçekleştirilir.
void wifi_setup() {
  // esp 96000 baud rate ile başlatılır.
  // normalde esp daha yüksek baud ratelerle çalışır ancak arduino ile 
  // eşdeğer çalışması için bu oran azaltıldı.
  esp.begin(9600);
  // server'ın ip adresi değişiklik gösterebileceğinden bilgisayardan alınmalıdır.
  Serial.print("ip adresini giriniz: ");
  while (Serial.available() == 0) delay(100);
  while (Serial.available() > 0) ip += (char)Serial.read(); delay(100);
  while (Serial.available() > 0) ip += (char)Serial.read(); delay(100);
  while (Serial.available() > 0) ip += (char)Serial.read();

  Serial.println(ip);

  // esp ye "AT" komutu yollanır eğer bir sorun yoksa espden "OK" cevabı gelecektir
  esp.println("AT");
  Serial.print("AT[");
  // "OK" komutu aranır. modül hazır olana kadar "OK" komutu gelmeyebilir.
  while (!esp.find("OK")) {
    esp.println("AT");
    Serial.print(".");
  }
  Serial.print("] ");
  Serial.print("[OK] ");
  // esp nin server ve client modları vardır wifiye bağlanabilmek için client moduna alıyoruz.
  esp.println("AT+CWMODE=1");
  Serial.print("CONF[");
  while (!esp.find("OK")) {
    esp.println("AT+CWMODE=1");
    Serial.print(".");
  } Serial.print("] ");
  Serial.print("[Client] ");
  // wifi bağlantı işlemini gerçekleştiriyoruz.
  esp.println("AT+CWJAP=\"" + agAdi + "\",\"" + agSifresi + "\"");
  Serial.print("AG[");
  // esp den "OK" değeri alınırsa kurulum tamamlanmış demektir.
  while (!esp.find("OK")) {
    Serial.print(".");
  } Serial.print("] ");
  Serial.println("[BAGLANDI]");
  delay(100);
}

// bu fonksiyon verilen ip adresinden veri çekmeye yaramaktadır.
// Yapılması gereken şey servera GET request atılmasıdır. Ancak esp bunu direkt desteklememektedir.
// Bunun için TCP protokolü üzerinden serrver'a bağlantı kurulmalı, HTTP GET requesti atılmalıdır.
bool wifi_get(String ip, String dir) {
  Serial.print(dir + ": ");
  
  // TCP protokolü ile server'ın 80. portuna bağlantı sağlanmaktadır.
  esp.println("AT+CIPSTART=\"TCP\",\"" + ip + "\"," + port);

  // Hata kontrolü yapılır ve devam edilir.
  if (esp.find("Error")) {
    Serial.println("AT+CIPSTART Error");
    return false;
  }
  // GET isteği bu şekilde hazırlanmaktadır.
  String metin = "GET " + dir + " HTTP/1.0\r\n";
  metin += "Host: " + ip + "\r\n";
  metin += "Connection: close\r\n";
  metin += "\r\n";
  String rest = "AT+CIPSEND=" + String(metin.length() + 4) + "\r\n";
  // istek espye yollanır.
  esp.println(rest);
  Serial.print("READY[");
  // esp hazır olana kadar beklenir.
  for (int i = 0; i < 5 && !esp.available(); i++) {
    Serial.print(".");
    delay(100);
    if (i == 4) {
      Serial.println("X\n");
      Serial.print("[AT+CIPCLOSE] ");
      esp.println("AT+CIPCLOSE");
      return false;
    }
  } Serial.print("] ");
  Serial.print("OK[");
  for (int i = 0; i < 5 && !esp.find("OK"); i++) {
    Serial.print(".");//delay(100);
    if (i == 4) {
      Serial.println("X\n");
      Serial.print("[AT+CIPCLOSE] ");
      esp.println("AT+CIPCLOSE");
      return false;
    }
  }
  Serial.print("] ");
  Serial.print("bulundu! ");

  esp.println(metin); delay(100);

  Serial.print("[AT+CIPCLOSE] ");
  esp.println("AT+CIPCLOSE");

  while (!esp.available()) delay(100);
  Serial.print("TRUE[");
  // serverdan esp'ye {TRUE} değerinin gelip gelmediği bakılır.
  // eğer geldiyse veriler server'a başarılı bir şekilde aktarılmıştır.
  for (int i = 0; i < 5 && !esp.find("{TRUE}"); i++) {
    Serial.print(".");
    if (i == 4) {
      Serial.println("X\n");
      return false;
    }
  }
  Serial.println("] [TRUE]");
}
