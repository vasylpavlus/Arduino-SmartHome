#include<String.h>
#include <Wire.h> 
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F,16,2);
#include <SPI.h>
#include <Ethernet.h>
#include <MFRC522.h>
#define SS_PIN 53
#define RST_PIN 5
MFRC522 mfrc522(SS_PIN, RST_PIN);
#include <Bounce.h> 
#include "Arduino.h"
#include "SoftwareSerial.h"  //Емуляція портів rx/tx (для модуля mp3)
#include "DFRobotDFPlayerMini.h" //Робота з mp3 модулем
#include "DHT.h"

#define DHTPIN      41 //пін датчика температури і вологи
#define trigPin     46 //пін ультразвук 11
#define echoPin     44 //пін ультразвук 10
#define daylightPin A3 //пін фотосенсора
#define fadePinKim  11 //пін керування MOSFET транзистором кімнати 44
#define fadePinVan   9 //пін керування MOSFET транзистором ванни 9
#define fadePinKor  10 //пін керування MOSFET транзистором коридору 46
#define fanPin      40 //пін керування вентелятором
#define movSPinKor  37 //пін сенсора руху в коридорі
#define movSPinKim  36 //пін сенсора руху в кімнаті
#define gerkonPin    3 //пін геркону
#define LED_G_Pin    7 //пін зеленого LED над дверима
#define LED_R_Pin    8 //пін червоного LED над дверима
#define servoPin    45 //пін сервопривода на вікні
#define buttonPin   18 //пін кнопки дзвінка

#define max_t       28 //Максимальна температура повітря
#define max_Thres  250 //Рівень спрацювування датчика газу
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial mySoftwareSerial(6, 4); // RX, TX для mp3 плеєра
DFRobotDFPlayerMini myDFPlayer;


int smokeA0 = A4; //Датчик газу
int analogSensor = 0;
int light;
int s_status  = 0;
int sh_status = 0;
int sensorThres = max_Thres; 
long duration, distance; //Ультразвук
int light_Kor = 0; //Освітлення в коридорі (1 - вкл. / 0 - викл.)
int light_Kim = 0; //Освітлення в кімнаті (1 - вкл. / 0 - викл.)
int light_Van = 0; //Освітлення у ванній (1 - вкл. / 0 - викл.)
int light_All = 0; //Освітлення в усіх кімнатах (1 - вкл. / 0 - викл.)
int fun_state = 0; //Стан вентилятора (1 - вкл. / 0 - викл.)
char fun_mode    = 'a'; //Режим вентилятора:    a - automatical (автоматичний), m - manual (ручний)
char system_mode = 'm'; //Режим роботи системи: a - automatical (автоматичний), m - manual (ручний)
unsigned long uidDec, uidDecTemp; //Змінні для зберігання ID мітки в десятковому виді
float h, t; //Вологість та температура
char authorization_result = '0';
int LED_G = LED_G_Pin;
int LED_R = LED_R_Pin;
int Gerkon_pin = gerkonPin;

int knopka_tmp = 0;
int datalightsensor = 0;

int state_Gerkon_pin = 0;

Bounce bouncer = Bounce(buttonPin, 10); //Для дверного двінка
Servo servo;

//===================//

void setup() {  
  servo.attach(servoPin);
  servo.write(0);
  lcd.init();                     
  lcd.backlight();
  lcd.clear();
  lcd.print("Starting...");  
    
  pinMode(LED_R,      OUTPUT);
  pinMode(LED_G,      OUTPUT); 
  pinMode(fadePinKim, OUTPUT);
  pinMode(fadePinKor, OUTPUT);  
  pinMode(fanPin,     OUTPUT);
  pinMode(movSPinKor,  INPUT);
  pinMode(movSPinKim,  INPUT);
  pinMode(smokeA0,     INPUT);
  pinMode(trigPin,    OUTPUT);
  pinMode(echoPin,     INPUT);
  pinMode(Gerkon_pin,  INPUT);
  digitalWrite(Gerkon_pin, HIGH); // активуємо внутрішній підтягуючий резистор виводу
  pinMode(buttonPin,       INPUT);
  digitalWrite(buttonPin,  HIGH); // активуємо внутрішній підтягуючий резистор виводу
     
  SPI.begin();        //Ініціалізація SPI
  mfrc522.PCD_Init(); //Ініціалізація MFRC522
  Serial.begin(38400); // Ініціалізація Bluetooth
  mySoftwareSerial.begin(9600); // Ініціалізація програмного com-порта для mp3
  myDFPlayer.begin(mySoftwareSerial); // Ініціалізація mp3
  myDFPlayer.volume(30);  //Задаємо гучність mp3 (0 ... 30)
  delay(1000);
  myDFPlayer.playMp3Folder(1);  //Play Вітання
  lcd.clear();
  lcd.print("System started!");    
  delay(3000);
  system_mode = 'a'; // "m" Ручний режим роботи системи (блутуз); "a" - автоматичний режим роботи системи
  digitalWrite(LED_G, HIGH);     
   
  //attachInterrupt(5, dzvinok, LOW); //Нажата кнопка дверного двоника

  light_Kor = 0;
  light_Kim = 0;
  light_Van = 0;
  light_All = 0;

  system_mode = 'm'; //Задаємо початковий режим роботи системи
}  

