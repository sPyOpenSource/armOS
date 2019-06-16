#include "UnitTest.h"
#include <Adafruit_ST7735.h> // Hardware-specific library

void drawtext(const char *text, uint16_t color);

void UnitTest::start(){
    testCount = 1;
    Serial.println("UnitTest:");
    drawtext("UnitTest:", ST77XX_WHITE);
    Serial.println();
    time = millis();
}

void UnitTest::end(){
    Serial.print("End: ");
    Serial.print((millis() - time) / 1000.0);
    Serial.println("s");
    Serial.println();
}

void UnitTest::assert(int a, int b){
    printf("Test %d: ", testCount);
    if(a == b){
        Serial.println("Passed");
        Serial.println();
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
    printf("Test %d: ", testCount);
    if(a == b){
        Serial.println("Passed");
        drawtext("Passed", ST77XX_WHITE);
        Serial.println();
    } else {
        Serial.println("Failed");
        drawtext("Failed", ST77XX_WHITE);
        Serial.print("Expected: ");
        Serial.print(a);
        Serial.print(". But: ");
        Serial.println(b);
        stop();
    }
    testCount++;
}

void UnitTest::assert(double a, double b){
    printf("Test %d: ", testCount);
    if(a == b){
        Serial.println("Passed");
        drawtext("Passed", ST77XX_WHITE);
        Serial.println();
    } else {
        Serial.println("Failed");
        drawtext("Failed", ST77XX_WHITE);
        Serial.print("Expected: ");
        Serial.print(a);
        Serial.print(". But: ");
        Serial.println(b);
        stop();
    }
    testCount++;
}
