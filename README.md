# ESP32 Farol Aeroporto

Firmware para ESP32 que controla um farol de aeroporto com interface web, leitura automática de dados meteorológicos e sobrescrita manual do estado do LED no GPIO 13.

## O que o projeto faz

O `main.cpp` atual implementa:

- conexão Wi-Fi com fallback para ponto de acesso próprio quando não há rede disponível;
- servidor HTTP na porta 80;
- página web com estado do farol, modo automático/manual e visualização de condições;
- atualização periódica de dados externos a cada 60 segundos;
- leitura de metadados METAR e cálculo de nascer/pôr do sol a partir de API externa;
- controle do LED do farol no pino 13;
- endpoint JSON para consumo por outras interfaces ou integrações.

## Comportamento de rede

Na inicialização o firmware tenta conectar na rede configurada no código:

- SSID: `Wokwi-GUEST`
- senha: vazia

Se a conexão falhar, ele sobe um AP local com:

- SSID: `Farol-Aeroporto`
- senha: `12345678`

Quando conectado com sucesso, o firmware também tenta anunciar o nome mDNS `farol-aeroporto.local`.

## Interface web

A página principal exibe:

- status do farol, com indicação visual de ligado/desligado;
- modo automático ou manual;
- controles para forçar ligar, forçar desligar e devolver o controle ao modo automático;
- campo para informar ICAO do aeroporto;
- painéis para nascer do sol, pôr do sol, teto e visibilidade;
- log histórico de eventos na interface.

A interface também alterna entre uma visão "real" e uma visão "simulada" apenas no navegador, com destaque visual dos blocos ativos.

## Rotas HTTP

O firmware expõe as seguintes rotas:

- `/` e `/on`, `/off`, `/auto` para a interface e controle do farol;
- `/estado` para retornar um JSON com o estado atual;
- qualquer rota no formato `/{comando}` também aceita `on`, `off` e `auto`.

Exemplo de resposta de `/estado`:

```json
{
	"visib": 9000,
	"ceiling": 3500,
	"sunrise": 348,
	"sunset": 1092,
	"lat": -23.0000,
	"lon": -47.0000,
	"farol": true,
	"override": false,
	"mode": "real",
	"icao": "SBGR",
	"station": "Guarulhos / Cumbica - SP"
}
```

## Lógica atual do firmware

- O LED do farol é controlado no GPIO 13.
- O modo manual sobrescreve a automação até que o usuário retorne para `auto`.
- A cada ciclo de atualização o firmware busca dados METAR e sunrise/sunset nas URLs configuradas no código.
- A visibilidade é convertida para metros com arredondamento compatível com a escala ICAO usada no projeto.
- A condição geral é classificada como `VMC` quando teto >= 1500 pés e visibilidade >= 5000 metros; caso contrário, `IMC`.

## Dependências

O projeto usa o framework Arduino via PlatformIO e depende de:

- `ArduinoJson` `^7.2.2`

## Build e upload

Projeto configurado para `esp32dev` no PlatformIO.

Comandos úteis:

```bash
pio run
pio run --target upload
pio device monitor
```

## Estrutura do repositório

- `src/main.cpp`: firmware principal;
- `platformio.ini`: configuração do ambiente de build;
- `diagram.json` e `wokwi.toml`: suporte ao Wokwi;
- `lib/`, `include/`, `test/`: pastas padrão do projeto.

## Observações

As URLs das APIs e algumas credenciais de teste estão definidas diretamente no código-fonte. Antes de uso em produção, vale mover esses valores para configuração externa e revisar as chaves de acesso.
