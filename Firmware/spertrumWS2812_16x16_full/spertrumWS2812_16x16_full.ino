// -------------------------------- НАСТРОЙКИ --------------------------------
// Режимы работы
#define MODE_EQUALIZER 1000 // Режим эквалайзера
#define MODE_LIGHT 1001     // Режим подсветки
#define MODE_SUNRISE 1002   // Будильник с плавным включением подсветки
#define MODE_CLOCK 1003     // Часы
#define MODE_RAINBOW 1004   // Радуга

// матрица
#define WIDTH 15          // ширина матрицы (число диодов)
#define HEIGHT 10         // высота матрицы (число диодов)
#define BRIGHTNESS 64     // яркость (0 - 255)

// цвета высоты полос спектра. Длины полос задаются примерно в строке 95
#define COLOR1 CRGB::Green
#define COLOR2 CRGB::Yellow
#define COLOR3 CRGB::Orange
#define COLOR4 CRGB::Red

// сигнал
#define INPUT_GAIN 1      // коэффициент усиления входного сигнала
#define LOW_PASS 35       // нижний порог чувствительности шумов (нет скачков при отсутствии звука)
#define MAX_COEF 1.1      // коэффициент, который делает "максимальные" пики чуть меньше максимума, для более приятного восприятия
#define NORMALIZE 1       // нормализовать пики (столбики низких и высоких частот будут одинаковой длины при одинаковой громкости) (1 вкл, 0 выкл)

// анимация
#define SMOOTH 0.32        // плавность движения столбиков (0 - 1)
#define DELAY 4           // задержка между обновлениями матрицы (периодичность основного цикла), миллиисекунды

// громкость
#define DEF_GAIN 40       // максимальный порог по умолчанию (при MANUAL_GAIN или AUTO_GAIN игнорируется)
#define MANUAL_GAIN 0     // ручная настройка потенциометром на громкость (1 вкл, 0 выкл)
#define AUTO_GAIN 1       // автонастройка по громкости (экспериментальная функция) (1 вкл, 0 выкл)

// точки максимума
#define MAX_DOTS 1        // включить/выключить отрисовку точек максимума (1 вкл, 0 выкл)
#define MAX_COLOR COLOR4  // цвет точек максимума
#define FALL_DELAY 64    // скорость падения точек максимума (задержка, миллисекунды)
#define FALL_PAUSE 300    // пауза перед падением точек максимума, миллисекунды

// массив тонов, расположены примерно по параболе. От 80 Гц до 16 кГц
// byte posOffset[11] = {2,3,5,7,11,16,25,39,62,77,96};
// byte frenqueList[16] = {2, 3, 4, 6, 8, 10, 12, 14, 16, 20, 25, 30, 35, 60, 80, 100}; // Список частот (порядковый номер) из выходного массива, получаемого из библиотеки FHT
//byte frenqueList[16] = {2, 4, 6, 8, 10, 12, 14, 16, 20, 25, 30, 35, 45, 50, 80, 90};
//byte frenqueList[16] = {5, 8, 13, 18, 23, 28, 33, 38, 43, 48, 53, 58, 63, 68, 73, 78}; // norm
byte frenqueList[16] = {3, 6, 9, 12, 15, 18, 21, 26, 31, 36, 41, 46, 51, 56, 61, 66}; 
// ---------------------- НАСТРОЙКИ --------------------------------

// ---------------------- ПИНЫ ----------------------
// для увеличения точности уменьшаем опорное напряжение,
// выставив EXTERNAL и подключив Aref к выходу 3.3V на плате через делитель
// GND ---[10-20 кОм] --- REF --- [10 кОм] --- 3V3

#define AUDIO_IN 0         // пин, куда подключен звук
#define DIN_PIN 6         // пин Din ленты (через резистор!)
#define POT_PIN 7         // пин потенциометра настройки (если нужен MANUAL_GAIN)
// ---------------------- ПИНЫ ----------------------

// ---------------------- ДЛЯ РАЗРАБОТЧИКОВ ---------------
#define NUM_LEDS WIDTH * HEIGHT
#define FHT_N 256         // ширина спектра х2
#define LOG_OUT 1         // Флаг для заполнения массива выходных данных
#include <FHT.h>          // преобразование Хартли
#include <FastLED.h>
CRGB leds[NUM_LEDS];

int idex = 0;
int ihue = 0;
int thisdelay = 10;          
int thisstep = 5;
int thissat = 255;

