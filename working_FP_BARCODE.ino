#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

// Barcode scanner on pins 1 (RX) and 3 (TX)
SoftwareSerial scannerSerial(2, 3); // RX2, TX2

// Fingerprint sensor on pins 16 (RX) and 17 (TX)
SoftwareSerial fingerSerial(10, 11);
Adafruit_Fingerprint finger(&fingerSerial);

// Barcode reading vars
String barcode = "";
unsigned long lastCharTime = 0;
const unsigned long barcodeTimeout = 100; // ms

void setup()
{
    Serial.begin(9600);

    scannerSerial.begin(9600);
    fingerSerial.begin(57600);
    finger.begin(57600);

    Serial.println("System Ready.");
    Serial.println("Enter 'e' to enroll, 'i' to identify fingerprint, 'b' to scan barcode.");

    if (finger.verifyPassword())
    {
        Serial.println("Fingerprint sensor detected.");
    }
    else
    {
        Serial.println("Fingerprint sensor not found. Check wiring.");
        while (1)
            delay(1);
    }
}

void loop()
{
    // Always read barcode in background
    readBarcode();

    if (Serial.available())
    {
        char cmd = Serial.read();

        if (cmd == 'e')
        {
            Serial.println("Enter ID to enroll (1–127):");
            uint8_t id = readNumberFromSerial();
            if (id > 0 && id < 128)
            {
                enrollFingerprint(id);
            }
            else
            {
                Serial.println("Invalid ID.");
            }
        }
        else if (cmd == 'i')
        {
            identifyFingerprint();
        }
        else if (cmd == 'b')
        {
            Serial.println("Ready to scan barcode...");
        }
        else
        {
            Serial.println("Invalid command. Use 'e', 'i', or 'b'.");
        }
    }
}

void readBarcode()
{
    scannerSerial.listen(); // Switch active SoftwareSerial to barcode scanner

    while (scannerSerial.available())
    {
        char c = scannerSerial.read();
        if (isPrintable(c))
        {
            barcode += c;
            lastCharTime = millis();
        }
    }

    if (barcode.length() > 0 && (millis() - lastCharTime > barcodeTimeout))
    {
        Serial.print("Scanned Barcode: ");
        Serial.println(barcode);
        barcode = "";
    }
}

uint8_t readNumberFromSerial()
{
    while (!Serial.available())
    {
        delay(10);
    }
    return Serial.parseInt();
}

void enrollFingerprint(uint8_t id)
{
    fingerSerial.listen(); // Switch active SoftwareSerial to fingerprint

    int p = -1;
    Serial.println("Place finger to enroll...");

    while ((p = finger.getImage()) != FINGERPRINT_OK)
    {
        if (p != FINGERPRINT_NOFINGER)
        {
            Serial.println("Image error.");
            return;
        }
        delay(100);
    }

    if (finger.image2Tz(1) != FINGERPRINT_OK)
    {
        Serial.println("Image conversion failed.");
        return;
    }

    Serial.println("Remove finger...");
    delay(2000);
    while (finger.getImage() != FINGERPRINT_NOFINGER)
        delay(100);

    Serial.println("Place same finger again...");
    while ((p = finger.getImage()) != FINGERPRINT_OK)
    {
        if (p != FINGERPRINT_NOFINGER)
        {
            Serial.println("Second image error.");
            return;
        }
        delay(100);
    }

    if (finger.image2Tz(2) != FINGERPRINT_OK)
    {
        Serial.println("Second conversion failed.");
        return;
    }

    if (finger.createModel() != FINGERPRINT_OK)
    {
        Serial.println("Model creation failed.");
        return;
    }

    if (finger.storeModel(id) == FINGERPRINT_OK)
    {
        Serial.println("Fingerprint enrolled successfully.");
    }
    else
    {
        Serial.println("Failed to store fingerprint.");
    }
}

void identifyFingerprint()
{
    fingerSerial.listen(); // Switch active SoftwareSerial to fingerprint

    Serial.println("Place finger to identify...");

    int p = finger.getImage();
    if (p != FINGERPRINT_OK)
    {
        Serial.println("No finger detected.");
        return;
    }

    p = finger.image2Tz();
    if (p != FINGERPRINT_OK)
    {
        Serial.println("Could not convert image.");
        return;
    }

    p = finger.fingerSearch();
    if (p == FINGERPRINT_OK)
    {
        Serial.print("Fingerprint matched! ID: ");
        Serial.println(finger.fingerID);
    }
    else
    {
        Serial.println("No match found.");
    }
}
