#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

class PlayerType;
void dispel_player(PlayerType *player_ptr);
MonsterSpellResult spell_RF4_DISPEL(MONSTER_IDX m_idx, PlayerType *player_ptr, MONSTER_IDX t_idx, int target_type);
