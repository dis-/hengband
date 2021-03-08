#pragma once

#include "player-info/base-status-types.h"
#include "player/player-classes-types.h"
#include "player/player-personalities-types.h"
#include "player/player-race-types.h"
#include "spell/spells-util.h"
#include "system/angband.h"

class PlayerStatus {
public:
    PlayerStatus(player_type *owner_ptr);
    virtual ~PlayerStatus(){};
    s16b Value();
    BIT_FLAGS Flags();
    virtual void UpdateValue();

protected:
    s16b default_value;
    s16b min_value;
    s16b max_value;
    s16b value;
    player_type *owner_ptr;
    virtual void init_value();
    virtual s16b race_diff();
    virtual s16b class_diff();
    virtual s16b personality_diff();
    virtual s16b equipment_diff();
    virtual s16b time_effect_diff();
    virtual s16b battleform_diff();
    virtual s16b mutation_diff();
    virtual s16b riding_diff();
    virtual s16b inventory_weight_diff();
    virtual s16b action_diff();
    virtual BIT_FLAGS get_equipment_flag();
};
