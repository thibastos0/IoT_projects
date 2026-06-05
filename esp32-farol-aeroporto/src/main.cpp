#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>


// const char *ssid = "Sala Maker";
// const char *password = "Maker@fatec";
#define ssid "Wokwi-GUEST"
#define password ""
#define channel 6

unsigned long lastFetch = 0;
const unsigned long FETCH_INTERVAL = 60000;

const char *apSsid = "Farol-Aeroporto";
const char *apPassword = "12345678";
const char *mdnsName = "farol-aeroporto";

//const char *apiUrl_AviationWeather = "https://aviationweather.gov/api/data/metar?ids=SBKP&format=json";
//const char* apiUrl_AviationWeather = "http://192.168.122.1/SBKP"; // na SALA MAKER
//const char* apiUrl_OpenWeather = "https://api.openweathermap.org/data/2.5/weather?lat=-23.007&lon=-47.135&appid=f814b1f74b001e40b3a18bf369b9d48d&units=metric&lang=pt_br";

WebServer server(80);

const int LED_PIN_FAROL = 13;
bool farolStatus = false; 
bool manualOverride = false;

// Variáveis globais — dados meteorológicos e outras informações relevantes
int    g_visib       = -1;
int    g_ceilingFt   = -1;
int    g_sunrise     = -1;
int    g_sunset      = -1;
float  g_lat         = 0.0;
float  g_lon         = 0.0;
String g_icaoId      = "";
String g_aeroportoName = "";
String g_mode        = "real";
bool   g_ssAtualizado = false; 

// Declarações de funções
void parseWeatherData(const String& json);
void parseWeatherData_SS(const String& json);
void fetchWeatherData();
void fetchSunriseSunset();
void avaliarFarol();

