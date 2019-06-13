
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
        Arming,
        LockedArming,
        Armed,
        Disarming,
        LockedDisarming,
        Disarmed,
        Exploded
    };

    const char* const getKeyBuffer() const {
        return this->keypadBuffer;
    }
    
    int getKeyBufferSize() const {
        return this->keypadBufferSize;
    }

    int getKeyPosition() const {
        return this->keypadBufferPosition;
    }

    const char* const getBombCode() const {
        return this->bombCode;
    }
    
    int getBombCodeSize() const {
        return this->bombCodeSize;
    }

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

    BombMachine() {
        this->keypadBuffer = new char[this->keypadBufferSize];
        for(int i = 0; i < this->keypadBufferSize; i++) {
           this->keypadBuffer[i] = '\0';
        }
    }

private:
    union BombCode
    {
        const char code[9];
    };
    
    static const BombCode bombCodes[];
    const char* configCode = "8715003*";

    BombState state = BombState::Idle;
    
    unsigned int keypadBufferSize = 8;
    unsigned int keypadBufferPosition = 0;
    char* keypadBuffer;

    unsigned int bombCodeSize = 8;
    char* bombCode;
};



#endif