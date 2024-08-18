/*
  Измерение температуры, влажности на уровне лотка (BME280 - нулевой)
  Измерение температуры вблизи внутреннего тена (DS18B20 - первый)
  Измерение наружной температуры в помещении (DS18B20 - второй) 
  Измерение атмосферного давления (BME280 - опция)

  PID регулятор температуры: На основе показаний датчиков температуры
                             {нулевой - основной, первый - вспомогательный}

                             Регулирует подвод тепла увеличивая или уменьшая ток тена
                             В качестве тена могут быть лампы накаливания
                             Регулировка посредством ШИМ (источник тока - аккумулятор 12в)

  На плате есть датчик освещенности, кот. используется для регулировки светодиодных лент

  RTC таймер DS с батарейкой можно использовать для настройки режимов работы всей установки
  на весь технологический цикл (основной алгоритм установки)

  Включать-выключать вентилятор, свет, нагрев и прочие узлы можно посредством блока реле                                                        
*/

//#define DEBUG_OUT

#define PWM_AMOUNT  3

//--- DHT Temperature & Humidity Sensor ------------------------------------------------------------
//--- DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
//--- Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 4     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;

//--- RTC library DS1307 --------------------------------------------------------------------------- 
#include <DS1307.h>
DS1307 rtc(SDA, SCL);

//--- Gyver библиотека таймеров --------------------------------------------------------------------
/* Как использовать
void setMicros(bool mode);      // включить микросекундный режим (true)
void setTimerMode();            // установить в режим таймера (остановится после срабатывания)
void setPeriodMode();           // установить в режим периода (перезапустится после срабатывания)
void setTime(uint32_t prd);     // установить время
void attach(void (*handler)()); // подключить коллбэк
void detach();                  // отключить коллбэк
void start();                   // запустить/перезапустить таймер
void resume();                  // продолжить после остановки
void stop();                    // остановить/приостановить таймер
void force();                   // принудительно переполнить таймер

// в режиме периода однократно вернёт true при каждом периоде
// в режиме таймера будет возвращать true при срабатывании
bool tick();

bool ready();           // однократно вернёт true при срабатывании (флаг)
bool elapsed();         // всегда возвращает true при срабатывании
bool active();          // работает ли таймер (start/resume)
bool status();          // elapsed+active: работает ли таймер + не сработал ли он

uint32_t timeLeft();    // остаток времени в мс
uint8_t timeLeft8();    // остаток времени в 0-255
uint16_t timeLeft16();  // остаток времени в 0-65535  
 */
//--------------------------------------------------------------------------------------------------
#include <TimerMs.h>
// (период, мс), (0 не запущен / 1 запущен), (режим: 0 период / 1 таймер)
TimerMs tmr(2000, 0, 0);   //--- DS опрашивать 1 раз //в секунду
TimerMs tmr2(2000, 0, 0);  //--- BME280 - 1 раз в //2 секунды
TimerMs tmr3(2000, 0, 0);  //--- Photosensor 1 раз в //5 секунд
TimerMs tmr4(2000, 0, 0);  //--- RTC опрос 1 раз в //5 секунд
TimerMs tmr5(2000, 0, 0);  //--- DHT опрос 1 раз в //5 секунд

TimerMs tmrOut(2000, 0, 0);//--- Для вывода

/*
//--- Вместо Serial -------------------------------------------------------------------------------- 
//#define MU_STREAM     // подключить Stream.h (readString, readBytes...)
#define MU_PRINT      // подключить Print.h (print, println)
//#define MU_TX_BUF 64  // буфер отправки. По умолч. 8. Можно отключить (0)
//#define MU_RX_BUF 64  // буфер приёма. По умолч. 8. Можно отключить (0)
#include <MicroSerial.h>
MicroSerial uart;
*/

//---   Для работы с датчиком DS18B20  -------------------------------------------------------------
#include <microDS18B20.h>
#define DS_SENSOR_AMOUNT  2
uint8_t addr[][8]= {{0x28, 0xEE, 0x1A, 0x1B, 0x2F, 0x16, 0x2, 0x78},  //--- на шилде
                    {0x28, 0xEE, 0xAE, 0x47, 0x30, 0x16, 0x1, 0x15},  //--- второй канал
                   }; 
// указываем DS_ADDR_MODE для подключения блока адресации
// и создаём массив датчиков на пине DS_PIN
#define DS_PIN  8  
MicroDS18B20<DS_PIN, DS_ADDR_MODE> ds[DS_SENSOR_AMOUNT];
#include <DS_raw.h>
#include <microOneWire.h>

