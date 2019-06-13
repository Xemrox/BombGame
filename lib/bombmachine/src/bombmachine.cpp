
#include "bombmachine.h"
#include <random>
#include <string.h>
#include <iostream>

bool BombMachine::allowStateChange(BombState newState) const {
    switch (this->state)
    {
    case Configuration: //config -> idle
        return (newState == Idle);
    case Configuring: //config -> config ||idle
        return (newState == Configuration || newState == Idle);
    case Idle: //idle -> arming || configuring
        return (newState == Arming || newState == Configuring);
    case Arming: //arming -> armed || idle || locked
        return (newState == Armed || newState == Idle || newState == LockedArming);
    case Armed: //armed -> disarming || exploded
        return (newState == Disarming || newState == Exploded);
    case Disarming: //disarming -> disarmed || locked || exploded
        return (newState == Disarmed || newState == LockedDisarming || newState == Exploded);
    case Disarmed: //disarmed -> idle
        return (newState == Idle);
    case Exploded: //exploded -> idle
        return (newState == Idle);
    default:
        return false;
    }
    return false;
}

void BombMachine::setState(BombState newState) {
    if(this->allowStateChange(newState))
        this->state = newState;
    //flush keypad after each state change
    this->keypadBufferPosition = 0;
    for(int i = 0; i < this->keypadBufferSize; i++) {
        this->keypadBuffer[i] = '\0';
    }
}

BombMachine::BombState BombMachine::getState() const {
    return this->state;
}

void BombMachine::pressButton() {
    switch (this->state)
    {
    case BombState::Idle:
        this->setState(BombState::Arming);
        break;
    case BombState::Arming: {
        bool armResult = this->tryArmBomb();
        //todo do something nasty here...
        break;
    }
    case BombState::Disarming: {
        bool disarmResult = this->tryDisarmBomb();
        //todo do something nasty here...
        break;
    }
    case BombState::Configuring: {
        if(memcmp(this->keypadBuffer, this->configCode, 8) == 0) {
            this->setState(BombState::Configuration);
        } else {
            this->setState(BombState::Idle);
        }
        break;
    }
    case BombState::Configuration:
        this->attemptConfigure();
        this->setState(BombState::Idle);
    case BombState::Disarmed:
    case BombState::Exploded:
        this->setState(BombState::Idle);
        break;
    default:
        return; //nothing happens if you are not in one of these states
    }
}

void BombMachine::inputKey(const char c) {
    if(
        this->state != BombState::Arming && 
        this->state != BombState::Disarming &&
        this->state != BombState::Configuring &&
        this->state != BombState::Configuration &&
        this->state != BombState::Idle
    ) {
        return; //nothing happens if you are not in one of these states
    }
    if(this->state == BombState::Idle && c == '*') {
        this->setState(BombState::Configuring);
        return;
    }

    if(c == '#') {
        if(this->keypadBufferPosition > 0)
            this->keypadBufferPosition--;
        this->keypadBuffer[this->keypadBufferPosition] = '\0';
        return;
    }

    if(this->keypadBufferPosition >= this->keypadBufferSize)
        return; //not allowed to enter more digits!
    
    this->keypadBuffer[this->keypadBufferPosition++] = c;
}

void BombMachine::prepareCode() {
    if(this->bombCode != nullptr) {
        delete[] this->bombCode;
    }
    //create a code with maximum length of the current code
    this->bombCode = new char[this->bombCodeSize + 1];
    //terminate code!
    this->bombCode[this->bombCodeSize] = '\0';

    static const char* rndBase = "0123456789";
    //int rnd = random(0, 10);
    for(int i = 0; i < this->bombCodeSize; i++) {
        unsigned int rnd = random() % 10;
        //std::cout << rnd << '-' << rndBase[rnd] << ' ';
        this->bombCode[i] = rndBase[rnd];
    }
    //std::cout << std::endl;
}

bool BombMachine::matchCode() const {
    if(this->keypadBufferPosition != this->bombCodeSize)
        return false;
    
    for(int i = 0; i < this->keypadBufferPosition; i++) {
        if(this->keypadBuffer[i] != this->bombCode[i])
            return false;
    }

    return true;
}

bool BombMachine::tryArmBomb() {
    if(this->state != BombState::Arming) {
        return false;
    }

    bool result = this->matchCode();
    if(!result)
        return false;
    
    this->armBomb();

    return true;
}

void BombMachine::armBomb() {
    /*if(this->state != BombState::Arming) {
        //add some kind of penalty here
        return;
    }*/

    this->setState(BombState::Armed);
    //todo set timer etc...
}

bool BombMachine::tryDisarmBomb() {
    if(this->state != BombState::Disarming) {
        return false;
    }

    bool result = this->matchCode();
    if(!result)
        return false;
    
    this->disarmBomb();

    return true;
}

void BombMachine::disarmBomb() {
    /*if(this->state != BombState::Disarming) {
        //maybe add some kind of penalty here
        return;
    }*/

    this->setState(BombState::Disarmed);
    //todo set timer etc...
}

void BombMachine::attemptConfigure() {
    static const char* codeLength = "0*";
    static const char* armMode = "1*";
    static const char* bombTimer = "2*";

    if(memcmp(this->keypadBuffer, codeLength, 2) == 0) {
        this->bombCodeSize = atoi((this->keypadBuffer + 2));
    }

    if(memcmp(this->keypadBuffer, armMode, 2) == 0) {
        //todo
    }

    if(memcmp(this->keypadBuffer, bombTimer, 2) == 0) {
        //todo
    }
}

const BombMachine::BombCode BombMachine::bombCodes[] = {
        {"8715003"},
        {"00000000"},
        {"99999999"},
        {"911"},
        {"8426"},
        {"9713"},
        {"84269713"},
        {"2206"},
        {"220694"},
        {"22061994"},
        {"31415926"},
        {"3141592"},
        {"314159"},
        {"31415"},
        {"3141"},
        {"9000"},
        {"29992"}, // -> lightspeed
        {"64"},
        {"128"},
        {"256"},
        {"512"},
        {"1024"},
        {"2048"},
        {"4096"},
        {"32768"},
        {"65536"},
        {"131072"},
        {"666"},
        {"7355608"}, // -> CS
        {"0815"},
        {"007"},
        {"404"},
        {"420"},
        {"42"},
        {"69"}
    };