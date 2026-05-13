#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <uri/UriBraces.h>

// const char *ssid = "Wokwi-GUEST";
// const char *password = "";
// const char *ssid = "Sala Maker";
// const char *password = "Maker@fatec";
#define ssid "Wokwi-GUEST"
#define password ""
#define channel 6

const char *apSsid = "Farol-Aeroporto";
const char *apPassword = "12345678";
const char *mdnsName = "farol-aeroporto";

WebServer server(80);

const int LED_PIN_FAROL = 13;
bool farolStatus = false; //estado do farol, inicialmente desligado
bool manualOverride = false; //indica se o override manual está ativo, para evitar que a lógica automática interfira durante o override

// ─────────────────────────────────────────────────────────────
//  Marcadores substituídos em sendHtml():
//    %%FAROL_STATUS%%   → "LIGADO" | "DESLIGADO"
//    %%FAROL_CLASS%%    → "text-success" | "text-danger"
//    %%BTN_LABEL%%      → "DESLIGAR" | "LIGAR"
//    %%BTN_CMD%%        → "off" | "on"
//    %%BTN_CLS%%        → "btn-outline-danger" | "btn-outline-success"
//    %%LED_CLS%%        → "led-on" | "led-off"
//    %%OVERRIDE_BADGE%% → badge HTML se override ativo, ou ""
// ─────────────────────────────────────────────────────────────

