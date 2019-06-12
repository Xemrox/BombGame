
#include "BombMachine.h"

bool BombMachine::allowStateChange(BombState newState) {
    switch (this->state)
    {
    case Configuration: //config -> idle
        if(newState != Idle)
            return false;
        break;
    case Idle: //idle -> arming || configuration
        if(newState != Arming || newState != Configuration)
            return false;
        break;
    case Arming: //arming -> armed || idle || locked
        if(newState != Armed || newState != Idle || newState != LockedArming)
            return false;
        break;
    case Armed: //armed -> disarming || exploded
        if(newState != Disarming || newState != Exploded)
            return false;
        break;
    case Disarming: //disarming -> disarmed || locked || exploded
        if(newState != Disarmed || newState != LockedDisarming || newState != Exploded)
            return false;
        break;
    case Disarmed: //disarmed -> idle
        if(newState != Idle)
            return false;
        break;
    case Exploded: //exploded -> idle
        if(newState != Idle)
            return false;
        break;
    default:
        return false;
    }
    return true;
}

void BombMachine::setState(BombState newState) {
    if(this->allowStateChange(newState))
        this->state = newState;
}

const BombMachine::BombCode BombMachine::bombCodes[3] = {
        {"8715003"},
        {"00000000"},
        {"99999999"}

        //911
        //8426
        //2206
        //220694
        //22061994
        //314159265
        //31415926
        //3141592
        //314159
        //31415
        //3141

        //9000
        //3000

        //1989
        //1945

        //666
        //7355608 -> CS
        //0815
        //007
        //404
        //420
        //13
        //42
        //17
        //69
    };