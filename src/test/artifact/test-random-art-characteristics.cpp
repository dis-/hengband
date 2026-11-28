/*!
 * @brief ランダムアーティファクトの特性の処理 (artifact/random-art-characteristics.cpp) のテスト
 *
 * 生成の途中で付いた祝福を、どちらの神の祝福にするか決め直す処理を確かめる。
 */

#include "artifact/random-art-bias-types.h"
#include "artifact/random-art-characteristics.h"
#include "object-enchant/tr-types.h"
#include "system/item/item-entity.h"
#include <doctest/doctest.h>
#include <tl/optional.hpp>

namespace {

/*!
 * @brief 善の神の祝福だけを持つ、生成中のランダムアーティファクトを返す
 */
ItemEntity make_blessed_item()
{
    ItemEntity item;
    item.art_flags.set(TR_BLESSED);
    return item;
}

}

TEST_CASE("decide_random_artifact_blessing does nothing to an item without a blessing")
{
    ItemEntity item;
    item.art_flags.set(TR_SLAY_GOOD);
    item.artifact_bias = RandomArtifactBias::NECROMANTIC;

    decide_random_artifact_blessing(item, tl::nullopt);
    CHECK(item.art_flags.has_not(TR_BLESSED));
    CHECK(item.art_flags.has_not(TR_BLESSED_EVIL));

    // 作り手がプリーストでも、祝福が付いていなければ足さない
    decide_random_artifact_blessing(item, TR_BLESSED_EVIL);
    CHECK(item.art_flags.has_not(TR_BLESSED));
    CHECK(item.art_flags.has_not(TR_BLESSED_EVIL));
}

TEST_CASE("decide_random_artifact_blessing keeps the blessing of the gods of good by default")
{
    auto item = make_blessed_item();
    item.art_flags.set(TR_SLAY_EVIL);
    item.artifact_bias = RandomArtifactBias::PRIESTLY;

    decide_random_artifact_blessing(item, tl::nullopt);
    CHECK(item.art_flags.has(TR_BLESSED));
    CHECK(item.art_flags.has_not(TR_BLESSED_EVIL));
}

TEST_CASE("decide_random_artifact_blessing turns the blessing evil for a weapon against the good")
{
    for (const auto slay : { TR_SLAY_GOOD, TR_KILL_GOOD }) {
        CAPTURE(static_cast<int>(slay));
        auto item = make_blessed_item();
        item.art_flags.set(slay);

        decide_random_artifact_blessing(item, tl::nullopt);
        CHECK(item.art_flags.has_not(TR_BLESSED));
        CHECK(item.art_flags.has(TR_BLESSED_EVIL));
    }
}

TEST_CASE("decide_random_artifact_blessing turns the blessing evil for the necromantic bias")
{
    auto item = make_blessed_item();
    item.artifact_bias = RandomArtifactBias::NECROMANTIC;

    decide_random_artifact_blessing(item, tl::nullopt);
    CHECK(item.art_flags.has_not(TR_BLESSED));
    CHECK(item.art_flags.has(TR_BLESSED_EVIL));

    // ほかの偏りでは変えない
    for (const auto bias : { RandomArtifactBias::NONE, RandomArtifactBias::CHAOS, RandomArtifactBias::MAGE, RandomArtifactBias::WARRIOR }) {
        CAPTURE(static_cast<int>(bias));
        auto other_item = make_blessed_item();
        other_item.artifact_bias = bias;

        decide_random_artifact_blessing(other_item, tl::nullopt);
        CHECK(other_item.art_flags.has(TR_BLESSED));
        CHECK(other_item.art_flags.has_not(TR_BLESSED_EVIL));
    }
}

TEST_CASE("decide_random_artifact_blessing follows the gods of the priest who made the item")
{
    SUBCASE("a priest of the gods of evil gets the blessing of the gods of evil")
    {
        auto item = make_blessed_item();
        item.artifact_bias = RandomArtifactBias::PRIESTLY;

        decide_random_artifact_blessing(item, TR_BLESSED_EVIL);
        CHECK(item.art_flags.has_not(TR_BLESSED));
        CHECK(item.art_flags.has(TR_BLESSED_EVIL));
    }

    SUBCASE("a priest of the gods of good keeps the blessing of the gods of good even on a weapon against the good")
    {
        auto item = make_blessed_item();
        item.art_flags.set(TR_SLAY_GOOD);
        item.artifact_bias = RandomArtifactBias::NECROMANTIC;

        decide_random_artifact_blessing(item, TR_BLESSED);
        CHECK(item.art_flags.has(TR_BLESSED));
        CHECK(item.art_flags.has_not(TR_BLESSED_EVIL));
    }
}

TEST_CASE("decide_random_artifact_blessing never leaves both blessings")
{
    ItemEntity item;
    item.art_flags.set(TR_BLESSED);
    item.art_flags.set(TR_BLESSED_EVIL);

    decide_random_artifact_blessing(item, tl::nullopt);
    CHECK(item.art_flags.has(TR_BLESSED));
    CHECK(item.art_flags.has_not(TR_BLESSED_EVIL));
}
