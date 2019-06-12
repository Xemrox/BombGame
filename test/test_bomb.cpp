
#include "BombMachine.h"
#include <unity.h>

BombMachine* bomb;

void setUp(void) {
    bomb = new BombMachine();
}

void tearDown(void) {
    delete bomb;
}

void test_function_test() {

}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_function_test);
    UNITY_END();
    
    return 0;
}