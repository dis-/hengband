#include "player-status/player-speed.h"
#include "player/player-status-calc.h"
#include "artifact/fixed-art-types.h"
#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-item/cmd-magiceat.h"
#include "combat/attack-power-table.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "floor/cave.h"
#include "floor/floor-events.h"
#include "floor/floor-leaver.h"
#include "floor/floor-save.h"
#include "floor/floor-util.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "market/arena-info-table.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-lite.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/smart-learn-types.h"
#include "mutation/mutation-calculator.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-investor-remover.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "perception/object-perception.h"
#include "pet/pet-util.h"
#include "player-info/avatar.h"
#include "player-status/player-speed.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/mimic-info-table.h"
#include "player/patron.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-personalities-types.h"
#include "player/player-personality.h"
#include "player/player-race-types.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/player-view.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-names-table.h"
#include "realm/realm-song-numbers.h"
#include "specific-object/bow.h"
#include "specific-object/torch.h"
#include "spell-realm/spells-hex.h"
#include "spell/range-calc.h"
#include "spell/spells-describer.h"
#include "spell/spells-execution.h"
#include "spell/spells-status.h"
#include "spell/technic-info-table.h"
#include "status/action-setter.h"
#include "status/base-status.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/angband.h"
#include "system/floor-type-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"


void PlayerSpeed::init_value()
{ 
    default_value = 110;
    min_value = 11;
    max_value = 209;
}

/*!
 * @brief 速度計算 - 種族
 * @return 速度値の増減分
 * @details
 * ** クラッコンと妖精に加算(+レベル/10)
 * ** 悪魔変化/吸血鬼変化で加算(+3)
 * ** 魔王変化で加算(+5)
 * ** マーフォークがFF_WATER地形にいれば加算(+2+レベル/10)
 * ** そうでなく浮遊を持っていないなら減算(-2)
 */
s16b PlayerSpeed::race_diff()
{
    s16b result = 0;

    if (is_specific_player_race(owner_ptr, RACE_KLACKON) || is_specific_player_race(owner_ptr, RACE_SPRITE))
        result += (owner_ptr->lev) / 10;

    if (is_specific_player_race(owner_ptr, RACE_MERFOLK)) {
        floor_type *floor_ptr = owner_ptr->current_floor_ptr;
        feature_type *f_ptr = &f_info[floor_ptr->grid_array[owner_ptr->y][owner_ptr->x].feat];
        if (has_flag(f_ptr->flags, FF_WATER)) {
            result += (2 + owner_ptr->lev / 10);
        } else if (!owner_ptr->levitation) {
            result -= 2;
        }
    }

    if (owner_ptr->mimic_form) {
        switch (owner_ptr->mimic_form) {
        case MIMIC_DEMON:
            result += 3;
            break;
        case MIMIC_DEMON_LORD:
            result += 5;
            break;
        case MIMIC_VAMPIRE:
            result += 3;
            break;
        }
    }
    return result;
}

/*!
 * @brief 速度計算 - 職業
 * @return 速度値の増減分
 * @details
 * ** 忍者の装備が重ければ減算(-レベル/10)
 * ** 忍者の装備が適正ならば加算(+3)さらにクラッコン、妖精、いかさま以外なら加算(+レベル/10)
 * ** 錬気術師で装備が重くなくクラッコン、妖精、いかさま以外なら加算(+レベル/10)
 * ** 狂戦士なら加算(+3),レベル20/30/40/50ごとに+1
 */
s16b PlayerSpeed::class_diff()
{
    SPEED result = 0;

    if (owner_ptr->pclass == CLASS_NINJA) {
        if (heavy_armor(owner_ptr)) {
            result -= (owner_ptr->lev) / 10;
        } else if ((!owner_ptr->inventory_list[INVEN_MAIN_HAND].k_idx || can_attack_with_main_hand(owner_ptr))
            && (!owner_ptr->inventory_list[INVEN_SUB_HAND].k_idx || can_attack_with_sub_hand(owner_ptr))) {
            result += 3;
            if (!(is_specific_player_race(owner_ptr, RACE_KLACKON) || is_specific_player_race(owner_ptr, RACE_SPRITE)
                    || (owner_ptr->pseikaku == PERSONALITY_MUNCHKIN)))
                result += (owner_ptr->lev) / 10;
        }
    }

    if ((owner_ptr->pclass == CLASS_MONK || owner_ptr->pclass == CLASS_FORCETRAINER) && !(heavy_armor(owner_ptr))) {
        if (!(is_specific_player_race(owner_ptr, RACE_KLACKON) || is_specific_player_race(owner_ptr, RACE_SPRITE)
                || (owner_ptr->pseikaku == PERSONALITY_MUNCHKIN)))
            result += (owner_ptr->lev) / 10;
    }

    if (owner_ptr->pclass == CLASS_BERSERKER) {
        result += 2;
        if (owner_ptr->lev > 29)
            result++;
        if (owner_ptr->lev > 39)
            result++;
        if (owner_ptr->lev > 44)
            result++;
        if (owner_ptr->lev > 49)
            result++;
    }
    return result;
}

