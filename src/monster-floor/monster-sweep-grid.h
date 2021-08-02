﻿#pragma once

#include "system/angband.h"

struct floor_type;
struct player_type;
class MonsterSweepGrid {
public:
    MonsterSweepGrid(player_type *target_ptr, MONSTER_IDX m_idx, DIRECTION *mm);
    MonsterSweepGrid() = delete;
    virtual ~MonsterSweepGrid() = default;
    player_type *target_ptr;
    MONSTER_IDX m_idx;
    DIRECTION *mm;
    bool get_movable_grid();

private:
    bool sweep_runnable_away_grid(POSITION *yp, POSITION *xp);
    void sweep_movable_grid(POSITION *yp, POSITION *xp, bool no_flow);
    bool sweep_ranged_attack_grid(POSITION *yp, POSITION *xp);
    bool mon_will_run();
};
