# ESP32 Workspace

Este diretório reúne os projetos do workspace voltados ao farol de aeroporto em ESP32.

## Projetos

- [esp32-farol-aeroporto](esp32-farol-aeroporto/README.md): firmware principal do farol, com interface web, modo real/simulado e controle automático baseado em meteorologia e horário.
- [esp32-wokwi](esp32-wokwi/): variante de simulação e testes com estrutura PlatformIO/Wokwi.

## Como começar

1. Abra [esp32-farol-aeroporto](esp32-farol-aeroporto/) no VS Code para trabalhar no firmware principal.
2. Use [esp32-wokwi](esp32-wokwi/) quando quiser testar a simulação localmente.
3. Consulte o README dentro de cada projeto para detalhes de execução e comportamento.

## Visão rápida

O firmware principal expõe um servidor HTTP, alterna entre modo real e simulado, pode cair em ponto de acesso quando não há Wi-Fi e controla o farol no GPIO 13. A documentação completa de funcionamento está em [esp32-farol-aeroporto/README.md](esp32-farol-aeroporto/README.md).
