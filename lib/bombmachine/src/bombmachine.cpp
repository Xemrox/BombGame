
#include "bombmachine.h"
#include <random>
#include <string.h>
#include <iostream>

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
        return (newState == PrepareDisarming || newState == Disarmed);
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
    for (int i = 0; i < this->keypadBufferSize; i++)
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
    this->state = newState;

    switch (this->state)
    {
    case BombState::PrepareArming:
    case BombState::PrepareDisarming:
        this->setActionTimer();
        this->prepareCode();
        break;
    case BombState::LockedArming:
    case BombState::LockedDisarming:
        this->setActionTimer();
        this->strikeCount++;
        if (this->strikeCount >= this->maximumStrikeCount)
            //you failed!!
            this->state = BombState::Exploded;
        break;
    case BombState::Armed:
        this->setActionTimer();
        //reset strike counter
        this->strikeCount = 0;
        break;
    default:
        break;
    }

    //flush keypad after each state change
    this->flushKeypad();
}

void BombMachine::setActionTimer()
{
    long mod = 0;
    switch (this->state)
    {
    case BombState::PrepareArming:
        this->actionTimer = this->armDisplayTime;
        break;
    case BombState::PrepareDisarming:
        this->actionTimer = this->disarmDisplayTime;
        break;
    case BombState::Armed:
        this->actionTimer = this->bombTime;
        if (this->activeFeatures & Quick)
            mod -= this->actionTimer / 10;
        if (this->activeFeatures & ExtraQuick)
            mod -= this->actionTimer / 10;
        if (this->activeFeatures & Slow)
            mod += this->actionTimer / 10;
        if (this->activeFeatures & ExtraSlow)
            mod += this->actionTimer / 10;
        break;
    case BombState::LockedArming:
    case BombState::LockedDisarming:
        this->actionTimer = this->lockdownTime;
    default:
        break;
    }
    this->actionTimer += mod;
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

int BombMachine::getBombCodeSize() const
{
    return this->bombCodeSize;
}

void BombMachine::pressButton()
{
    switch (this->state)
    {
    case BombState::Idle:
        this->setState(BombState::Arming);
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
        {
            this->setState(BombState::LockedDisarming);
        }
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

    const char *rndBase = "0123456789";
    //int rnd = random(0, 10);

    int effectiveCodeSize = this->bombCodeSize;
    if (this->activeFeatures & FastCode)
        effectiveCodeSize -= 1;
    if (this->activeFeatures & ExtraFastCode)
        effectiveCodeSize -= 2;

    for (int i = 0; i < effectiveCodeSize; i++)
    {
        unsigned int rnd = random() % 10;
        //std::cout << rnd << '-' << rndBase[rnd] << ' ';
        this->bombCode[i] = rndBase[rnd];
    }
    //std::cout << std::endl;
}

bool BombMachine::matchCode() const
{
    if (this->keypadBufferPosition != this->bombCodeSize)
        return false;

    for (int i = 0; i < this->keypadBufferPosition; i++)
    {
        if (this->keypadBuffer[i] != this->bombCode[i])
            return false;
    }

    return true;
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

    this->armBomb();

    return true;
}

void BombMachine::armBomb()
{
    /*if(this->state != BombState::Arming) {
        //add some kind of penalty here
        return;
    }*/

    this->setState(BombState::Armed);
    //todo set timer etc...
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

    this->disarmBomb();

    return true;
}

void BombMachine::disarmBomb()
{
    /*if(this->state != BombState::Disarming) {
        //maybe add some kind of penalty here
        return;
    }*/

    this->setState(BombState::Disarmed);
    //todo set timer etc...
}

void BombMachine::attemptConfigure()
{
    static const char *codeLength = "0*";
    static const char *armMode = "1*";
    static const char *bombTimer = "2*";

    if (memcmp(this->keypadBuffer, codeLength, 2) == 0)
    {
        this->bombCodeSize = atoi((this->keypadBuffer + 2));
    }

    if (memcmp(this->keypadBuffer, armMode, 2) == 0)
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
    }
}

void BombMachine::tick(unsigned long delta)
{
    if (this->actionTimer > 0)
        this->actionTimer -= delta;

    if (this->actionTimer > 0)
        return;

    switch (this->state)
    {
    case BombState::Armed:
    {
        //bomb timer expired
        this->setState(BombState::Exploded);
        break;
    }
    case BombState::PrepareArming:
    case BombState::PrepareDisarming:
    {
        //display time expired -> move to next state
        this->setState(static_cast<BombState>(static_cast<int>(this->state) + 1));
        break;
    }
    case BombState::LockedArming:
    case BombState::LockedDisarming:
    {
        //lockdown over -> move to previous prepare state
        this->setState(static_cast<BombState>(static_cast<int>(this->state) - 2));
        break;
    }
    default:
        break;
    }
}

const BombMachine::BombCode BombMachine::bombCodes[] = {
    //{"8715003"},
    //{"00000000"},
    {"99999999", ExtraFastCode},
    //{"911"},
    //{"8426"},
    //{"9713"},
    //{"84269713"},
    //{"2206"},
    //{"220694"},
    {"22061994", FastCode | AllSilent},
    {"31415926", ExtraQuick | SilentArm | Noisy | FastCode},
    {"3141592", ExtraQuick | ExtraNoisy},
    {"314159", Quick | Noisy},
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
    //{"7355608"}, // -> CS
    //{"0815"},
    //{"007"},
    //{"404"},
    //{"420"},
    //{"42"},
    //{"69"}
};