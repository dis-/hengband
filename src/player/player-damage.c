﻿#include "player/player-damage.h"
#include "autopick/autopick-pref-processor.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/cmd-process-screen.h"
#include "core/stuff-handler.h"
#include "dungeon/quest.h"
#include "floor/floor.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "game-option/play-record-options.h"
#include "inventory/inventory-damage.h"
#include "io/files-util.h"
#include "io/report.h"
#include "io/save.h"
#include "io/write-diary.h"
#include "main/music-definitions-table.h"
#include "main/sound-definitions-table.h"
#include "market/arena-info-table.h"
#include "mind/mind-mirror-master.h"
#include "monster/monster-describer.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "mspell/monster-spell.h"
#include "object-enchant/tr-types.h"
#include "object/object-broken.h"
#include "object/object-flags.h"
#include "object/object-flavor.h"
#include "object/object-hook.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-effects.h"
#include "player/player-move.h"
#include "player/player-personalities-table.h"
#include "player/player-races-table.h"
#include "player/player-status.h"
#include "realm/realm-song-numbers.h"
#include "term/term-color-types.h"
#include "util/util.h"
#include "view/display-main-window.h"
#include "world/world.h"

/*!
* @brief 酸攻撃による装備のAC劣化処理 /
* Acid has hit the player, attempt to affect some armor.
* @param 酸を浴びたキャラクタへの参照ポインタ
* @return 装備による軽減があったならTRUEを返す
* @details
* Note that the "base armor" of an object never changes.
* If any armor is damaged (or resists), the player takes less damage.
*/
static bool acid_minus_ac(player_type *creature_ptr)
{
	object_type *o_ptr = NULL;
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	GAME_TEXT o_name[MAX_NLEN];

	/* Pick a (possibly empty) creature_ptr->inventory_list slot */
	switch (randint1(7))
	{
	case 1: o_ptr = &creature_ptr->inventory_list[INVEN_RARM]; break;
	case 2: o_ptr = &creature_ptr->inventory_list[INVEN_LARM]; break;
	case 3: o_ptr = &creature_ptr->inventory_list[INVEN_BODY]; break;
	case 4: o_ptr = &creature_ptr->inventory_list[INVEN_OUTER]; break;
	case 5: o_ptr = &creature_ptr->inventory_list[INVEN_HANDS]; break;
	case 6: o_ptr = &creature_ptr->inventory_list[INVEN_HEAD]; break;
	case 7: o_ptr = &creature_ptr->inventory_list[INVEN_FEET]; break;
	}

	if (!o_ptr->k_idx) return FALSE;
	if (!object_is_armour(o_ptr)) return FALSE;

	object_desc(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
	object_flags(o_ptr, flgs);
	/* No damage left to be done */
	if (o_ptr->ac + o_ptr->to_a <= 0)
	{
		msg_format(_("%sは既にボロボロだ！", "Your %s is already fully corroded!"), o_name);
		return FALSE;
	}

	/* Object resists */
	if (have_flag(flgs, TR_IGNORE_ACID))
	{
		msg_format(_("しかし%sには効果がなかった！", "Your %s is unaffected!"), o_name);
		return TRUE;
	}

	msg_format(_("%sが酸で腐食した！", "Your %s is corroded!"), o_name);

	/* Damage the item */
	o_ptr->to_a--;

	/* Calculate bonuses */
	creature_ptr->update |= (PU_BONUS);
	creature_ptr->window |= (PW_EQUIP | PW_PLAYER);

	calc_android_exp(creature_ptr);

	/* Item was damaged */
	return TRUE;
}


/*!
* @brief 酸属性によるプレイヤー損害処理 /
* Hurt the player with Acid
* @param creature_ptr 酸を浴びたキャラクタへの参照ポインタ
* @param dam 基本ダメージ量
* @param kb_str ダメージ原因記述
* @param monspell 原因となったモンスター特殊攻撃ID
* @param aura オーラよるダメージが原因ならばTRUE
* @return 修正HPダメージ量
*/
HIT_POINT acid_dam(player_type *creature_ptr, HIT_POINT dam, concptr kb_str, int monspell, bool aura)
{
	HIT_POINT get_damage;
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;
	bool double_resist = is_oppose_acid(creature_ptr);

	/* Total Immunity */
	if (creature_ptr->immune_acid || (dam <= 0))
	{
		learn_spell(creature_ptr, monspell);
		return 0;
	}

	/* Vulnerability (Ouch!) */
	if (creature_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
	if (creature_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;

	/* Resist the damage */
	if (creature_ptr->resist_acid) dam = (dam + 2) / 3;
	if (double_resist) dam = (dam + 2) / 3;

	if (aura || !check_multishadow(creature_ptr))
	{
		if ((!(double_resist || creature_ptr->resist_acid)) &&
			one_in_(HURT_CHANCE))
			(void)do_dec_stat(creature_ptr, A_CHR);

		/* If any armor gets hit, defend the player */
		if (acid_minus_ac(creature_ptr)) dam = (dam + 1) / 2;
	}

	get_damage = take_hit(creature_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str, monspell);

	/* Inventory damage */
	if (!aura && !(double_resist && creature_ptr->resist_acid))
		inventory_damage(creature_ptr, set_acid_destroy, inv);
	return get_damage;
}


/*!
* @brief 電撃属性によるプレイヤー損害処理 /
* Hurt the player with electricity
* @param creature_ptr 電撃を浴びたキャラクタへの参照ポインタ
* @param dam 基本ダメージ量
* @param kb_str ダメージ原因記述
* @param monspell 原因となったモンスター特殊攻撃ID
* @param aura オーラよるダメージが原因ならばTRUE
* @return 修正HPダメージ量
*/
HIT_POINT elec_dam(player_type *creature_ptr, HIT_POINT dam, concptr kb_str, int monspell, bool aura)
{
	HIT_POINT get_damage;
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;
	bool double_resist = is_oppose_elec(creature_ptr);

	/* Total immunity */
	if (creature_ptr->immune_elec || (dam <= 0))
	{
		learn_spell(creature_ptr, monspell);
		return 0;
	}

	/* Vulnerability (Ouch!) */
	if (creature_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
	if (creature_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;
	if (PRACE_IS_(creature_ptr, RACE_ANDROID)) dam += dam / 3;

	/* Resist the damage */
	if (creature_ptr->resist_elec) dam = (dam + 2) / 3;
	if (double_resist) dam = (dam + 2) / 3;

	if (aura || !check_multishadow(creature_ptr))
	{
		if ((!(double_resist || creature_ptr->resist_elec)) &&
			one_in_(HURT_CHANCE))
			(void)do_dec_stat(creature_ptr, A_DEX);
	}

	get_damage = take_hit(creature_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str, monspell);

	/* Inventory damage */
	if (!aura && !(double_resist && creature_ptr->resist_elec))
		inventory_damage(creature_ptr, set_elec_destroy, inv);

	return get_damage;
}


/*!
* @brief 火炎属性によるプレイヤー損害処理 /
* Hurt the player with Fire
* @param creature_ptr 火炎を浴びたキャラクタへの参照ポインタ
* @param dam 基本ダメージ量
* @param kb_str ダメージ原因記述
* @param monspell 原因となったモンスター特殊攻撃ID
* @param aura オーラよるダメージが原因ならばTRUE
* @return 修正HPダメージ量
*/
HIT_POINT fire_dam(player_type *creature_ptr, HIT_POINT dam, concptr kb_str, int monspell, bool aura)
{
	HIT_POINT get_damage;
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;
	bool double_resist = is_oppose_fire(creature_ptr);

	/* Totally immune */
	if (creature_ptr->immune_fire || (dam <= 0))
	{
		learn_spell(creature_ptr, monspell);
		return 0;
	}

	/* Vulnerability (Ouch!) */
	if (creature_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
	if (PRACE_IS_(creature_ptr, RACE_ENT)) dam += dam / 3;
	if (creature_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;

	/* Resist the damage */
	if (creature_ptr->resist_fire) dam = (dam + 2) / 3;
	if (double_resist) dam = (dam + 2) / 3;

	if (aura || !check_multishadow(creature_ptr))
	{
		if ((!(double_resist || creature_ptr->resist_fire)) &&
			one_in_(HURT_CHANCE))
			(void)do_dec_stat(creature_ptr, A_STR);
	}

	get_damage = take_hit(creature_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str, monspell);

	/* Inventory damage */
	if (!aura && !(double_resist && creature_ptr->resist_fire))
		inventory_damage(creature_ptr, set_fire_destroy, inv);

	return get_damage;
}


/*!
* @brief 冷気属性によるプレイヤー損害処理 /
* Hurt the player with Cold
* @param creature_ptr 冷気を浴びたキャラクタへの参照ポインタ
* @param dam 基本ダメージ量
* @param kb_str ダメージ原因記述
* @param monspell 原因となったモンスター特殊攻撃ID
* @param aura オーラよるダメージが原因ならばTRUE
* @return 修正HPダメージ量
*/
HIT_POINT cold_dam(player_type *creature_ptr, HIT_POINT dam, concptr kb_str, int monspell, bool aura)
{
	HIT_POINT get_damage;
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;
	bool double_resist = is_oppose_cold(creature_ptr);

	/* Total immunity */
	if (creature_ptr->immune_cold || (dam <= 0))
	{
		learn_spell(creature_ptr, monspell);
		return 0;
	}

	/* Vulnerability (Ouch!) */
	if (creature_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
	if (creature_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;

	/* Resist the damage */
	if (creature_ptr->resist_cold) dam = (dam + 2) / 3;
	if (double_resist) dam = (dam + 2) / 3;

	if (aura || !check_multishadow(creature_ptr))
	{
		if ((!(double_resist || creature_ptr->resist_cold)) &&
			one_in_(HURT_CHANCE))
			(void)do_dec_stat(creature_ptr, A_STR);
	}

	get_damage = take_hit(creature_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str, monspell);

	/* Inventory damage */
	if (!aura && !(double_resist && creature_ptr->resist_cold))
		inventory_damage(creature_ptr, set_cold_destroy, inv);

	return get_damage;
}


/*
 * Decreases players hit points and sets death flag if necessary
 *
 * Invulnerability needs to be changed into a "shield"
 *
 * Hack -- this function allows the user to save (or quit)
 * the game when he dies, since the "You die." message is shown before
 * setting the player to "dead".
 */
int take_hit(player_type *creature_ptr, int damage_type, HIT_POINT damage, concptr hit_from, int monspell)
{
	int old_chp = creature_ptr->chp;

	char death_message[1024];
	char tmp[1024];

	int warning = (creature_ptr->mhp * hitpoint_warn / 10);
	if (creature_ptr->is_dead) return 0;

	if (creature_ptr->sutemi) damage *= 2;
	if (creature_ptr->special_defense & KATA_IAI) damage += (damage + 4) / 5;

	if (easy_band) damage = (damage + 1) / 2;

	if (damage_type != DAMAGE_USELIFE)
	{
		disturb(creature_ptr, TRUE, TRUE);
		if (auto_more)
		{
			creature_ptr->now_damaged = TRUE;
		}
	}

	if (monspell >= 0) learn_spell(creature_ptr, monspell);

	/* Mega-Hack -- Apply "invulnerability" */
	if ((damage_type != DAMAGE_USELIFE) && (damage_type != DAMAGE_LOSELIFE))
	{
		if (IS_INVULN(creature_ptr) && (damage < 9000))
		{
			if (damage_type == DAMAGE_FORCE)
			{
				msg_print(_("バリアが切り裂かれた！", "The attack cuts your shield of invulnerability open!"));
			}
			else if (one_in_(PENETRATE_INVULNERABILITY))
			{
				msg_print(_("無敵のバリアを破って攻撃された！", "The attack penetrates your shield of invulnerability!"));
			}
			else
			{
				return 0;
			}
		}

		if (check_multishadow(creature_ptr))
		{
			if (damage_type == DAMAGE_FORCE)
			{
				msg_print(_("幻影もろとも体が切り裂かれた！", "The attack hits Shadow together with you!"));
			}
			else if (damage_type == DAMAGE_ATTACK)
			{
				msg_print(_("攻撃は幻影に命中し、あなたには届かなかった。", "The attack hits Shadow, but you are unharmed!"));
				return 0;
			}
		}

		if (creature_ptr->wraith_form)
		{
			if (damage_type == DAMAGE_FORCE)
			{
				msg_print(_("半物質の体が切り裂かれた！", "The attack cuts through your ethereal body!"));
			}
			else
			{
				damage /= 2;
				if ((damage == 0) && one_in_(2)) damage = 1;
			}
		}

		if (creature_ptr->special_defense & KATA_MUSOU)
		{
			damage /= 2;
			if ((damage == 0) && one_in_(2)) damage = 1;
		}
	} /* not if LOSELIFE USELIFE */

	/* Hurt the player */
	creature_ptr->chp -= damage;
	if (damage_type == DAMAGE_GENO && creature_ptr->chp < 0)
	{
		damage += creature_ptr->chp;
		creature_ptr->chp = 0;
	}

	/* Display the hitpoints */
	creature_ptr->redraw |= (PR_HP);

	creature_ptr->window |= (PW_PLAYER);

	if (damage_type != DAMAGE_GENO && creature_ptr->chp == 0)
	{
		chg_virtue(creature_ptr, V_SACRIFICE, 1);
		chg_virtue(creature_ptr, V_CHANCE, 2);
	}

	/* Dead player */
	if (creature_ptr->chp < 0)
	{
		bool android = (creature_ptr->prace == RACE_ANDROID ? TRUE : FALSE);

#ifdef JP
		/* 死んだ時に強制終了して死を回避できなくしてみた by Habu */
		if (!cheat_save)
			if (!save_player(creature_ptr)) msg_print("セーブ失敗！");
#endif

		sound(SOUND_DEATH);

		chg_virtue(creature_ptr, V_SACRIFICE, 10);

		handle_stuff(creature_ptr);
		creature_ptr->leaving = TRUE;

		/* Note death */
		creature_ptr->is_dead = TRUE;

		if (creature_ptr->current_floor_ptr->inside_arena)
		{
			concptr m_name = r_name + r_info[arena_info[creature_ptr->arena_number].r_idx].name;
			msg_format(_("あなたは%sの前に敗れ去った。", "You are beaten by %s."), m_name);
			msg_print(NULL);
			if (record_arena) exe_write_diary(creature_ptr, DIARY_ARENA, -1 - creature_ptr->arena_number, m_name);
		}
		else
		{
			QUEST_IDX q_idx = quest_number(creature_ptr, creature_ptr->current_floor_ptr->dun_level);
			bool seppuku = streq(hit_from, "Seppuku");
			bool winning_seppuku = current_world_ptr->total_winner && seppuku;

			play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_GAMEOVER);

#ifdef WORLD_SCORE
			/* Make screen dump */
			screen_dump = make_screen_dump(creature_ptr, process_autopick_file_command);
#endif

			/* Note cause of death */
			if (seppuku)
			{
				strcpy(creature_ptr->died_from, hit_from);
#ifdef JP
				if (!winning_seppuku) strcpy(creature_ptr->died_from, "切腹");
#endif
			}
			else
			{
				char dummy[1024];
#ifdef JP
				sprintf(dummy, "%s%s%s", !creature_ptr->paralyzed ? "" : creature_ptr->free_act ? "彫像状態で" : "麻痺状態で", creature_ptr->image ? "幻覚に歪んだ" : "", hit_from);
#else
				sprintf(dummy, "%s%s", hit_from, !creature_ptr->paralyzed ? "" : " while helpless");
#endif
				my_strcpy(creature_ptr->died_from, dummy, sizeof creature_ptr->died_from);
			}

			/* No longer a winner */
			current_world_ptr->total_winner = FALSE;

			if (winning_seppuku)
			{
				exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 0, _("勝利の後切腹した。", "committed seppuku after the winning."));
			}
			else
			{
				char buf[20];

				if (creature_ptr->current_floor_ptr->inside_arena)
					strcpy(buf, _("アリーナ", "in the Arena"));
				else if (!creature_ptr->current_floor_ptr->dun_level)
					strcpy(buf, _("地上", "on the surface"));
				else if (q_idx && (is_fixed_quest_idx(q_idx) &&
					!((q_idx == QUEST_OBERON) || (q_idx == QUEST_SERPENT))))
					strcpy(buf, _("クエスト", "in a quest"));
				else
					sprintf(buf, _("%d階", "level %d"), (int)creature_ptr->current_floor_ptr->dun_level);

				sprintf(tmp, _("%sで%sに殺された。", "killed by %s %s."), buf, creature_ptr->died_from);
				exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 0, tmp);
			}

			exe_write_diary(creature_ptr, DIARY_GAMESTART, 1, _("-------- ゲームオーバー --------", "--------   Game  Over   --------"));
			exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, "\n\n\n\n");

			flush();

			if (get_check_strict(creature_ptr, _("画面を保存しますか？", "Dump the screen? "), CHECK_NO_HISTORY))
			{
				do_cmd_save_screen(creature_ptr, process_autopick_file_command);
			}

			flush();

			/* Initialize "last message" buffer */
			if (creature_ptr->last_message) string_free(creature_ptr->last_message);
			creature_ptr->last_message = NULL;

			/* Hack -- Note death */
			if (!last_words)
			{
#ifdef JP
				msg_format("あなたは%sました。", android ? "壊れ" : "死に");
#else
				msg_print(android ? "You are broken." : "You die.");
#endif

				msg_print(NULL);
			}
			else
			{
				if (winning_seppuku)
				{
					get_rnd_line(_("seppuku_j.txt", "seppuku.txt"), 0, death_message);
				}
				else
				{
					get_rnd_line(_("death_j.txt", "death.txt"), 0, death_message);
				}

				do
				{
#ifdef JP
					while (!get_string(winning_seppuku ? "辞世の句: " : "断末魔の叫び: ", death_message, 1024));
#else
					while (!get_string("Last word: ", death_message, 1024));
#endif
				} while (winning_seppuku && !get_check_strict(creature_ptr, _("よろしいですか？", "Are you sure? "), CHECK_NO_HISTORY));

				if (death_message[0] == '\0')
				{
#ifdef JP
					strcpy(death_message, format("あなたは%sました。", android ? "壊れ" : "死に"));
#else
					strcpy(death_message, android ? "You are broken." : "You die.");
#endif
				}
				else creature_ptr->last_message = string_make(death_message);

#ifdef JP
				if (winning_seppuku)
				{
					int i, len;
					int w = Term->wid;
					int h = Term->hgt;
					int msg_pos_x[9] = { 5,  7,  9, 12,  14,  17,  19,  21, 23 };
					int msg_pos_y[9] = { 3,  4,  5,  4,   5,   4,   5,   6,  4 };
					concptr str;
					char* str2;

					Term_clear();

					/* 桜散る */
					for (i = 0; i < 40; i++)
						Term_putstr(randint0(w / 2) * 2, randint0(h), 2, TERM_VIOLET, "υ");

					str = death_message;
					if (strncmp(str, "「", 2) == 0) str += 2;

					str2 = my_strstr(str, "」");
					if (str2 != NULL) *str2 = '\0';

					i = 0;
					while (i < 9)
					{
						str2 = my_strstr(str, " ");
						if (str2 == NULL) len = strlen(str);
						else len = str2 - str;

						if (len != 0)
						{
							Term_putstr_v(w * 3 / 4 - 2 - msg_pos_x[i] * 2, msg_pos_y[i], len,
								TERM_WHITE, str);
							if (str2 == NULL) break;
							i++;
						}
						str = str2 + 1;
						if (*str == 0) break;
					}

					/* Hide cursor */
					Term_putstr(w - 1, h - 1, 1, TERM_WHITE, " ");

					flush();
#ifdef WORLD_SCORE
					/* Make screen dump */
					screen_dump = make_screen_dump(creature_ptr, process_autopick_file_command);
#endif

					/* Wait a key press */
					(void)inkey();
				}
				else
#endif
					msg_print(death_message);
			}
		}

		/* Dead */
		return damage;
	}

	handle_stuff(creature_ptr);

	/* Hitpoint warning */
	if (creature_ptr->chp < warning)
	{
		/* Hack -- bell on first notice */
		if (old_chp > warning) bell();

		sound(SOUND_WARN);

		if (record_danger && (old_chp > warning))
		{
			if (creature_ptr->image && damage_type == DAMAGE_ATTACK)
				hit_from = _("何か", "something");

			sprintf(tmp, _("%sによってピンチに陥った。", "was in a critical situation because of %s."), hit_from);
			exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 0, tmp);
		}

		if (auto_more)
		{
			/* stop auto_more even if DAMAGE_USELIFE */
			creature_ptr->now_damaged = TRUE;
		}

		msg_print(_("*** 警告:低ヒット・ポイント！ ***", "*** LOW HITPOINT WARNING! ***"));
		msg_print(NULL);
		flush();
	}
	if (creature_ptr->wild_mode && !creature_ptr->leaving && (creature_ptr->chp < MAX(warning, creature_ptr->mhp / 5)))
	{
		change_wild_mode(creature_ptr, FALSE);
	}
	return damage;
}

/*!
* @brief 属性に応じた敵オーラによるプレイヤーのダメージ処理
* @param m_ptr オーラを持つモンスターの構造体参照ポインタ
* @param immune ダメージを回避できる免疫フラグ
* @param flags_offset オーラフラグ配列の参照オフセット
* @param r_flags_offset モンスターの耐性配列の参照オフセット
* @param aura_flag オーラフラグ配列
* @param dam_func ダメージ処理を行う関数の参照ポインタ
* @param message オーラダメージを受けた際のメッセージ
* @return なし
*/
static void process_aura_damage(monster_type *m_ptr, player_type *touched_ptr, bool immune, int flags_offset, int r_flags_offset, u32b aura_flag,
    HIT_POINT (*dam_func)(player_type *creature_type, HIT_POINT dam, concptr kb_str, int monspell, bool aura), concptr message)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (!(atoffset(BIT_FLAGS, r_ptr, flags_offset) & aura_flag) || immune)
        return;

    GAME_TEXT mon_name[MAX_NLEN];
    int aura_damage = damroll(1 + (r_ptr->level / 26), 1 + (r_ptr->level / 17));

    monster_desc(touched_ptr, mon_name, m_ptr, MD_WRONGDOER_NAME);
    msg_print(message);
    dam_func(touched_ptr, aura_damage, mon_name, -1, TRUE);

    if (is_original_ap_and_seen(touched_ptr, m_ptr)) {
        atoffset(BIT_FLAGS, r_ptr, r_flags_offset) |= aura_flag;
    }

    handle_stuff(touched_ptr);
}

/*!
* @brief 敵オーラによるプレイヤーのダメージ処理
* @param m_ptr オーラを持つモンスターの構造体参照ポインタ
* @param touched_ptr オーラを持つ相手に振れたクリーチャーの参照ポインタ
* @return なし
*/
void touch_zap_player(monster_type *m_ptr, player_type *touched_ptr)
{
    process_aura_damage(m_ptr, touched_ptr, touched_ptr->immune_fire, offsetof(monster_race, flags2), offsetof(monster_race, r_flags2), RF2_AURA_FIRE,
        fire_dam, _("突然とても熱くなった！", "You are suddenly very hot!"));
    process_aura_damage(m_ptr, touched_ptr, touched_ptr->immune_cold, offsetof(monster_race, flags3), offsetof(monster_race, r_flags3), RF3_AURA_COLD,
        cold_dam, _("突然とても寒くなった！", "You are suddenly very cold!"));
    process_aura_damage(m_ptr, touched_ptr, touched_ptr->immune_elec, offsetof(monster_race, flags2), offsetof(monster_race, r_flags2), RF2_AURA_ELEC,
        elec_dam, _("電撃をくらった！", "You get zapped!"));
}
