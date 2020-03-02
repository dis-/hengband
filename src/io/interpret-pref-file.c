﻿/*!
 * @brief io/ ではなくcmd/ の方がいいかもしれない (do_cmd_process_pref_file() に改名)
 * @date 2020/03/01
 * @author Hourier
 */

#include "io/interpret-pref-file.h"
#include "io/gf-descriptions.h"
#include "io/tokenizer.h"
#include "objectkind.h"
#include "birth.h"
#include "world.h"
#include "term.h"
#include "view-mainwindow.h" // 暫定。apply_default_feat_lighting()。後で消す.

/*!
 * @brief 設定ファイルの各行から各種テキスト情報を取得する /
 * Parse a sub-file of the "extra info" (format shown below)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param buf データテキストの参照ポインタ
 * @return エラーコード
 * @details
 * <pre>
 * Each "action" line has an "action symbol" in the first column,
 * followed by a colon, followed by some command specific info,
 * usually in the form of "tokens" separated by colons or slashes.
 * Blank lines, lines starting with white space, and lines starting
 * with pound signs ("#") are ignored (as comments).
 * Note the use of "tokenize()" to allow the use of both colons and
 * slashes as delimeters, while still allowing final tokens which
 * may contain any characters including "delimiters".
 * Note the use of "strtol()" to allow all "integers" to be encoded
 * in decimal, hexidecimal, or octal form.
 * Note that "monster zero" is used for the "player" attr/char, "object
 * zero" will be used for the "stack" attr/char, and "feature zero" is
 * used for the "nothing" attr/char.
 * Parse another file recursively, see below for details
 *   %:\<filename\>
 * Specify the attr/char values for "monsters" by race index
 *   R:\<num\>:\<a\>:\<c\>
 * Specify the attr/char values for "objects" by kind index
 *   K:\<num\>:\<a\>:\<c\>
 * Specify the attr/char values for "features" by feature index
 *   F:\<num\>:\<a\>:\<c\>
 * Specify the attr/char values for unaware "objects" by kind tval
 *   U:\<tv\>:\<a\>:\<c\>
 * Specify the attr/char values for inventory "objects" by kind tval
 *   E:\<tv\>:\<a\>:\<c\>
 * Define a macro action, given an encoded macro action
 *   A:\<str\>
 * Create a normal macro, given an encoded macro trigger
 *   P:\<str\>
 * Create a command macro, given an encoded macro trigger
 *   C:\<str\>
 * Create a keyset mapping
 *   S:\<key\>:\<key\>:\<dir\>
 * Turn an option off, given its name
 *   X:\<str\>
 * Turn an option on, given its name
 *   Y:\<str\>
 * Specify visual information, given an index, and some data
 *   V:\<num\>:\<kv\>:\<rv\>:\<gv\>:\<bv\>
 * Specify the set of colors to use when drawing a zapped spell
 *   Z:\<type\>:\<str\>
 * Specify a macro trigger template and macro trigger names.
 *   T:\<template\>:\<modifier chr\>:\<modifier name1\>:\<modifier name2\>:...
 *   T:\<trigger\>:\<keycode\>:\<shift-keycode\>
 * </pre>
 */
