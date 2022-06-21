/*
keypad.h kütüphanesi
Bu kütüphane orijinal keypad kütüphanesi proje için yeterli olmadığı için takım üyeleri tarafından yazılmıştır.
keypad'in ilk iki pini analog pinlerden okunmaktadır.
*/

// basılan son tuş birden çok tıklamayı engellemek için depo edilir.
String last_key = "";

// bu fonksiyonda keypad için kullanılacak pinler input ve output pullup olarak ayaralanmaktadır.
void keypad_initialize()
{
  pinMode(4, OUTPUT);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  // UYARI: A4 ve A5 pinleri de keypad için kullanılmakta ancak input
  // veya output olarak ayarlanmalarına gerek olmamaktadır.
}

// bu fonkisyon basılan tuşu döndürmektedir.
String keypad_read()
{
  /*
    keypad'in temel olarak bir digital impulse'ın belli pinlerden çıkıp (A4 A5 D4)
    belli pinlerden (D5 D6 D7 D8) algılanması ile çalıştığı söylenebilir.

    Aşağıdaki boolean değişkenler hangi pinden çıkan gerilimin hangi pinden
    okunduğunu göstermek amacıyla tanımlanmıştır.

        A4  A5  D4
    D5 b25 b35 b45
    D6 b26 b36 b46
    D7 b27 b37 b47
    D8 b28 b38 b48

    Yukarıdaki tabloya göre örneğin A4 ten verilen ve D5 pininden algılanan 
    gerilimin değeri b25 isimli değişkende deppolanacaktır.
  */
  bool b25, b26, b27, b28, b35, b36, b37, b38, b45, b46, b47, b48;

  // Output olan A4 A5 ve D4 pinleri sırasıyla HIGH konumuna getirilip D5 D6 D7 D8 pinlerinden gelen değerler okunmaktadır.

  // A4 pini HIGH konuma getirildi.
  analogWrite(A4, 255);
  analogWrite(A5, 0);
  digitalWrite(4, LOW);
  delay(10);
  // D5 D6 D7 D8 pinleri b2* ile başlayan değişkenlere kaydedildi
  b25 = !digitalRead(5);
  b26 = !digitalRead(6);
  b27 = !digitalRead(7);
  b28 = !digitalRead(8);

  // A5 pini HIGH konuma getirildi.
  analogWrite(A4, 0);
  analogWrite(A5, 255);
  digitalWrite(4, LOW);
  delay(10);
  // D5 D6 D7 D8 pinleri b3* ile başlayan değişkenlere kaydedildi
  b35 = !digitalRead(5);
  b36 = !digitalRead(6);
  b37 = !digitalRead(7);
  b38 = !digitalRead(8);

  // D4 pini HIGH konuma getirildi.
  analogWrite(A4, 0);
  analogWrite(A5, 0);
  digitalWrite(4, HIGH);
  delay(10);
  // D5 D6 D7 D8 pinleri b4* ile başlayan değişkenlere kaydedildi
  b45 = !digitalRead(5);
  b46 = !digitalRead(6);
  b47 = !digitalRead(7);
  b48 = !digitalRead(8);

  /*
    Bu kısımda keypad için deneme yanılma yapılmıştır. 
    bir tuşa basıldığında hangi boolean değerlerin true olduğuna 
    bakılarak aşağıdaki if ifadeleri oluşturulmuştur.

    örneğin keypad'de "*" butonuna tıklandığında b25 ve b35 değişkenleri true olmaktadır.
    o halde b25 ve b35 değişkenleri true olduğunda "*" karakterinin algılandığı söylenebilir.
  */

  if (b25 && b35) return "*";
  if (b26 && b36) return "7";
  if (b27 && b37) return "4";
  if (b28 && b38) return "1";

  if (b25 && b45) return "0";
  if (b26 && b46) return "8";
  if (b27 && b47) return "5";
  if (b28 && b48) return "2";

  if (b35 && b45) return "#";
  if (b36 && b46) return "9";
  if (b37 && b47) return "6";
  if (b38 && b48) return "3";
  /*
    yukarıdaki ifadelerden hiçbiri gerçekleşmediyse bu hiçbir 
    tuşa basılmadığı anlamına gelir. Bu durumlarda dönüş 
    olarak boş string yollanır.
  */
  return "";
}
