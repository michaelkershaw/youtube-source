/* YouTube Source v3 - WebView2 UI */
'use strict';

// ===== Bridge (WebView2 on Windows, WKWebView on macOS) =====
const bridge = {
  send(type, payload = {}) {
    if (window.chrome && window.chrome.webview) {
      // Windows: WebView2
      window.chrome.webview.postMessage({ type, payload });
    } else if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.host) {
      // macOS: WKWebView (expects a JSON string)
      window.webkit.messageHandlers.host.postMessage(JSON.stringify({ type, payload }));
    } else {
      console.log('[bridge:no-host]', type, payload);
    }
  }
};

// macOS host calls this to deliver messages (Windows uses chrome.webview 'message' events)
window.__hostMessage = function (json) {
  try {
    handleHostMessage(typeof json === 'string' ? JSON.parse(json) : json);
  } catch (e) {
    console.error('[bridge:bad-message]', e);
  }
};

// ===== State =====
const state = {
  view: 'search',
  format: 'mp3',
  results: [],        // current search results
  playlist: [],       // imported playlist tracks
  trending: [],
  charts: [],
  history: [],
  downloadHistory: [],
  genre: 'all',
  selectedId: null,
  licensed: false,
  license: {},
  searching: false,
  downloads: {}       // id -> progress
};

const GENRES = ['all', 'pop', 'hip hop', 'edm', 'house', 'rock', 'latin', 'afrobeats', 'r&b', 'reggaeton', 'country', 'jazz'];

// ===== DOM =====
const $ = s => document.querySelector(s);
const content = $('#content');
const statusText = $('#status-text');

// ===== Window chrome =====
$('#titlebar').addEventListener('mousedown', e => {
  if (e.target.closest('button')) return;
  bridge.send('dragWindow');
});
$('#btn-close').onclick = () => bridge.send('closeWindow');
$('#btn-min').onclick = () => bridge.send('minimizeWindow');

// ===== Tabs =====
document.querySelectorAll('.tab').forEach(t => {
  t.onclick = () => switchView(t.dataset.view);
});

function switchView(view) {
  state.view = view;
  document.querySelectorAll('.tab').forEach(t =>
    t.classList.toggle('active', t.dataset.view === view));
  $('#toolbar').style.display = (view === 'search') ? 'flex' : 'none';
  render();
  if (view === 'trending' && state.trending.length === 0) loadTrending();
  if (view === 'charts' && state.charts.length === 0) loadCharts(state.genre);
  if (view === 'history') bridge.send('getHistory');
}

// ===== Search =====
$('#btn-search').onclick = doSearch;
$('#search-input').addEventListener('keydown', e => {
  if (e.key === 'Enter') { hideSearchDropdown(); doSearch(); }
  if (e.key === 'Escape') hideSearchDropdown();
});

function doSearch() {
  const q = $('#search-input').value.trim();
  if (!q || state.searching) return;
  hideSearchDropdown();
  state.searching = true;
  state.results = [];
  // Update local history immediately (backend saves it too, but doesn't push back)
  state.history = state.history.filter(h => h.query !== q);
  const now = new Date();
  const pad = n => String(n).padStart(2, '0');
  const date = `${now.getFullYear()}-${pad(now.getMonth() + 1)}-${pad(now.getDate())} ${pad(now.getHours())}:${pad(now.getMinutes())}`;
  state.history.unshift({ query: q, date });
  if (state.history.length > 15) state.history.length = 15;
  setStatus('Searching: ' + q);
  renderSkeletons();
  bridge.send('search', { query: q, limit: 25 });
}

// ===== Search history dropdown =====
const searchDropdown = $('#search-dropdown');