int maxColorChannelValue = 200;
int rainbowStep = 1;
int rainbowStart_R = random(199);
int rainbowStart_G = random(199);
int rainbowStart_B = random(199);

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

int gain = DEF_GAIN;   // усиление по умолчанию
unsigned long gainTimer, fallTimer;
byte maxValue;
float k = 0.05, maxValue_f = 0.0;
int maxLevel[WIDTH];
byte posLevel_old[WIDTH];
unsigned long timeLevel[WIDTH], mainDelay;
boolean fallFlag;
int currentMode = MODE_EQUALIZER;
int currentBrightness = 128;
int rColorCoefficientStepArray[WIDTH];
int gColorCoefficientStepArray[WIDTH];
int bColorCoefficientStepArray[WIDTH];
int rColorCoefficientArray[WIDTH];
int gColorCoefficientArray[WIDTH];
int bColorCoefficientArray[WIDTH];

byte LIGHT_R = 0;
byte LIGHT_G = 0;
byte LIGHT_B = 0;
// --------------- ДЛЯ РАЗРАБОТЧИКОВ ---------------

void setup() {
  delay( 2000 );
  // поднимаем частоту опроса аналогового порта до 38.4 кГц, по теореме
  // Котельникова (Найквиста) максимальная частота дискретизации будет 19 кГц
  // http://yaab-arduino.blogspot.ru/2015/02/fast-sampling-from-analog-input.html
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);

  analogReference(EXTERNAL);

  Serial.begin(9600);
  
  FastLED.addLeds<WS2812B, DIN_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  for (int i = 0; i < WIDTH; i++) {
    rColorCoefficientStepArray[i] = 1;
    gColorCoefficientStepArray[i] = -2;
    bColorCoefficientStepArray[i] = 2;

    int coefficientValue = i * 10 + 10;

    rColorCoefficientArray[i] = coefficientValue;
    gColorCoefficientArray[i] = coefficientValue;
    bColorCoefficientArray[i] = coefficientValue;
  }
  
  Serial.print("Setup was finished successful.\n");
  delay(500);
}

void loop() {
  if (Serial.available()) {
      String value = Serial.readString();
      int intVal = value.toInt();
      Serial.println(intVal);
      if (intVal == 1) {
        currentMode = MODE_EQUALIZER;
      } else if (intVal == 2) {
        currentMode = MODE_LIGHT;
      } else if (intVal = 3) {
        rainbowStart_R = random(199);
        rainbowStart_G = random(199);
        rainbowStart_B = random(199);
        FastLED.setBrightness(64);
        currentMode = MODE_RAINBOW;
      }
  } else {
  if (currentMode == MODE_EQUALIZER) {
    if (millis() - mainDelay > DELAY) {     // итерация главного цикла
      mainDelay = millis();
      analyzeAudio();        // функция FHT, забивает массив fht_log_out[] величинами по спектру
      filterAndNormilizeInputSignal(); // Фильтруем и нормализуем сигнал
    
      maxValue = 0;
      FastLED.clear();  // Очищаем матрицу
    
      // logToMonitor();   // Выводим в лог информацию по частотам
   
      for (byte pos = 0; pos < WIDTH; pos++) {    // для кажого столбца матрицы
        int invertedPos = WIDTH - pos - 1;
        int posLevel = fht_log_out[frenqueList[invertedPos]];
        posLevel = getPosLevelFromNearlyColumns(invertedPos, posLevel); // Тут мы добавляем значения из соседних столбиков, которые "не вошли" в массив после преобразования количества столбцов 128 - >15
      
        // найти максимум из пачки тонов
        if (posLevel > maxValue) { 
          maxValue = posLevel;
        }

        // фильтрация длины столбиков, для их плавного движения
        posLevel = posLevel * SMOOTH + posLevel_old[pos] * (1 - SMOOTH);
        posLevel_old[pos] = posLevel;

        // преобразовать значение величины спектра в диапазон 0..HEIGHT с учётом настроек
        posLevel = map(posLevel, LOW_PASS, gain, 0, HEIGHT);
        posLevel = constrain(posLevel, 0, HEIGHT);

        paintLeds(posLevel, pos);

        if (posLevel > 0 && posLevel > maxLevel[pos]) {    // если для этой полосы есть максимум, который больше предыдущего
          maxLevel[pos] = posLevel;                        // запомнить его
          timeLevel[pos] = millis();                       // запомнить время
        }

        // если точка максимума выше нуля (или равна ему) - включить пиксель
        if (maxLevel[pos] >= 0 && maxLevel[pos] < HEIGHT && MAX_DOTS) {
          if (pos % 2 != 0)                                                   // если чётная строка
            leds[pos * HEIGHT + maxLevel[pos]] = MAX_COLOR;                   // заливаем в прямом порядке
          else                                                                // если нечётная
            leds[pos * HEIGHT + HEIGHT - maxLevel[pos] - 1] = MAX_COLOR;      // заливаем в обратном порядке
         }

        if (fallFlag) {                                           // если падаем на шаг
          if ((long)millis() - timeLevel[pos] > FALL_PAUSE) {     // если максимум держался на своей высоте дольше FALL_PAUSE
            if (maxLevel[pos] >= 0) maxLevel[pos]--;              // уменьшить высоту точки на 1
            // внимание! Принимает минимальное значение -1 !
          }
        }

        //Serial.print(posLevel);
        //Serial.print('\t');
        //if (pos == (WIDTH - 1)) {
          //Serial.print('\n');
        //}
      }

      // -------
      FastLED.show();  // отправить на матрицу

      fallFlag = 0;                                 // сбросить флаг падения
      if (millis() - fallTimer > FALL_DELAY) {      // если настало время следующего падения
        fallFlag = 1;                               // поднять флаг
        fallTimer = millis();
      }

      // если разрешена ручная настройка уровня громкости
      if (MANUAL_GAIN) gain = map(analogRead(POT_PIN), 0, 1023, 0, 150);

      // если разрешена авто настройка уровня громкости
      if (AUTO_GAIN) {
        if (millis() - gainTimer > 10) {      // каждые 10 мс
          maxValue_f = maxValue * k + maxValue_f * (1 - k);
          // если максимальное значение больше порога, взять его как максимум для отображения
          if (maxValue_f > LOW_PASS) gain = (float) MAX_COEF * maxValue_f;
          // если нет, то взять порог побольше, чтобы шумы вообще не проходили
          else gain = 100;
          gainTimer = millis();
        }
      }
    }
  } else if (currentMode == MODE_LIGHT) {
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CRGB::Red; // Цвет подсветки.
    }
    FastLED.setBrightness(currentBrightness);
    FastLED.show();
  } else if (currentMode == MODE_RAINBOW) {
    //rainbow_loop();
    testRainbow();
  }
  }
}

