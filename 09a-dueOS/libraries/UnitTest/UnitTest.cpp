#include "UnitTest.h"
#include <Adafruit_ST7735.h> // Hardware-specific library

void print(const char *text, uint16_t color);

void UnitTest::start(){
    testCount = 1;
    print("UnitTest:", ST77XX_WHITE);
    time = millis();
}

void UnitTest::end(){
    Serial.print("End: ");
    Serial.print((millis() - time) / 1000.0);
    Serial.println("s");
}

void UnitTest::assert(int a, int b){
    //Serial.println("Test %d: ", testCount);
    if(a == b){
        Serial.println("Passed");
    } else {
        Serial.println("Failed");
        Serial.print("Expected: ");
        Serial.print(a);
        Serial.print(". But: ");
        Serial.println(b);
        stop();
    }
    testCount++;
}

void UnitTest::assert(unsigned int a, unsigned int b){
    //Serial.print("Test %d: ", testCount);
    if(a == b){
        print("Passed", ST77XX_WHITE);
    } else {
        print("Failed", ST77XX_WHITE);
        Serial.print("Expected: ");
        Serial.print(a);
        Serial.print(". But: ");
        Serial.println(b);
        stop();
    }
    testCount++;
}

void UnitTest::assert(double a, double b){
    //Serial.print("Test %d: ", testCount);
    if(a == b){
        print("Passed", ST77XX_WHITE);
    } else {
        print("Failed", ST77XX_WHITE);
        Serial.print("Expected: ");
        Serial.print(a);
        Serial.print(". But: ");
        Serial.println(b);
        stop();
    }
    testCount++;
}
