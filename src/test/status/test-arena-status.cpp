#include "player-info/spell-hex-data-type.h"
#include "spell-realm/spells-hex.h"
#include "status/buff-setter.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/finalizer.h"

#include <doctest/doctest.h>

#include <array>
#include <memory>

TEST_CASE("Arena timed reset preserves Kutar flattening")
{
    PlayerType player;
    player.hero = 20;
    player.tsubureru = 20;

    reset_tim_flags(&player);

    CHECK(player.hero == 0);
    CHECK(player.tsubureru == 20);
}

TEST_CASE("Stopping hex spells discards interrupted spells awaiting restart")
{
    PlayerType player;
    auto data = std::make_shared<spell_hex_data_type>();
    player.class_specific_data = data;
    data->casting_spells.set(HEX_BLESS);
    SpellHex(&player).interrupt_spelling();
    REQUIRE(data->interrupting_spells.has(HEX_BLESS));

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    constexpr std::array status_flags = { StatusRecalculatingFlag::BONUS, StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP, StatusRecalculatingFlag::SPELLS };
    constexpr std::array main_flags = { MainWindowRedrawingFlag::EXTRA, MainWindowRedrawingFlag::HP, MainWindowRedrawingFlag::MP };
    std::array<bool, status_flags.size()> previous_status{};
    std::array<bool, main_flags.size()> previous_main{};
    for (auto i = 0U; i < status_flags.size(); ++i) {
        previous_status[i] = rfu.has(status_flags[i]);
    }
    for (auto i = 0U; i < main_flags.size(); ++i) {
        previous_main[i] = rfu.has(main_flags[i]);
    }
    const auto restore_flags = util::make_finalizer([&] {
        for (auto i = 0U; i < status_flags.size(); ++i) {
            if (!previous_status[i]) {
                rfu.reset_flag(status_flags[i]);
            }
        }
        for (auto i = 0U; i < main_flags.size(); ++i) {
            if (!previous_main[i]) {
                rfu.reset_flag(main_flags[i]);
            }
        }
    });

    SpellHex(&player).stop_all_spells();

    CHECK(data->casting_spells.none());
    CHECK(data->interrupting_spells.none());
}
