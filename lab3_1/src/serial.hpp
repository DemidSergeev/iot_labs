#include <Arduino.h>

namespace serial {

int readInt() {
  while (!Serial.available()) {
    delay(100);
  }
  Serial.print("> ");
  int input = Serial.parseInt();
  Serial.println(input);
  return input;
}

} // namespace serial