/*!
 * @brief 速度計算 - 性格
 * @return 速度値の増減分
 * @details
 * ** いかさまでクラッコン/妖精以外なら加算(+5+レベル/10)
 */
s16b PlayerSpeed::personality_diff()
{
    s16b result = 0;
    if (owner_ptr->pseikaku == PERSONALITY_MUNCHKIN && owner_ptr->prace != RACE_KLACKON && owner_ptr->prace != RACE_SPRITE) {
        result += (owner_ptr->lev) / 10 + 5;
    }
    return result;
}

/*!
 * @brief 速度計算 - 装備品特殊セット
 * @return 速度値の増減分
 * @details
 * ** 棘セット装備中ならば加算(+7)
 * ** アイシングデス-トゥインクル装備中ならば加算(+7)
 */
s16b PlayerSpeed::special_weapon_set_diff()
{
    s16b result = 0;
    if (has_melee_weapon(owner_ptr, INVEN_MAIN_HAND) && has_melee_weapon(owner_ptr, INVEN_SUB_HAND)) {
        if ((owner_ptr->inventory_list[INVEN_MAIN_HAND].name1 == ART_QUICKTHORN) && (owner_ptr->inventory_list[INVEN_SUB_HAND].name1 == ART_TINYTHORN)) {
            result += 7;
        }

        if ((owner_ptr->inventory_list[INVEN_MAIN_HAND].name1 == ART_ICINGDEATH) && (owner_ptr->inventory_list[INVEN_SUB_HAND].name1 == ART_TWINKLE)) {
            result += 5;
        }
    }
    return result;
}

/*!
 * @brief 速度計算 - 装備品
 * @return 速度値の増減分
 * @details
 * ** 装備品にTR_SPEEDがあれば加算(+pval+1
 * ** セットで加速増減があるものを計算
 */
s16b PlayerSpeed::equipment_diff()
{
    SPEED result = 0;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &owner_ptr->inventory_list[i];
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        object_flags(owner_ptr, o_ptr, flgs);

        if (!o_ptr->k_idx)
            continue;
        if (has_flag(flgs, TR_SPEED))
            result += o_ptr->pval;
    }
    result += special_weapon_set_diff();

    return result;
}

/*!
 * @brief 速度計算 - 一時的効果
 * @return 速度値の増減分
 * @details
 * ** 加速状態中なら加算(+10)
 * ** 減速状態中なら減算(-10)
 * ** 呪術「衝撃のクローク」で加算(+3)
 * ** 食い過ぎなら減算(-10)
 * ** 光速移動中は+999(最終的に+99になる)
 */
s16b PlayerSpeed::time_effect_diff()
{
    s16b result = 0;

    if (is_fast(owner_ptr)) {
        result += 10;
    }

    if (owner_ptr->slow) {
        result -= 10;
    }

    if (owner_ptr->realm1 == REALM_HEX) {
        if (hex_spelling(owner_ptr, HEX_SHOCK_CLOAK)) {
            result += 3;
        }
    }

    if (owner_ptr->food >= PY_FOOD_MAX)
        result -= 10;

    /* Temporary lightspeed forces to be maximum speed */
    if (owner_ptr->lightspeed)
        result += 999;

    return result;
}

/*!
 * @brief 速度計算 - 型
 * @return 速度値の増減分
 * @details
 * * 基礎値110(+-0に対応)
 * ** 朱雀の構えなら加算(+10)
 */
s16b PlayerSpeed::battleform_diff()
{
    s16b result = 0;
    if (any_bits(owner_ptr->special_defense, KAMAE_SUZAKU))
        result += 10;
    return result;
}

