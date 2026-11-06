// ============================================================================
// pagina.h — webpagina's van ESP32_PWM_Test (hoort bij v1.5)
// Apart bestand: de Arduino IDE zet anders automatisch C++-vooraankondigingen
// ("#line …" + "function x();") midden in het JavaScript, wat de pagina breekt.
// De IDE bewerkt alleen .ino-bestanden, .h-bestanden laat ze ongemoeid.
//
// PAGINA_STATUS   → /status   (bediening: knoppen, schuifbalk, frequentie)
// PAGINA_SETTINGS → /settings (netwerk, opstartstand, kalibratie, systeem, OTA)
// PAGINA_OTA      → /update   (firmware uploaden met voortgangsbalk)
// ============================================================================
#pragma once
#include <Arduino.h>

// ── Gedeelde opmaak ─────────────────────────────────────────────────────────
#define PAGINA_CSS \
"body{font-family:Arial,sans-serif;margin:0;background:#fff}" \
".hdr{background:#ffcc00;padding:12px 16px;font-weight:bold;font-size:18px}" \
".box{padding:16px;max-width:520px}" \
".sub{color:#666;font-size:15px}" \
".knop{display:inline-block;background:#336699;color:#fff;border:0;border-radius:6px;padding:12px 0;width:18%;margin:1%;font-size:15px;font-weight:bold}" \
".knop.f{width:31%}.knop.actief{background:#c00}.knop.b{width:100%;margin:8px 0}.knop.r{background:#c00}" \
"h3{color:#336699;margin:22px 0 6px}" \
".lnk{color:#336699;font-size:14px;text-decoration:underline;cursor:pointer}"

