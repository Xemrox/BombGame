#include "bombmachine.h"
#include <unity.h>
#include <time.h>
#include <iostream>

BombMachine* bomb;

void setUp(void) {
    srandom(time(NULL));
    //randomSeed(analogRead(0));
    bomb = new BombMachine("1234567*");
}

void tearDown(void) {
    delete bomb;
}

void checkStateChanges(BombMachine::BombState changes[], int size, bool allowed) {
    for(int i = 0; i < size; i++) {
        /*bool result = bomb->allowStateChange([changes[i]]);
        if(allowed && !result) {

        } elseif(!allowed && result) {
            TEST_FAIL_MESSAGE();
        }*/
        if(allowed) {
            TEST_ASSERT_TRUE(bomb->allowStateChange(changes[i]));
        } else {
            TEST_ASSERT_FALSE(bomb->allowStateChange(changes[i]));
        }
    }
}

void test_function_initialState() {
    BombMachine::BombState allowedTransitions[] = {
        BombMachine::BombState::Configuring,
        BombMachine::BombState::PrepareArming
    };
    BombMachine::BombState disallowedTransitions[] = {
        BombMachine::BombState::Idle,
        BombMachine::BombState::Arming,
        BombMachine::BombState::Armed,
        BombMachine::BombState::Disarmed,
        BombMachine::BombState::PrepareDisarming,
        BombMachine::BombState::Disarming,
        BombMachine::BombState::Exploded,
        BombMachine::BombState::LockedArming,
        BombMachine::BombState::LockedDisarming,
        BombMachine::BombState::Configuration
    };

    TEST_ASSERT_EQUAL_INT32(BombMachine::Idle, bomb->getState());

    checkStateChanges(allowedTransitions, 2, true);
    checkStateChanges(disallowedTransitions, 7, false);
}

void test_function_armingBombState() {
    bomb->setState(BombMachine::BombState::Arming);
    TEST_ASSERT_EQUAL_INT32(BombMachine::Idle, bomb->getState());

    bomb->setState(BombMachine::BombState::PrepareArming);
    TEST_ASSERT_EQUAL_INT32(BombMachine::PrepareArming, bomb->getState());
    bomb->setState(BombMachine::BombState::Arming);
    TEST_ASSERT_EQUAL_INT32(BombMachine::Arming, bomb->getState());

    BombMachine::BombState allowedTransitions[] = {
        BombMachine::BombState::Armed,
        BombMachine::BombState::LockedArming
    };
    BombMachine::BombState disallowedTransitions[] = {
        BombMachine::BombState::Idle,
        BombMachine::BombState::Disarmed,
        BombMachine::BombState::Disarming,
        BombMachine::BombState::Disarming,
        BombMachine::BombState::Exploded,
        BombMachine::BombState::LockedDisarming,
        BombMachine::BombState::Configuring,
        BombMachine::BombState::Configuration
    };
    checkStateChanges(allowedTransitions, 2, true);
    checkStateChanges(disallowedTransitions, 6, false);
}

void test_function_inputNormal() {
    bomb->setState(BombMachine::BombState::Arming);

    //bomb->inputKey('#');
    bomb->inputKey('0');

    const char* const buffer = bomb->getKeyBuffer();
    int bufferSize = bomb->getKeyBufferSize();
    int keyposition = bomb->getKeyPosition();

    TEST_ASSERT_EQUAL_INT32(8, bufferSize);
    TEST_ASSERT_EQUAL_INT32(1, keyposition);
    TEST_ASSERT_TRUE(buffer[0] == '0');
    TEST_ASSERT_TRUE(buffer[1] == '\0');
}

void test_function_inputOver() {
    bomb->setState(BombMachine::BombState::Arming);

    //bomb->inputKey('#');
    bomb->inputKey('0');//0
    bomb->inputKey('1');//01
    bomb->inputKey('2');//01 2
    bomb->inputKey('3');//01 23
    bomb->inputKey('4');//01 23 4
    bomb->inputKey('5');//01 23 45
    bomb->inputKey('6');//01 23 45 6
    bomb->inputKey('7');//01 23 45 67
    bomb->inputKey('8');//01 23 45 67
    bomb->inputKey('9');//01 23 45 67

    const char* const buffer = bomb->getKeyBuffer();
    int bufferSize = bomb->getKeyBufferSize();
    int keyposition = bomb->getKeyPosition();

    TEST_ASSERT_EQUAL_INT32(8, bufferSize);
    TEST_ASSERT_EQUAL_INT32(8, keyposition);
    TEST_ASSERT_TRUE(buffer[0] == '0');
    TEST_ASSERT_TRUE(buffer[1] == '1');
    TEST_ASSERT_TRUE(buffer[2] == '2');
    TEST_ASSERT_TRUE(buffer[3] == '3');
    TEST_ASSERT_TRUE(buffer[4] == '4');
    TEST_ASSERT_TRUE(buffer[5] == '5');
    TEST_ASSERT_TRUE(buffer[6] == '6');
    TEST_ASSERT_TRUE(buffer[7] == '7');
    TEST_ASSERT_TRUE(buffer[8] != '8');
}

