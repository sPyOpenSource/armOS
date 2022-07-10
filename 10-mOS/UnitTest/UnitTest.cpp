#include "UnitTest.h"

void UnitTest::start(){
    testCount = 1;
    printf("UnitTest:\n\r");
    unitTest.start();
}

void UnitTest::end(){
    unitTest.stop();
    printf("End: %fs\n\r", unitTest.read());
}

void UnitTest::assertOn(int a, int b){
    printf("Test %d: ", testCount);
    if(a == b){
        printf("Passed\n\r");
    } else {
        printf("Failed\n\r");
        while(true){
            wait(1);
        }
    }
    testCount++;
}