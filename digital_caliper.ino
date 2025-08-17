/*
 * Firmware for reading data from a digital caliper using a 2-wire protocol
 * * Protocol Description:
 * - Lines: CLK (clock signal) and DATA (data).
 * - Packet: 24 bits, transmitted continuously.
 * - Bit Order: LSB (least significant bit first).
 * - Synchronization: Data is read on the rising edge (when CLK transitions from LOW to HIGH).
 * * Logic:
 * - CLK HIGH & DATA HIGH = 1
 * - CLK HIGH & DATA LOW = 0
 * - Packet Start: A long pause (>5 ms), after which CLK goes LOW.
 */

#include <GyverOLED.h>

const uint8_t ruler_bitmap[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00,
  0x80, 0x00, 0xf8, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0xf8, 0x00,
  0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x44, 0x44, 0x38, 0x00, 0x02, 0x0f, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02,
  0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02,
  0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02,
  0x03, 0x02, 0x03, 0x02, 0x0f, 0x02, 0x00, 0x48, 0x7c, 0x40, 0x00, 0x5c, 0x54, 0x54, 0x24, 0x00, 0x38, 0x44, 0x44, 0x38
};
const uint8_t rulerStartX = 29;  // depends on bitmap


const int CLK_PIN = 13;
const int DATA_PIN = 14;

// Raw 24-bit data of caliper.
volatile uint32_t caliper_raw_data = 0;

// Received bit counter in chunk.
volatile uint8_t bit_counter = 0;

volatile bool new_packet_ready = false;

// Last interrupt time to be able to know end of chunk
volatile unsigned long last_interrupt_time = 0;

// if last interrupt time is greater than this value than it's new chunk of data. Value in uS
const unsigned long PACKET_TIMEOUT_US = 5000;  // 5 ms
const uint8_t UPDATE_OLED_PERIOD = 100;        // 10 fps

static uint8_t oledTimer = 0;

static float measurement = 0.0f;
static bool inch = false;

GyverOLED<SSD1306_128x32> oled;

/**
 * @brief ISR call on every LOW -> HIGH transition
 */
void IRAM_ATTR read_bit() {
  unsigned long current_time = micros();

  if (current_time - last_interrupt_time > PACKET_TIMEOUT_US) {
    // New packet. Reset collected data
    bit_counter = 0;
    caliper_raw_data = 0;
  }

  last_interrupt_time = current_time;

  if (bit_counter < 24) {
    if (digitalRead(DATA_PIN) == HIGH) {
      // Если на DATA высокий уровень, это бит '1'.
      // Устанавливаем соответствующий бит в нашей переменной.
      // Используем побитовое 'ИЛИ' и сдвиг. (1UL << bit_counter) создает маску
      // вида 00...1...00, где '1' стоит на нужной позиции.
      caliper_raw_data |= (1UL << bit_counter);
    }
    // Если на DATA низкий уровень, это бит '0'. Ничего делать не нужно,
    // так как при сбросе caliper_raw_data уже заполнена нулями.

    // increment received bits. 'value++' won't work on volatile variable
    uint8_t temp = bit_counter;
    temp++;
    bit_counter = temp;
  }

  // if it's last 24th bit of data then packet is ready
  if (bit_counter == 24) {
    new_packet_ready = true;
  }
}

/**
 * @brief Print binary data to serial monitor
 * @param value to print in binary format
 * @param bits bits count to print
 */
void print_binary(uint32_t value, uint8_t bits) {
  for (int i = bits - 1; i >= 0; i--) {
    Serial.print(bitRead(value, i));
    if (i % 4 == 0) Serial.print(" ");  // space every 4th bit just for readability
  }
}

/**
 * @brief Draw rectangle on ruler proportional to the measured value
 */
void fillMeasuredDistanceOnRuler() {
  if (measurement < 0 || measurement > 150) return;

  oled.drawBitmap(23, 0, ruler_bitmap, 82, 15, BITMAP_NORMAL, BUF_REPLACE);
  oled.rect(rulerStartX, 5, ceilf(measurement / 2.5) + rulerStartX, 9);
}

/**
 * @brief Draw value on the display
 */
void drawMeasuredValue() {
  oled.setCursorXY(18, 16);
  oled.setScale(2);

  if (inch) {
    oled.printf("%0.3f inch  ", measurement);
  } else {
    oled.printf("%0.2f mm  ", measurement);
  }
}

void updateOled(bool force = false) {
  uint8_t ms = millis();
  if (force || (uint8_t)(ms - oledTimer) >= UPDATE_OLED_PERIOD) {
    oledTimer = ms;

    fillMeasuredDistanceOnRuler();
    drawMeasuredValue();
    oled.update();
  }
}

void handleData(uint32_t data_copy) {
  // 1. Extract measurement data (first 20 bits, from 0 to 19)
  // Hex mask 0x0FFFFF == 20 bits.
  long integerMeasurement = data_copy & 0x0FFFFF;

  // 2. Extract flags (next 4 bits, from 20 to 23)
  // First shift 20 bits right, then using hex mask 0x0F extract 4 lower bits.
  uint8_t flags = (data_copy >> 20) & 0x0F;

  // 3. Flags handling
  integerMeasurement *= bitRead(flags, 0) ? -1 : 1;  // check the first bit to get value sign (+ or -)
  inch = bitRead(flags, 3);                          // check if measurements in inches. Received value always in mm, so we need to convert it to inches

  integerMeasurement = constrain(integerMeasurement, -15000, 15000);  // limit in mm
  measurement = integerMeasurement / 100.0;                           // divide received value by 100, as most digital calipers send data in integer format, like 149.42 will be 14942

  if (inch) measurement /= 25.4;  // convert mm to inches

  printResultToOutput(data_copy, flags);
}

void printResultToOutput(uint32_t binaryData, uint8_t flags) {
  Serial.print("Binary data: ");
  print_binary(binaryData, 24);

  Serial.print(" | Value: ");
  Serial.print(measurement, 2);
  Serial.print(" mm");

  Serial.print(" | Flags: ");
  print_binary(flags, 4);
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  oled.init();
  Wire.setClock(400000L);
  oled.clear();

  oled.println("Init...");
  oled.update();
  Serial.println("Init digital caliper measurements...");

  pinMode(CLK_PIN, INPUT_PULLUP);
  pinMode(DATA_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(CLK_PIN), read_bit, RISING);

  Serial.println("Ready. Waiting data...");
  delay(3000);

  oled.clear();
  oled.drawBitmap(23, 0, ruler_bitmap, 82, 15);
  drawMeasuredValue();
  oled.update();
  oled.home();
}

void loop() {
  // if packet is not ready then we don't need to update
  if (new_packet_ready) {
    uint32_t data_copy;  // local copy of data for safety reasons

    // disable interrupts -> copy data -> enable interrupts, just to be sure that ISR won't change our data
    noInterrupts();
    data_copy = caliper_raw_data;
    new_packet_ready = false;
    interrupts();

    handleData(data_copy);
    updateOled();
  }
}