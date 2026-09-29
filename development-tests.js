(function(){
var table=document.getElementById('test-targets');
if(!table||!table.tBodies[0])return;
var rows=[].slice.call(table.tBodies[0].rows);
var query=document.getElementById('test-query');
var area=document.getElementById('test-area');
var reset=document.getElementById('test-reset');
var count=document.getElementById('test-count');
var areas=[];
rows.forEach(function(row){var value=row.cells[0].textContent.trim();if(areas.indexOf(value)<0)areas.push(value)});
areas.sort().forEach(function(value){var option=document.createElement('option');option.value=value;option.textContent=value;area.appendChild(option)});
function normalize(value){return (value||'').normalize('NFKC').toLowerCase()}
function update(){
  var q=normalize(query.value).trim(),selected=area.value,files=0,cases=0;
  rows.forEach(function(row){
    var visible=(!selected||row.cells[0].textContent.trim()===selected)&&(!q||normalize(row.textContent).indexOf(q)>=0);
    row.hidden=!visible;
    if(visible){files++;cases+=Number(row.cells[3].textContent)||0}
  });
  count.textContent=files+'ファイル・'+cases+'ケースを表示';
}
query.addEventListener('input',update);area.addEventListener('change',update);
reset.addEventListener('click',function(){query.value='';area.value='';update();query.focus()});
})();