void testRainbow() {
  LEDS.clear();

  for (byte i = 0; i < WIDTH; i++) {
    byte startIndex = i * 10;

    int rColorCoefficientStep = rColorCoefficientStepArray[i];
    int gColorCoefficientStep = gColorCoefficientStepArray[i];
    int bColorCoefficientStep = bColorCoefficientStepArray[i];
    
    rColorCoefficientArray[i] += rColorCoefficientStep;
    gColorCoefficientArray[i] += gColorCoefficientStep;
    bColorCoefficientArray[i] += bColorCoefficientStep;
    if (rColorCoefficientArray[i] > 160) {
     rColorCoefficientStep = -1;
     rColorCoefficientStepArray[i] = rColorCoefficientStep;
    } else if (rColorCoefficientArray[i] < 30) {
      rColorCoefficientStep = 1;
      rColorCoefficientStepArray[i] = rColorCoefficientStep;
    }

    if (gColorCoefficientArray[i] > 160) {
     gColorCoefficientStep = -2;
     gColorCoefficientStepArray[i] = gColorCoefficientStep;
    } else if (gColorCoefficientArray[i] < 30) {
      gColorCoefficientStep = 2;
      gColorCoefficientStepArray[i] = gColorCoefficientStep;
    }

    if (bColorCoefficientArray[i] > 160) {
     bColorCoefficientStep = -2;
     bColorCoefficientStepArray[i] = bColorCoefficientStep;
    } else if (bColorCoefficientArray[i] < 30) {
      bColorCoefficientStep = 2;
      bColorCoefficientStepArray[i] = bColorCoefficientStep;
    }

    byte R = rColorCoefficientArray[i];
    byte G = gColorCoefficientArray[i];
    byte B = bColorCoefficientArray[i];

    //Serial.print(R);
    //Serial.print('\t');
    //Serial.print(rColorCoefficientStepArray[i]);
    //Serial.print('\n');
    
    for (byte k = startIndex; k < startIndex + 10; k++) {
        leds[k].red   = R;
        leds[k].green = G;
        leds[k].blue  = B;
    }
  }
  
  LEDS.show();
  delay(100);
}

