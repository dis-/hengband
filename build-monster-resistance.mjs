import { execFileSync } from 'node:child_process';
import { readFileSync, readdirSync, writeFileSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

// Keep the table on the same game-data revision as the existing generated site.
const SOURCE_REVISION = process.argv[2] || '559732b34f';
const SOURCE_REPO = process.argv[3] || resolve(dirname(fileURLToPath(import.meta.url)), '..');
const source = execFileSync('git', ['-c', `safe.directory=${SOURCE_REPO.replaceAll('\\', '/')}`, 'show', `${SOURCE_REVISION}:lib/edit/MonraceDefinitions.jsonc`], {
  cwd: SOURCE_REPO,
  encoding: 'utf8',
  maxBuffer: 8 * 1024 * 1024,
});

function stripJsonc(input) {
  let output = '';
  let quoted = false;
  let escaped = false;
  for (let i = 0; i < input.length; i++) {
    const ch = input[i];
    const next = input[i + 1];
    if (quoted) {
      output += ch;
      if (escaped) escaped = false;
      else if (ch === '\\') escaped = true;
      else if (ch === '"') quoted = false;
    } else if (ch === '"') {
      quoted = true;
      output += ch;
    } else if (ch === '/' && next === '/') {
      i += 2;
      while (i < input.length && input[i] !== '\n') i++;
      output += '\n';
    } else if (ch === '/' && next === '*') {
      i += 2;
      while (i < input.length && !(input[i] === '*' && input[i + 1] === '/')) i++;
      i++;
    } else {
      output += ch;
    }
  }
  let withoutTrailingCommas = '';
  quoted = false;
  escaped = false;
  for (let i = 0; i < output.length; i++) {
    const ch = output[i];
    if (quoted) {
      withoutTrailingCommas += ch;
      if (escaped) escaped = false;
      else if (ch === '\\') escaped = true;
      else if (ch === '"') quoted = false;
    } else if (ch === '"') {
      quoted = true;
      withoutTrailingCommas += ch;
    } else if (ch === ',') {
      let next = i + 1;
      while (/\s/.test(output[next] || '')) next++;
      if (output[next] !== '}' && output[next] !== ']') withoutTrailingCommas += ch;
    } else {
      withoutTrailingCommas += ch;
    }
  }
  return withoutTrailingCommas;
}

const monsters = JSON.parse(stripJsonc(source)).monsters;
const filenames = new Map();
for (const entry of JSON.parse(readFileSync('search-index.json', 'utf8'))) {
  const match = /^Monster-(\d+)-.*\.html$/.exec(entry.url);
  if (match) filenames.set(Number(match[1]), entry.url);
}
const resistanceFlags = /^(?:RES_|IM_|NO_)/;
const rows = monsters.filter(monster => monster.id > 0).map(monster => ({
  id: monster.id,
  ja: monster.name.ja,
  en: monster.name.en,
  level: monster.level,
  flags: (monster.flags || []).filter(flag => resistanceFlags.test(flag)),
  kinds: (monster.flags || []).filter(flag => flag === 'EVIL' || flag === 'DEMON'),
  url: filenames.get(monster.id) || null,
}));
if (rows.length !== 1416) throw new Error(`Expected 1416 monsters, got ${rows.length}`);
if (rows.some(row => !row.url)) throw new Error('Some monster detail links are missing');
writeFileSync('monster-resistance-data.json', JSON.stringify(rows));

const index = readFileSync('index.html', 'utf8');
const sidebar = index.match(/<header class="site-header">[\s\S]*?<\/aside>/)?.[0];
if (!sidebar) throw new Error('Cannot find the existing site shell');
const pageSidebar = sidebar.includes('<li><a href="Monster-Resistance.html">🛡️ 耐性表 / Resistance table</a></li>')
  ? sidebar
  : sidebar.replace(
      '<li><a href="Monsters-by-Name.html">🔎 名前で検索 / By name</a></li>',
      '<li><a href="Monsters-by-Name.html">🔎 名前で検索 / By name</a></li>\n<li><a href="Monster-Resistance.html">🛡️ 耐性表 / Resistance table</a></li>',
    );
const html = `<!doctype html>
<html lang="ja"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>モンスター耐性表 / Monster Resistance Table · 変愚蛮怒 データ仕様</title>
<link rel="stylesheet" href="style.css"><link rel="stylesheet" href="monster-resistance.css"></head>
<body>
${pageSidebar}<main class="content">
<h1>モンスター耐性表 / Monster Resistance Table</h1>
<p>名前・レベルで行を、耐性名で列を絞り込めます。● は定義上の耐性フラグ、½ は邪悪（EVIL）による地獄ダメージ半減、⅓ は悪魔（DEMON）に3分の1の確率で起こるカオス軽減です。</p>
<p>Filter rows by name and level, and columns by resistance name. ● is an explicit resistance flag; ½ means nether damage is halved for EVIL monsters; ⅓ means chaos mitigation activates with a 1-in-3 chance for DEMON monsters.</p>
<div class="resistance-controls">
<fieldset><legend>行を絞り込み / Filter rows</legend>
<label>モンスター名 / Name <input id="monster-query" type="search" placeholder="日本語名・English name" autocomplete="off"></label>
<label>レベル / Level <input id="min-level" type="number" min="0" placeholder="最小 / Min" aria-label="最小レベル"> ～ <input id="max-level" type="number" min="0" placeholder="最大 / Max" aria-label="最大レベル"></label>
</fieldset>
<fieldset><legend>列を絞り込み / Filter columns</legend>
<label>耐性名 / Resistance <input id="resistance-query" type="search" placeholder="炎・カオス・teleport・RES_CHAO" autocomplete="off"></label>
<label><input id="only-held" type="checkbox"> 表示中の耐性を持つモンスターのみ / Only monsters with a visible resistance</label>
</fieldset>
</div>
<p id="resistance-count" role="status" aria-live="polite"></p>
<div class="tablewrap resistance-tablewrap"><table id="resistance-table"><thead></thead><tbody></tbody></table></div>
<p class="resistance-note">※ 地獄・カオスの一部軽減のみ、種別フラグから導出しています。定義上の耐性がある場合は ● を優先します。弱点フラグ（HURT_*）やその他の複合ダメージ計算は含めていません。 / Only partial nether and chaos mitigation is derived from kind flags. Explicit resistance takes precedence; vulnerability flags and other derived calculations are excluded.</p>
<blockquote>🤖 データ元 / Source: <code>lib/edit/MonraceDefinitions.jsonc</code> · <code>src/effect/effect-monster-resist-hurt.cpp</code> · hengband <code>${SOURCE_REVISION}</code> · <code>build-monster-resistance.mjs</code></blockquote>
</main></div>
<script src="app.js"></script><script src="monster-resistance.js"></script>
</body></html>
`;
writeFileSync('Monster-Resistance.html', html);

const navTarget = '<li><a href="Monsters-by-Name.html">🔎 名前で検索 / By name</a></li>';
const navWithTable = `${navTarget}\n<li><a href="Monster-Resistance.html">🛡️ 耐性表 / Resistance table</a></li>`;
const homeTarget = '<li>🔤 <a href="Monsters-by-Symbol.html">記号から / By symbol</a> ・ 🔎 <a href="Monsters-by-Name.html">名前で検索 / By name</a></li>';
const homeWithTable = homeTarget.replace('</li>', ' ・ 🛡️ <a href="Monster-Resistance.html">耐性表 / Resistance table</a></li>');
const monsterTarget = '<li>🔎 <strong><a href="Monsters-by-Name.html">名前で検索（五十音）/ Search by name</a></strong> — 日本語名の読みで五十音順に整列。Ctrl+F で日英どちらも検索可 / gojūon-ordered index, searchable with Ctrl+F in either language</li>';
const monsterWithTable = `${monsterTarget}\n<li>🛡️ <strong><a href="Monster-Resistance.html">耐性表 / Resistance table</a></strong> — 名前・レベルで行、耐性名で列を絞り込み / filter rows and columns independently</li>`;

function replaceOnce(file, before, after) {
  let content = readFileSync(file, 'utf8');
  if (content.includes(after)) return;
  if (!content.includes(before)) throw new Error(`Cannot find insertion point in ${file}`);
  content = content.replace(before, after);
  writeFileSync(file, content);
}
// Generated pages contain a static copy of the sidebar, so update every page.
for (const file of readdirSync('.').filter(name => name.endsWith('.html'))) {
  replaceOnce(file, navTarget, navWithTable);
}
replaceOnce('index.html', homeTarget, homeWithTable);
replaceOnce('Monsters.html', monsterTarget, monsterWithTable);

const searchIndex = JSON.parse(readFileSync('search-index.json', 'utf8'));
if (!searchIndex.some(entry => entry.url === 'Monster-Resistance.html')) {
  searchIndex.push({ ja: 'モンスター耐性表', en: 'Monster Resistance Table', url: 'Monster-Resistance.html', t: 'ページ' });
  writeFileSync('search-index.json', JSON.stringify(searchIndex));
}
