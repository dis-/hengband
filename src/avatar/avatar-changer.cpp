﻿#include "avatar/avatar-changer.h"
#include "dungeon/dungeon.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-indice-types.h"
#include "player-info/avatar.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

AvatarChanger::AvatarChanger(player_type *target_ptr, monster_type *m_ptr)
    : target_ptr(target_ptr)
    , m_ptr(m_ptr)
{
}

void AvatarChanger::change_virtue()
{
    this->change_virtue_non_beginner();
    this->change_virtue_unique();
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (m_ptr->r_idx == MON_BEGGAR || m_ptr->r_idx == MON_LEPER) {
        chg_virtue(this->target_ptr, V_COMPASSION, -1);
    }

    this->change_virtue_good_evil();
    if (any_bits(r_ptr->flags3, RF3_UNDEAD) && any_bits(r_ptr->flags1, RF1_UNIQUE)) {
        chg_virtue(this->target_ptr, V_VITALITY, 2);
    }

    this->change_virtue_revenge();
    if (any_bits(r_ptr->flags2, RF2_MULTIPLY) && (r_ptr->r_akills > 1000) && one_in_(10)) {
        chg_virtue(this->target_ptr, V_VALOUR, -1);
    }

    this->change_virtue_wild_thief();
    this->change_virtue_good_animal();
}

void AvatarChanger::change_virtue_non_beginner()
{
    auto *floor_ptr = this->target_ptr->current_floor_ptr;
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (d_info[this->target_ptr->dungeon_idx].flags.has(DF::BEGINNER)) {
        return;
    }

    if ((floor_ptr->dun_level == 0) && !this->target_ptr->ambush_flag && !floor_ptr->inside_arena) {
        chg_virtue(this->target_ptr, V_VALOUR, -1);
    } else if (r_ptr->level > floor_ptr->dun_level) {
        if (randint1(10) <= (r_ptr->level - floor_ptr->dun_level)) {
            chg_virtue(this->target_ptr, V_VALOUR, 1);
        }
    }

    if (r_ptr->level > 60) {
        chg_virtue(this->target_ptr, V_VALOUR, 1);
    }

    if (r_ptr->level >= 2 * (this->target_ptr->lev + 1)) {
        chg_virtue(this->target_ptr, V_VALOUR, 2);
    }
}

void AvatarChanger::change_virtue_unique()
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (none_bits(r_ptr->flags1, RF1_UNIQUE)) {
        return;
    }

    if (any_bits(r_ptr->flags3, RF3_EVIL | RF3_GOOD)) {
        chg_virtue(this->target_ptr, V_HARMONY, 2);
    }

    if (any_bits(r_ptr->flags3, RF3_GOOD)) {
        chg_virtue(this->target_ptr, V_UNLIFE, 2);
        chg_virtue(this->target_ptr, V_VITALITY, -2);
    }

    if (one_in_(3)) {
        chg_virtue(this->target_ptr, V_INDIVIDUALISM, -1);
    }
}

/*
 * @brief 攻撃を与えたモンスターが天使か悪魔だった場合、徳を変化させる
 * @details 天使かつ悪魔だった場合、天使であることが優先される
 */
void AvatarChanger::change_virtue_good_evil()
{
    auto *floor_ptr = this->target_ptr->current_floor_ptr;
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (any_bits(r_ptr->flags3, RF3_GOOD) && ((r_ptr->level) / 10 + (3 * floor_ptr->dun_level) >= randint1(100))) {
        chg_virtue(this->target_ptr, V_UNLIFE, 1);
    }

    if (any_bits(r_ptr->flags3, RF3_ANGEL)) {
        if (any_bits(r_ptr->flags1, RF1_UNIQUE)) {
            chg_virtue(this->target_ptr, V_FAITH, -2);
        } else if ((r_ptr->level) / 10 + (3 * floor_ptr->dun_level) >= randint1(100)) {
            auto change_value = any_bits(r_ptr->flags3, RF3_GOOD) ? -1 : 1;
            chg_virtue(this->target_ptr, V_FAITH, change_value);
        }

        return;
    }

    if (any_bits(r_ptr->flags3, RF3_DEMON)) {
        if (any_bits(r_ptr->flags1, RF1_UNIQUE)) {
            chg_virtue(this->target_ptr, V_FAITH, 2);
        } else if ((r_ptr->level) / 10 + (3 * floor_ptr->dun_level) >= randint1(100)) {
            chg_virtue(this->target_ptr, V_FAITH, 1);
        }
    }
}

/*
 * @brief 過去に＠を殺したことがあるユニークにリゾンべを果たせたら徳を上げる
 */
void AvatarChanger::change_virtue_revenge()
{
    auto *floor_ptr = this->target_ptr->current_floor_ptr;
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (r_ptr->r_deaths == 0) {
        return;
    }

    if (any_bits(r_ptr->flags1, RF1_UNIQUE)) {
        chg_virtue(this->target_ptr, V_HONOUR, 10);
        return;
    }

    if ((r_ptr->level) / 10 + (2 * floor_ptr->dun_level) >= randint1(100)) {
        chg_virtue(this->target_ptr, V_HONOUR, 1);
    }
}

/*
 * @brief 盗み逃げをするモンスター及び地上のモンスターを攻撃した際に徳を変化させる
 */
void AvatarChanger::change_virtue_wild_thief()
{
    auto *floor_ptr = this->target_ptr->current_floor_ptr;
    auto *r_ptr = &r_info[m_ptr->r_idx];
    auto innocent = true;
    auto thief = false;
    for (auto i = 0; i < MAX_NUM_BLOWS; i++) {
        if (r_ptr->blow[i].d_dice != 0) {
            innocent = false;
        }

        if ((r_ptr->blow[i].effect == RBE_EAT_ITEM) || (r_ptr->blow[i].effect == RBE_EAT_GOLD)) {
            thief = true;
        }
    }

    if (r_ptr->level > 0) {
        innocent = false;
    }

    if (thief) {
        if (any_bits(r_ptr->flags1, RF1_UNIQUE)) {
            chg_virtue(this->target_ptr, V_JUSTICE, 3);
            return;
        }

        if (1 + ((r_ptr->level) / 10 + (2 * floor_ptr->dun_level)) >= randint1(100)) {
            chg_virtue(this->target_ptr, V_JUSTICE, 1);
        }

        return;
    }

    if (innocent) {
        chg_virtue(this->target_ptr, V_JUSTICE, -1);
    }
}

/*
 * @brief 邪悪でなく、魔法も持たない動物を攻撃した時に徳を下げる
 */
void AvatarChanger::change_virtue_good_animal()
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    auto magic_ability_flags = r_ptr->ability_flags;
    magic_ability_flags.reset(RF_ABILITY_NOMAGIC_MASK);
    if (none_bits(r_ptr->flags3, RF3_ANIMAL) || any_bits(r_ptr->flags3, RF3_EVIL) || magic_ability_flags.any()) {
        return;
    }

    if (one_in_(4)) {
        chg_virtue(this->target_ptr, V_NATURE, -1);
    }
}
