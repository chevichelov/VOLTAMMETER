#include <INA226.h>                             //Добавляем библиотеку из архива
#include <SPI.h>                                //Добавляем библиотеку из архива
#include <TFT_ST7735.h>                         //Добавляем библиотеку из архива

TFT_ST7735 tft = TFT_ST7735();                  //Инициируем класс
INA226_Class INA226;                            //Инициируем класс


//Объявляем переменные
unsigned long TIME              = 0;            //Обявляем переменную для милисекунд
const uint32_t SHUNT_MICRO_OHM  = 10000;        //Сопротивление шунта в микромах, 0,1 Ом это 100000 микрон
const int MAXIMUM_AMPS          = 16;           //Максимальное значение тока, значения 1 А - ограничено до 16 А

const float MINIMUM_TEMPERATURE = 30 ;          // Минимальная температура при которой начнет работать ШИМ вентилятора.
const float MAXIMUM_TEMPERATURE = 60 ;          // Температура при которой скорость вентилятора будет максимальной. 

#define B 3950                                  // B-коэффициент
#define SERIAL_R 100000                         // сопротивление последовательного резистора, 100 кОм
#define THERMISTOR_R 74000                      // номинальное сопротивления термистора, 74 кОм
#define NOMINAL_T 25                            // номинальная температура (при которой TR = 74 кОм)

byte PIN_RESET    = A0;                         //Устанавливаем пин А0 для кнопки сброса счётчика мАч
byte PIN_INPUT    = A1;                         //Устанавливаем пин А1 на приём даных с терморезистрора
int PIN_FAN       = 3;                          //Устанавливаем пин D3 для шин сигнала на вентилятор


int PIN_BUTTON_4  = 4;                          //Устанавливаем пин D4 для кнопки 1
int PIN_BUTTON_5  = 5;                          //Устанавливаем пин D5 для кнопки 2
int PIN_BUTTON_6  = 6;                          //Устанавливаем пин D6 для кнопки 3
int PIN_BUTTON_8  = 8;                          //Устанавливаем пин D8 для кнопки 4
int PIN_BUTTON_12 = 12;                         //Устанавливаем пин D12 для кнопки 5


//Вспомогательные переменные для обработки данных
float TEMPERATURE = 0, VOLTS, AMPS, WATTS, mAh, Wh;
int FAN = 0 ;
unsigned long  SET_MILLISECOND;



void setup() {
  Serial.begin(9600);
  TCCR2B = TCCR2B & 0b11111000 | 0x01;            //Включаем частоту ШИМ'а  вентилятора на ногах 3 и 11: 31250 Гц.

  pinMode(PIN_RESET, INPUT_PULLUP);               //Устанавливаем пин на приём сигнала
  pinMode(PIN_INPUT, INPUT);                      //Устанавливаем пин на приём сигнала
  pinMode(PIN_BUTTON_4, INPUT_PULLUP);            //Устанавливаем пин на приём сигнала
  pinMode(PIN_BUTTON_5, INPUT_PULLUP);            //Устанавливаем пин на приём сигнала
  pinMode(PIN_BUTTON_6, INPUT_PULLUP);            //Устанавливаем пин на приём сигнала
  pinMode(PIN_BUTTON_8, INPUT_PULLUP);            //Устанавливаем пин на приём сигнала
  pinMode(PIN_BUTTON_12, INPUT_PULLUP);           //Устанавливаем пин на приём сигнала
  
  INA226.begin(MAXIMUM_AMPS, SHUNT_MICRO_OHM);    //Установливаем максимальный ток и сопротивление шунта
  INA226.setBusConversion(8244);                  //Время конверсии в микросекундах (140,204,332,588,1100,2116,4156,8244)8244µs=8.244 ms
  INA226.setShuntConversion(8244);                //Время конверсии в микросекундах (140,204,332,588,1100,2116,4156,8244)8244µs=8.244 ms
  INA226.setAveraging(4);                         //Среднее количество чтений n раз (1,4,16,64,128,256,512,1024)
  INA226.setMode(INA_MODE_CONTINUOUS_BOTH);       //Шина / шунт измеряется постоянно

  tft.init();                                     //Инициализация дисплея.
  tft.setRotation(1);                             //Переворачиваем дисплей.
  tft.fillScreen(TFT_BLACK);                      //Указываем цвет заливки дисплея
  
  //Закрашиваем не закрашенные места на экране
  tft.fillRect(-1, -2,160, 128, ST7735_BLACK);
  tft.fillRect(2, 1, 154, 122, ST7735_BLACK);

  //Все статические данные, которые будут отображаться на дисплее
  tft.setTextColor( ST7735_WHITE, ST7735_BLACK);  //цвет текста белый, цвет заливки текста чёрный
  tft.drawRightString("t:", 0, 0, 2);             //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.drawRightString("C", 55, 0, 2);             //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.drawRightString("o", 62, 0, 1);             //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.setTextColor( ST7735_WHITE, ST7735_BLACK);  //цвет текста белый, цвет заливки текста чёрный
  tft.drawRightString("fan:", 115, 0, 2);         //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.drawRightString("%", 155, 0, 2);            //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.setTextColor( ST7735_WHITE, ST7735_BLACK);  //цвет текста белый, цвет заливки текста чёрный
  tft.drawRightString("V", 155, 37, 4);           //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.setTextColor(ST7735_RED, ST7735_BLACK);     //цвет текста белый, цвет заливки текста чёрный
  tft.drawRightString("A", 120, 71, 2);           //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);   //цвет текста зелёный, цвет заливки текста чёрный
  tft.drawRightString("W", 122, 95, 2);           //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.drawRightString("mAh:", 0, 114, 2);         //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  

  
  
  //Инициируем ошибку
  while (INA226.begin(MAXIMUM_AMPS, SHUNT_MICRO_OHM) == 0) {
    tft.setTextColor(ST7735_RED, ST7735_BLACK);
    tft.drawRightString("Error INA229 " , 150, 45, 4);
    delay(300);
    tft.fillRect(5, 40, 150, 31,ST7735_BLACK) ;
    delay(300);
  }

  SET_MILLISECOND = millis();                                         //Сохраняем текущее время

}