void rainbow_loop() {                        //-m3-LOOP HSV RAINBOW
  idex++;
  ihue = ihue + thisstep;
  if (idex >= NUM_LEDS) {
    idex = 0;
  }
  if (ihue > 255) {
    ihue = 0;
  }
  leds[idex] = CHSV(ihue, thissat, 64);
  LEDS.show();
  delay(thisdelay);
}

void logToMonitor() {
  for (byte pos = 0; pos < WIDTH; pos++) {    // для кажого столбца матрицы
    Serial.print(fht_log_out[frenqueList[pos]]);
    Serial.print("\t");
  }
  Serial.print("\n");
}

void analyzeAudio() {
  for (int i = 0 ; i < FHT_N ; i++) {
    int sample = analogRead(AUDIO_IN);
    fht_input[i] = sample; // put real data into bins
  }
  fht_window();  // window the data for better frequency response
  fht_reorder(); // reorder the data before doing the fht
  fht_run();     // process the data in the fht
  fht_mag_log(); // take the output of the fht
}

void filterAndNormilizeInputSignal() {
  for (int i = 0; i < FHT_N/2; i ++) {
      // вот здесь сразу фильтруем весь спектр по минимальному LOW_PASS
      if (fht_log_out[i] < LOW_PASS) {
        fht_log_out[i] = 0;
      } 
      // усиляем сигнал
      fht_log_out[i] = (float)fht_log_out[i] * INPUT_GAIN;

      // уменьшаем громкость высоких частот (пропорционально частоте) если включено
      if (NORMALIZE) {
        fht_log_out[i] = (float)fht_log_out[i] / ((float)1 + (float)i / FHT_N);
      }
    }
}

int getPosLevelFromNearlyColumns(int invertedPos, int posLevel) {
  byte linesBefore, linesAfter;
  if (invertedPos > 0 && invertedPos < WIDTH) {
          linesBefore = frenqueList[invertedPos] - frenqueList[invertedPos - 1];
          for (byte i = 0; i < linesBefore; i++) {  // от предыдущей полосы до текущей
            posLevel += (float) ((float)i / linesBefore) * fht_log_out[frenqueList[invertedPos] - linesBefore + i];
          }
          linesAfter = frenqueList[invertedPos + 1] - frenqueList[invertedPos];
          for (byte i = 0; i < linesAfter; i++) {  // от предыдущей полосы до текущей
            posLevel += (float) ((float)i / linesAfter) * fht_log_out[frenqueList[invertedPos] + linesAfter - i];
          }
        } else if (invertedPos == 0) {
          //Serial.println(frenqueList[invertedPos]);
          linesBefore = frenqueList[invertedPos] - 1;
          for (byte i = 0; i < linesBefore; i++) {  // от предыдущей полосы до текущей
            posLevel += (float) ((float)i / linesBefore) * fht_log_out[frenqueList[invertedPos] - linesBefore + i];
          }
          linesAfter = frenqueList[invertedPos + 1] - frenqueList[invertedPos];
          for (byte i = 0; i < linesAfter; i++) {  // от предыдущей полосы до текущей
            posLevel += (float) ((float)i / linesAfter) * fht_log_out[frenqueList[invertedPos] + linesAfter - i];
          }
        } 
  return posLevel;
}

void paintLeds(int posLevel, byte pos) {
  if (posLevel > 0) {
    for (int j = 0; j < posLevel; j++) {                 // столбцы
      uint32_t color;
      if (j < 3) color = COLOR1;
      else if (j < 5) color = COLOR2;
      else if (j < 7) color = COLOR3;
      else if (j < 9) color = COLOR4;

      if (pos % 2 != 0) {                                 // если чётная строка
        leds[pos * HEIGHT + j] = color;                  // заливаем в прямом порядке
      } else {                                             // если нечётная
        leds[pos * HEIGHT + HEIGHT - j - 1] = color;      // заливаем в обратном порядке
      }
    }
  }
}

/*
   Алгоритм работы:
   Анализ спектра, на выходе имеем массив величин полос спектра (128 полос)
   Фильтрация по нижним значениям для каждой полосы (128 полос)
   Переход от 128 полос к 16 полосам с сохранением межполосных значений по линейной зависимости
   Поиск максимумов для коррекции высоты столбиков на матрице
   Перевод чистого "веса" полосы к высоте матрицы
   Отправка полос на матрицу
   Расчёт позиций точек максимума и отправка их на мтарицу

   Мимоходом фильтрация верхних пиков, коррекция высоты столбиков от громкости и прочее
*/
