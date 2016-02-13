#include "i_audible_component.h"

int32_t const AudibleEffectDesc::TTL_Infinity = -100;

AudibleEffectDesc::AudibleEffectDesc( int32_t id, bool autoLoopUntilDeath/*=false*/ )
        : UID( 0 ), Id ( id ), TTL( TTL_Infinity ), AutoLoopUntilDeath(autoLoopUntilDeath)
{
    static int32_t uid = 0;
    UID = ++uid;
}

AudibleEffectDesc::AudibleEffectDesc()
    : UID( 0 ), Id ( -1 ), TTL( TTL_Infinity ), AutoLoopUntilDeath(false)
{

}