char DATA_RESULT[20];                                                 //Объявляем переменную
static char* DISPLAY_TEXT(float DATA, int COUNT, int FLOAT)           //функция затирает предыдущие показания пробелами 
{
  char DATA_DISPLAY[20];                                              //Объявляем переменную
  dtostrf(DATA, COUNT, FLOAT, DATA_DISPLAY);                          //Конвертируем показания в привычные глазу данные для дальнейшего вывода на экран

  int LEN = strlen(DATA_DISPLAY);                                     //Узнаём длину полученных данных
  
  for(int i=0; i < COUNT - (LEN - 1); ++i)                            //Вычисляем сколько пробелов не хватает
  {
    strcpy(DATA_RESULT, " ");                                         //Создаём строку из недостающих пробелов
  }
  strcat(DATA_RESULT, DATA_DISPLAY);                                  //Добавляем недостающие пробелы
  
  return DATA_RESULT;                                                 //Отдаём результат
}

static char* DISPLAY_INFO(int PIN, char* DATA)                        //Функция при нажатии кнопки выводит данные, при отжатии затирает их
{  
  if (!digitalRead(PIN))                                              //Читаем нужный пин
    return DATA;                                                      //Возвращаем данные
  else
    return "  ";                                                      //Затираем их пробелами
}


