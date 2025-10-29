#include <Arduino.h>

constexpr int led_pin = 2;
constexpr int delay_ms = 100;
uint16_t i = 1;
uint64_t previous = 0, current = 1;

uint64_t next_fibonacci(uint64_t current, uint64_t previous) {
  return current + previous;
}

bool is_overflow(uint64_t x, uint64_t y) {
  if (x < y) {
    Serial.printf("Overflow detected at index %d, starting over.\n", i);
    i = 1;
    previous = 0;
    current = 1;
    return true;
  }
  return false;
}

void setup() {
  pinMode(led_pin, OUTPUT);
  Serial.begin(115200);
  Serial.printf("Fibonacci 0: %llu\n", previous);
  Serial.printf("Fibonacci 1: %llu\n", current);
}

void loop() {
  digitalWrite(led_pin, HIGH);
  delay(delay_ms);

  ++i;
  uint64_t next = next_fibonacci(current, previous);
  if (!is_overflow(next, current)) {
    previous = current;
    current = next;
  }
  Serial.printf("Fibonacci %d: %llu\n", i, current);

  digitalWrite(led_pin, LOW);
  delay(delay_ms);
}