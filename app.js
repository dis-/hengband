(function(){
var input=document.getElementById('search'),box=document.getElementById('results'),idx=[],shown=[],active=-1;
function esc(s){return (s||'').replace(/[&<>]/g,function(c){return {'&':'&amp;','<':'&lt;','>':'&gt;'}[c]})}
// Fold kana only for matching; keep the original labels and URLs for display.
function searchKey(s){return (s||'').normalize('NFKC').toLowerCase().replace(/[ァ-ヶヽヾ]/g,function(c){return String.fromCharCode(c.charCodeAt(0)-0x60)})}
if(input){
  fetch('search-index.json').then(function(r){return r.json()}).then(function(d){idx=d.map(function(e){return {entry:e,ja:searchKey(e.ja),en:searchKey(e.en)}});run()}).catch(function(){});
  var t;
  input.addEventListener('input',function(){clearTimeout(t);t=setTimeout(run,110)});
  function run(){
    var q=searchKey(input.value).trim();active=-1;
    if(!q){box.className='results';box.innerHTML='';return;}
    var out=[];
    for(var i=0;i<idx.length&&out.length<40;i++){var e=idx[i];
      if(e.ja.indexOf(q)>=0||e.en.indexOf(q)>=0)out.push(e.entry);}
    shown=out;
    box.innerHTML=out.map(function(e,i){return '<a href="'+e.url+'" data-i="'+i+'"><span>'+esc(e.ja)+'</span> <span class="en">'+esc(e.en)+'</span><span class="tag">'+esc(e.t)+'</span></a>'}).join('');
    box.className=out.length?'results open':'results';
  }
  input.addEventListener('keydown',function(ev){
    var links=box.querySelectorAll('a');
    if(ev.key==='ArrowDown'){active=Math.min(active+1,links.length-1);mark(links);ev.preventDefault()}
    else if(ev.key==='ArrowUp'){active=Math.max(active-1,-1);mark(links);ev.preventDefault()}
    else if(ev.key==='Enter'){var a=active>=0?links[active]:links[0];if(a)location.href=a.getAttribute('href')}
    else if(ev.key==='Escape'){box.className='results';input.blur()}
  });
  function mark(links){for(var i=0;i<links.length;i++)links[i].classList.toggle('active',i===active)}
  document.addEventListener('click',function(e){if(!box.contains(e.target)&&e.target!==input)box.className='results'});
}
function parseNum(s){s=(s||'').trim();if(!/^[-+]?[.\d]/.test(s))return null;var m=s.replace(/,/g,'').match(/^[-+]?\d+(\.\d+)?/);return m?parseFloat(m[0]):null}
var tables=document.querySelectorAll('table.sortable');
for(var k=0;k<tables.length;k++)(function(tb){
  if(!tb.tHead||!tb.tBodies[0])return;
  var ths=tb.tHead.rows[0].cells;
  for(var c=0;c<ths.length;c++)(function(th,ci){
    th.addEventListener('click',function(){
      var body=tb.tBodies[0],rows=[].slice.call(body.rows),asc=!th.classList.contains('asc');
      for(var j=0;j<ths.length;j++){ths[j].classList.remove('asc','desc')}
      th.classList.add(asc?'asc':'desc');
      rows.sort(function(a,b){
        var x=(a.cells[ci]?a.cells[ci].textContent:'').trim(),y=(b.cells[ci]?b.cells[ci].textContent:'').trim();
        var nx=parseNum(x),ny=parseNum(y);
        if(nx!==null&&ny!==null)return asc?nx-ny:ny-nx;
        return asc?x.localeCompare(y,'ja'):y.localeCompare(x,'ja');
      });
      for(var r=0;r<rows.length;r++)body.appendChild(rows[r]);
    });
  })(ths[c],c);
})(tables[k]);
function setupSidebarTree(){
  var current=(location.pathname.split('/').pop()||'index.html')+location.hash;
  var pathOnly=current.split('#')[0];
  var groups=document.querySelectorAll('.sidebar li');
  for(var i=0;i<groups.length;i++){
    var li=groups[i], sub=null, topLink=null;
    for(var c=0;c<li.children.length;c++){
      if(li.children[c].tagName==='A')topLink=li.children[c];
      if(li.children[c].tagName==='UL'){sub=li.children[c];break;}
    }
    if(!sub)continue;
    var href=topLink?topLink.getAttribute('href')||'':'';
    if(!(href.endsWith('Ironman.html')||href.endsWith('Analysis.html')||href.endsWith('Classes.html')))continue;
    var details=document.createElement('details'),summary=document.createElement('summary');
    details.className='side-group';
    while(li.firstChild&&li.firstChild!==sub)summary.appendChild(li.firstChild);
    details.appendChild(summary);
    details.appendChild(sub);
    li.appendChild(details);
    var active=sub.querySelector('a[href="'+pathOnly+'"],a[href="'+current+'"]')||summary.querySelector('a[href="'+pathOnly+'"],a[href="'+current+'"]');
    if(active)details.open=true;
  }
}
setupSidebarTree();
})();