void loop() {

  if (millis() - TIME <= 10)                                          //Добавляем задержку в 10 милисекунд
    return;
    
  TIME = millis();                                                    //Устанавливаем новое время
  
  //Получаем данные от INA226
  VOLTS = INA226.getBusMilliVolts() / 10e2;
  AMPS = INA226.getBusMicroAmps()  /10e4;
  WATTS = INA226.getBusMicroWatts() / 10e6;


  //Обнуляем отрицательные показания
  if (VOLTS<0)
    VOLTS=0;
    
  if (AMPS<0)
    AMPS=0;
    
  if (WATTS<0)
    WATTS=0;
    
  mAh += AMPS * (millis() - SET_MILLISECOND) / 3600000 * 1000;          //расчет емкости  в мАч
  SET_MILLISECOND = millis(); //Обнавляем текущее время

  
  //Определяем температуру на датчике с помощью модифицированной формулы Стейнхарта — Харта: 1/T = 1/T + 1/B * ln (R/R0)
  //Подробней можете прочитать в статье http://psenyukov.ru/%D0%BF%D0%BE%D0%B4%D0%BA%D0%BB%D1%8E%D1%87%D0%B5%D0%BD%D0%B8%D0%B5-%D1%82%D0%B5%D1%80%D0%BC%D0%B8%D1%81%D1%82%D0%BE%D1%80%D0%B0-%D0%BA-arduino/
  int t = analogRead(PIN_INPUT);                                        //Считываем показания датчика температуры с пина А1
  float tr = 1023.0 / t - 1;
  tr = SERIAL_R / tr;
  float TEMPERATURE;
  TEMPERATURE = tr / THERMISTOR_R;
  TEMPERATURE = log(TEMPERATURE);
  TEMPERATURE /= B;
  TEMPERATURE += 1.0 / (NOMINAL_T + 273.15);
  TEMPERATURE = 1.0 / TEMPERATURE;
  TEMPERATURE -= 273.15;



  //Рассчитываем  ШИМ вентилятора.
  if (TEMPERATURE >= MINIMUM_TEMPERATURE && TEMPERATURE <= MAXIMUM_TEMPERATURE)  //Если температура в среднем показателе, расчитываем обороты
  {
    FAN = (TEMPERATURE - MINIMUM_TEMPERATURE) * 255 / (MAXIMUM_TEMPERATURE - MINIMUM_TEMPERATURE); 
  }
  else if (TEMPERATURE < MINIMUM_TEMPERATURE)                                   //Если температура минимум
  {
    FAN = 0;                                                                    //Отключаем вентилятор
  }
  else if (TEMPERATURE >= MAXIMUM_TEMPERATURE)                                  //Если температура максимум
  {
    FAN = 255;                                                                  //Включаем вентилятор
  }

  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);                                 //цвет текста белый, цвет заливки текста чёрный
  tft.drawRightString(DISPLAY_TEXT(TEMPERATURE, 4, 0), 45 , 0, 2);              //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.drawRightString(DISPLAY_TEXT(FAN / 2.55, 3, 0), 145, 0, 2);               //"Текст", положение по оси Х, положение по оси Y, размер шрифта

  tft.setTextColor( ST7735_WHITE, ST7735_BLACK);                                //цвет текста белый, цвет заливки текста чёрный
  tft.drawRightString(DISPLAY_TEXT(VOLTS, 5, 2), 138, 19, 6);                   //"Текст", положение по оси Х, положение по оси Y, размер шрифта


  tft.setTextColor(ST7735_RED, ST7735_BLACK);                                   //цвет текста белый, цвет заливки текста чёрный
  tft.drawRightString(DISPLAY_TEXT(AMPS,  6, 3), 110, 65, 4);                   //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  
 
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);                                 //цвет текста белый, цвет заливки текста чёрный
  tft.drawRightString(DISPLAY_TEXT(WATTS, 7, 3), 110, 89, 4);                   //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);                                 //цвет текста белый, цвет заливки текста чёрный
  char mAh_DISPLAY[12]; dtostrf(mAh , 10, 2, mAh_DISPLAY);                      //Конвертируем показания в привычные глазу данные для дальнейшего вывода на экран 
  tft.drawRightString(mAh_DISPLAY, 155 , 114, 2);                               //"Текст", положение по оси Х, положение по оси Y, размер шрифта


  tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  if (!digitalRead(PIN_RESET))
    mAh = 0;
  

  tft.drawRightString(DISPLAY_INFO(PIN_BUTTON_4, "QS"), 0 , 30, 1);             //При нажатии кнопки на PIN_BUTTON_4 пине появится сообщение QS

  tft.drawRightString(DISPLAY_INFO(PIN_BUTTON_5, "Qi"), 0 , 40, 1);             //При нажатии кнопки на PIN_BUTTON_5 пине появится сообщение Qi

  tft.drawRightString(DISPLAY_INFO(PIN_BUTTON_6, "GB"), 0 , 50, 1);             //При нажатии кнопки на PIN_BUTTON_6 пине появится сообщение GB

  tft.drawRightString(DISPLAY_INFO(PIN_BUTTON_8, "Sp"), 0 , 60, 1);             //При нажатии кнопки на PIN_BUTTON_8 пине появится сообщение Sp
  tft.drawRightString(DISPLAY_INFO(PIN_BUTTON_8, "Sm"), 0 , 70, 1);             //При нажатии кнопки на PIN_BUTTON_8 пине появится сообщение Sm

  tft.drawRightString(DISPLAY_INFO(PIN_BUTTON_12, "Tr"), 0 , 80, 1);            //При нажатии кнопки на PIN_BUTTON_12 пине появится сообщение Tr
  

  analogWrite(PIN_FAN, FAN);                                                    //Передаём сигнал для вентилятора на 3 пин
}
