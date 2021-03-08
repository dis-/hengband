#include "player-status/player-status.h"
#include "player/player-status-flags.h"
#include "util/bit-flags-calculator.h"


PlayerStatus::PlayerStatus(player_type *owner_ptr) { 
    this->owner_ptr = owner_ptr; 
    init_value();
}

s16b PlayerStatus::Value()
{
    s16b pow = this->default_value;

    pow += this->action_diff();
    pow += this->battleform_diff();
    pow += this->class_diff();
    pow += this->equipment_diff();
    pow += this->inventory_weight_diff();
    pow += this->mutation_diff();
    pow += this->personality_diff();
    pow += this->race_diff();
    pow += this->riding_diff();
    pow += this->time_effect_diff();

    if ((pow > this->max_value)) {
        pow = this->max_value;
    }

    if (pow < this->min_value)
        pow = this->min_value;

    this->value = pow;
}

BIT_FLAGS PlayerStatus::Flags()
{
    BIT_FLAGS result = get_equipment_flag();

    if (this->class_diff() != 0)
        set_bits(result, FLAG_CAUSE_CLASS);

    if (this->race_diff() != 0)
        set_bits(result, FLAG_CAUSE_RACE);

    if (this->battleform_diff() != 0)
        set_bits(result, FLAG_CAUSE_BATTLE_FORM);

    if (this->mutation_diff() != 0)
        set_bits(result, FLAG_CAUSE_MUTATION);

    if (this->time_effect_diff() != 0)
        set_bits(result, FLAG_CAUSE_MAGIC_TIME_EFFECT);

    if (this->personality_diff() != 0)
        set_bits(result, FLAG_CAUSE_PERSONALITY);

    if (this->riding_diff() != 0)
        set_bits(result, FLAG_CAUSE_RIDING);

    if (this->inventory_weight_diff() != 0)
        set_bits(result, FLAG_CAUSE_INVEN_PACK);

    if (this->action_diff() != 0)
        set_bits(result, FLAG_CAUSE_ACTION);

    return result;
}

void PlayerStatus::UpdateValue()
{
    s16b pow = this->default_value;

    pow += this->action_diff();
    pow += this->battleform_diff();
    pow += this->class_diff();
    pow += this->equipment_diff();
    pow += this->inventory_weight_diff();
    pow += this->mutation_diff();
    pow += this->personality_diff();
    pow += this->race_diff();
    pow += this->riding_diff();
    pow += this->time_effect_diff();

    if ((pow > this->max_value)) {
        pow = this->max_value;
    }

    if (pow < this->min_value)
        pow = this->min_value;

    this->value = pow;
}

void PlayerStatus::init_value()
{
    this->default_value = 0;
    this->min_value = 0;
    this->max_value = 0;
}

s16b PlayerStatus::race_diff() { return 0; }
s16b PlayerStatus::class_diff() { return 0; }
s16b PlayerStatus::personality_diff() { return 0; }
s16b PlayerStatus::equipment_diff() { return 0; }
s16b PlayerStatus::time_effect_diff() { return 0; }
s16b PlayerStatus::battleform_diff() { return 0; }
s16b PlayerStatus::mutation_diff() { return 0; }
s16b PlayerStatus::riding_diff() { return 0; }
s16b PlayerStatus::inventory_weight_diff() { return 0; }
s16b PlayerStatus::action_diff() { return 0; }
BIT_FLAGS PlayerStatus::get_equipment_flag() { return FLAG_CAUSE_NONE; }
