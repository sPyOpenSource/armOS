#include "UnitTest.h"

void UnitTest::start(){
    testCount = 1;
    Serial.println("UnitTest:");
    Serial.println();
}

void UnitTest::end(){
    Serial.println("End");
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