// Página HTML estilizada e otimizada
void sendHtml()
{
  String Htmlresponse = R"RAW(
<!DOCTYPE html>
<html lang="pt-BR" data-bs-theme="dark">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>ESP32 FAROL-CTL</title>

<link href="https://cdnjs.cloudflare.com/ajax/libs/bootstrap/5.3.3/css/bootstrap.min.css" rel="stylesheet"/>
 
<style>
body{background:#0b0e13;font-family:'Barlow Condensed',sans-serif}
.font-mono{font-family:'Share Tech Mono',monospace}
.bg-panel{background:#111620!important}
.b-dark{border-color:#1e2a3a!important}
.text-ax{color:#00c8ff!important}
.text-sim{color:#ff6b35!important}
.text-dim{color:#ffffff!important} /* Alterado para text-white/branco para melhorar a leitura */
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
.log-ts{color:#00c8ff;white-space:nowrap}
.log-on .log-m{color:#00e676}
.log-off .log-m{color:#ff3d3d}
.log-info .log-m{color:#ffab00}
.log-m{color:#cdd8e3}
input[type=number]::-webkit-inner-spin-button{display:none}
</style>
</head>
<body>

  <nav class="navbar bg-panel border-bottom b-dark px-3 py-2">
    <span class="font-mono text-ax fw-bold me-auto" style="font-size:1.1rem;letter-spacing:.2em">
      ESP32 <span class="text-sim">//</span> Controle-Farol
    </span>
    <div class="d-flex align-items-center gap-3">
      <div class="btn-group btn-group-sm">
        <button id="btnReal" class="btn btn-outline-info font-mono" style="font-size:.7rem;letter-spacing:.1em" onclick="setMode('real')">REAL</button>
        <button id="btnSim" class="btn btn-outline-warning font-mono" style="font-size:.7rem;letter-spacing:.1em" onclick="setMode('sim')">SIM</button>
      </div>
      <span id="clockEl" class="font-mono text-white d-none d-sm-inline" style="font-size:0.9rem;letter-spacing:.1em; font-weight: bold;"></span>
    </div>
  </nav>

<div class="container-fluid px-3 py-3 d-flex flex-column gap-3">

  <div class="bg-panel border b-dark rounded p-2 d-flex flex-wrap align-items-center gap-2">
    <label class="font-mono text-dim mb-0" style="font-size:.7rem;letter-spacing:.14em">ICAO</label>
    <input id="icaoInput" type="text" maxlength="4" placeholder="SBGR" value="SBGR"
           class="form-control form-control-sm inp-icao font-mono" style="width:88px"/>
    <button class="btn btn-sm btn-outline-info font-mono" style="font-size:.68rem;letter-spacing:.1em"
            onclick="loadAeroporto()">CARREGAR</button>
    <span id="aeroportoName" class="flex-grow-1 text-white" style="font-size:.9rem">—</span>
    <span id="condPill2" class="badge border font-mono" style="font-size:.68rem;letter-spacing:.12em"></span>
  </div>

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
          <small class="text-white opacity-75">hora local</small>
        </div>
      </div>

      <div class="col-6 col-md-3">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-r">
          <div class="d-flex justify-content-between align-items-start mb-1">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">PÔR DO SOL</span>
            <span class="badge bg-info bg-opacity-10 border border-info text-info font-mono" style="font-size:.52rem">REAL</span>
          </div>
          <div id="r-sunset" class="font-mono text-ax fs-4 lh-1">—</div>
          <small class="text-white opacity-75">hora local</small>
        </div>
      </div>

      <div class="col-6 col-md-3">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-r">
          <div class="d-flex justify-content-between align-items-start mb-1">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">TETO</span>
            <span class="badge bg-info bg-opacity-10 border border-info text-info font-mono" style="font-size:.52rem">REAL</span>
          </div>
          <div id="r-ceiling" class="font-mono text-ax fs-4 lh-1">—</div>
          <small class="text-white opacity-75">pés</small>
        </div>
      </div>

      <div class="col-6 col-md-3">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-r">
          <div class="d-flex justify-content-between align-items-start mb-1">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">VISIBILIDADE</span>
            <span class="badge bg-info bg-opacity-10 border border-info text-info font-mono" style="font-size:.52rem">REAL</span>
          </div>
          <div id="r-vis" class="font-mono text-ax fs-4 lh-1">—</div>
          <small class="text-white opacity-75">metros</small>
        </div>
      </div>

    </div>
  </div>

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
          <small class="text-white opacity-75">pés</small>
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
          <small class="text-white opacity-75">metros</small>
        </div>
      </div>

    </div>
  </div>

  <div>
    <p class="font-mono text-dim mb-1" style="font-size:.6rem;letter-spacing:.2em">SISTEMA DO FAROL</p>
    <div class="row g-2">

      <div class="col-12 col-md-6">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-f">
          <div class="d-flex justify-content-between align-items-start mb-2">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">STATUS DO HARDWARE</span>
            %%OVERRIDE_BADGE%%
          </div>
          <div class="d-flex align-items-center gap-3">
            <span class="led %%LED_CLS%% %%LED_PULSE%%"></span>
            <span class="font-mono fw-bold %%FAROL_CLASS%%" style="font-size:1.8rem;letter-spacing:.15em">%%FAROL_STATUS%%</span>
          </div>
          <small class="text-white opacity-75 font-mono" style="font-size:.6rem;letter-spacing:.1em">
            Fonte: GPIO 13 · Monitoramento em Tempo Real
          </small>
        </div>
      </div>

      <div class="col-12 col-md-6">
        <div class="bg-panel border b-dark rounded p-3 h-100 position-relative overflow-hidden card-stripe-f">
          <div class="d-flex justify-content-between align-items-start mb-3">
            <span class="font-mono text-dim" style="font-size:.6rem;letter-spacing:.13em">CONTROLE DE OPERAÇÃO</span>
            <span class="badge bg-info bg-opacity-10 border border-info text-info font-mono" style="font-size:.52rem">INTERRUPTOR</span>
          </div>
          <div class="d-flex gap-2">
            %%DYNAMIC_CONTROLS%%
          </div>
          <small class="text-white opacity-75 font-mono mt-2 d-block" style="font-size:.6rem;letter-spacing:.1em">
            O comando manual sobrescreve a automação de leitura de teto/visibilidade.
          </small>
        </div>
      </div>

    </div>
  </div>
  <div class="bg-panel border b-dark rounded p-2" style="max-height:200px;overflow-y:auto">
    <div class="font-mono text-dim border-bottom b-dark pb-2 mb-2" style="font-size:.6rem;letter-spacing:.18em">&#9632; LOG DE EVENTOS HISTÓRICOS</div>
    <div id="logEntries"></div>
  </div>

</div><script src="https://cdnjs.cloudflare.com/ajax/libs/bootstrap/5.3.3/js/bootstrap.bundle.min.js"></script>
<script>
let mode='real';

// Clock UTC
function tick(){
  const n=new Date(),z=v=>String(v).padStart(2,'0');
  document.getElementById('clockEl').textContent=z(n.getUTCHours())+':'+z(n.getUTCMinutes())+':'+z(n.getUTCSeconds())+' UTC';
}
setInterval(tick,1000);tick();

function loadAeroporto() {
  const code = document.getElementById('icaoInput').value.toUpperCase().trim();
  if (code.length < 2) return;
  addLog('Carregando ICAO: ' + code, 'info');
  fetch('/icao?id=' + code)
    .then(() => pollEstado()); // após o C++ buscar os dados, atualiza a tela
}

function acionarFarol(cmd) { // cmd: 'on', 'off' ou 'auto'
  fetch('/' + cmd)
    .then(() => pollEstado());
}

// Mode REAL / SIM - Restringida a lógica de esmaecimento visual (dim)
function setMode(m, notificar = true) {
  mode = m;
  document.getElementById('btnReal').classList.toggle('active', m === 'real');
  document.getElementById('btnSim').classList.toggle('active',  m === 'sim');
  dim('simCards',m==='real');
  dim('realCards',m==='sim');
  addLog('Fonte ativa alterada para: '+m.toUpperCase(),'info');

  if (notificar) {
    fetch('/modo?v=' + m);
  }
}

// Controla opacidade dos cards baseado no modo selecionado
function dim(id,on){
  document.getElementById(id).querySelectorAll('.col-6').forEach(c=>{
    c.style.opacity=on?'0.35':'1';
    c.style.pointerEvents=on?'none':'';
    c.style.transition='opacity .3s';
  });
}

function setCond(c) {
  const ok = c === 'VMC';
  const el = document.getElementById('condPill2');
  el.textContent = c;
  el.className = ok
    ? 'badge border font-mono bg-success bg-opacity-10 text-success border-success'
    : 'badge border font-mono bg-danger  bg-opacity-10 text-danger  border-danger';
  el.style.fontSize      = '.68rem';
  el.style.letterSpacing = '.12em';
}

function minToHHMM(min) {
  if (min < 0) return '—';
  const h = Math.floor(min / 60);
  const m = min % 60;
  return String(h).padStart(2, '0') + ':' + String(m).padStart(2, '0');
}

function atualizarTela(data) {
  // Dados meteorológicos reais
  document.getElementById('r-sunrise').textContent = minToHHMM(data.sunrise);
  document.getElementById('r-sunset').textContent  = minToHHMM(data.sunset);
  document.getElementById('r-ceiling').textContent = data.ceiling >= 0 ? data.ceiling : '—';
  document.getElementById('r-vis').textContent     = data.visib >= 0   ? data.visib   : '—';

  // Nome da estação e ICAO
  document.getElementById('aeroportoName').textContent = data.aeroporto || '—';
  document.getElementById('icaoInput').value         = data.icao   || '';

  setCond(data.condicao);

  // Status do farol
  const led    = document.getElementById('equipLed');
  const status = document.getElementById('equipStatus');
  led.className    = data.farol ? 'led led-on led-pulse' : 'led led-off';
  status.textContent = data.farol ? 'LIGADO' : 'DESLIGADO';
  status.style.color = data.farol ? '#00e676' : '#3a4a5c';

  // Badge de override
  const badge = document.getElementById('overrideBadge');
  if (badge) badge.textContent = data.override ? 'MANUAL' : 'AUTO';

  // Modo ativo — sincroniza o toggle visual com o que o C++ tem
  setMode(data.mode, false); // o segundo argumento false = não chama fetch de volta
}

function pollEstado() {
  fetch('/estado')
    .then(r => r.json())
    .then(data => atualizarTela(data))
    .catch(err => console.warn('Erro ao buscar /estado:', err));
}

setInterval(pollEstado, 30000); // atualiza a cada 30 s
pollEstado();                   // chama imediatamente ao carregar

function addLog(msg,type){
  const n=new Date(),z=v=>String(v).padStart(2,'0');
  const ts=z(n.getHours())+':'+z(n.getMinutes())+':'+z(n.getSeconds());
  const el=document.createElement('div');
  el.className='log-e log-'+type;
  el.innerHTML='<span class="log-ts">'+ts+'</span><span class="log-m">'+msg+'</span>';
  const box=document.getElementById('logEntries');
  box.prepend(el);
  if(box.children.length>100)box.removeChild(box.lastChild);
}

// Configuração inicial das abas visuais
setTimeout(() => {
  setMode('real');
  loadAeroporto();
}, 200);

addLog('Sistema inicializado no navegador.','info');
</script>
</body>
</html>)RAW";

  // Status textual e cor do Hardware
  Htmlresponse.replace("%%FAROL_STATUS%%", farolStatus ? "LIGADO"       : "DESLIGADO");
  Htmlresponse.replace("%%FAROL_CLASS%%",  farolStatus ? "text-success" : "text-danger");

  // LED do painel
  Htmlresponse.replace("%%LED_CLS%%",   farolStatus ? "led-on"  : "led-off");
  Htmlresponse.replace("%%LED_PULSE%%", farolStatus ? "led-pulse" : "");

  // Badge de status de automação e injeção dinâmica de botões
  if (manualOverride) {
    Htmlresponse.replace("%%OVERRIDE_BADGE%%",
      "<span class=\"badge bg-warning bg-opacity-10 border border-warning text-warning font-mono\" style=\"font-size:.52rem\">MODO MANUAL</span>");
    
    Htmlresponse.replace("%%DYNAMIC_CONTROLS%%",
      "<button onclick=\"acionarFarol('auto')\" class=\"btn btn-warning font-mono flex-fill text-center\" style=\"font-size:.8rem;letter-spacing:.12em\">"
      "&#9842; RETOMAR CONTROLE AUTOMÁTICO</button>");
  } else {
    Htmlresponse.replace("%%OVERRIDE_BADGE%%",
      "<span class=\"badge bg-success bg-opacity-10 border border-success text-success font-mono\" style=\"font-size:.52rem\">MODO AUTOMÁTICO</span>");
    
    Htmlresponse.replace("%%DYNAMIC_CONTROLS%%",
      "<button onclick=\"acionarFarol('on')\"   class=\"btn btn-outline-success font-mono flex-fill\" style=\"font-size:.8rem;letter-spacing:.12em\">&#9654; LIGAR</button>"
      "<button onclick=\"acionarFarol('off')\"  class=\"btn btn-outline-danger  font-mono flex-fill\" style=\"font-size:.8rem;letter-spacing:.12em\">&#9646;&#9646; DESLIGAR</button>");
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

  int tentatives = 40;
  while (WiFi.status() != WL_CONNECTED && tentatives > 0)
  {
    delay(500);
    Serial.print(".");
    tentatives--;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nConectado ao WiFi!");
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("IP local: ");
    Serial.println(WiFi.localIP());
    if (MDNS.begin(mdnsName))
    {
      Serial.print("Nome mDNS: http://");
      Serial.print(mdnsName);
      Serial.println(".local");
    }
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov", "a.ntp.br"); // Configura fuso horário GMT-3 e servidores NTP sincronizados para o ESP32
    Serial.println("Sincronizando horário via NTP...");
    struct tm t;
    while (!getLocalTime(&t)) delay(500);
    Serial.println("Horário sincronizado: " + String(t.tm_hour) + ":" + String(t.tm_min) + ":" + String(t.tm_sec));
  }
  else
  {
    Serial.println("\nNão conectou ao WiFi. Iniciando AP...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid, apPassword, channel);
    IPAddress apIp = WiFi.softAPIP();
    Serial.print("IP do AP: ");
    Serial.println(apIp);
  }

  // Rotas do Servidor HTTP
  server.on("/estado", []() {
    bool imc = (g_ceilingFt >= 0 && g_ceilingFt < 1500)
        || (g_visib     >= 0 && g_visib     < 5000);
    String json = "{";
    json += "\"visib\":"     + String(g_visib)      + ",";
    json += "\"ceiling\":"   + String(g_ceilingFt)  + ",";
    json += "\"sunrise\":"   + String(g_sunrise)    + ",";
    json += "\"sunset\":"    + String(g_sunset)     + ",";
    json += "\"lat\":"       + String(g_lat, 4)     + ",";
    json += "\"lon\":"       + String(g_lon, 4)     + ",";
    json += "\"condicao\":\"" + String(imc ? "IMC" : "VMC") + "\",";
    json += "\"farol\":"     + String(farolStatus   ? "true" : "false") + ",";
    json += "\"override\":"  + String(manualOverride? "true" : "false") + ",";
    json += "\"mode\":\""    + g_mode               + "\",";
    json += "\"icao\":\""    + g_icaoId             + "\",";
    json += "\"aeroporto\":\"" + g_aeroportoName        + "\"";
    json += "}";
    server.send(200, "application/json", json);
  });

  
server.on("/on", []() {
    farolStatus    = true;
    manualOverride = true;
    digitalWrite(LED_PIN_FAROL, HIGH);
    Serial.println("Farol LIGADO (Manual)");
    server.send(200, "application/json", "{\"ok\":true}");
});

server.on("/off", []() {
    farolStatus    = false;
    manualOverride = true;
    digitalWrite(LED_PIN_FAROL, LOW);
    Serial.println("Farol DESLIGADO (Manual)");
    server.send(200, "application/json", "{\"ok\":true}");
});

server.on("/auto", []() {
    manualOverride = false;
    Serial.println("Controle devolvido para automático.");
    server.send(200, "application/json", "{\"ok\":true}");
});

 server.on("/icao", []() {
  if (server.hasArg("id")) {
    String id = server.arg("id");
    id.toUpperCase();
    id.trim();
    g_icaoId = id;
    g_ssAtualizado = false; // força atualização de nascer/pôr do sol para nova localização
    fetchWeatherData();           
    fetchSunriseSunset();         
  }
  server.send(200, "application/json", "{\"ok\":true}");
});

  server.on("/modo", []() {
  if (server.hasArg("v")) {
    String v = server.arg("v");
    if (v == "real" || v == "sim") g_mode = v;
  }
  server.send(200, "application/json", "{\"ok\":true}");
  });

  server.on(UriBraces("/{}"), []() {
    String param = server.pathArg(0);
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

  g_icaoId = "SBKP"; // aeroporto padrão ao inicializar
  server.begin();
 
  Serial.println("Servidor HTTP ativo na porta 80");
}

void loop()
{
  server.handleClient();

  if (millis() - lastFetch >= FETCH_INTERVAL) {
    lastFetch = millis();
    fetchWeatherData();
    if (!g_ssAtualizado) fetchSunriseSunset();
    avaliarFarol();
  }
}

//**************************************************************************
// Busca e processamento de dados METAR da API AviationWeather
//**************************************************************************

void fetchWeatherData()
{
    
  //WiFiClientSecure client;
  //client.setInsecure(); // Ignora erros de certificado SSL

  if(WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  //http.begin(client, "https://aviationweather.gov/api/data/metar?ids=" + g_icaoId + "&format=json");
  //String url = "https://aviationweather.gov/api/data/metar?ids=" + g_icaoId + "&format=json";
  String url = "http://192.168.122.1/" + g_icaoId; // para testes locais na SALA MAKER
  url = "http://10.108.5.3/" + g_icaoId; // para testes locais no trabalho
  http.begin(url);
  http.addHeader("User-Agent", "ESP32-FarolAeroporto/1.0");
  http.addHeader("Accept", "application/json");

  int httpCode = http.GET();
  if(httpCode == HTTP_CODE_OK){
    String payload = http.getString();
    parseWeatherData(payload);
  }
  http.end();
}

// Arredondamento de visibilidade conforme escala ICAO Anexo 3:
//   < 800 m  -> multiplos de 50 m
//   800-5000 -> multiplos de 100 m
//   > 5000   -> multiplos de 1000 m
int roundVisibICAO(float meters) {
  int v = (int)meters;
  if (v > 5000)  return (v / 1000) * 1000;
  if (v >= 800)  return (v / 100)  * 100;
  return         (v / 50)   * 50;
}

void parseWeatherData(const String& json)
{
  JsonDocument doc; 
  DeserializationError error = deserializeJson(doc, json);
  if (error) return;

  JsonObject metar = doc[0];
  if (metar.isNull()) return;
    
  String icaoId = metar["icaoId"];
  String name = metar["name"];
  String visibStr = metar["visib"].as<String>();
  String lat = metar["lat"].as<String>();
  String lon = metar["lon"].as<String>();

  // Visibilidade: o sufixo "+" em "6+" significa ">= 6 SM" = irrestrita -> 9999 m (ICAO)
  // Sem sufixo: converte SM -> m e arredonda pela escala ICAO Anexo 3
  int visib;
  if (visibStr.endsWith("+")) {
    visib = 9999;
  } else {
    visib = roundVisibICAO(atof(visibStr.c_str()) * 1609.34f);
  }

  printf("METAR carregado para %s (%s).\n", name.c_str(), icaoId.c_str());
  printf("Localização: %s, %s.\n", lat.c_str(), lon.c_str()); //para buscar SS e SS no Openweather

  if (visib == 9999)
    printf("Visibilidade: 9999 m (irrestrita >= 10 km)\n");
  else
    printf("Visibilidade: %d m\n", visib);

  
  // Base mais baixa de BKN ou OVC = teto operacional
  // Percorre todo o array clouds e guarda o menor "base" entre camadas relevantes
  int    ceilingFt   = -1;
  String ceilingType = "";
  for (JsonObject cloud : metar["clouds"].as<JsonArray>()) {
    String type = cloud["cover"].as<String>();
    int    base = cloud["base"] | 0;
    if (type == "BKN" || type == "OVC") {
      if (ceilingFt < 0 || base < ceilingFt) {
        ceilingFt   = base;
        ceilingType = type;
      }
    }
  }

  // Log de todas as camadas para diagnostico
  printf("--- Camadas de nuvem ---\n");
  for (JsonObject cloud : metar["clouds"].as<JsonArray>()) {
    printf("  %s a %d pes\n",
      cloud["cover"].as<const char*>(),
      cloud["base"] | 0);
  }

g_icaoId      = icaoId;
g_aeroportoName = name;
g_lat         = atof(lat.c_str());
g_lon         = atof(lon.c_str());
g_visib       = visib;
g_ceilingFt   = ceilingFt;

}

//**************************************************************************
// Busca dados de nascer e pôr do sol no OpenWeather usando lat/lon do METAR
//**************************************************************************

void fetchSunriseSunset()
{
  if(WiFi.status() != WL_CONNECTED) return;

  String url = "https://api.openweathermap.org/data/2.5/weather?lat="
             + String(g_lat, 4)
             + "&lon=" + String(g_lon, 4)
             + "&appid=f814b1f74b001e40b3a18bf369b9d48d&units=metric&lang=pt_br";
  HTTPClient http;
  //http.begin(apiUrl_OpenWeather);
  http.begin(url);
  http.addHeader("User-Agent", "ESP32-FarolAeroporto/1.0");
  http.addHeader("Accept", "application/json");

  int httpCode = http.GET();
  if(httpCode == HTTP_CODE_OK){
    String payload = http.getString();
    parseWeatherData_SS(payload);
  }
  http.end();
}

void parseWeatherData_SS(const String& json)
{
  JsonDocument doc; 
  DeserializationError error = deserializeJson(doc, json);
  if (error) return;

  long sunrise  = doc["sys"]["sunrise"];  
  long sunset   = doc["sys"]["sunset"];
  int  tz = doc["timezone"];

  auto toHHMM = [](long ts, int tz_offset, char* buf) {
    long local = ts + tz_offset; 
    int h = (local % 86400) / 3600;
    int m = (local % 3600) / 60;
    sprintf(buf, "%02d:%02d", h, m);
  };

  char srStr[6], ssStr[6];
  toHHMM(sunrise, tz, srStr);
  toHHMM(sunset,  tz, ssStr);

  Serial.printf("Nascer do sol : %s | Pôr do sol : %s\n", srStr, ssStr);

  g_sunrise = atoi(srStr) * 60 + atoi(srStr + 3);
  g_sunset  = atoi(ssStr) * 60 + atoi(ssStr + 3);
  g_ssAtualizado = true;
  
}

void avaliarFarol() {
  if (manualOverride) return;
  if (g_sunrise < 0 || g_sunset < 0) return; // SR/SS ainda não carregados

  struct tm t;
  if (!getLocalTime(&t)) return;
  int agora = t.tm_hour * 60 + t.tm_min;

  bool ehNoite = (agora < g_sunrise || agora >= g_sunset);
  bool ehIMC   = (g_ceilingFt >= 0 && g_ceilingFt < 1500)
              || (g_visib     >= 0 && g_visib     < 5000);

  bool deveLigar = ehNoite || ehIMC;

  if (deveLigar != farolStatus) {
    farolStatus = deveLigar;
    digitalWrite(LED_PIN_FAROL, deveLigar ? HIGH : LOW);
    Serial.printf("Farol %s (AUTO) — Noite:%d IMC:%d Agora:%d SR:%d SS:%d\n",
      deveLigar ? "LIGADO" : "DESLIGADO",
      ehNoite, ehIMC, agora, g_sunrise, g_sunset);
  }
}