void loop() {
  dzvinok(); //Перевірка чи нажата кнопка дзвінка  
  authorization_result = get_authorization(); //Здійснюємо авторизацію 

  state_Gerkon_pin = digitalRead(Gerkon_pin);
  
  if ((authorization_result == '1') && (system_mode == 'a')) {
     authorization_result = '0';
     system_mode = 'm';
     
     digitalWrite(LED_G, HIGH);
     digitalWrite(LED_R, LOW);     
     myDFPlayer.playMp3Folder(5);
     delay(300);
  }
  else if ((authorization_result == '1') && (system_mode == 'm') || (authorization_result == '1') && (system_mode == 't')) {
     authorization_result = '0';
     system_mode = 'a';
     if (datalightsensor == 1){     
           datalightsensor = 0;
           servo.write(0);
      }
     light_Kim_OFF();
     light_Kor_OFF();
     light_Van_OFF(); 
     digitalWrite(LED_G, LOW);
     digitalWrite(LED_R, HIGH);  
     myDFPlayer.playMp3Folder(4);
     delay(300);
  }   

  get_bluetooth(); //Перевірити наявність команд через Bluetooth   
  get_t_h();       //Виміряти температуру і вологість
  sensor_gas();    //Просканувати наявність газу
  foto_sensor();   //Виміряти освітлення
  lcd.clear();
  lcd.print("Hum. (%):");
  lcd.setCursor(10, 0);
  lcd.print(h);
  lcd.setCursor(0, 1);
  lcd.print("Temp.(C):");
  lcd.setCursor(10, 1);
  lcd.print(t);    
  delay(200);

  //ПОЧАТОК Якщо увімкнутий РУЧНИЙ режим роботи системи
  if(system_mode == 'm') //Якщо увімкнутий ручний режим роботи системи
  {
  if(digitalRead(movSPinKor) == HIGH)  //Перевіряємо датчик руху в коридорі
  {
   if(light_Kor == 0)   //і світло було вимкнуте
   {
    light_Kor_ON(); //analogWrite(fadePinKor, 150);
    light_Kor = 1;
   }
  }
  if(digitalRead(movSPinKor) == LOW)
  {
   if(light_Kor == 1) //і світлу було увімкнуте
   {
     light_Kor_OFF(); //analogWrite(fadePinKor, 0);
     light_Kor = 0;
   }
  }

  if(digitalRead(movSPinKim) == HIGH)  //Перевіряємо датчик руху в кімнаті
  {
   if(light_Kim == 0)   //і світло було вимкнуте
   {
    light_Kim_ON(); //analogWrite(fadePinKim, 150);
    light_Kim = 1;
   }
  }
  if(digitalRead(movSPinKim) == LOW)
  {
   if(light_Kim == 1) //якщо сітло увімкнуте
   {
     light_Kim_OFF(); //analogWrite(fadePinKim, 0);
     light_Kim = 0;
   }
  }
  ultrasound(); //Зчитуємо датчик відстані у ванній
 } 
  //КІНЕЦЬ Якщо увімкнутий РУЧНИЙ режим роботи системи

  
  //ПОЧАТОК Якщо увімкнута сигналізація будинку
  if(system_mode == 'a') //Якщо увімкнутий автоматичний режим роботи системи
  {
  if (digitalRead(movSPinKor) == HIGH || digitalRead(movSPinKim) == HIGH)  //Перевіряємо датчик руху в коридорі та кімнаті
   {     
      lcd.clear();
      lcd.print("Motion detected!");  
      alarm();
   }
   if (state_Gerkon_pin == 0){
      lcd.clear();
      lcd.print("Door open!");  
      alarm();
    }
  } 
  //КІНЕЦЬ Якщо увімкнутий АВТОМАТИЧНИЙ режим роботи системи
}

