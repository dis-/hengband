#include "grid/object-placer.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "system/artifact-type-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "world/world-object.h"

/*!
 * @brief フロアの指定位置に生成階に応じた財宝オブジェクトの生成を行う。
 * Places a treasure (Gold or Gems) at given location
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @return 生成に成功したらTRUEを返す。
 * @details
 * The location must be a legal, clean, floor grid.
 */
void place_gold(PlayerType *player_ptr, POSITION y, POSITION x)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.grid_array[y][x];
    if (!in_bounds(&floor, y, x)) {
        return;
    }
    if (!cave_drop_bold(&floor, y, x)) {
        return;
    }
    if (!grid.o_idx_list.empty()) {
        return;
    }

    const auto item = floor.make_gold();
    const auto o_idx = o_pop(&floor);
    if (o_idx == 0) {
        return;
    }

    auto &item_pop = floor.o_list[o_idx];
    item_pop = item;
    item_pop.iy = y;
    item_pop.ix = x;
    grid.o_idx_list.add(&floor, o_idx);

    note_spot(player_ptr, y, x);
    lite_spot(player_ptr, y, x);
}

/*!
 * @brief フロアの指定位置に生成階に応じたベースアイテムの生成を行う。
 * Attempt to place an object (normal or good/great) at the given location.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @param mode オプションフラグ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * This routine plays nasty games to generate the "special artifacts".\n
 * This routine uses "object_level" for the "generation level".\n
 * This routine requires a clean floor grid destination.\n
 */
void place_object(PlayerType *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    if (!in_bounds(floor_ptr, y, x) || !cave_drop_bold(floor_ptr, y, x) || !g_ptr->o_idx_list.empty()) {
        return;
    }

    OBJECT_IDX o_idx = o_pop(floor_ptr);
    if (o_idx == 0) {
        return;
    }

    auto &item = floor_ptr->o_list[o_idx];
    item.wipe();
    if (!make_object(player_ptr, &item, mode)) {
        return;
    }

    item.iy = y;
    item.ix = x;
    g_ptr->o_idx_list.add(floor_ptr, o_idx);

    note_spot(player_ptr, y, x);
    lite_spot(player_ptr, y, x);
}
