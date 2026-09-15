(function () {
  'use strict';

  // Most cells show literal source flags. Nether and chaos additionally show
  // partial mitigation derived from the EVIL and DEMON kind flags.
  const columns = [
    ['RES_ALL', '全耐性', 'Resist all'],
    ['RES_ACID', '酸', 'Acid'], ['IM_ACID', '酸免疫', 'Acid immunity'],
    ['RES_ELEC', '電撃', 'Electricity'], ['IM_ELEC', '電撃免疫', 'Electricity immunity'],
    ['RES_FIRE', '炎', 'Fire'], ['IM_FIRE', '炎免疫', 'Fire immunity'],
    ['RES_COLD', '冷気', 'Cold'], ['IM_COLD', '冷気免疫', 'Cold immunity'],
    ['RES_POIS', '毒', 'Poison'], ['IM_POIS', '毒免疫', 'Poison immunity'],
    ['RES_LITE', '閃光', 'Light'], ['RES_DARK', '暗黒', 'Dark'],
    ['RES_NETH', '地獄', 'Nether'], ['RES_WATE', '水', 'Water'],
    ['RES_PLAS', 'プラズマ', 'Plasma'], ['RES_SHAR', '破片', 'Shards'],
    ['RES_SOUN', '轟音', 'Sound'], ['RES_CHAO', 'カオス', 'Chaos'],
    ['RES_NEXU', '因果混乱', 'Nexus'], ['RES_DISE', '劣化', 'Disenchantment'],
    ['RES_WALL', '魔力', 'Force'], ['RES_INER', '遅鈍', 'Inertia'],
    ['RES_TIME', '時間逆転', 'Time'], ['RES_GRAV', '重力', 'Gravity'],
    ['RES_TELE', 'テレポート', 'Teleportation'], ['RES_ROCK', '岩石', 'Rock'],
    ['RES_ABYSS', '深淵', 'Abyss'], ['RES_VOID', '虚無', 'Void magic'],
    ['RES_METEOR', '隕石', 'Meteor'],
    ['NO_FEAR', '恐怖無効', 'No fear'], ['NO_STUN', '朦朧無効', 'No stun'],
    ['NO_CONF', '混乱無効', 'No confusion'], ['NO_SLEEP', '睡眠無効', 'No sleep'],
    ['NO_INSTANTLY_DEATH', '即死無効', 'No instant death'],
  ];
  const byId = id => document.getElementById(id);
  const query = byId('monster-query');
  const minLevel = byId('min-level');
  const maxLevel = byId('max-level');
  const columnQuery = byId('resistance-query');
  const onlyHeld = byId('only-held');
  const count = byId('resistance-count');
  const table = byId('resistance-table');
  let monsters = [];

  function key(value) {
    return String(value || '').normalize('NFKC').toLowerCase()
      .replace(/[ァ-ヶヽヾ]/g, char => String.fromCharCode(char.charCodeAt(0) - 0x60));
  }
  function cell(tag, className, text) {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (text !== undefined) element.textContent = text;
    return element;
  }
  function resistance(monster, flag) {
    if (monster.flags.includes(flag)) return { text: '●', className: 'held', detail: '定義上の耐性 / explicit flag' };
    if (flag === 'RES_NETH' && (monster.kinds || []).includes('EVIL')) {
      return { text: '½', className: 'partial', detail: '邪悪 (EVIL): 地獄ダメージ半減 / EVIL: nether damage halved' };
    }
    if (flag === 'RES_CHAO' && (monster.kinds || []).includes('DEMON')) {
      return { text: '⅓', className: 'partial', detail: '悪魔 (DEMON): 3分の1の確率でカオス軽減 / DEMON: 1-in-3 chance of chaos mitigation' };
    }
    return { text: '—', className: '', detail: 'なし / no' };
  }
  function render() {
    const nameNeedle = key(query.value).trim();
    const resistanceNeedle = key(columnQuery.value).trim();
    const minimum = minLevel.value === '' ? -Infinity : Number(minLevel.value);
    const maximum = maxLevel.value === '' ? Infinity : Number(maxLevel.value);
    const availableColumns = columns.filter(column => monsters.some(monster => resistance(monster, column[0]).text !== '—'));
    const visibleColumns = availableColumns.filter(column =>
      column.some(part => key(part).includes(resistanceNeedle)) ||
      ((key('一部軽減 partial mitigation').includes(resistanceNeedle)) &&
        (column[0] === 'RES_NETH' || column[0] === 'RES_CHAO')));
    const visibleMonsters = monsters.filter(monster =>
      monster.level >= minimum && monster.level <= maximum &&
      (!nameNeedle || key(monster.ja).includes(nameNeedle) || key(monster.en).includes(nameNeedle)) &&
      (!onlyHeld.checked || visibleColumns.some(column => resistance(monster, column[0]).text !== '—')));

    const headRow = document.createElement('tr');
    headRow.append(cell('th', 'monster-name', 'モンスター / Monster'));
    headRow.append(cell('th', 'monster-level', 'Lv'));
    for (const [flag, ja, en] of visibleColumns) {
      const heading = cell('th', '', ja);
      heading.title = `${ja} / ${en} (${flag})`;
      const code = cell('span', 'flag-code', flag);
      heading.append(code);
      headRow.append(heading);
    }
    table.tHead.replaceChildren(headRow);

    const body = document.createDocumentFragment();
    for (const monster of visibleMonsters) {
      const row = document.createElement('tr');
      const name = cell('td', 'monster-name');
      const link = cell('a', '', monster.ja);
      link.href = monster.url;
      name.append(link, cell('span', 'en', monster.en));
      row.append(name, cell('td', 'monster-level', monster.level));
      for (const [flag, ja, en] of visibleColumns) {
        const state = resistance(monster, flag);
        const value = cell('td', state.className, state.text);
        value.title = `${monster.ja}: ${ja} / ${en} — ${state.detail}`;
        row.append(value);
      }
      body.append(row);
    }
    table.tBodies[0].replaceChildren(body);
    count.textContent = `${visibleMonsters.length} / ${monsters.length} モンスター、${visibleColumns.length} / ${availableColumns.length} 耐性列 / monsters and resistance columns`;
  }

  let pending;
  for (const input of [query, minLevel, maxLevel, columnQuery, onlyHeld]) {
    input.addEventListener(input === onlyHeld ? 'change' : 'input', () => {
      clearTimeout(pending);
      pending = setTimeout(render, 120);
    });
  }
  count.textContent = 'モンスター耐性データを読み込み中… / Loading monster resistance data…';
  fetch('monster-resistance-data.json')
    .then(response => { if (!response.ok) throw new Error(response.status); return response.json(); })
    .then(data => { monsters = data.sort((a, b) => a.level - b.level || a.ja.localeCompare(b.ja, 'ja')); render(); })
    .catch(() => { count.textContent = 'データを読み込めませんでした。 / Could not load monster resistance data.'; });
})();