errr interpret_pref_file(player_type *creature_ptr, char *buf)
{
	if (buf[1] != ':') return 1;

	char *zz[16];
	switch (buf[0])
	{
	case 'H':
	{
		/* Process "H:<history>" */
		add_history_from_pref_line(buf + 2);
		return 0;
	}
	case 'R':
	{
		/* Process "R:<num>:<a>/<c>" -- attr/char for monster races */
		if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) return 1;

		monster_race *r_ptr;
		int i = (huge)strtol(zz[0], NULL, 0);
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		if (i >= max_r_idx) return 1;
		r_ptr = &r_info[i];
		if (n1 || (!(n2 & 0x80) && n2)) r_ptr->x_attr = n1; /* Allow TERM_DARK text */
		if (n2) r_ptr->x_char = n2;
		return 0;
	}
	case 'K':
	{
		/* Process "K:<num>:<a>/<c>"  -- attr/char for object kinds */
		if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) return 1;
		
		object_kind *k_ptr;
		int i = (huge)strtol(zz[0], NULL, 0);
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		if (i >= max_k_idx) return 1;
		k_ptr = &k_info[i];
		if (n1 || (!(n2 & 0x80) && n2)) k_ptr->x_attr = n1; /* Allow TERM_DARK text */
		if (n2) k_ptr->x_char = n2;
		return 0;
	}
	case 'F':
	{
		/* Process "F:<num>:<a>/<c>" -- attr/char for terrain features */
		/* "F:<num>:<a>/<c>" */
		/* "F:<num>:<a>/<c>:LIT" */
		/* "F:<num>:<a>/<c>:<la>/<lc>:<da>/<dc>" */
		feature_type *f_ptr;
		int num = tokenize(buf + 2, F_LIT_MAX * 2 + 1, zz, TOKENIZE_CHECKQUOTE);

		if ((num != 3) && (num != 4) && (num != F_LIT_MAX * 2 + 1)) return 1;
		else if ((num == 4) && !streq(zz[3], "LIT")) return 1;

		int i = (huge)strtol(zz[0], NULL, 0);
		if (i >= max_f_idx) return 1;
		f_ptr = &f_info[i];

		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		if (n1 || (!(n2 & 0x80) && n2)) f_ptr->x_attr[F_LIT_STANDARD] = n1; /* Allow TERM_DARK text */
		if (n2) f_ptr->x_char[F_LIT_STANDARD] = n2;

		switch (num)
		{
		case 3:
		{
			/* No lighting support */
			n1 = f_ptr->x_attr[F_LIT_STANDARD];
			n2 = f_ptr->x_char[F_LIT_STANDARD];
			for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
			{
				f_ptr->x_attr[j] = n1;
				f_ptr->x_char[j] = n2;
			}

			return 0;
		}
		case 4:
		{
			/* Use default lighting */
			apply_default_feat_lighting(f_ptr->x_attr, f_ptr->x_char);
			return 0;
		}
		case F_LIT_MAX * 2 + 1:
		{
			/* Use desired lighting */
			for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
			{
				n1 = (TERM_COLOR)strtol(zz[j * 2 + 1], NULL, 0);
				n2 = (SYMBOL_CODE)strtol(zz[j * 2 + 2], NULL, 0);
				if (n1 || (!(n2 & 0x80) && n2)) f_ptr->x_attr[j] = n1; /* Allow TERM_DARK text */
				if (n2) f_ptr->x_char[j] = n2;
			}

			return 0;
		}
		default:
			return 0;
		}
	}
	case 'S':
	{
		/* Process "S:<num>:<a>/<c>" -- attr/char for special things */
		if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) return 1;

		int j = (byte)strtol(zz[0], NULL, 0);
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		misc_to_attr[j] = n1;
		misc_to_char[j] = n2;
		return 0;
	}
	case 'U':
	{
		/* Process "U:<tv>:<a>/<c>" -- attr/char for unaware items */
		if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) return 1;

		int j = (huge)strtol(zz[0], NULL, 0);
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		for (int i = 1; i < max_k_idx; i++)
		{
			object_kind *k_ptr = &k_info[i];
			if (k_ptr->tval == j)
			{
				if (n1) k_ptr->d_attr = n1;
				if (n2) k_ptr->d_char = n2;
			}
		}

		return 0;
	}
	case 'E':
	{
		/* Process "E:<tv>:<a>" -- attribute for inventory objects */
		if (tokenize(buf + 2, 2, zz, TOKENIZE_CHECKQUOTE) != 2) return 1;

		int j = (byte)strtol(zz[0], NULL, 0) % 128;
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		if (n1) tval_to_attr[j] = n1;
		return 0;
	}
	case 'A':
	{
		/* Process "A:<str>" -- save an "action" for later */
		text_to_ascii(macro__buf, buf + 2);
		return 0;
	}
	case 'P':
	{
		/* Process "P:<str>" -- normal macro */
		char tmp[1024];
		text_to_ascii(tmp, buf + 2);
		macro_add(tmp, macro__buf);
		return 0;
	}
	case 'C':
	{
		/* Process "C:<str>" -- create keymap */
		if (tokenize(buf + 2, 2, zz, TOKENIZE_CHECKQUOTE) != 2) return 1;

		int mode = strtol(zz[0], NULL, 0);
		if ((mode < 0) || (mode >= KEYMAP_MODES)) return 1;

		char tmp[1024];
		text_to_ascii(tmp, zz[1]);
		if (!tmp[0] || tmp[1]) return 1;

		int i = (byte)(tmp[0]);
		string_free(keymap_act[mode][i]);
		keymap_act[mode][i] = string_make(macro__buf);
		return 0;
	}
	case 'V':
	{
		/* Process "V:<num>:<kv>:<rv>:<gv>:<bv>" -- visual info */
		if (tokenize(buf + 2, 5, zz, TOKENIZE_CHECKQUOTE) != 5) return 1;

		int i = (byte)strtol(zz[0], NULL, 0);
		angband_color_table[i][0] = (byte)strtol(zz[1], NULL, 0);
		angband_color_table[i][1] = (byte)strtol(zz[2], NULL, 0);
		angband_color_table[i][2] = (byte)strtol(zz[3], NULL, 0);
		angband_color_table[i][3] = (byte)strtol(zz[4], NULL, 0);
		return 0;
	}
	case 'X':
	case 'Y':
	{
		/* Process "X:<str>" -- turn option off */
		/* Process "Y:<str>" -- turn option on */
		for (int i = 0; option_info[i].o_desc; i++)
		{
			bool is_option = option_info[i].o_var != NULL;
			is_option &= option_info[i].o_text != NULL;
			is_option &= streq(option_info[i].o_text, buf + 2);
			if (!is_option) continue;

			int os = option_info[i].o_set;
			int ob = option_info[i].o_bit;

			if ((creature_ptr->playing || current_world_ptr->character_xtra) &&
				(OPT_PAGE_BIRTH == option_info[i].o_page) && !current_world_ptr->wizard)
			{
				msg_format(_("初期オプションは変更できません! '%s'", "Birth options can not changed! '%s'"), buf);
				msg_print(NULL);
				return 0;
			}

			if (buf[0] == 'X')
			{
				option_flag[os] &= ~(1L << ob);
				(*option_info[i].o_var) = FALSE;
				return 0;
			}

			option_flag[os] |= (1L << ob);
			(*option_info[i].o_var) = TRUE;
			return 0;
		}

		msg_format(_("オプションの名前が正しくありません： %s", "Ignored invalid option: %s"), buf);
		msg_print(NULL);
		return 0;
	}
	case 'Z':
	{
		/* Process "Z:<type>:<str>" -- set spell color */
		char *t = my_strchr(buf + 2, ':');
		if (!t) return 1;

		*(t++) = '\0';
		for (int i = 0; i < MAX_NAMED_NUM; i++)
		{
			if (!streq(gf_desc[i].name, buf + 2)) continue;

			gf_color[gf_desc[i].num] = (TERM_COLOR)quark_add(t);
			return 0;
		}

		return 1;
	}
	case 'T':
	{
		/* Initialize macro trigger names and a template */
		/* Process "T:<trigger>:<keycode>:<shift-keycode>" */
		/* Process "T:<template>:<modifier chr>:<modifier name>:..." */
		int tok = tokenize(buf + 2, 2 + MAX_MACRO_MOD, zz, 0);

		/* Process "T:<template>:<modifier chr>:<modifier name>:..." */
		if (tok >= 4)
		{
			if (macro_template != NULL)
			{
				int macro_modifier_length = strlen(macro_modifier_chr);
				string_free(macro_template);
				macro_template = NULL;
				string_free(macro_modifier_chr);
				for (int i = 0; i < macro_modifier_length; i++)
				{
					string_free(macro_modifier_name[i]);
				}

				for (int i = 0; i < max_macrotrigger; i++)
				{
					string_free(macro_trigger_name[i]);
					string_free(macro_trigger_keycode[0][i]);
					string_free(macro_trigger_keycode[1][i]);
				}

				max_macrotrigger = 0;
			}

			if (*zz[0] == '\0') return 0;

			int zz_length = strlen(zz[1]);
			zz_length = MIN(MAX_MACRO_MOD, zz_length);
			if (2 + zz_length != tok) return 1;

			macro_template = string_make(zz[0]);
			macro_modifier_chr = string_make(zz[1]);
			for (int i = 0; i < zz_length; i++)
			{
				macro_modifier_name[i] = string_make(zz[2 + i]);
			}

			return 0;
		}

		/* Process "T:<trigger>:<keycode>:<shift-keycode>" */
		if (tok < 2) return 0;

		char buf_aux[1024];
		char *t, *s;
		if (max_macrotrigger >= MAX_MACRO_TRIG)
		{
			msg_print(_("マクロトリガーの設定が多すぎます!", "Too many macro triggers!"));
			return 1;
		}

		int m = max_macrotrigger;
		max_macrotrigger++;
		t = buf_aux;
		s = zz[0];
		while (*s)
		{
			if ('\\' == *s) s++;
			*t++ = *s++;
		}

		*t = '\0';
		macro_trigger_name[m] = string_make(buf_aux);
		macro_trigger_keycode[0][m] = string_make(zz[1]);
		if (tok == 3)
		{
			macro_trigger_keycode[1][m] = string_make(zz[2]);
			return 0;
		}

		macro_trigger_keycode[1][m] = string_make(zz[1]);
		return 0;
	}
	}

	return 1;
}
