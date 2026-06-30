# SmokeGuard

Sistema IoT para monitoramento de temperatura, umidade e presença de fumaça, desenvolvido no **IFSP — Campus Catanduva**.

O SmokeGuard utiliza um ESP32 para realizar a aquisição dos sensores, enviar informações ao servidor e permitir o acionamento remoto dos indicadores luminosos por meio de MQTT.

## Visão geral

O sistema integra:

- ESP32;
- sensor DHT22;
- sensor de fumaça MQ-2;
- FreeRTOS;
- protocolo MQTT;
- Node-RED;
- Apache e PHP;
- banco de dados MySQL;
- Cloudflare Tunnel;
- dashboard web para supervisão.

## Arquitetura

```text
DHT22 + MQ-2
      │
      ▼
    ESP32
      │
      ├── MQTT por WebSockets seguros
      │        │
      │        ▼
      │     Mosquitto
      │        │
      │        ▼
      │     Node-RED
      │        │
      │        ▼
      │     Dashboard
      │
      └── HTTP GET
               │
               ▼
            API PHP
               │
               ▼
             MySQL
```

## Funcionamento

O ESP32 realiza as seguintes funções:

- leitura de temperatura e umidade pelo DHT22;
- leitura analógica do nível de fumaça pelo MQ-2;
- execução das tarefas com FreeRTOS;
- publicação periódica dos dados por MQTT;
- envio das leituras para a API PHP;
- recebimento de comandos enviados pelo Node-RED;
- confirmação dos estados aplicados;
- operação nos modos manual e automático.

## Sinalização luminosa

Os LEDs físicos indicam o estado atual do sistema:

- **verde:** operação normal;
- **vermelho piscando:** alerta de fumaça ou possível incêndio;
- **azul:** sistema de sprinklers acionado.

## Tópicos MQTT

```text
smokeguard/sensores
smokeguard/comandos/leds
smokeguard/status/leds
smokeguard/comandos/modo
smokeguard/status/modo
```

## Estrutura do repositório

```text
SmokeGuard/
├── database/
│   └── smokeguard_schema.sql
├── docs/
├── esp32/
│   ├── README.md
│   ├── SHA256SUMS
│   └── smokeguard_freertos_tcp_direto/
│       ├── secrets.example.h
│       └── smokeguard_freertos_tcp_direto.ino
├── node-red/
│   └── flows.json
├── web/
│   ├── api.php
│   ├── index.php
│   └── smokeguard_db.example.php
├── .gitignore
└── README.md
```

## Firmware ESP32

O firmware principal está localizado em:

```text
esp32/smokeguard_freertos_tcp_direto/
```

Antes da compilação:

1. copie `secrets.example.h` para `secrets.h`;
2. preencha as credenciais de Wi-Fi e MQTT;
3. mantenha o arquivo `secrets.h` fora do Git;
4. compile o projeto pela Arduino IDE;
5. envie o firmware ao ESP32.

O arquivo real de credenciais não faz parte deste repositório.

## Node-RED

O fluxo está disponível em:

```text
node-red/flows.json
```

Para utilizá-lo:

1. instale o Node-RED;
2. instale os pacotes necessários para dashboard e MySQL;
3. importe o arquivo `flows.json`;
4. configure as credenciais do broker MQTT;
5. configure a conexão com o banco MySQL;
6. faça o deploy.

## Banco de dados

O script de criação da estrutura está disponível em:

```text
database/smokeguard_schema.sql
```

A tabela de leituras armazena:

- identificador da leitura;
- nível de fumaça;
- temperatura;
- umidade;
- data e hora.

## Aplicação web

Os arquivos da aplicação PHP estão em:

```text
web/
```

O arquivo `smokeguard_db.example.php` contém somente valores de exemplo.

A configuração real do banco de dados deve permanecer fora do repositório.

## Links do sistema

- Página web Apache: https://embarcados.lzmeister.uk/
- Aplicação web API: https://embarcados.lzmeister.uk/api.php
- Node-RED: https://nodered.lzmeister.uk/
- Dashboard: https://nodered.lzmeister.uk/ui

## Segurança

Este repositório não inclui:

- senhas de Wi-Fi;
- credenciais MQTT;
- credenciais do MySQL;
- arquivos de credenciais do Node-RED;
- chaves privadas;
- certificados privados.

## Instituição

Projeto desenvolvido no **Instituto Federal de Educação, Ciência e Tecnologia de São Paulo — IFSP, Campus Catanduva**.
