# ESP32 Farol Aeroporto

Firmware para ESP32 que controla um farol com base em condições meteorológicas e horário local. O projeto expõe uma interface web para operar o sistema em modo real ou simulado, além de manter um fallback em ponto de acesso quando o Wi-Fi não está disponível.

## Visão geral

O firmware busca dados de meteorologia para decidir se o farol deve ficar ligado ou desligado:

- Visibilidade e teto operacional vêm de um METAR consultado pelo código.
- Nascer e pôr do sol são obtidos pela API da OpenWeather usando a latitude e longitude extraídas do METAR.
- A lógica liga o farol automaticamente quando está de noite ou quando há condição IMC.
- Existe override manual para ligar, desligar ou devolver o controle para o modo automático.

## Recursos atuais

- Servidor HTTP na porta 80 com página de controle.
- Modo `real` e modo `sim` para testar a lógica sem depender dos dados externos.
- Fallback para AP com SSID `Farol-Aeroporto` e senha `12345678`.
- mDNS com o nome `farol-aeroporto.local` quando conectado ao Wi-Fi.
- Atualização periódica dos dados a cada 60 segundos.
- LED do farol no GPIO 13.

## Requisitos

- ESP32 com framework Arduino.
- PlatformIO.
- Conectividade com Wi-Fi para o modo real.
- Acesso aos endpoints usados para METAR e OpenWeather.

## Como executar

1. Abra o projeto no VS Code com PlatformIO instalado.
2. Compile e grave o firmware no ESP32 com o ambiente `esp32dev`.
3. Se houver conexão Wi-Fi, o dispositivo tenta sincronizar o horário via NTP e expõe a interface web.
4. Se não conseguir conectar, ele sobe um AP chamado `Farol-Aeroporto`.

## Interface web

A interface permite:

- Informar o código ICAO do aeroporto.
- Alternar entre modo real e simulado.
- Ajustar visibilidade, teto, nascer do sol e pôr do sol no modo simulado.
- Ligar, desligar ou devolver o controle ao modo automático.

## Endpoints HTTP

- `GET /estado` retorna o estado atual do sistema em JSON.
- `GET /on` liga o farol em modo manual.
- `GET /off` desliga o farol em modo manual.
- `GET /auto` devolve o controle ao automático.
- `GET /icao?id=SBKP` define o aeroporto e recarrega os dados.
- `GET /modo?v=real|sim` alterna o modo de operação.
- `GET /sim?visib=9000&ceiling=3500&sunrise=05:48&sunset=18:12` atualiza os valores simulados.

## Estrutura do projeto

- `src/main.cpp`: lógica principal do firmware, servidor HTTP e controle do farol.
- `platformio.ini`: configuração do ambiente PlatformIO.
- `diagram.json` e `wokwi.toml`: suporte para simulação no Wokwi.

## Estado do projeto

O projeto já possui a base funcional de controle, interface web e aquisição de dados. O próximo passo natural é refinar a robustez das integrações externas e melhorar a experiência da interface.