function showSearchDropdown(filter) {
  const typed = filter === true ? $('#search-input').value.trim().toLowerCase() : '';
  const items = state.history.filter(h => !typed || h.query.toLowerCase().includes(typed));
  if (!items.length) { hideSearchDropdown(); return; }
  searchDropdown.innerHTML = items.map((h, i) => `
    <div class="sd-item" data-q="${esc(h.query)}">
      <span class="sd-icon">&#128336;</span>
      <span class="sd-q">${esc(h.query)}</span>
      <span class="sd-d">${esc(h.date || '')}</span>
    </div>`).join('');
  searchDropdown.style.display = 'block';
  searchDropdown.querySelectorAll('.sd-item').forEach(item => {
    item.onmousedown = e => {
      e.preventDefault();
      $('#search-input').value = item.dataset.q;
      hideSearchDropdown();
      doSearch();
    };
  });
}

function hideSearchDropdown() {
  searchDropdown.style.display = 'none';
}

$('#search-input').addEventListener('focus', () => showSearchDropdown(false));
$('#search-input').addEventListener('click', () => showSearchDropdown(false));
$('#search-input').addEventListener('input', () => showSearchDropdown(true));
$('#search-input').addEventListener('blur', () => setTimeout(hideSearchDropdown, 150));

// ===== Format toggle =====
$('#fmt-mp3').onclick = () => setFormat('mp3');
$('#fmt-mp4').onclick = () => setFormat('mp4');

function setFormat(fmt) {
  state.format = fmt;
  $('#fmt-mp3').className = fmt === 'mp3' ? 'active mp3' : '';
  $('#fmt-mp4').className = fmt === 'mp4' ? 'active mp4' : '';
  bridge.send('setFormat', { format: fmt });
}

// ===== Machine ID copy =====
$('#machine-id').onclick = () => {
  const id = (state.license.machineId || '');
  if (id) { navigator.clipboard.writeText(id); toast('Machine ID copied', 'ok'); }
};

// ===== Rendering =====
function render() {
  switch (state.view) {
    case 'search':    renderTracks(state.results, 'Search YouTube to get started', '&#127925;'); break;
    case 'playlists': renderPlaylists(); break;
    case 'trending':  renderTracks(state.trending, 'Loading trending music...', '&#128293;'); break;
    case 'charts':    renderCharts(); break;
    case 'history':   renderHistory(); break;
    case 'license':   renderLicense(); break;
  }
}

