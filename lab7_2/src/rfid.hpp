#ifndef RFID_HPP
#define RFID_HPP

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

namespace rfid {

const uint8_t PIN_RST = 17; // Reset pin
const uint8_t PIN_SS  = 5;  // Slave Select (SDA on module)

MFRC522 mfrc522(PIN_SS, PIN_RST);

void init() {
    Serial.println("[RFID] Initializing RC522...");
    SPI.begin(); 
    mfrc522.PCD_Init();
}

String scanCard() {
    if (!mfrc522.PICC_IsNewCardPresent()) return "";
    if (!mfrc522.PICC_ReadCardSerial()) return "";

    String uidRaw = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        uidRaw += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
        uidRaw += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidRaw.toUpperCase();

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    return uidRaw;
}

} // namespace rfid
#endif // RFID_HPP