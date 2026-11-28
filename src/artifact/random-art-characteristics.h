#pragma once
/*!
 * @file random-art-characteristics.h
 * @brief ランダムアーティファクトのバイアス付加処理ヘッダ
 */

#include "object-enchant/tr-types.h"
#include <string>
#include <tl/optional.hpp>

class ItemEntity;
class PlayerType;
void decide_random_artifact_blessing(ItemEntity &item, const tl::optional<tr_type> &creator_blessing);
void curse_artifact(PlayerType *player_ptr, ItemEntity *o_ptr);
std::string get_random_name(const ItemEntity &item, bool armour, int power);
bool has_extreme_damage_rate(PlayerType *player_ptr, ItemEntity *o_ptr);