// Página HTML estilizada
void sendHtml()
{
  String Htmlresponse = R"RAW(
<!DOCTYPE html>
<html lang="pt-BR" data-bs-theme="dark">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>ESP32 AV-CTL</title>

<link href="https://cdnjs.cloudflare.com/ajax/libs/bootstrap/5.3.3/css/bootstrap.min.css" rel="stylesheet"/>
 
<style>
body{background:#0b0e13;font-family:'Barlow Condensed',sans-serif}
.font-mono{font-family:'Share Tech Mono',monospace}
.bg-panel{background:#111620!important}
.b-dark{border-color:#1e2a3a!important}
.text-ax{color:#00c8ff!important}
.text-sim{color:#ff6b35!important}
.text-dim{color:#3a4a5c!important}
.card-stripe-r::before,.card-stripe-s::before,.card-stripe-f::before{content:'';position:absolute;top:0;left:0;right:0;height:2px}
.card-stripe-r::before{background:#00c8ff;opacity:.6}
.card-stripe-s::before{background:#ff6b35;opacity:.6}
.card-stripe-f::before{background:#ffd600;opacity:.7}
.led{width:12px;height:12px;border-radius:50%;display:inline-block;flex-shrink:0}
.led-on{background:#00e676;box-shadow:0 0 10px #00e676}
.led-off{background:#3a4a5c}
.form-control,.form-control:focus{background:#0d1219;border-color:#1e2a3a;color:#cdd8e3;box-shadow:none}
.inp-sim{color:#ff6b35!important;font-family:'Share Tech Mono',monospace}
.inp-icao{color:#00c8ff;text-transform:uppercase;letter-spacing:.2em;text-align:center}
@keyframes bk{0%,100%{opacity:1}50%{opacity:0}}
.dot{display:inline-block;width:6px;height:6px;border-radius:50%;background:#00c8ff;vertical-align:middle;margin-right:4px;animation:bk 1.5s infinite}
@keyframes pulse{0%,100%{box-shadow:0 0 6px #00e676}50%{box-shadow:0 0 18px #00e676}}
.led-pulse{animation:pulse 1.2s ease-in-out infinite}
.log-e{font-family:'Share Tech Mono',monospace;font-size:.72rem;line-height:1.8;display:flex;gap:8px}
.log-ts{color:#3a4a5c;white-space:nowrap}
.log-on .log-m{color:#00e676}
.log-off .log-m{color:#ff3d3d}
.log-info .log-m{color:#ffab00}
.log-m{color:#cdd8e3}
input[type=number]::-webkit-inner-spin-button{display:none}
</style>
</head>
<body>

  <!-- NAVBAR -->
  <nav class="navbar bg-panel border-bottom b-dark px-3 py-2">
    <span class="font-mono text-ax fw-bold me-auto" style="font-size:1.1rem;letter-spacing:.2em">
      ESP32 <span class="text-sim">//</span> Controle-Farol
    </span>
    <div class="d-flex align-items-center gap-3">
      <span id="condPill" class="badge border font-mono" style="font-size:.68rem;letter-spacing:.12em"></span>
      <div class="btn-group btn-group-sm">
        <button id="btnReal" class="btn btn-outline-info font-mono" style="font-size:.7rem;letter-spacing:.1em" onclick="setMode('real')">REAL</button>
        <button id="btnSim" class="btn btn-outline-warning font-mono" style="font-size:.7rem;letter-spacing:.1em" onclick="setMode('sim')">SIM</button>
      </div>
      <span id="clockEl" class="font-mono text-dim d-none d-sm-inline" style="font-size:.65rem;letter-spacing:.1em"></span>
    </div>
  </nav>

<!-- MAIN -->
<div class="container-fluid px-3 py-3 d-flex flex-column gap-3">

  <!-- ICAO -->
  <div class="bg-panel border b-dark rounded p-2 d-flex flex-wrap align-items-center gap-2">
    <label class="font-mono text-dim mb-0" style="font-size:.7rem;letter-spacing:.14em">ICAO</label>
    <input id="icaoInput" type="text" maxlength="4" placeholder="SBGR" value="SBGR"
           class="form-control form-control-sm inp-icao font-mono" style="width:88px"/>
    <button class="btn btn-sm btn-outline-info font-mono" style="font-size:.68rem;letter-spacing:.1em"
            onclick="loadStation()">CARREGAR</button>
    <span id="stationName" class="flex-grow-1" style="font-size:.9rem">—</span>
    <span id="condPill2" class="badge border font-mono" style="font-size:.68rem;letter-spacing:.12em"></span>
  </div>

  <!-- LEITURA REAL -->
  <div>
    <p class="font-mono text-dim mb-1" style="font-size:.6rem;letter-spacing:.2em"><span class="dot"></span>LEITURA REAL</p>
    <div class="row g-2" id="realCards">

      <div class="col-6 col-md-3">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-r">
          <div class="d-flex justify-content-between align-items-start mb-1">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">NASCER DO SOL</span>
            <span class="badge bg-info bg-opacity-10 border border-info text-info font-mono" style="font-size:.52rem">REAL</span>
          </div>
          <div id="r-sunrise" class="font-mono text-ax fs-4 lh-1">—</div>
          <small class="text-dim">hora local</small>
        </div>
      </div>

      <div class="col-6 col-md-3">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-r">
          <div class="d-flex justify-content-between align-items-start mb-1">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">PÔR DO SOL</span>
            <span class="badge bg-info bg-opacity-10 border border-info text-info font-mono" style="font-size:.52rem">REAL</span>
          </div>
          <div id="r-sunset" class="font-mono text-ax fs-4 lh-1">—</div>
          <small class="text-dim">hora local</small>
        </div>
      </div>

      <div class="col-6 col-md-3">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-r">
          <div class="d-flex justify-content-between align-items-start mb-1">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">TETO</span>
            <span class="badge bg-info bg-opacity-10 border border-info text-info font-mono" style="font-size:.52rem">REAL</span>
          </div>
          <div id="r-ceiling" class="font-mono text-ax fs-4 lh-1">—</div>
          <small class="text-dim">pés</small>
        </div>
      </div>

      <div class="col-6 col-md-3">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-r">
          <div class="d-flex justify-content-between align-items-start mb-1">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">VISIBILIDADE</span>
            <span class="badge bg-info bg-opacity-10 border border-info text-info font-mono" style="font-size:.52rem">REAL</span>
          </div>
          <div id="r-vis" class="font-mono text-ax fs-4 lh-1">—</div>
          <small class="text-dim">metros</small>
        </div>
      </div>

    </div>
  </div>

  <!-- ENTRADA SIMULADA -->
  <div>
    <p class="font-mono text-dim mb-1" style="font-size:.6rem;letter-spacing:.2em">ENTRADA SIMULADA</p>
    <div class="row g-2" id="simCards">

      <div class="col-6 col-md-3">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-s">
          <div class="d-flex justify-content-between align-items-start mb-2">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">NASCER DO SOL</span>
            <span class="badge bg-warning bg-opacity-10 border border-warning text-warning font-mono" style="font-size:.52rem">SIM</span>
          </div>
          <input id="s-sunrise" type="time" value="05:48" class="form-control form-control-sm inp-sim"/>
        </div>
      </div>

      <div class="col-6 col-md-3">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-s">
          <div class="d-flex justify-content-between align-items-start mb-2">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">PÔR DO SOL</span>
            <span class="badge bg-warning bg-opacity-10 border border-warning text-warning font-mono" style="font-size:.52rem">SIM</span>
          </div>
          <input id="s-sunset" type="time" value="18:12" class="form-control form-control-sm inp-sim"/>
        </div>
      </div>

      <div class="col-6 col-md-3">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-s">
          <div class="d-flex justify-content-between align-items-start mb-2">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">TETO</span>
            <span class="badge bg-warning bg-opacity-10 border border-warning text-warning font-mono" style="font-size:.52rem">SIM</span>
          </div>
          <input id="s-ceiling" type="number" value="3500" min="0" max="99999" step="100"
                 class="form-control form-control-sm inp-sim"/>
          <small class="text-dim">pés</small>
        </div>
      </div>

      <div class="col-6 col-md-3">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-s">
          <div class="d-flex justify-content-between align-items-start mb-2">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">VISIBILIDADE</span>
            <span class="badge bg-warning bg-opacity-10 border border-warning text-warning font-mono" style="font-size:.52rem">SIM</span>
          </div>
          <input id="s-vis" type="number" value="9000" min="0" max="9999" step="100"
                 class="form-control form-control-sm inp-sim"/>
          <small class="text-dim">metros</small>
        </div>
      </div>

    </div>
  </div>

  <!-- ══ PAINEL DO FAROL ══════════════════════════════════════ -->
  <!-- Status e controle manual — dados injetados pelo C++ -->
  <div>
    <p class="font-mono text-dim mb-1" style="font-size:.6rem;letter-spacing:.2em">FAROL</p>
    <div class="row g-2">

      <!-- STATUS DO FAROL (vem do C++) -->
      <div class="col-12 col-md-6">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-f">
          <div class="d-flex justify-content-between align-items-start mb-2">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">STATUS DO FAROL</span>
            %%OVERRIDE_BADGE%%
          </div>
          <div class="d-flex align-items-center gap-3">
            <span class="led %%LED_CLS%% %%LED_PULSE%%"></span>
            <span class="font-mono fw-bold %%FAROL_CLASS%%" style="font-size:1.8rem;letter-spacing:.15em">%%FAROL_STATUS%%</span>
          </div>
          <small class="text-dim font-mono" style="font-size:.6rem;letter-spacing:.1em">
            Fonte: GPIO 13 · Estado real do hardware
          </small>
        </div>
      </div>

      <!-- CONTROLE MANUAL -->
      <div class="col-12 col-md-6">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-f">
          <div class="d-flex justify-content-between align-items-start mb-3">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">CONTROLE MANUAL</span>
            <span class="badge bg-warning bg-opacity-10 border border-warning text-warning font-mono" style="font-size:.52rem">OVERRIDE</span>
          </div>
          <div class="d-flex gap-2">
            <!-- Botão LIGAR → GET /on -->
            <a href="/on" class="btn btn-outline-success font-mono flex-fill text-center"
               style="font-size:.8rem;letter-spacing:.12em">
              &#9654; LIGAR
            </a>
            <!-- Botão DESLIGAR → GET /off -->
            <a href="/off" class="btn btn-outline-danger font-mono flex-fill text-center"
               style="font-size:.8rem;letter-spacing:.12em">
              &#9646;&#9646; DESLIGAR
            </a>
          </div>
          <small class="text-dim font-mono mt-2 d-block" style="font-size:.6rem;letter-spacing:.1em">
            Acionamento direto via ESP32 · Recarrega a página com status atualizado
          </small>
        </div>
      </div>

    </div>
  </div>
  <!-- ══ FIM PAINEL FAROL ══════════════════════════════════════ -->

  <!-- BARRA DE MODO / FONTE ATIVA -->
  <div class="bg-panel border b-dark rounded px-3 py-2 d-flex flex-wrap align-items-center gap-3">
    <span class="font-mono text-dim" style="font-size:.68rem;letter-spacing:.13em">LÓGICA AUTO</span>
    <span class="led" id="equipLed"></span>
    <span id="equipStatus" class="font-mono fw-bold" style="font-size:1rem;letter-spacing:.15em">—</span>
    <div class="vr opacity-25"></div>
    <span class="font-mono text-dim" style="font-size:.65rem;letter-spacing:.1em">FONTE ATIVA:</span>
    <span id="sourceLabel" class="font-mono fw-bold" style="font-size:.85rem;letter-spacing:.12em"></span>
    <button id="btnToggleEquip" class="btn btn-sm btn-outline-success font-mono ms-auto"
            style="font-size:.68rem;letter-spacing:.1em" onclick="toggleEquipLogic()">FORÇAR LIGAR</button>
  </div>

  <!-- LOG -->
  <div class="bg-panel border b-dark rounded p-2" style="max-height:200px;overflow-y:auto">
    <div class="font-mono text-dim border-bottom b-dark pb-2 mb-2" style="font-size:.6rem;letter-spacing:.18em">&#9632; LOG DE EVENTOS</div>
    <div id="logEntries"></div>
  </div>

</div><!-- /container -->

<script src="https://cdnjs.cloudflare.com/ajax/libs/bootstrap/5.3.3/js/bootstrap.bundle.min.js"></script>
<script>
let mode='real',equipOn=false,manualOverride=false;

const STATIONS={
  'SBGR':'Guarulhos / Cumbica - SP','SBSP':'Congonhas - SP',
  'SBBR':'Brasilia Internacional','SBGL':'Galeao - RJ',
  'SBSV':'Salvador - BA','SBCF':'Tancredo Neves - MG',
  'SBRF':'Recife - PE','SBPA':'Porto Alegre - RS',
  'SBCT':'Curitiba - PR','SBFZ':'Fortaleza - CE'
};

// Clock UTC
function tick(){
  const n=new Date(),z=v=>String(v).padStart(2,'0');
  document.getElementById('clockEl').textContent=z(n.getUTCHours())+':'+z(n.getUTCMinutes())+':'+z(n.getUTCSeconds())+' UTC';
}
setInterval(tick,1000);tick();

// Mode REAL / SIM
function setMode(m){
  mode=m;
  document.getElementById('btnReal').classList.toggle('active',m==='real');
  document.getElementById('btnSim').classList.toggle('active',m==='sim');
  const lbl=document.getElementById('sourceLabel');
  lbl.textContent=m==='real'?'REAL':'SIMULADA';
  lbl.style.color=m==='real'?'#00c8ff':'#ff6b35';
  dim('simCards',m==='real');
  dim('realCards',m==='sim');
  addLog('Fonte: '+m.toUpperCase(),'info');
  evalEquip();
}
function dim(id,on){
  document.getElementById(id).querySelectorAll('.col-6').forEach(c=>{
    c.style.opacity=on?'0.35':'1';
    c.style.pointerEvents=on?'none':'';
    c.style.transition='opacity .3s';
  });
}

// ICAO
function loadStation(){
  const code=document.getElementById('icaoInput').value.toUpperCase().trim();
  document.getElementById('icaoInput').value=code;
  const name=STATIONS[code]||('Estacao '+code);
  document.getElementById('stationName').textContent=name;
  addLog('ICAO: '+code+' - '+name,'info');
  fetchReal(code);
}

function fetchReal(code){
  // Producao: fetch('/api/metar?icao='+code).then(r=>r.json()).then(d=>{ ... })
  const sr=rndTime(5,7),ss=rndTime(17,19);
  const ceil=Math.round((Math.random()*9000+500)/100)*100;
  const vis=Math.round((Math.random()*9000+500)/100)*100;
  document.getElementById('r-sunrise').textContent=sr;
  document.getElementById('r-sunset').textContent=ss;
  document.getElementById('r-ceiling').textContent=ceil;
  document.getElementById('r-vis').textContent=vis;
  setCond(ceil>=1500&&vis>=5000?'VMC':'IMC');
  evalEquip();
}

function rndTime(a,b){
  const h=Math.floor(Math.random()*(b-a)+a),m=Math.floor(Math.random()*60);
  return String(h).padStart(2,'0')+':'+String(m).padStart(2,'0');
}

function setCond(c){
  const ok=c==='VMC';
  ['condPill','condPill2'].forEach(id=>{
    const el=document.getElementById(id);
    el.textContent=c;
    el.className=ok
      ?'badge border font-mono bg-success bg-opacity-10 text-success border-success'
      :'badge border font-mono bg-danger bg-opacity-10 text-danger border-danger';
    el.style.fontSize='.68rem';el.style.letterSpacing='.12em';
  });
}

// Lógica automática (JS)
function evalEquip(){
  if(manualOverride)return;
  let ceil,vis,sr,ss;
  if(mode==='real'){
    ceil=parseInt(document.getElementById('r-ceiling').textContent)||0;
    vis=parseInt(document.getElementById('r-vis').textContent)||0;
    sr=document.getElementById('r-sunrise').textContent;
    ss=document.getElementById('r-sunset').textContent;
  }else{
    ceil=parseInt(document.getElementById('s-ceiling').value)||0;
    vis=parseInt(document.getElementById('s-vis').value)||0;
    sr=document.getElementById('s-sunrise').value;
    ss=document.getElementById('s-sunset').value;
  }
  const now=nowMin(),day=now>=toMin(sr)&&now<toMin(ss);
  const vmc=ceil>=1500&&vis>=5000;
  setEquipLogic(!day||!vmc,'AUTO');
}

// Forçar lógica manual (JS apenas — não aciona GPIO diretamente)
function toggleEquipLogic(){
  const desired=!equipOn;
  setEquipLogic(desired,'MANUAL-JS');
  manualOverride=true;
  setTimeout(()=>{
    manualOverride=false;evalEquip();
    addLog('Override JS encerrado. Retomando automatico.','info');
  },30000);
}

function setEquipLogic(on,src){
  if(equipOn===on)return;
  equipOn=on;
  document.getElementById('equipLed').className=on?'led led-on':'led led-off';
  const st=document.getElementById('equipStatus');
  st.textContent=on?'LIGAR':'DESLIGAR';
  st.style.color=on?'#00e676':'#3a4a5c';
  const btn=document.getElementById('btnToggleEquip');
  btn.textContent=on?'FORÇAR DESLIGAR':'FORÇAR LIGAR';
  btn.className='btn btn-sm font-mono ms-auto '+(on?'btn-outline-danger':'btn-outline-success');
  btn.style.fontSize='.68rem';btn.style.letterSpacing='.1em';
  addLog('Logica sugere: '+(on?'LIGAR':'DESLIGAR')+' ['+src+']',on?'on':'off');
}

function nowMin(){const d=new Date();return d.getHours()*60+d.getMinutes();}
function toMin(t){const p=(t||'00:00').split(':');return parseInt(p[0])*60+(parseInt(p[1])||0);}

function addLog(msg,type){
  const n=new Date(),z=v=>String(v).padStart(2,'0');
  const ts=z(n.getHours())+':'+z(n.getMinutes())+':'+z(n.getSeconds());
  const el=document.createElement('div');
  el.className='log-e log-'+type;
  el.innerHTML='<span class="log-ts">'+ts+'</span><span class="log-m">'+msg+'</span>';
  const box=document.getElementById('logEntries');
  box.prepend(el);
  while(box.children.length>100)box.removeChild(box.lastChild);
}

['s-sunrise','s-sunset','s-ceiling','s-vis'].forEach(id=>{
  document.getElementById(id).addEventListener('change',()=>{if(mode==='sim')evalEquip();});
});

// Poll simulado (modo real)
function poll(){
  if(mode!=='real')return;
  const c=parseInt(document.getElementById('r-ceiling').textContent)||3500;
  const v=parseInt(document.getElementById('r-vis').textContent)||9000;
  const nc=Math.max(0,c+(Math.random()>.5?100:-100));
  const nv=Math.max(0,Math.min(9999,v+(Math.random()>.5?100:-100)));
  document.getElementById('r-ceiling').textContent=nc;
  document.getElementById('r-vis').textContent=nv;
  setCond(nc>=1500&&nv>=5000?'VMC':'IMC');
  evalEquip();
}
setInterval(poll,15000);

setMode('real');
loadStation();
addLog('Sistema iniciado.','info');
</script>
</body>
</html>)RAW";

// Status textual e cor
  Htmlresponse.replace("%%FAROL_STATUS%%", farolStatus ? "LIGADO"       : "DESLIGADO");
  Htmlresponse.replace("%%FAROL_CLASS%%",  farolStatus ? "text-success" : "text-danger");

  // LED
  Htmlresponse.replace("%%LED_CLS%%",   farolStatus ? "led-on"  : "led-off");
  Htmlresponse.replace("%%LED_PULSE%%", farolStatus ? "led-pulse" : "");

  // Badge de override
  if (manualOverride) {
    Htmlresponse.replace("%%OVERRIDE_BADGE%%",
      "<span class=\"badge bg-warning bg-opacity-10 border border-warning text-warning font-mono\" "
      "style=\"font-size:.52rem\">MANUAL</span>");
  } else {
    Htmlresponse.replace("%%OVERRIDE_BADGE%%",
      "<span class=\"badge bg-secondary bg-opacity-10 border border-secondary text-secondary font-mono\" "
      "style=\"font-size:.52rem\">AUTO</span>");
  }

  server.send(200, "text/html", Htmlresponse);

}


void setup()
{
  pinMode(LED_PIN_FAROL, OUTPUT);
  digitalWrite(LED_PIN_FAROL, LOW);
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=== ESP32 Farol Aeroporto ===");
  Serial.print("Conectando ao WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Timeout de 20 segundos para conectar
  int tentativas = 40;
  while (WiFi.status() != WL_CONNECTED && tentativas > 0)
  {
    delay(500);
    Serial.print(".");
    tentativas--;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nConectado ao WiFi!");
    Serial.print("IP local: ");
    Serial.println(WiFi.localIP());
    if (MDNS.begin(mdnsName))
    {
      Serial.print("Nome mDNS: http://");
      Serial.print(mdnsName);
      Serial.println(".local");
    }
  }
  else
  {
    Serial.println("\nNão conectou ao WiFi. Iniciando ponto de acesso para acesso direto...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid, apPassword, channel);
    IPAddress apIp = WiFi.softAPIP();
    Serial.print("Rede AP: ");
    Serial.println(apSsid);
    Serial.print("IP do AP: ");
    Serial.println(apIp);
    Serial.print("Acesse: http://");
    Serial.println(apIp);
  }

  // Configura as rotas do servidor
  // /on  → liga farol
  server.on("/on", []() {
    farolStatus    = true;
    manualOverride = true;
    digitalWrite(LED_PIN_FAROL, HIGH);
    Serial.println("Farol LIGADO (manual)");
    sendHtml();
  });

  // /off → desliga farol
  server.on("/off", []() {
    farolStatus    = false;
    manualOverride = true;
    digitalWrite(LED_PIN_FAROL, LOW);
    Serial.println("Farol DESLIGADO (manual)");
    sendHtml();
  });

  // /auto → devolve controle para a lógica automática
  server.on("/auto", []() {
    manualOverride = false;
    Serial.println("Override manual cancelado. Modo AUTO.");
    sendHtml();
  });

  // Rota genérica (compatibilidade com UriBraces anterior)
  server.on(UriBraces("/{}"), []() {
    String param = server.pathArg(0);
    Serial.println("Comando: " + param);
    if (param == "on") {
      farolStatus = true; manualOverride = true;
      digitalWrite(LED_PIN_FAROL, HIGH);
    } else if (param == "off") {
      farolStatus = false; manualOverride = true;
      digitalWrite(LED_PIN_FAROL, LOW);
    } else if (param == "auto") {
      manualOverride = false;
    }
    sendHtml();
  });

  server.begin();
  Serial.println("Servidor HTTP iniciado");
  Serial.println("Acesse o IP mostrado acima na porta 8180");
}

void loop()
{
  server.handleClient();
  delay(2);
}