//--------------------------------------------------------------------------------------------------
//   Пример индивидуальных настроек датчика под ваше применение
//   См. константы в GyverBME280.h , стандартные настройки можно изменить там же в классе GyverMBE280
//   Настройки вступают в силу только ПОСЛЕ .begin();
//--------------------------------------------------------------------------------------------------
#include <GyverBME280.h>                            // Подключение библиотеки
GyverBME280 bme;                                    // Создание обьекта bme

//--------------------------------------------------------------------------------------------------
//   Фоторезистор
//--------------------------------------------------------------------------------------------------
#define PIN_PHOTORESISTOR A3

//#define GRAD_SIMB   " °C"
#define GRAD_SIMB   " ^C"

//==================================================================================================
typedef struct {
  uint32_t  prt;  //--- etap:3 bit (3 или 4); day: 5 bit (~ 17); rot: 8 bit (>= 102)      
  uint32_t  alarm;//--- Мл. 4 бита - алармы {гл. температура, гл. влажность, свет, вент.}
}stPortVal;
stPortVal portval;

typedef struct {
  char* dwk;
  char* dat;
  char* tim;  
}stDATETIME_t;
stDATETIME_t datetime;

typedef struct {
  float t;
  float h;
  float p;  
}stBME280_t;
stBME280_t clim;

typedef struct {
  float t;
  float h;
}stDHT_t;
stDHT_t my_dht;

typedef struct {
  uint32_t etap;  //--- etap (0..3)
  uint32_t day;   //--- day  (0..17, 20)
  uint32_t rot;   //--- rot  (0..102, 116)  
}stHod_t;
stHod_t hod;

float ds_t[DS_SENSOR_AMOUNT];

float pwm[PWM_AMOUNT];

uint16_t photo_sens;  

//==================================================================================================


  
void setup() {
  
  //--- Запуск последовательного порта  
  Serial.begin(19200);
  pinMode(13, 1);

  pwm[0] = 0.25;
  pwm[1] = 0.50;
  pwm[2] = 0.75;  

  //--- Настраиваем port
  hod.etap = 0;
  hod.day = 0;
  hod.rot = 0;
  portval.prt   = 0x00000000; //0x7166;
  portval.alarm = 0x00000000; 

  //--- Настраиваем DHT11
  DHT_Setup();
  
  //--- настраиваем режимы BME280
  bme.setFilter(FILTER_COEF_8);                     // Настраиваем коофициент фильтрации
  bme.setTempOversampling(OVERSAMPLING_8);          // Настраиваем передискретизацию для датчика температуры
  bme.setPressOversampling(OVERSAMPLING_16);        // Настраиваем передискретизацию для датчика давления
  bme.setStandbyTime(STANDBY_250MS);                // Устанавливаем время сна между измерениями (у нас обычный циклический режим)
  bme.begin();                                      // Если на этом настройки окончены - инициализируем датчик

  //--- устанавливаем адреса на датчики DS18B20
  for (int i = 0; i < DS_SENSOR_AMOUNT; i++) {
    ds[i].setAddress(addr[i]);
  }  

  // Initialize the rtc object
  rtc.begin();  
  // Set the clock to run-mode
  rtc.halt(false);  
  // The following lines can be uncommented to set the time
  rtc.setDOW(THURSDAY);       // Set Day-of-Week to SUNDAY
  rtc.setTime(14, 11, 0);     // Set the time to 12:00:00 (24hr format)
  rtc.setDate(24, 8, 2023);   // Set the date to October 3th, 2010  

  //--- таймеры в режиме периодического срабатывания
  tmr.setPeriodMode();                              // для DS
  tmr.start();

  tmr2.setPeriodMode();                             // Для BME280
  tmr2.start();

  tmr3.setPeriodMode();                             // Для Photosensor
  tmr3.start(); 

  tmr4.setPeriodMode();                             // Для снятия показаний RTC
  tmr4.start();   

  tmr5.setPeriodMode();                             // Для DHT
  tmr5.start();

  tmrOut.setPeriodMode();                           // Для вывода
  tmrOut.start();
}

//===================================================================================================

void loop() {

  //--- Работа на прием из Serial -------------------------------------------
  while (Serial.available()) Serial.write(Serial.read());
  //-----------------------------------------------------------------------

  //--- Получение данных из RTC -------------------------------------------  
  Work_RTC();
  //-----------------------------------------------------------------------  

  //--- Опрос датчика DHT -------------------------------------------------
  Work_DHT();
  //-----------------------------------------------------------------------
  
  //--- Получение результата фотосенсора ----------------------------------  
  Work_Photosensor();
  //-----------------------------------------------------------------------  

  //--- Получение результата преобразования от датчиков DS18B20 -----------  
  Work_DS18B20();
  //-----------------------------------------------------------------------

  //--- Опрос датчика BME280 ----------------------------------------------
  Work_BME280();
  //-----------------------------------------------------------------------



  //--- Вывод всех данных -------------------------------------------------
#ifndef DEBUG_OUT     
  Out_Allchain() ;
#endif
  
#ifdef DEBUG_OUT   
  Out_All();
#endif  
  //-----------------------------------------------------------------------

  
  
}

