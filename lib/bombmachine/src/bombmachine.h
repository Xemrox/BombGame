
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

    //enum BombFeature : int
    //{
        //#define SilentArm 1 << 0
        //#define SilentDisarm 1 << 1
        //#define AllSilent 3 << 0 //0+1
        #define Quick 1 << 3
        #define ExtraQuick 3 << 3 //3+4
        #define Slow 1 << 5
        #define ExtraSlow 3 << 5 //5+6
        
        //#define ReverseSound 1 << 7
        //#define Noisy 1 << 8
        //#define ExtraNoisy 3 << 8 //8+9

        //#define InterruptKeypad 1 << 10
        #define FastCode 1 << 11
        #define ExtraFastCode 3 << 11 //11+12

        #define ReverseBomb 1 << 13
    //};

    const char *const getKeyBuffer() const;
    int getKeyBufferSize() const;
    int getKeyPosition() const;

    const char *const getBombCode() const;
    unsigned int getBombCodeSize() const;

    BombState getState() const;
    void setState(BombState);
    bool allowStateChange(BombState) const;

    void pressButton();
    void inputKey(const char);

    void prepareCode();

    bool tryArmBomb();
    bool tryDisarmBomb();

    void attemptConfigure();

    bool matchCode() const;
    //gets called every few ticks
    void tick(unsigned long delta);

    unsigned long getRemainingBombTime() const;
    unsigned long getTotalBombTime() const;

    unsigned long getRemainingActionTime() const;

    unsigned int getStrikeCount() const;

    BombMachine(const char configCode[9], void (*changeHandler)(void)) : configCode(configCode)
    {
        this->keypadBuffer = new char[this->keypadBufferSize];
        for (unsigned int i = 0; i < this->keypadBufferSize; i++)
        {
            this->keypadBuffer[i] = '\0';
        }
    }

private:
    struct BombCode
    {
        const char *code;
        const int features;
        BombCode(const char code[9], int features) : code(code), features(features){};
    };

    void flushKeypad();

    static const BombCode bombCodes[];
    const char *configCode;

    BombState state = BombState::Idle;

    unsigned int keypadBufferSize = 8;
    unsigned int keypadBufferPosition = 0;
    char *keypadBuffer;

    unsigned int bombCodeSize = 5;
    char *bombCode;

    int activeFeatures;

    unsigned long maximumStrikeCount = 4;

    void setActionTimer(BombState newState);

    unsigned long int armDisplayTime = 2UL * 1000UL;
    unsigned long int disarmDisplayTime = 2UL * 1000UL;
    unsigned long int bombTime = /*1UL */ 10UL * 1000UL;
    unsigned long int lockdownTime = 15UL * 1000UL;

    unsigned int strikeCount = 0;
    unsigned long actionTimer = 0;
    unsigned long bombTimer = 0;
};


#endif