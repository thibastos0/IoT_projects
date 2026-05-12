# ESP32 Farol Aeroporto

Projeto em fase inicial para um farol inteligente baseado em ESP32, com foco em automação para uso em ambiente aeroportuário.

A ideia principal é consultar APIs externas para obter dados atualizados do aeroporto e do clima, usando essas informações para definir o comportamento do farol e os horários de operação:

- OpenWeather API para obter dados de nascer e pôr do sol.
- AviationWeather para buscar informações do aeroporto, incluindo latitude e longitude.

Com a latitude e longitude do aeroporto, o projeto poderá calcular com mais precisão os horários de nascer e pôr do sol e tomar decisões automáticas com base nesses dados.

## Objetivo

Este repositório vai concentrar o desenvolvimento do firmware do ESP32, a integração com as APIs e a lógica de automação do farol.

## Status atual

- Estrutura inicial do projeto criada.
- Servidor web básico em desenvolvimento.
- Integração com APIs ainda não implementada.

## Próximos passos

- Integrar com a API da OpenWeather.
- Integrar com a API da AviationWeather.
- Obter e armazenar latitude e longitude do aeroporto.
- Calcular os horários de nascer e pôr do sol.
- Ajustar a lógica do farol com base nos dados coletados.

## Observação

Este projeto ainda está em construção e a estrutura pode mudar conforme a implementação evoluir.
