#include "core/buffs/move_speed_buff.h"

MoveSpeedBuff::MoveSpeedBuff()
    : Buff()
    , mFlatBonus(0)
    , mPercentBonus(25.0)
{
    mSecsToEnd=15.0;
}

void MoveSpeedBuff::SetFlatBonus(int32_t flatBonus)
{
    mFlatBonus=flatBonus;
}

int32_t MoveSpeedBuff::GetFlatBonus()const
{
    return mFlatBonus;
}

void MoveSpeedBuff::SetPercentBonus(double percentBonus)
{
    mPercentBonus=percentBonus;
}

double MoveSpeedBuff::GetPercentBonus()const
{
    return mPercentBonus;
}


