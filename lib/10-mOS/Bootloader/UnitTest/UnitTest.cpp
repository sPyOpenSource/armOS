#include "UnitTest.h"

void UnitTest::start(){
    testCount = 1;
    printf("UnitTest:\n\n");
    unitTest.start();
}

void UnitTest::end(){
    unitTest.stop();
    printf("End: %fs\n", unitTest.read());
}

void UnitTest::assertOn(int a, int b){
    printf("Test %d: ", testCount);
    if(a == b){
        printf("Passed\n\n");
    } else {
        printf("Failed\n");
        while(true){
            wait(1);
        }
    }
    testCount++;
}