//========AUTHORIZATION======//
char get_authorization() {
  // Look for new cards
  if ( !mfrc522.PICC_IsNewCardPresent()) {
    return 0;
  }
  // Select one of the cards
  if ( !mfrc522.PICC_ReadCardSerial()) {
    return 0;
  }  
  char card_type; //тип ключа (1 - підходить / 0 - не підходить)
  uidDec = 0;
  // Визначення номера мітки в 10-ковому виді
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    uidDecTemp = mfrc522.uid.uidByte[i];
    uidDec = uidDec * 256 + uidDecTemp;
  }
  Serial.println(uidDec);
  if (uidDec == 1916874702 || uidDec == 2733185742 || uidDec == 1274945819 ) // Порівнюємо код мітки - якщо співпадає, то K/OK, інакще E/Error.
  {
    card_type = '1';  
    lcd.clear();
    lcd.print("Card OK!");  
    myDFPlayer.playMp3Folder(2);  //Play Авторизацію пройдено
    delay(3000);
  }
  else {
    card_type = '0';   
    lcd.clear();
    lcd.print("Bad card!");  
    myDFPlayer.playMp3Folder(3);  //Play Авторизацію не пройдено
    delay(3000);   
  }
  return card_type;
  card_type='0';
  uidDec = 0;  
}

//===========BLUETOOTH===========//
void get_bluetooth() {
  //Перевіряємо наявність команд через Bluetooth  
  if (Serial.available()) 
  {
    char command = Serial.read();
    lcd.clear();
    lcd.print(command);     
    
    switch (command)
    {
      case '0':
       if(fun_mode == 'm') { 
        change_fun(1);
        myDFPlayer.playMp3Folder(6); //Play sound 
        delay(1500);
        fun_state = '1'; //Увімкнути вентелятор
       }
       else 
        myDFPlayer.playMp3Folder(20); //Play Увімкніть ручний режим роботи кондиціонера 
        break;        
      case '1': 
       if(fun_mode == 'm') {
        change_fun(0);
        myDFPlayer.playMp3Folder(7);  //Play sound 
        delay(1500);
        fun_state = '1'; //Вимкнути вентелятор
       }
       else 
        myDFPlayer.playMp3Folder(20); //Play Увімкніть ручний режим роботи кондиціонера 
        break; 
      case '2': //Увімкнути світло в коридорі 
        if (system_mode == 'm') {
        light_Kor_ON();
        light_Kor = 1;
        myDFPlayer.playMp3Folder(8); 
        break;
        }
      case '3': //Вимкнути світло в коридорі 
        if (system_mode == 'm') {
        light_Kor_OFF();
        light_Kor = 0;
        myDFPlayer.playMp3Folder(9);
        break;
        }
      case '4': //Увімкнути світло в ванній 
      if (system_mode == 'm') {
        light_Van_ON();
        light_Van = 1;
        myDFPlayer.playMp3Folder(8);
        break;
      }
      case '5': //Вимкнути світло в ванній 
      if (system_mode == 'm') {
        light_Van_OFF();
        light_Van = 0; 
        myDFPlayer.playMp3Folder(9);
        break;    
      }
       case '6': //Увімкнути світло в кімнаті 
       if (system_mode == 'm') {
        light_Kim_ON();
        light_Kim = 1;
        myDFPlayer.playMp3Folder(8);  //Play sound
        break;
       }
      case '7': //Вимкнути світло в кімнаті
      if (system_mode == 'm') {
        light_Kim_OFF();
        light_Kim = 0;
        myDFPlayer.playMp3Folder(9);  //Play sound
        break; 
      } 
      case '8': //Увімкнути світло усюди
      if (system_mode == 'm') {
        light_Kim_ON();
        light_Kor_ON();
        light_Van_ON();      
        light_All = 1; 
        myDFPlayer.playMp3Folder(8);  //Play sound
        break;    
      }
      case '9': //Вимкнути світло усюди
        light_Kim_OFF();
        light_Kor_OFF();
        light_Van_OFF();      
        light_All = 0;
        myDFPlayer.playMp3Folder(9);  //Play sound
        break;  
      case 'a': //Увімкнути авторежим вентилятора 
        fun_mode = 'a';
        myDFPlayer.playMp3Folder(21);
        break;   
      case 'b': //Увімкнути ручний режим вентилятора 
        fun_mode = 'm'; 
        change_fun(0);
        myDFPlayer.playMp3Folder(22);
        break; 
      case 'c': //Увімкнути автоматичний режим системи 
        system_mode = 't'; 
        myDFPlayer.playMp3Folder(4);
        break;       
       case 'd': //Увімкнути ручний режим системи
        system_mode = 'm'; 
        myDFPlayer.playMp3Folder(5); 
        break; 
    }  
  }
}



