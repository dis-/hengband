#pragma once
#include "player-status/player-status.h"

class PlayerSpeed : public PlayerStatus {
public:
    using PlayerStatus::PlayerStatus;
    void UpdateValue() override;

private:
    void init_value() override;
    s16b race_diff() override;
    s16b class_diff() override;
    s16b personality_diff() override;
    s16b equipment_diff() override;
    s16b time_effect_diff() override;
    s16b battleform_diff() override;
    s16b mutation_diff() override;
    s16b riding_diff() override;
    s16b inventory_weight_diff() override;
    s16b action_diff() override;
    BIT_FLAGS get_equipment_flag() override;
    s16b special_weapon_set_diff();
};