//------------------------------------------------------------------------

void Work_Hod() {
  hod.rot += 1;
  if ((hod.rot % 6) == 0) {    
    hod.day += 1;
    
    if(hod.day < 3) {
      hod.etap = 0;
    }else
    if(hod.day >= 3 && hod.day < 16) {
      hod.etap = 1;      
    }else
    if(hod.day >= 16 && hod.day < 18) {
      hod.etap = 2;
    }else
    if(hod.day >= 17 && hod.day < 20) {
      hod.etap = 3;
    }else {
      hod.rot = 0;
      hod.day = 0;
      hod.etap = 0;
    }    
  }  

  //--- etap:3 bit (3 или 4); day: 5 bit (~ 17); rot: 8 bit (>= 102) 

  portval.prt = 0x00000000;
  uint32_t e = hod.etap;
  e <<= 13; 
  portval.prt = e;
  
  uint32_t d = hod.day;
  d <<= 8;
  portval.prt += d;
  
  portval.prt += hod.rot; 
}

void DHT_Setup() {
    dht.begin();
#ifdef DEBUG_OUT      
    Serial.println(F("DHTxx Unified Sensor Example"));
#endif    
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);

#ifdef DEBUG_OUT    
    Serial.println(F("------------------------------------"));
    Serial.println(F("Temperature Sensor"));
    Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
    Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
    Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
    Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
    Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
    Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
    Serial.println(F("------------------------------------"));
#endif    

    // Print humidity sensor details.
    dht.humidity().getSensor(&sensor);

#ifdef DEBUG_OUT    
    Serial.println(F("Humidity Sensor"));
    Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
    Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
    Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
    Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
    Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
    Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
    Serial.println(F("------------------------------------"));
#endif
  
    // Set delay between sensor readings based on sensor details.
    delayMS = sensor.min_delay / 1000;  
}

void Work_DHT() {
  if (tmr5.tick()) {  

    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
#ifdef DEBUG_OUT        
      Serial.println(F("DHT: Error reading temperature!"));
#endif      
    }
    else {
      my_dht.t = event.temperature;       
    }
    
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
#ifdef DEBUG_OUT        
      Serial.println(F("DHT: Error reading humidity!"));
#endif      
    }
    else {
      my_dht.h = event.relative_humidity;
    }
  }  
}


void Work_RTC() {
  if (tmr4.tick()) {    
    // День недели
    datetime.dwk = rtc.getDOWStr();
 
    // Дата
    datetime.dat = rtc.getDateStr();

    // Время
    datetime.tim = rtc.getTimeStr(); 
  }
}

void Work_BME280() {
  if (tmr2.tick()) {    
    // получаем данные из BME280
    clim.t = bme.readTemperature();
    clim.h = bme.readHumidity();
    clim.p = pressureToMmHg(bme.readPressure());
  }
} 

/*
  //--- В какой последовательности будут уходить данные из контроллера в ПК
  //--- Памятка в виде структуры данных с датчиков и доп данных
  typedef struct {
  uint32_t  prt;  //--- etap:3 bit (3 или 4); day: 5 bit (~ 17); rot: 8 bit (>= 102)      
  uint32_t  alarm;//--- Мл. 4 бита - алармы {гл. температура, гл. влажность, свет, вент.}
  }stPortVal;

  typedef struct {
  payval PWM;     //--- Ten, Led, Vent duty(%){Ten-наполнение, а также: Led, Vent в %}   
  payval DhtL;    //--- DHT+Light{t,h,l}      {температура, влажность, свет}
  payval Ds;      //--- array DS {t1,t2,t3}   {3 температуры с массива DS18B20}
  payval Bme;     //--- Bme280   {t,h,p}      {температура, влажность, давление}
  }stArrPayval; 
  или
                port.prt, 
                dt.PWM.f0,  dt.PWM.f1, dt.PWM.f2, 
                dt.DhtL.f0, dt.DhtL.f1,dt.DhtL.f2, 
                dt.Ds.f0,   dt.Ds.f1,  dt.Ds.f2,
                dt.Bme.f0,  dt.Bme.f1, dt.Bme.f2,
                port.alarm);
 
*/
#ifndef DEBUG_OUT 
void Out_prt() {
  Serial.print(portval.prt);
  Serial.print(" ");
}

