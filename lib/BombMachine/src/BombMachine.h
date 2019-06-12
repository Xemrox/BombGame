
#ifndef BombMachine_h
#define BombMachine_h

//this is a bomb game state machine!
class BombMachine
{
public:
    enum BombState
    {
        Undefined = 0,
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

    BombState getState();
    void setState(BombState newState);
    bool allowStateChange(BombState newState);

private:
    union BombCode
    {
        const char code[9];
    };
    
    static const BombCode bombCodes[3];

    BombState state;
    char keypadBuffer[8];
};



#endif