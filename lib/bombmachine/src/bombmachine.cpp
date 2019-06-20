
#include "bombmachine.h"
#include <string.h>

#ifdef TEST
#include <ArduinoFake.h>
#else
#include "Arduino.h"
#endif

//#include <iostream>

bool BombMachine::allowStateChange(BombState newState) const
{
    switch (this->state)
    {
    case Configuration: //config -> idle
        return (newState == Idle);
    case Configuring: //config -> config ||idle
        return (newState == Configuration || newState == Idle);
    case Idle: //idle -> preparearming || configuring
        return (newState == PrepareArming || newState == Configuring);
    case PrepareArming:
        return (newState == Arming);
    case Arming: //arming -> armed || locked
        return (newState == Armed || newState == LockedArming);
    case LockedArming:
        return (newState == PrepareArming);
    case Armed: //armed -> disarming || exploded
        return (newState == PrepareDisarming || newState == Exploded);
    case PrepareDisarming:
        return (newState == Disarming || newState == Exploded);
    case Disarming: //disarming -> disarmed || locked || exploded
        return (newState == Disarmed || newState == LockedDisarming || newState == Exploded);
    case LockedDisarming:
        return (newState == Armed || newState == Exploded);
    case Disarmed: //disarmed -> idle
        return (newState == Idle);
    case Exploded: //exploded -> idle
        return (newState == Idle);
    default:
        return false;
    }
    return false;
}

void BombMachine::flushKeypad()
{
    this->keypadBufferPosition = 0;
    for (unsigned int i = 0; i < this->keypadBufferSize; i++)
    {
        this->keypadBuffer[i] = '\0';
    }
}

void BombMachine::setState(BombState newState)
{
    if (!this->allowStateChange(newState))
        return;

    //instead of disarming to lock -> disarm bomb!
    if (this->state == BombState::Disarming && newState == BombState::LockedDisarming && this->activeFeatures & ReverseBomb)
        newState = BombState::Disarmed;

    //TODO maybe notify about change for components here...

    this->setActionTimer(newState);

    switch (newState)
    {
    case BombState::Idle:
    {
        this->strikeCount = 0;
        break;
    }
    case BombState::PrepareArming:
    case BombState::PrepareDisarming:
    {
        this->prepareCode();
        break;
    }
    case BombState::LockedArming:
    case BombState::LockedDisarming:
    {

        if (newState == BombState::LockedArming)
            break;

        this->strikeCount++;
        if (this->strikeCount >= this->maximumStrikeCount)
            newState = BombState::Exploded;

        break;
    }
    case BombState::Armed:
    {
        break;
    }
    default:
        break;
    }

    this->state = newState;

    //flush keypad after each state change
    this->flushKeypad();
}

void BombMachine::setActionTimer(BombMachine::BombState newState)
{
    switch (newState)
    {
    case BombState::Idle:
    {
        this->actionTimer = 0;
        this->bombTimer = 0;
        break;
    }
    case BombState::PrepareArming:
    {
        this->actionTimer = this->armDisplayTime;
        break;
    }
    case BombState::PrepareDisarming:
    {
        this->actionTimer = this->disarmDisplayTime;
        break;
    }
    case BombState::Armed:
        //only set the timer if we come from arming
        if (this->state == BombState::Arming)
        {
            this->bombTimer = this->getTotalBombTime();
        }
        break;
    case BombState::LockedArming:
    case BombState::LockedDisarming:
    {
        this->actionTimer = this->lockdownTime;
        break;
    }
    case BombState::Disarmed:
    case BombState::Exploded:
    {
        this->bombTimer = 0;
        break;
    }
    default:
        break;
    }
}

unsigned long BombMachine::getTotalBombTime() const
{
    unsigned long modPos = 0;
    unsigned long modNeg = 0;
    unsigned long timer = this->bombTime;
    if (this->activeFeatures & Quick)
        modNeg += timer / 10;
    if (this->activeFeatures & ExtraQuick)
        modNeg += timer / 10;
    if (this->activeFeatures & Slow)
        modPos += timer / 10;
    if (this->activeFeatures & ExtraSlow)
        modPos += timer / 10;

    if (
        (modPos > modNeg && modPos - modNeg > this->actionTimer) ||
        (modNeg >= modPos && modNeg - modPos > this->actionTimer))
    {
        //lower limit for modifications
        return 100;
    }
    else
    {
        return timer + modPos - modNeg;
    }
}

unsigned long BombMachine::getRemainingBombTime() const
{
    return this->bombTimer;
}