void Out_PWM() {
  Serial.print(pwm[0]);
  Serial.print(" ");
  Serial.print(pwm[1]);
  Serial.print(" ");
  Serial.print(pwm[2]);  
  Serial.print(" ");   
}

void Out_DhtL() {
  Serial.print(my_dht.t);
  Serial.print(" ");
  Serial.print(my_dht.h);
  Serial.print(" ");
  Serial.print((float)photo_sens);  
  Serial.print(" ");   
}

void Out_Ds() {
  Serial.print(ds_t[0]);
  Serial.print(" ");
  Serial.print(ds_t[1]);
  Serial.print(" ");
  Serial.print(31.41);  
  Serial.print(" ");    //--- ***************************
}

void Out_Bme() {
  Serial.print(clim.t);
  Serial.print(" ");
  Serial.print(clim.h);
  Serial.print(" ");
  Serial.print(clim.p);  
  Serial.print(" ");   
}

void Out_alarm() {
  static uint32_t stat = 0;
  
  switch(stat) {
  case 0: portval.alarm = 1; stat = 1; break;
  case 1: portval.alarm = 2; stat = 2; break;
  case 2: portval.alarm = 4; stat = 3; break;
  case 3: portval.alarm = 8; stat = 0; break;    
  }
 
  Serial.print(portval.alarm);
  Serial.println(" ");
}

void Out_Allchain() {
  //--- Выводим все данные цепочкой
  if (tmrOut.tick()) {
    Out_prt();
    Out_PWM();
    Out_DhtL();
    Out_Ds();
    Out_Bme();
    Out_alarm();
    Work_Hod();
  }
}
#endif

#ifdef DEBUG_OUT   
void Out_All() {

  // Выводим данные из BME280
  if (tmrOut.tick()) {

      Serial.println(F("---------------------------------------------"));    
      // День недели
      Serial.print(datetime.dwk);
      Serial.print(F(" "));  
      // Дата
      Serial.print(datetime.dat);
      Serial.print(F(" -- "));
      // Время
      Serial.println(datetime.tim);
      Serial.println(F("---------------------------------------------")); 
    
      Serial.print(F("BME.t:     "));
      Serial.print(clim.t);             // Выводим темперутуру в [*C]
      Serial.println(F(GRAD_SIMB));

      Serial.print(F("BME.h:     "));  
      Serial.print(clim.h);             // Выводим влажность в [%]
      Serial.println(F("  %"));

      Serial.print(F("BME.p:     "));
      Serial.print(clim.p);             // Выводим давление в мм рт. столба
      Serial.println(F(" mm Hg"));
      Serial.println("");  
                                        // Выводим DHT {t, h}
      Serial.print(F("DHT.t:     "));
      Serial.print(my_dht.t);
      Serial.println(F(GRAD_SIMB));  
        
      Serial.print(F("DHT.h:     "));
      Serial.print(my_dht.h);
      Serial.println(F("  %"));
      Serial.println(); 

                                        // выводим показания массива датчиков ds
      for (int i = 0; i < DS_SENSOR_AMOUNT; i++) {
        Serial.print(F("DS["));
        Serial.print(i+1);
        Serial.print(F("].t:   "));
        Serial.print(ds_t[i]);
        Serial.println(F(GRAD_SIMB));
      }
      Serial.println(); 

                                        // выводим показание фотосенсора
      Serial.print(F("Light:     "));
      Serial.println(photo_sens); 
      Serial.println(); 
  }  
}
#endif

 void Work_DS18B20() {
  if (tmr.tick()) {
    
    // получаем данные из ds массива датчиков
    for (int i = 0; i < DS_SENSOR_AMOUNT; i++) {
      ds_t[i] = ds[i].getTemp();
    }

    // запрашиваем новые данные
    for (int i = 0; i < DS_SENSOR_AMOUNT; i++) {
      ds[i].requestTemp();
    }
  }
}

void Work_Photosensor() {
  if (tmr3.tick()) {
    photo_sens = analogRead(PIN_PHOTORESISTOR);
  }
}

// вызывается в прерывании при приёме байта
void MU_serialEvent() {
  static bool val = 0;
  digitalWrite(13, val = !val);
  //Serial.write(Serial.read());
}

float k = 0.1;  // коэффициент фильтрации, 0.0-1.0
// бегущее среднее
float expRunningAverage(float newVal) {
  static float filVal = 0;
  filVal += (newVal - filVal) * k;
  return filVal;
}