function esc(s) {
  return String(s || '').replace(/[&<>"']/g, c =>
    ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
}

function fmtViews(v) {
  if (!v) return '';
  if (v >= 1e9) return (v / 1e9).toFixed(1) + 'B views';
  if (v >= 1e6) return (v / 1e6).toFixed(1) + 'M views';
  if (v >= 1e3) return (v / 1e3).toFixed(0) + 'K views';
  return v + ' views';
}

function trackCard(t) {
  const chips = (t.cachedMp3 ? '<span class="chip mp3">MP3</span>' : '') +
                (t.cachedMp4 ? '<span class="chip mp4">MP4</span>' : '');
  return `
  <div class="card ${state.selectedId === t.id ? 'selected' : ''}" data-id="${esc(t.id)}">
    <img class="thumb" loading="lazy" src="https://i.ytimg.com/vi/${esc(t.id)}/mqdefault.jpg" alt="">
    ${t.duration ? `<span class="dur">${esc(t.duration)}</span>` : ''}
    <div class="meta">
      <div class="t">${esc(t.title)}</div>
      <div class="c">
        <span>${esc(t.channel)}</span>
        ${t.views ? `<span>&middot; ${fmtViews(t.views)}</span>` : ''}
        ${chips}
      </div>
    </div>
    <div class="actions">
      <button class="abtn decka" data-act="deckA">DECK A</button>
      <button class="abtn deckb" data-act="deckB">DECK B</button>
      <button class="abtn more" data-act="menu">&#8942;</button>
    </div>
  </div>`;
}

function renderTracks(list, emptyMsg, emptyIcon) {
  if (!list.length) {
    content.innerHTML = `<div class="empty-state"><div class="big">${emptyIcon}</div><div>${emptyMsg}</div></div>`;
    return;
  }
  content.innerHTML = list.map(trackCard).join('');
  wireCards(list);
}

function renderSkeletons(n = 8) {
  content.innerHTML = Array(n).fill('<div class="skeleton"></div>').join('');
}

function wireCards(list) {
  content.querySelectorAll('.card').forEach(card => {
    const id = card.dataset.id;
    const track = list.find(t => t.id === id);
    if (!track) return;

    card.onclick = e => {
      const act = e.target.dataset && e.target.dataset.act;
      if (act === 'deckA') return loadTrack(track, 'deckA');
      if (act === 'deckB') return loadTrack(track, 'deckB');
      if (act === 'menu') return showMenu(e, track);
      state.selectedId = id;
      content.querySelectorAll('.card').forEach(c => c.classList.remove('selected'));
      card.classList.add('selected');
    };
    card.ondblclick = () => loadTrack(track, 'deckA');
    card.oncontextmenu = e => { e.preventDefault(); showMenu(e, track); };
  });
}

function loadTrack(track, target, mode = 'download') {
  setStatus(`${mode === 'stream' ? 'Streaming' : 'Loading'}: ${track.title}`);
  bridge.send('loadTrack', { videoId: track.id, title: track.title, target, mode });
}

// ===== Context menu =====
const ctx = $('#ctxmenu');
function showMenu(e, track) {
  e.preventDefault(); e.stopPropagation();
  const items = [
    { h: 'STREAM (INSTANT)' },
    { l: 'Stream to Deck A', f: () => loadTrack(track, 'deckA', 'stream') },
    { l: 'Stream to Deck B', f: () => loadTrack(track, 'deckB', 'stream') },
    { l: 'Stream to Automix', f: () => loadTrack(track, 'automix', 'stream') },
    { sep: 1 },
    { h: 'DOWNLOAD & LOAD' },
    { l: 'Load to Deck A', f: () => loadTrack(track, 'deckA') },
    { l: 'Load to Deck B', f: () => loadTrack(track, 'deckB') },
    { l: 'Add to Automix', f: () => loadTrack(track, 'automix') },
    { l: 'Add to Sidelist', f: () => loadTrack(track, 'sidelist') },
    { sep: 1 },
    { l: 'Download MP3', f: () => bridge.send('downloadTrack', { videoId: track.id, title: track.title, format: 'mp3' }) },
    { l: 'Download MP4', f: () => bridge.send('downloadTrack', { videoId: track.id, title: track.title, format: 'mp4' }) },
    { sep: 1 },
    { l: 'Open in Browser', f: () => bridge.send('openUrl', { url: track.url || ('https://www.youtube.com/watch?v=' + track.id) }) }
  ];
  ctx.innerHTML = items.map((it, i) =>
    it.sep ? '<div class="sep"></div>' :
    it.h ? `<div class="mh">${it.h}</div>` :
    `<div class="mi" data-i="${i}">${it.l}</div>`).join('');
  ctx.querySelectorAll('.mi').forEach(mi => {
    mi.onclick = () => { hideMenu(); items[+mi.dataset.i].f(); };
  });
  ctx.style.display = 'block';
  const mw = ctx.offsetWidth, mh = ctx.offsetHeight;
  ctx.style.left = Math.min(e.clientX, innerWidth - mw - 8) + 'px';
  ctx.style.top = Math.min(e.clientY, innerHeight - mh - 8) + 'px';
}
function hideMenu() { ctx.style.display = 'none'; }
document.addEventListener('click', hideMenu);
document.addEventListener('keydown', e => { if (e.key === 'Escape') hideMenu(); });

// ===== Playlists view =====
function renderPlaylists() {
  content.innerHTML = `
    <div class="plrow">
      <div class="searchwrap" style="flex:1">
        <span class="icon">&#128279;</span>
        <input type="text" id="pl-url" placeholder="Paste a YouTube playlist URL...">
      </div>
      <button class="btn primary" id="pl-import">IMPORT</button>
      ${state.playlist.length ? `
        <button class="btn" id="pl-automix">ADD ALL TO AUTOMIX</button>
        <button class="btn" id="pl-dlall">DOWNLOAD ALL</button>` : ''}
    </div>
    <div id="pl-list"></div>`;

  const listEl = $('#pl-list');
  if (state.playlist.length) {
    listEl.innerHTML = state.playlist.map(trackCard).join('');
    wireCards(state.playlist);
    $('#pl-automix').onclick = () => bridge.send('playlistBatch', { action: 'automix', tracks: state.playlist.map(t => ({ videoId: t.id, title: t.title })) });
    $('#pl-dlall').onclick = () => bridge.send('playlistBatch', { action: 'download', tracks: state.playlist.map(t => ({ videoId: t.id, title: t.title })) });
  } else {
    listEl.innerHTML = '<div class="empty-state" style="height:300px"><div class="big">&#128478;</div><div>Import a playlist to see its tracks here</div></div>';
  }

  $('#pl-import').onclick = importPlaylist;
  $('#pl-url').addEventListener('keydown', e => { if (e.key === 'Enter') importPlaylist(); });

  function importPlaylist() {
    const url = $('#pl-url').value.trim();
    if (!url) return;
    setStatus('Importing playlist...');
    listEl.innerHTML = '';
    renderSkeletonsIn(listEl, 6);
    bridge.send('importPlaylist', { url });
  }
}

function renderSkeletonsIn(el, n) {
  el.innerHTML = Array(n).fill('<div class="skeleton"></div>').join('');
}

// ===== Trending / Charts =====
function loadTrending() {
  setStatus('Loading trending...');
  bridge.send('getTrending');
}

function renderCharts() {
  content.innerHTML = `
    <div class="chips">${GENRES.map(g =>
      `<button class="gchip ${state.genre === g ? 'active' : ''}" data-g="${g}">${g.toUpperCase()}</button>`).join('')}
    </div>
    <div id="chart-list"></div>`;
  content.querySelectorAll('.gchip').forEach(c => {
    c.onclick = () => { state.genre = c.dataset.g; state.charts = []; renderCharts(); loadCharts(c.dataset.g); };
  });
  const listEl = $('#chart-list');
  if (state.charts.length) {
    listEl.innerHTML = state.charts.map(trackCard).join('');
    wireCards(state.charts);
  } else {
    renderSkeletonsIn(listEl, 6);
  }
}

function loadCharts(genre) {
  setStatus('Loading charts: ' + genre);
  bridge.send('getCharts', { genre });
}

// ===== Downloads (history tab) =====
function renderHistory() {
  if (!state.downloadHistory.length) {
    content.innerHTML = '<div class="empty-state"><div class="big">&#128190;</div><div>Your downloaded tracks will appear here</div></div>';
    return;
  }
  content.innerHTML = state.downloadHistory.map((d, i) => `
    <div class="hrow dlrow" data-i="${i}">
      <span>${d.format === 'mp4' ? '&#127916;' : '&#127925;'}</span>
      <span class="q">${esc(d.title)}</span>
      <span class="fmt">${esc((d.format || '').toUpperCase())}</span>
      <span class="d">${esc(d.date || '')}</span>
    </div>`).join('');
  content.querySelectorAll('.dlrow').forEach(row => {
    row.onclick = () => {
      const d = state.downloadHistory[+row.dataset.i];
      bridge.send('loadTrack', { videoId: d.videoId, title: d.title, target: 'deckA', mode: 'download' });
      toast('Loading "' + d.title + '" to Deck A', 'ok');
    };
  });
}

// ===== License view =====
function renderLicense() {
  const lic = state.license || {};
  const ok = state.licensed;
  const days = lic.daysRemaining;
  const ringCls = ok ? (days !== undefined && days <= 7 ? 'warn' : '') : 'bad';
  content.innerHTML = `
  <div class="lic-wrap">
    <div class="lic-card">
      <h3>LICENSE STATUS</h3>
      <div class="lic-status">
        <div class="lic-ring ${ringCls}">${ok ? '&#10003;' : '&#10007;'}</div>
        <div>
          <div class="lic-big">${ok ? 'License VALID' : (lic.statusText || 'Not Activated')}</div>
          <div class="lic-sub">${ok ? `Expires: ${esc(lic.expiry || 'never')}${days !== undefined ? ` &middot; ${days} days remaining` : ''}` : 'Log in with your marketplace account below'}</div>
        </div>
      </div>
      <div style="margin-top:16px">
        <div class="kv"><span class="k">Machine ID</span><span class="v">${esc(lic.machineId || '-')} <button class="btn small" id="lic-copy">COPY</button></span></div>
        <div class="kv"><span class="k">License Key</span><span class="v">${esc(lic.key || '-')}</span></div>
        <div class="kv"><span class="k">Type</span><span class="v">${esc(lic.type || '-')}</span></div>
        ${lic.account ? `<div class="kv"><span class="k">Account</span><span class="v">${esc(lic.account)}</span></div>` : ''}
        ${lic.activations ? `<div class="kv"><span class="k">Activations</span><span class="v">${esc(lic.activations)}</span></div>` : ''}
      </div>
    </div>

    <div class="lic-card">
      <h3>MARKETPLACE ACCOUNT</h3>
      ${lic.loggedIn ? `
      <div class="lic-status" style="margin-bottom:4px">
        <div class="lic-ring" style="width:48px;height:48px;font-size:18px">&#128100;</div>
        <div>
          <div class="lic-big" style="font-size:16px">${esc(lic.accountName || lic.account)}</div>
          <div class="lic-sub">Logged in${lic.accountName ? ' as ' + esc(lic.account) : ''}</div>
        </div>
      </div>
      <div style="margin-top:14px; display:flex; gap:10px">
        <button class="btn small" id="lic-dashboard">OPEN DASHBOARD</button>
        <button class="btn small" id="lic-logout">LOG OUT</button>
      </div>
      ` : `
      <div class="lic-field"><input type="email" id="lic-email" placeholder="Email" style="font-family:inherit" value="${esc(lic.account || '')}"></div>
      <div class="lic-field">
        <input type="password" id="lic-pass" placeholder="Password" style="font-family:inherit">
        <button class="btn primary" id="lic-login">LOG IN</button>
      </div>
      <div style="margin-top:12px; display:flex; gap:10px">
        <button class="btn small" id="lic-dashboard">OPEN DASHBOARD</button>
      </div>
      `}
    </div>
  </div>`;

  $('#lic-copy').onclick = () => { navigator.clipboard.writeText(lic.machineId || ''); toast('Machine ID copied', 'ok'); };
  const loginBtn = $('#lic-login');
  if (loginBtn) {
    const submitLogin = () => {
      const email = $('#lic-email').value.trim();
      const password = $('#lic-pass').value;
      if (!email || !password) return;
      setStatus('Logging in...');
      bridge.send('license.login', { email, password, savePassword: true });
    };
    loginBtn.onclick = submitLogin;
    $('#lic-pass').addEventListener('keydown', e => { if (e.key === 'Enter') submitLogin(); });
  }
  $('#lic-dashboard').onclick = () => bridge.send('license.openDashboard');
  const lo = $('#lic-logout');
  if (lo) lo.onclick = () => bridge.send('license.logout');
}

// ===== Status / progress / toasts =====
function setStatus(text) { statusText.textContent = text; }

function renderProgress() {
  const area = $('#progress-area');
  const active = Object.values(state.downloads).filter(d => d.active);
  area.innerHTML = active.map(d => `
    <div>
      <div class="prog-label">
        <span>${esc(d.title)} &middot; ${esc(d.phase)}</span>
        <span>${esc(d.speed || '')} ${d.eta ? 'ETA ' + esc(d.eta) : ''} ${Math.round(d.percent)}%</span>
      </div>
      <div class="prog-track"><div class="prog-fill" style="width:${d.percent}%"></div></div>
    </div>`).join('');
}

function toast(msg, cls = '') {
  const el = document.createElement('div');
  el.className = 'toast ' + cls;
  el.textContent = msg;
  $('#toasts').appendChild(el);
  setTimeout(() => el.remove(), 4000);
}

// ===== Incoming messages from C++ =====
function handleHostMessage(m) {
    if (!m || !m.type) return;
    switch (m.type) {
      case 'init':
        $('#version').textContent = 'v' + (m.payload.version || '');
        setFormat(m.payload.format || 'mp3');
        break;

      case 'searchResults':
        state.searching = false;
        state.results = m.payload.results || [];
        setStatus(state.results.length + ' results');
        if (state.view === 'search') render();
        break;

      case 'playlistResults':
        state.playlist = m.payload.results || [];
        setStatus('Playlist: ' + state.playlist.length + ' tracks');
        if (state.view === 'playlists') render();
        break;

      case 'trendingResults':
        state.trending = m.payload.results || [];
        setStatus('Trending: ' + state.trending.length + ' tracks');
        if (state.view === 'trending') render();
        break;

      case 'chartResults':
        state.charts = m.payload.results || [];
        setStatus('Charts: ' + state.charts.length + ' tracks');
        if (state.view === 'charts') render();
        break;

      case 'historyResults':
        state.history = m.payload.results || [];
        if (state.view === 'history') render();
        // Always refresh dropdown content so it's ready when user clicks search box
        const typed = $('#search-input').value.trim().toLowerCase();
        const items = state.history.filter(h => !typed || h.query.toLowerCase().includes(typed));
        if (items.length) {
          searchDropdown.innerHTML = items.map((h, i) => `
            <div class="sd-item" data-q="${esc(h.query)}">
              <span class="sd-icon">&#128336;</span>
              <span class="sd-q">${esc(h.query)}</span>
              <span class="sd-d">${esc(h.date || '')}</span>
            </div>`).join('');
          searchDropdown.querySelectorAll('.sd-item').forEach(item => {
            item.onmousedown = e => {
              e.preventDefault();
              $('#search-input').value = item.dataset.q;
              hideSearchDropdown();
              doSearch();
            };
          });
        }
        break;

      case 'downloadHistoryResults':
        state.downloadHistory = m.payload.results || [];
        if (state.view === 'history') render();
        break;

      case 'downloadProgress': {
        const p = m.payload;
        state.downloads[p.id || p.title] = p;
        if (!p.active) setTimeout(() => { delete state.downloads[p.id || p.title]; renderProgress(); }, 2500);
        renderProgress();
        if (p.phase === 'Complete') { toast('Downloaded: ' + p.title, 'ok'); refreshCachedFlags(p.id, p.format); }
        if (p.phase === 'Error') toast('Download failed: ' + p.title, 'err');
        break;
      }

      case 'licenseStatus': {
        state.licensed = !!m.payload.licensed;
        state.license = m.payload;
        const badge = $('#license-badge');
        badge.textContent = state.licensed ? 'PRO' : 'UNLICENSED';
        badge.className = 'badge ' + (state.licensed ? 'pro' : 'trial');
        $('#machine-id').textContent = m.payload.machineId ? 'Machine ID: ' + m.payload.machineId : '';
        if (state.view === 'license') render();
        break;
      }

      case 'status':
        setStatus(m.payload.text || '');
        break;

      case 'toast':
        toast(m.payload.text || '', m.payload.kind || '');
        break;

      case 'error':
        state.searching = false;
        toast(m.payload.text || 'An error occurred', 'err');
        setStatus('Error');
        if (state.view === 'search') render();
        break;
    }
}

if (window.chrome && window.chrome.webview) {
  window.chrome.webview.addEventListener('message', e => handleHostMessage(e.data));
}

function refreshCachedFlags(id, format) {
  [state.results, state.playlist, state.trending, state.charts].forEach(list => {
    const t = list.find(x => x.id === id);
    if (t) { if (format === 'mp4') t.cachedMp4 = true; else t.cachedMp3 = true; }
  });
  render();
}

// Boot
bridge.send('uiReady');
render();
