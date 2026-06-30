# Firmware do SmokeGuard

Firmware atual e funcional do sistema SmokeGuard, desenvolvido no IFSP — Campus Catanduva.

## Recursos principais

- aquisição de temperatura e umidade pelo sensor DHT22;
- aquisição do nível de fumaça pelo sensor MQ-2;
- execução das tarefas com FreeRTOS;
- comunicação MQTT por WebSockets seguros;
- publicação das leituras no tópico `smokeguard/sensores`;
- recebimento de comandos para os LEDs;
- controle dos modos MANUAL e AUTOMÁTICO;
- confirmação dos estados aplicados pelo ESP32;
- envio das leituras para a API PHP;
- armazenamento das informações no banco MySQL.

## Arquivo principal

```text
smokeguard_freertos_tcp_direto/
├── smokeguard_freertos_tcp_direto.ino
└── secrets.example.h
```

## Credenciais

O arquivo `secrets.h` real não está incluído no repositório.

Para utilizar o firmware:

1. copie `secrets.example.h`;
2. renomeie para `secrets.h`;
3. preencha as credenciais de Wi-Fi e MQTT;
4. compile e carregue o firmware pela Arduino IDE.

## Tópicos MQTT

```text
smokeguard/sensores
smokeguard/comandos/leds
smokeguard/status/leds
smokeguard/comandos/modo
smokeguard/status/modo
```