//===================//
//====LIGHT_KOR_ON====//
//Увімкнення світла в коридорі
void light_Kor_ON() {
  if (light_Kor == 0) {    
       for(int i=0; i<=150; i++)  //то плавно включаем свет
       {
       analogWrite(fadePinKor, i); 
       delay(5);   //каждые 5мс увелияение на 1
       } 
       light_Kor = 1;
  }       
}
//====LIGHT_KOR_OFF====//
//Вимкнення світла в коридорі
void light_Kor_OFF() {
  if (light_Kor == 1) {    
       for(int i=150; i>=0; i--) //плавно гасим его
       {
       analogWrite(fadePinKor, i); 
       delay(5);
       } 
       light_Kor = 0;
  }       
}

//====LIGHT_KIM_ON====//
//Увімкнення світла в кімнаті
void light_Kim_ON() {
  if (light_Kim == 0) {    
       for(int i=0; i<=150; i++)  //то плавно включаем свет
       {
       analogWrite(fadePinKim, i); 
       delay(5);   //каждые 10мс увелияение на 1
       }
       light_Kim = 1;
  } 
}
//====LIGHT_KIM_OFF====//
//Вимкнення світла в кімнаті
void light_Kim_OFF() {
  if (light_Kim == 1) {  
       for(int i=150; i>=0; i--) //плавно гасим его
       {
       analogWrite(fadePinKim, i); 
       delay(5);
       } 
       light_Kim = 0;
  }       
}

//====LIGHT_VAN_ON====//
//Увімкнення світла у ванній
void light_Van_ON() {
  if (light_Van == 0) {  
       for(int i=0; i<=150; i++)  //то плавно включаем свет
       {
       analogWrite(fadePinVan, i); 
       delay(5);   //каждые 10мс увелияение на 1
       } 
       light_Van = 1;
  }        
}
//====LIGHT_VAN_OFF====//
//Вимкнення світла у ванній
void light_Van_OFF() {
  if (light_Van == 1) {
       for(int i=150; i>=0; i--) //плавно гасимо його
       {
       analogWrite(fadePinVan, i); 
       delay(5);
       } 
       light_Van = 0;
  }  
}
//====CHANGE_FUN====//
//Регулювання вентилятора
char change_fun(int fun_state) { 
     if(fun_state == 1)   //якщо треба увімкнути вентилятор
     {
       digitalWrite(fanPin, HIGH);
       return '1'; //передаємо значення, що вентилятор увімкнуто
     }
     else 
     {
       digitalWrite(fanPin, LOW);
       return '0'; //передаємо значення, що вентилятор вимкнуто
     } 
}

void get_t_h() {      
   h = dht.readHumidity();
   t = dht.readTemperature();
   if(fun_mode=='a') {
   if(t < max_t-1) change_fun(0);
   if(t >= max_t)  change_fun(1);
  } 
}
void sensor_gas(){
  analogSensor = analogRead(smokeA0);
  if (analogSensor > sensorThres)
  {   
    lcd.clear();
    lcd.print("Gas detected!");  
    alarm();
  }
  delay(100);
 }

  void alarm(){
  //Serial.println("GAS detected!");
  for (int i=1; i<10; i++) {
    analogWrite(fadePinKor, 150);
    analogWrite(fadePinVan, 150);
    analogWrite(fadePinKim, 150);
    delay(200);
    analogWrite(fadePinKor, 0);
    analogWrite(fadePinVan, 0);
    analogWrite(fadePinKim, 0);
    delay(200);
  }
  myDFPlayer.volume(10);
  myDFPlayer.playMp3Folder(100);
  myDFPlayer.volume(25);
  delay(5000);   
  }
  
  void foto_sensor()
    {
    light = analogRead(daylightPin);
    //Serial.println(light);
      if(system_mode != 'a'){
        if(light>500){
          datalightsensor = 0;
          //servo.attach(servoPin);
          servo.write(0);
          //servo.detach();
          }
         if(light<500){
           datalightsensor = 1 ;
           //servo.attach(servoPin);  
           servo.write(170);
           //servo.detach();   
          } 
        delay(10);
      }
    }

void ultrasound() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1;
  if (distance < 22) {
   //Serial.println("Distance!");
   light_Van_ON();
   //analogWrite(fadePinVan, 100); 
   }
   else {
   light_Van_OFF();
   }
}

void dzvinok() { //нажата кнопка дзвінка
  if (bouncer.update()) {     //перевірка настання події
    if (bouncer.read() == LOW) {  //якщо кнопка нажата
       lcd.clear();
       lcd.print("Ding-dong!");
       myDFPlayer.playMp3Folder(1000);
       bouncer.rebounce(500); //повторити через 500мс
       delay(2000); 
       }
  }
}

