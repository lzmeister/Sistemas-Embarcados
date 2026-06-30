# Documentação do SmokeGuard

Esta pasta reúne a documentação técnica e operacional do **SmokeGuard**, sistema IoT desenvolvido no **IFSP — Campus Catanduva** para monitoramento de temperatura, umidade e presença de fumaça.

## IFSP — Campus Catanduva** para monitoramento de temperatura, umidade e presença de fumaça.

## Acesso rápido

- [README principal do projeto](../README.md)
- [Firmware do ESP32](../esp32/)
- [Código principal do ESP32](../esp32/smokeguard_freertos_tcp_direto/smokeguard_freertos_tcp_direto.ino)
- [Exemplo de configuração de credenciais](../esp32/smokeguard_freertos_tcp_direto/secrets.example.h)
- [Fluxo do Node-RED](../node-red/flows.json)
- [Aplicação web em PHP](../web/)
- [Estrutura do banco de dados](../database/smokeguard_schema.sql)

## Arquitetura do sistema

O SmokeGuard é formado pelos seguintes componentes:

1. **ESP32**, responsável pela leitura dos sensores e controle dos LEDs;
2. **DHT22**, utilizado para medir temperatura e umidade;
3. **MQ-2**, utilizado para detectar a presença de fumaça;
4. **Mosquitto**, utilizado como broker MQTT;
5. **Node-RED**, responsável pela supervisão, comandos e dashboard;
6. **API PHP**, responsável pelo recebimento das leituras;
7. **MySQL**, utilizado para armazenar o histórico dos dados;
8. **Cloudflare Tunnel**, utilizado para disponibilizar os serviços externos.

## Comunicação do sistema

O ESP32 utiliza dois meios principais de comunicação:

- **MQTT:** envio das leituras e recebimento dos comandos;
- **HTTP GET:** envio dos dados para a API PHP e armazenamento no MySQL.

### Tópicos MQTT

```text
smokeguard/sensores
smokeguard/comandos/leds
smokeguard/status/leds
smokeguard/comandos/modo
smokeguard/status/modo