// ── /status ─────────────────────────────────────────────────────────────────
const char PAGINA_STATUS[] PROGMEM = R"HTML(<!DOCTYPE html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Ventilatietest</title><style>)HTML" PAGINA_CSS R"HTML(
.rij{display:flex;align-items:center;gap:14px;margin:18px 0}
.groot{font-size:44px;font-weight:bold;color:#336699;min-width:110px;text-align:right}
.groot.vol{color:#c00}
input[type=range]{flex:1;height:36px}
.voet{margin-top:28px;text-align:right}
</style></head><body>
<div class="hdr">Ventilatietest</div><div class="box">
<div><button class="knop" onclick="zet(3)">3%</button><button class="knop" onclick="zet(25)">25%</button><button class="knop" onclick="zet(50)">50%</button><button class="knop" onclick="zet(75)">75%</button><button class="knop" onclick="zet(100)">100%</button></div>
<div class="rij"><div class="groot" id="v">…</div><input type="range" id="s" min="0" max="100" step="1" value="0"></div>
<h3>PWM-frequentie</h3>
<div><button class="knop f" id="f1000" onclick="fq(1000)">1 kHz</button><button class="knop f" id="f5000" onclick="fq(5000)">5 kHz</button><button class="knop f" id="f20000" onclick="fq(20000)">20 kHz</button></div>
<div class="voet"><a class="lnk" href="/settings">⚙ Instellingen</a></div>
</div>
<script>
const $=i=>document.getElementById(i),s=$('s');let t=null;
function toon(j){$('v').textContent=j.vol?'VOL':Math.round(j.p)+'%';$('v').classList.toggle('vol',j.vol);
 if(document.activeElement!==s)s.value=j.vol?0:Math.round(j.p);
 [1000,5000,20000].forEach(f=>$('f'+f).classList.toggle('actief',f==j.f));}
function stuurP(p){fetch('/setp?p='+p).then(r=>r.json()).then(toon).catch(()=>{});}
function zet(p){$('v').textContent=p+'%';stuurP(p);}
s.addEventListener('input',()=>{$('v').textContent=s.value+'%';$('v').classList.remove('vol');clearTimeout(t);t=setTimeout(()=>stuurP(s.value),80);});
function fq(f){fetch('/freq?f='+f).then(r=>r.json()).then(toon).catch(()=>{});}
function ververs(){fetch('/json').then(r=>r.json()).then(toon).catch(()=>{});}
document.addEventListener('DOMContentLoaded',()=>{ververs();setInterval(ververs,5000);});
</script></body></html>)HTML";

// ── /settings ───────────────────────────────────────────────────────────────
const char PAGINA_SETTINGS[] PROGMEM = R"HTML(<!DOCTYPE html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Ventilatietest — instellingen</title><style>)HTML" PAGINA_CSS R"HTML(
.net{background:#e6f0ff;border-radius:6px;padding:10px 12px;font-size:15px}
.bars{display:inline-flex;align-items:flex-end;gap:2px;height:16px;margin-left:8px;vertical-align:middle}
.bars i{display:block;width:5px;background:#ccc}.bars i.on{background:#336699}
.rij{display:flex;align-items:center;justify-content:space-between;padding:10px 8px;border-bottom:1px solid #ddd;font-size:15px;cursor:pointer}
.rij:hover{background:#f3f7ff}.rij .r{color:#666;font-size:13px;white-space:nowrap}
.mini{background:#c00;color:#fff;border:0;border-radius:4px;padding:6px 10px;font-size:13px}
input[type=password]{font-size:16px;padding:10px;width:100%;box-sizing:border-box;margin:6px 0}
input[type=number]{font-size:16px;padding:8px;width:110px;margin:4px 8px 4px 0}
.mld{margin-top:8px;font-size:14px;font-weight:bold}
.ok{color:#0a0;font-size:13px;white-space:nowrap}
table{border-collapse:collapse;width:100%;font-size:15px}td{padding:6px 4px;border-bottom:1px solid #eee}td:first-child{color:#666}
</style></head><body>
<div class="hdr">Ventilatietest — instellingen</div><div class="box">
<a class="lnk" href="/status">← Ventilatietest</a>

<h3>Netwerk</h3>
<div class="net"><b id="ssid">…</b><span class="bars" id="bars"><i style="height:25%"></i><i style="height:50%"></i><i style="height:75%"></i><i style="height:100%"></i></span>
<div class="sub" id="sterkte"></div></div>
<div class="mld" id="mld"></div>
<button class="knop b" onclick="zoek()">Zoek netwerken</button>
<div id="lijst"></div>
<div id="form" style="display:none"><div class="sub">Wachtwoord voor <b id="fssid"></b>:</div>
<input type="password" id="pw" placeholder="wachtwoord (leeg bij open netwerk)">
<button class="knop b" onclick="verbind()">Verbind &amp; bewaar</button></div>
<div id="opnieuw" style="display:none" class="sub"><span class="lnk" onclick="anderWw()">ander wachtwoord</span> voor <b id="ossid"></b></div>
<h3>Bewaarde netwerken</h3><div id="bewaard" class="sub"></div>

<h3>Opstartstand</h3>
<div class="sub">Bij het opstarten, ook zonder wifi: <b id="st">…</b></div>
<button class="knop b" onclick="bewaarStart()">Huidige stand als opstartstand bewaren</button>
<div class="mld" id="stm"></div>

<h3>Kalibratie</h3>
<div class="sub">Vmax = spanning op ANA-OUT bij 100 %, met de unit aangesloten. k = kromming van de AnaBoX (gemeten: 2).</div>
<div><label class="sub">Vmax (V) <input type="number" id="vmax" step="0.01" min="5" max="12"></label>
<label class="sub">k <input type="number" id="kk" step="0.05" min="0.5" max="5"></label></div>
<button class="knop b" onclick="cfg()">Kalibratie opslaan</button>

<h3>Systeem</h3>
<table>
<tr><td>Firmware</td><td id="ver">…</td></tr>
<tr><td>Free memory</td><td id="mem">…</td></tr>
<tr><td>On-line</td><td id="up">…</td></tr>
<tr><td>Laatste herstart</td><td id="rst">…</td></tr>
<tr><td>Uitgang nu</td><td id="uit">…</td></tr>
</table>
<button class="knop b" onclick="location.href='/update'">⬆ OTA update</button>
<button class="knop b r" onclick="if(confirm('Herstarten?'))fetch('/reboot')">⟳ Herstarten</button>
</div>
<script>
const $=i=>document.getElementById(i);let keuze='',scanMld='',poging=0,bewaard=[],nuOp='';
function oordeel(r){return r>=-55?['uitstekend',4]:r>=-67?['goed',3]:r>=-75?['matig',2]:['zwak',1];}
function balk(r){const n=oordeel(r)[1];let h='<span class="bars">';for(let i=0;i<4;i++)h+='<i style="height:'+(25*(i+1))+'%" class="'+(i<n?'on':'')+'"></i>';return h+'</span>';}
function esc(x){return String(x).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}
function st(j){
 $('ssid').textContent=j.sta?j.ssid:'Geen WiFi';
 const b=document.querySelectorAll('#bars i'),n=j.sta?oordeel(j.rssi)[1]:0;b.forEach((e,i)=>e.classList.toggle('on',i<n));
 $('sterkte').textContent=(j.sta?j.rssi+' dBm · '+oordeel(j.rssi)[0]+' · http://'+j.ip:'')+(j.ap?(j.sta?' · ':'')+'eigen netwerk PWM-Test actief (http://192.168.4.1)':'');
 $('mld').textContent=j.bezig?'Verbinden… even geduld (max. 15 s)':(scanMld||j.melding);
 bewaard=j.bewaard;nuOp=j.sta?j.ssid:'';
 $('bewaard').innerHTML=j.bewaard.length?j.bewaard.map((x,i)=>'<div class="rij"><span onclick="gebruik('+i+')" style="flex:1">'+esc(x)+'</span>'+(x==nuOp?'<span class="ok">&#9679; verbonden&nbsp;</span>':'')+'<button class="mini" onclick="vergeet('+i+')">Vergeet</button></div>').join(''):'(nog geen)';
 $('st').textContent=(j.stvol?'VOLLE KRACHT':Math.round(j.stp)+' %')+' (PWM '+j.st+')';
 if(document.activeElement!==$('vmax'))$('vmax').value=j.vmax;
 if(document.activeElement!==$('kk'))$('kk').value=j.k;
 $('ver').textContent='v'+j.ver;
 $('mem').textContent=j.heap+' kB heap – '+j.heapp+' % (grootste blok '+j.blok+' kB)';
 $('up').textContent=j.up+' s';
 $('rst').textContent=j.rst;
 $('uit').textContent=(j.vol?'VOLLE KRACHT':Math.round(j.p)+' %')+' · ≈ '+j.u.toFixed(2)+' V · PWM '+j.v+'/255 · '+(j.f/1000)+' kHz';}
function ververs(){fetch('/json').then(r=>r.json()).then(st).catch(()=>{});}
function bewaarStart(){fetch('/savestart').then(r=>r.json()).then(j=>{st(j);$('stm').textContent='✔ Opstartstand bewaard';}).catch(()=>{$('stm').textContent='Bewaren mislukt';});}
function cfg(){const v=$('vmax').value.replace(',','.'),k=$('kk').value.replace(',','.');
 fetch('/cfg?vmax='+encodeURIComponent(v)+'&k='+encodeURIComponent(k)).then(r=>r.json()).then(st).catch(()=>{});}
function lijst(j,extra){
 if(!j.netten.length){$('lijst').innerHTML='<div class="sub">Geen netwerken gevonden.'+(extra||'')+'</div>';return;}
 $('lijst').innerHTML=(extra?'<div class="sub">'+extra+'</div>':'')+j.netten.map(n=>'<div class="rij" data-s="'+esc(n.s)+'" data-o="'+n.o+'" onclick="kies(this)"><span>'+(n.o?'':'&#128274; ')+esc(n.s)+(bewaard.includes(n.s)?' <span class="ok">&#10003; bewaard</span>':'')+'</span><span class="r">'+n.r+' dBm'+balk(n.r)+'</span></div>').join('');}
function zoek(){$('form').style.display='none';scanMld='Zoeken… (±5 s — de verbinding kan even haperen)';$('mld').textContent=scanMld;poging=0;
 fetch('/scanres').then(r=>r.json()).then(j=>{if(j.netten.length)lijst(j,'Vorige zoekopdracht ('+j.leeftijd+' s geleden) — nieuwe zoekopdracht loopt…');}).catch(()=>{});
 setTimeout(nieuw,300);}
function nieuw(){fetch('/scan').then(r=>r.json()).then(j=>{scanMld=j.ok?'':'Opnieuw zoeken mislukt — vorige lijst getoond.';$('mld').textContent=scanMld;lijst(j);})
 .catch(()=>{if(++poging<=5){scanMld='Geen antwoord van het bord — opnieuw proberen ('+poging+'/5)…';$('mld').textContent=scanMld;setTimeout(cache,3000);}
 else{scanMld='Het bord antwoordt niet. Verbind opnieuw met PWM-Test (of je WiFi) en ververs de pagina.';$('mld').textContent=scanMld;}});}
function cache(){fetch('/scanres').then(r=>r.json()).then(j=>{scanMld='';$('mld').textContent='';lijst(j);})
 .catch(()=>{if(++poging<=5){scanMld='Geen antwoord van het bord — opnieuw proberen ('+poging+'/5)…';$('mld').textContent=scanMld;setTimeout(cache,3000);}
 else{scanMld='Het bord antwoordt niet. Verbind opnieuw met PWM-Test (of je WiFi) en ververs de pagina.';$('mld').textContent=scanMld;}});}
function kies(e){keuze=e.dataset.s;$('opnieuw').style.display='none';
 if(bewaard.includes(keuze)){$('form').style.display='none';$('ossid').textContent=keuze;$('opnieuw').style.display='block';stuurVerbind(keuze,null);return;}
 toonForm(e.dataset.o=='1');}
function toonForm(open){$('fssid').textContent=keuze;$('pw').value='';$('form').style.display='block';
 $('pw').style.display=open?'none':'block';if(!open)$('pw').focus();}
function anderWw(){$('opnieuw').style.display='none';toonForm(false);}
function gebruik(i){const x=bewaard[i];if(!x||x==nuOp)return;keuze=x;$('form').style.display='none';$('ossid').textContent=x;$('opnieuw').style.display='block';stuurVerbind(x,null);}
function stuurVerbind(ssid,pw){const b=new URLSearchParams();b.append('ssid',ssid);if(pw===null)b.append('bewaard','1');else b.append('pass',pw);
 fetch('/connect',{method:'POST',body:b}).then(r=>{if(!r.ok)throw 0;$('form').style.display='none';$('lijst').innerHTML='';
 scanMld='';$('mld').textContent='Verbinden met '+ssid+'… Zit je gsm op een ander netwerk, schakel dan over naar '+ssid+' en open http://pwmtest.local';})
 .catch(()=>{$('mld').textContent='Verbinden met '+ssid+' kon niet starten (al bezig?) — probeer opnieuw.';});}
function verbind(){if(!keuze)return;$('opnieuw').style.display='none';stuurVerbind(keuze,$('pw').value);}
function vergeet(i){if(confirm('Dit netwerk vergeten?'))fetch('/forget?i='+i).then(ververs).catch(()=>{});}
document.addEventListener('DOMContentLoaded',()=>{ververs();setInterval(ververs,5000);});
</script></body></html>)HTML";

// ── /update ─────────────────────────────────────────────────────────────────
const char PAGINA_OTA[] PROGMEM = R"HTML(<!DOCTYPE html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Ventilatietest — OTA</title><style>)HTML" PAGINA_CSS R"HTML(
progress{width:100%;height:26px;margin:12px 0}
#s{font-weight:bold;color:#336699;margin-top:6px}
</style></head><body>
<div class="hdr">Ventilatietest — OTA update</div><div class="box">
<a class="lnk" href="/settings">← Instellingen</a>
<p class="sub">Arduino IDE: <b>Sketch → Export Compiled Binary</b>. Kies daarna enkel <b>ESP32_PWM_Test.ino.bin</b>
(niet de .bootloader-, .partitions- of .merged-bestanden).</p>
<input type="file" id="f" accept=".bin">
<button class="knop b" onclick="up()">⬆ Uploaden</button>
<progress id="p" max="100" value="0"></progress>
<div id="s"></div>
</div>
<script>
function up(){var f=document.getElementById('f').files[0];if(!f)return;
 var s=document.getElementById('s'),p=document.getElementById('p'),d=new FormData();d.append('update',f);
 var x=new XMLHttpRequest();x.open('POST','/update');
 x.upload.onprogress=function(e){if(e.lengthComputable){var v=Math.round(e.loaded*100/e.total);p.value=v;s.textContent='Uploaden… '+v+' %';}};
 x.onload=function(){if(x.responseText=='OK'){p.value=100;s.textContent='✔ Gelukt — herstart… (±15 s)';setTimeout(function(){location.href='/status';},15000);}
  else s.textContent='✘ Mislukt: '+x.responseText;};
 x.onerror=function(){s.textContent='✘ Verbinding verbroken';};x.send(d);}
</script></body></html>)HTML";
