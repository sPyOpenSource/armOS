#include "UnitTest.h"

void UnitTest::start(){
    Serial.begin(115200);
    testCount = 1;
    Serial.println("UnitTest:");
    Serial.println();
    time = millis();
}

void UnitTest::end(){
    Serial.print("End: ");
    Serial.print((millis()-time)/1000.0);
    Serial.println("s");
    Serial.println();
}

void UnitTest::assert(int a, int b){
    printf("Test %d: ", testCount);
    if(a==b){
        Serial.println("Passed");
        Serial.println();
    } else {
        Serial.println("Failed");
        Serial.print("Expected: ");
        Serial.print(a);
        Serial.print(". But: ");
        Serial.println(b);
        while(true){}
    }
    testCount++;
}

void UnitTest::assert(unsigned int a, unsigned int b){
    printf("Test %d: ", testCount);
    if(a==b){
        Serial.println("Passed");
        Serial.println();
    } else {
        Serial.println("Failed");
        Serial.print("Expected: ");
        Serial.print(a);
        Serial.print(". But: ");
        Serial.println(b);
        while(true){}
    }
    testCount++;
}

void UnitTest::assert(double a, double b){
    printf("Test %d: ", testCount);
    if(a==b){
        Serial.println("Passed");
        Serial.println();
    } else {
        Serial.println("Failed");
        Serial.print("Expected: ");
        Serial.print(a);
        Serial.print(". But: ");
        Serial.println(b);
        while(true){}
    }
    testCount++;
}