/*!
 * @brief 速度計算 - 変異
 * @return 速度値の増減分
 * @details
 * * 基礎値110(+-0に対応)
 * ** 変異MUT3_XTRA_FATなら減算(-2)
 * ** 変異MUT3_XTRA_LEGなら加算(+3)
 * ** 変異MUT3_SHORT_LEGなら減算(-3)
 */
s16b PlayerSpeed::mutation_diff()
{
    SPEED result = 0;
    if (owner_ptr->muta3) {
        if (any_bits(owner_ptr->muta3, MUT3_XTRA_FAT)) {
            result -= 2;
        }

        if (any_bits(owner_ptr->muta3, MUT3_XTRA_LEGS)) {
            result += 3;
        }

        if (any_bits(owner_ptr->muta3, MUT3_SHORT_LEG)) {
            result -= 3;
        }
    }
    return result;
}

/*!
 * @brief 速度計算 - 乗馬
 * @return 速度値の増減分
 * @details
 * * 騎乗中ならばモンスターの加速に準拠、ただし騎乗技能値とモンスターレベルによるキャップ処理あり
 */
s16b PlayerSpeed::riding_diff()
{
    monster_type *riding_m_ptr = &owner_ptr->current_floor_ptr->m_list[owner_ptr->riding];
    SPEED speed = riding_m_ptr->mspeed;
    SPEED result = 0;

    if (!owner_ptr->riding) {
        return 0;
    }

    if (riding_m_ptr->mspeed > 110) {
        result = (s16b)((speed - 110) * (owner_ptr->skill_exp[GINOU_RIDING] * 3 + owner_ptr->lev * 160L - 10000L) / (22000L));
        if (result < 0)
            result = 0;
    } else {
        result = speed - 110;
    }

    result += (owner_ptr->skill_exp[GINOU_RIDING] + owner_ptr->lev * 160L) / 3200;

    if (monster_fast_remaining(riding_m_ptr))
        result += 10;
    if (monster_slow_remaining(riding_m_ptr))
        result -= 10;

    return result;
}

/*!
 * @brief 速度計算 - 重量
 * @return 速度値の増減分
 * @details
 * * 所持品の重量による減速処理。乗馬時は別計算。
 */
s16b PlayerSpeed::inventory_weight_diff()
{
    SPEED result = 0;

    int weight = calc_inventory_weight(owner_ptr);
    int count;

    if (owner_ptr->riding) {
        monster_type *riding_m_ptr = &owner_ptr->current_floor_ptr->m_list[owner_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        count = 1500 + riding_r_ptr->level * 25;

        if (owner_ptr->skill_exp[GINOU_RIDING] < RIDING_EXP_SKILLED) {
            weight += (owner_ptr->wt * 3 * (RIDING_EXP_SKILLED - owner_ptr->skill_exp[GINOU_RIDING])) / RIDING_EXP_SKILLED;
        }

        if (weight > count) {
            result -= ((weight - count) / (count / 5));
        }
    } else {
        count = (int)calc_weight_limit(owner_ptr);
        if (weight > count) {
            result -= ((weight - count) / (count / 5));
        }
    }
    return result;
}

/*!
 * @brief 速度計算 - ACTION
 * @return 速度値の増減分
 * @details
 * * 探索中なら減算(-10)
 */
s16b PlayerSpeed::action_diff()
{
    SPEED result = 0;
    if (owner_ptr->action == ACTION_SEARCH)
        result -= 10;
    return result;
}

BIT_FLAGS PlayerSpeed::get_equipment_flag()
{
    BIT_FLAGS result = PlayerStatus::get_equipment_flag();

    if (special_weapon_set_diff() != 0)
        set_bits(result, FLAG_CAUSE_INVEN_MAIN_HAND | FLAG_CAUSE_INVEN_SUB_HAND);

    return result;
}

void PlayerSpeed::UpdateValue()
{
    PlayerStatus::UpdateValue();

    if (owner_ptr->riding) {
        s16b pow = this->default_value;
        pow += riding_diff();
        pow += inventory_weight_diff();
        this->value = pow;

        if ((pow > this->max_value)) {
            pow = this->max_value;
        }

        if (pow < this->min_value)
            pow = this->min_value;
    }
}