void test_function_inputRemoveMiddle() {
    bomb->setState(BombMachine::BombState::Arming);

    bomb->inputKey('0');//0
    bomb->inputKey('1');//01
    bomb->inputKey('2');//01 2
    bomb->inputKey('3');//01 23
    bomb->inputKey('4');//01 23 4
    bomb->inputKey('5');//01 23 45
    bomb->inputKey('6');//01 23 45 6
    bomb->inputKey('7');//01 23 45 67
    bomb->inputKey('#');//01 23 45 6
    TEST_ASSERT_EQUAL_INT32(7, bomb->getKeyPosition());
    bomb->inputKey('#');//01 23 45
    bomb->inputKey('8');//01 23 45 8
    bomb->inputKey('9');//01 23 45 89

    const char* const buffer = bomb->getKeyBuffer();
    int bufferSize = bomb->getKeyBufferSize();
    int keyposition = bomb->getKeyPosition();

    TEST_ASSERT_EQUAL_INT32(8, bufferSize);
    TEST_ASSERT_EQUAL_INT32(8, keyposition);
    TEST_ASSERT_TRUE(buffer[0] == '0');
    TEST_ASSERT_TRUE(buffer[1] == '1');
    TEST_ASSERT_TRUE(buffer[2] == '2');
    TEST_ASSERT_TRUE(buffer[3] == '3');
    TEST_ASSERT_TRUE(buffer[4] == '4');
    TEST_ASSERT_TRUE(buffer[5] == '5');
    TEST_ASSERT_TRUE(buffer[6] == '8');
    TEST_ASSERT_TRUE(buffer[7] == '9');
}

void test_function_inputRemoveToStart() {
    bomb->setState(BombMachine::BombState::Arming);
    TEST_ASSERT_EQUAL_INT32(0, bomb->getKeyPosition());
    bomb->inputKey('0');//0
    TEST_ASSERT_EQUAL_INT32(1, bomb->getKeyPosition());
    bomb->inputKey('1');//01
    TEST_ASSERT_EQUAL_INT32(2, bomb->getKeyPosition());
    bomb->inputKey('#');//0
    TEST_ASSERT_EQUAL_INT32(1, bomb->getKeyPosition());
    const char* const buffer = bomb->getKeyBuffer();
    TEST_ASSERT_TRUE(buffer[0] == '0');
    TEST_ASSERT_TRUE(buffer[1] == '\0');

    bomb->inputKey('#');//
    TEST_ASSERT_TRUE(buffer[0] == '\0');
    TEST_ASSERT_EQUAL_INT32(0, bomb->getKeyPosition());
    bomb->inputKey('#');//
    TEST_ASSERT_EQUAL_INT32(0, bomb->getKeyPosition());
}

void test_function_createBombCode() {
    unsigned int rnd = random() % 10;
    TEST_ASSERT_EQUAL_INT32(0, 10 % 10);
    TEST_ASSERT_EQUAL_INT32(9, 9 % 10);

    TEST_ASSERT_EQUAL_INT32(8, bomb->getBombCodeSize());
    bomb->prepareCode();
    //std::cout << bomb->getBombCode() << std::endl;
}

void test_function_testCode() {
    bomb->setState(BombMachine::BombState::Arming);
    bomb->prepareCode();
    const char* const bombCode = bomb->getBombCode();

    for(int i = 0; i < bomb->getBombCodeSize(); i++) {
        bomb->inputKey(bombCode[i]);
    }
    TEST_ASSERT_EQUAL_INT32(bomb->getKeyPosition(), bomb->getBombCodeSize());

    TEST_ASSERT_TRUE(bomb->matchCode());
}

void test_function_enterConfig() {

    TEST_ASSERT_TRUE(bomb->getState() == BombMachine::BombState::Idle);
    bomb->inputKey('*');
    TEST_ASSERT_TRUE(bomb->getState() == BombMachine::BombState::Configuring);

    bomb->inputKey('1');
    bomb->inputKey('2');
    bomb->inputKey('3');
    bomb->inputKey('4');
    bomb->inputKey('5');
    bomb->inputKey('6');
    bomb->inputKey('7');
    bomb->inputKey('*');

    bomb->pressButton();

    TEST_ASSERT_TRUE(bomb->getState() == BombMachine::BombState::Configuration);
}

void test_function_failEnterConfig() {

    TEST_ASSERT_TRUE(bomb->getState() == BombMachine::BombState::Idle);
    bomb->inputKey('*');
    TEST_ASSERT_TRUE(bomb->getState() == BombMachine::BombState::Configuring);

    bomb->inputKey('1');
    bomb->inputKey('2');
    bomb->inputKey('3');
    bomb->inputKey('4');
    bomb->inputKey('5');
    bomb->inputKey('6');
    bomb->inputKey('0');
    bomb->inputKey('*');

    bomb->pressButton();
    TEST_ASSERT_FALSE(bomb->getState() == BombMachine::BombState::Configuring);
    TEST_ASSERT_FALSE(bomb->getState() == BombMachine::BombState::Configuration);
    TEST_ASSERT_TRUE(bomb->getState() == BombMachine::BombState::Idle);
}

void test_function_configureCodeLength() {
    bomb->setState(BombMachine::BombState::Configuring);
    //skip check here
    bomb->setState(BombMachine::BombState::Configuration);
    TEST_ASSERT_TRUE(bomb->getState() == BombMachine::BombState::Configuration);

    bomb->inputKey('0');//config option 0
    bomb->inputKey('*');//delimiter
    bomb->inputKey('0');
    bomb->inputKey('5');

    bomb->pressButton();
    TEST_ASSERT_TRUE(bomb->getState() == BombMachine::BombState::Idle);

    TEST_ASSERT_EQUAL_INT32(5, bomb->getBombCodeSize());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_function_initialState);
    RUN_TEST(test_function_armingBombState);
    RUN_TEST(test_function_inputNormal);
    RUN_TEST(test_function_inputOver);
    RUN_TEST(test_function_inputRemoveMiddle);
    RUN_TEST(test_function_inputRemoveToStart);
    RUN_TEST(test_function_createBombCode);
    RUN_TEST(test_function_testCode);
    RUN_TEST(test_function_enterConfig);
    RUN_TEST(test_function_failEnterConfig);
    RUN_TEST(test_function_configureCodeLength);

    UNITY_END();

    return 0;
}