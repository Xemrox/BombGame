
#ifndef BombMachine_h
#define BombMachine_h

//this is a bomb game state machine!
class BombMachine
{
public:
    enum BombState
    {
        Undefined = 0,
        Configuring,
        Configuration,
        Idle,

        PrepareArming,
        Arming,
        LockedArming,
        Armed,

        PrepareDisarming,
        Disarming,
        LockedDisarming,
        Disarmed,

        Exploded
    };

    enum BombFeature
    {
        SilentArm = 1 << 0,
        SilentDisarm = 1 << 2,
        AllSilent = 3 << 0, //0+1
        Quick = 1 << 3,
        ExtraQuick = 3 << 3, //3+4
        Slow = 1 << 5,
        ExtraSlow = 3 << 5, //5+6
        ReverseSound = 1 << 7,
        Noisy = 1 << 8,
        ExtraNoisy = 3 << 8, //8+9

        InterruptKeypad = 1 << 10,
        FastCode = 1 << 11,
        ExtraFastCode = 3 << 11, //11+12

        ReverseBomb = 1 << 13
    };

    const char *const getKeyBuffer() const;
    int getKeyBufferSize() const;
    int getKeyPosition() const;

    const char *const getBombCode() const;
    int getBombCodeSize() const;

    BombState getState() const;
    void setState(BombState);
    bool allowStateChange(BombState) const;

    void pressButton();
    void inputKey(const char);

    void prepareCode();

    bool tryArmBomb();
    void armBomb();

    bool tryDisarmBomb();
    void disarmBomb();

    void attemptConfigure();

    bool matchCode() const;
    //gets called every few ticks
    void tick(unsigned long delta);

    BombMachine(const char[9] configCode) : configCode(configCode)
    {
        this->keypadBuffer = new char[this->keypadBufferSize];
        for (int i = 0; i < this->keypadBufferSize; i++)
        {
            this->keypadBuffer[i] = '\0';
        }
    }

private:
    struct BombCode
    {
        const char code[9];
        BombFeatures features;
        BombCode(const char[9] code, BombFeature features) : code(code), features(features{};
    };

    void flushKeypad();

    static const BombCode bombCodes[];
    const char *configCode;

    BombState state = BombState::Idle;

    unsigned int keypadBufferSize = 8;
    unsigned int keypadBufferPosition = 0;
    char *keypadBuffer;

    unsigned int bombCodeSize = 8;
    char *bombCode;

    BombFeature activeFeatures;

    unsigned long maximumStrikeCount = 3;

    void setActionTimer();

    unsigned long armDisplayTime = 5 * 1000;
    unsigned long disarmDisplayTime = 5 * 1000;
    unsigned long bombTime = 5 * 60 * 1000;
    unsigned long lockdownTime = 5 * 1000;

    unsigned int strikeCount = 0;
    unsigned long actionTimer = 0;
};

#endif