unsigned long BombMachine::getRemainingActionTime() const {
    return this->actionTimer;
}

unsigned int BombMachine::getStrikeCount() const {
    return this->strikeCount;
}

BombMachine::BombState BombMachine::getState() const
{
    return this->state;
}

const char *const BombMachine::getKeyBuffer() const
{
    return this->keypadBuffer;
}

int BombMachine::getKeyBufferSize() const
{
    return this->keypadBufferSize;
}

int BombMachine::getKeyPosition() const
{
    return this->keypadBufferPosition;
}

const char *const BombMachine::getBombCode() const
{
    return this->bombCode;
}

unsigned int BombMachine::getBombCodeSize() const
{
    return this->bombCodeSize;
}

void BombMachine::pressButton()
{
    switch (this->state)
    {
    case BombState::Idle:
        //strlen(code)
        for(int i = 0; i < 0; i++) {
            BombCode bc = BombMachine::bombCodes[i];
            if(strlen(bc.code) != this->keypadBufferPosition)
                continue;

            if(strcmp(bc.code, this->keypadBuffer) == 0) {
                this->activeFeatures |= bc.features;
            }
        }

        this->setState(BombState::PrepareArming);
        break;
    case BombState::Arming:
    {
        bool armResult = this->tryArmBomb();
        if (!armResult)
            this->setState(BombState::LockedArming);
        break;
    }
    case BombState::Disarming:
    {
        bool disarmResult = this->tryDisarmBomb();
        if (!disarmResult)
            this->setState(BombState::LockedDisarming);
        break;
    }
    case BombState::Armed:
    {
        this->setState(BombState::PrepareDisarming);
        break;
    }
    case BombState::Configuring:
    {
        if (memcmp(this->keypadBuffer, this->configCode, 8) == 0)
        {
            this->setState(BombState::Configuration);
        }
        else
        {
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

void BombMachine::inputKey(const char c)
{
    if (
        this->state != BombState::Arming &&
        this->state != BombState::Disarming &&
        this->state != BombState::Configuring &&
        this->state != BombState::Configuration &&
        this->state != BombState::Idle)
    {
        return; //nothing happens if you are not in one of these states
    }
    if (this->state == BombState::Idle && c == '*')
    {
        this->setState(BombState::Configuring);
        return;
    }

    if (c == '#')
    {
        if (this->keypadBufferPosition > 0)
            this->keypadBufferPosition--;
        this->keypadBuffer[this->keypadBufferPosition] = '\0';
        return;
    }

    if (this->keypadBufferPosition >= this->keypadBufferSize)
        return; //not allowed to enter more digits!

    this->keypadBuffer[this->keypadBufferPosition++] = c;
}

void BombMachine::prepareCode()
{
    if (this->bombCode != nullptr)
    {
        delete[] this->bombCode;
    }
    //create a code with maximum length of the current code
    this->bombCode = new char[this->bombCodeSize + 1];
    //terminate code!
    this->bombCode[this->bombCodeSize] = '\0';

    const char rndBase[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

    int effectiveCodeSize = this->bombCodeSize;
    if (this->activeFeatures & FastCode)
        effectiveCodeSize -= 1;
    if (this->activeFeatures & ExtraFastCode)
        effectiveCodeSize -= 2;

    for (int i = 0; i < effectiveCodeSize; i++)
    {
        unsigned int rnd = random(10);
        //std::cout << rnd << '-' << rndBase[rnd] << ' ';
        this->bombCode[i] = rndBase[rnd];
    }
    //std::cout << std::endl;
}

bool BombMachine::matchCode() const
{
    if (this->keypadBufferPosition != this->bombCodeSize)
        return false;

    //alternative memcmp

    return 0 == memcmp(this->keypadBuffer, this->bombCode, this->bombCodeSize);

    /*for (unsigned int i = 0; i < this->keypadBufferPosition; i++)
    {
        if (this->keypadBuffer[i] != this->bombCode[i])
            return false;
    }*/

    //return true;
}

bool BombMachine::tryArmBomb()
{
    if (this->state != BombState::Arming)
    {
        return false;
    }

    bool result = this->matchCode();
    if (!result)
        return false;

    this->setState(BombState::Armed);

    return true;
}

bool BombMachine::tryDisarmBomb()
{
    if (this->state != BombState::Disarming)
    {
        return false;
    }

    bool result = this->matchCode();
    if (!result)
        return false;

    this->setState(BombState::Disarmed);

    return true;
}

void BombMachine::attemptConfigure()
{
    static const char *codeLength = "0*";
    static const char *bombTimer = "1*";
    static const char *displayTimer = "2*";
    static const char *lockdownTimer = "3*";
    static const char *strikes = "4*";
    //static const char *armMode = "2*";

    if (memcmp(this->keypadBuffer, codeLength, 2) == 0)
    {
        this->bombCodeSize = atol((this->keypadBuffer + 2));
    }

    if (memcmp(this->keypadBuffer, bombTimer, 2) == 0) {
        this->bombTime = atol((this->keypadBuffer + 2)) * 1000L;
    }

    if (memcmp(this->keypadBuffer, displayTimer, 2) == 0) {
        this->armDisplayTime = this->disarmDisplayTime = atol((this->keypadBuffer + 2)) * 1000L;
    }

    if (memcmp(this->keypadBuffer, bombTimer, 2) == 0) {
        this->lockdownTimer = atol((this->keypadBuffer + 2)) * 1000L;
    }   

    if (memcmp(this->keypadBuffer, strikes, 2) == 0) {
        this->maximumStrikeCount = atol((this->keypadBuffer + 2));
    }

    

    /*if (memcmp(this->keypadBuffer, armMode, 2) == 0)
    {
        //todo
        //0 -> InterruptKeypad
        if (this->keypadBuffer[2] > 0)
        {
            this->activeFeatures |= InterruptKeypad;
        }
        else
        {
            this->activeFeatures ^= this->activeFeatures & InterruptKeypad;
        }

        if (memcmp(this->keypadBuffer, bombTimer, 2) == 0)
        {
            //todo
        }
    }*/
}

void BombMachine::tick(unsigned long delta)
{
    //watch out for underflow
    if (this->bombTimer > delta)
    {
        this->bombTimer -= delta;
    }
    else if (this->bombTimer <= delta)
    {
        this->bombTimer = 0;
    }

    //watch out for underflow
    if (this->actionTimer > delta)
    {
        this->actionTimer -= delta;
    }
    else if (this->actionTimer <= delta)
    {
        this->actionTimer = 0;
    }

    if (this->bombTimer > 0 && this->actionTimer > 0)
        return;

    switch (this->state)
    {
    case BombState::PrepareArming:
    {
        if (this->actionTimer == 0)
            this->setState(BombState::Arming);
        break;
    }
    case BombState::LockedArming:
    {
        //lockdown over -> move to previous prepare state
        if (this->actionTimer == 0)
            this->setState(BombState::PrepareArming);
        break;
    }
    case BombState::Armed:
    {
        //bomb timer expired
        if (this->bombTimer == 0)
            this->setState(BombState::Exploded);
        break;
    }
    case BombState::Disarming:
    {
        if (this->bombTimer == 0)
            this->setState(BombState::Exploded);
        break;
    }
    case BombState::PrepareDisarming:
    {
        //display time expired -> move to next state
        if (this->bombTimer == 0)
        {
            this->setState(BombState::Exploded);
            return;
        }
        if (this->actionTimer == 0)
        {
            this->setState(BombState::Disarming);
        }
        break;
    }
    case BombState::LockedDisarming:
    {
        if (this->bombTimer == 0)
        {
            this->setState(BombState::Exploded);
            return;
        }
        if (this->actionTimer == 0)
        {
            //lockdown over -> move to armed state
            this->setState(BombState::Armed);
        }
        break;
    }
    default:
        break;
    }
}

const BombMachine::BombCode BombMachine::bombCodes[] = {
    //{"8715003"},
    {"00000000", ExtraSlow},
    {"99999999", ExtraSlow},
    //{"911"},
    {"8426", ReverseBomb},
    //{"9713"},
    //{"84269713"},
    {"2206", Slow},
    //{"220694"},
    {"22061994", ExtraFastCode},
    {"31415926", ExtraQuick | FastCode},
    {"3141592", ExtraQuick},
    {"314159", Quick},
    //{"31415"},
    //{"3141"},
    //{"9000"},
    //{"29992"}, // -> lightspeed
    //{"64"},
    //{"128"},
    //{"256"},
    //{"512"},
    //{"1024"},
    //{"2048"},
    //{"4096"},
    //{"32768"},
    //{"65536"},
    //{"131072"},
    //{"666"},
    {"7355608", Quick | FastCode}, // -> CS
    {"0815", Slow},
    //{"007"},
    //{"404"},
    //{"420"},
    //{"42"},
    //{"69"}
};
