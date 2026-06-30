# SmokeGuard

Sistema IoT para monitoramento de temperatura, umidade e presença de fumaça, desenvolvido para a disciplina de **Sistemas Embarcados** no **IFSP — Campus Catanduva**.

O SmokeGuard utiliza um ESP32 para realizar a aquisição dos sensores, enviar informações ao servidor e permitir o monitoramento e o acionamento remoto dos indicadores luminosos por meio de MQTT.

## Acesso rápido

- [Documentação técnica](https://github.com/lzmeister/Sistemas-Embarcados/tree/main/docs)
- [Firmware do ESP32](https://github.com/lzmeister/Sistemas-Embarcados/tree/main/esp32)
- [Código principal do ESP32](https://github.com/lzmeister/Sistemas-Embarcados/blob/main/esp32/smokeguard_freertos_tcp_direto/smokeguard_freertos_tcp_direto.ino)
- [Exemplo de configuração de credenciais](https://github.com/lzmeister/Sistemas-Embarcados/blob/main/esp32/smokeguard_freertos_tcp_direto/secrets.example.h)
- [Fluxo do Node-RED](https://github.com/lzmeister/Sistemas-Embarcados/blob/main/node-red/flows.json)
- [Aplicação web em PHP](https://github.com/lzmeister/Sistemas-Embarcados/tree/main/web)
- [API PHP](https://github.com/lzmeister/Sistemas-Embarcados/blob/main/web/api.php)
- [Estrutura do banco de dados](https://github.com/lzmeister/Sistemas-Embarcados/blob/main/database/smokeguard_schema.sql)

## Visão geral

O sistema integra:

- ESP32;
- sensor de temperatura e umidade DHT22;
- sensor de fumaça MQ-2;
- sistema operacional FreeRTOS;
- protocolo MQTT;
- broker Mosquitto;
- Node-RED;
- dashboard de supervisão e controle;
- servidor Apache;
- aplicação em PHP;
- banco de dados MySQL;
- Cloudflare Tunnel.

## Componentes do sistema

O SmokeGuard é formado pelos seguintes componentes principais:

1. **ESP32:** responsável pela leitura dos sensores, execução das tarefas, controle dos LEDs e comunicação com o servidor;
2. **DHT22:** utilizado para medir temperatura e umidade;
3. **MQ-2:** utilizado para detectar a presença de fumaça;
4. **Mosquitto:** utilizado como broker MQTT;
5. **Node-RED:** responsável pela supervisão, envio de comandos e exibição do dashboard;
6. **API PHP:** responsável pelo recebimento das leituras enviadas pelo ESP32;
7. **MySQL:** utilizado para armazenar o histórico das medições;
8. **Cloudflare Tunnel:** utilizado para disponibilizar externamente a aplicação web e o Node-RED.

## Arquitetura do sistema

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

## Comunicação do sistema

O ESP32 utiliza dois meios principais de comunicação:

- **MQTT:** utilizado para publicar as leituras dos sensores, receber comandos e informar os estados aplicados;
- **HTTP GET:** utilizado para enviar as medições para a API PHP, que realiza o armazenamento no MySQL.

A comunicação MQTT externa é realizada por meio de WebSockets seguros.

## Funcionamento

O ESP32 realiza as seguintes funções:

- leitura de temperatura e umidade pelo DHT22;
- leitura analógica do nível de fumaça pelo MQ-2;
- execução simultânea das tarefas com FreeRTOS;
- publicação periódica dos dados por MQTT;
- envio das leituras para a API PHP;
- recebimento de comandos enviados pelo Node-RED;
- recebimento de comandos por botões físicos;
- controle dos LEDs RGB;
- confirmação dos estados aplicados;
- operação nos modos manual e automático.

## Modos de operação

### Modo automático

No modo automático, o ESP32 analisa as leituras dos sensores e altera a sinalização dos LEDs conforme as condições detectadas.

Esse modo é utilizado para identificar automaticamente situações como:

- temperatura acima do limite configurado;
- umidade abaixo do limite configurado;
- presença de fumaça;
- possível situação de incêndio.

### Modo manual

No modo manual, o operador pode alterar os estados de sinalização por meio:

- dos comandos disponíveis no dashboard do Node-RED;
- dos botões físicos conectados ao ESP32.

O ESP32 publica o estado aplicado para que o dashboard permaneça sincronizado com o sistema físico.

## Sinalização luminosa

Os LEDs físicos indicam o estado atual do sistema:

| Sinalização | Condição representada |
|---|---|
| **Verde contínuo** | Operação normal |
| **Verde e vermelho alternantes** | Temperatura acima do limite ou umidade baixa |
| **Vermelho e azul alternantes** | Alerta de fumaça ou possível incêndio com acionamento dos sprinklers |
| **Azul piscante** | Sistema de sprinklers acionado |

## Tópicos MQTT

O sistema utiliza os seguintes tópicos:

```text
smokeguard/sensores
smokeguard/comandos/leds
smokeguard/status/leds
smokeguard/comandos/modo
smokeguard/status/modo
```

### Descrição dos tópicos

| Tópico | Função |
|---|---|
| `smokeguard/sensores` | Publicação das leituras de temperatura, umidade e fumaça |
| `smokeguard/comandos/leds` | Envio de comandos para alteração dos LEDs |
| `smokeguard/status/leds` | Confirmação do estado atual dos LEDs |
| `smokeguard/comandos/modo` | Envio do modo de operação desejado |
| `smokeguard/status/modo` | Confirmação do modo de operação atual |

## Estrutura do repositório

```text
SmokeGuard/
├── database/
│   └── smokeguard_schema.sql
├── docs/
│   └── README.md
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

[Código principal do ESP](https://github.com/lzmeister/Sistemas-Embarcados/blob/main/esp32/smokeguard_freertos_tcp_direto/smokeguard_freertos_tcp_direto.ino)

Para utilizar o firmware:

1. acesse a pasta do [Firmware do ESP32](https://github.com/lzmeister/Sistemas-Embarcados/tree/main/esp32);
2. copie `secrets.example.h`;
3. renomeie a cópia para `secrets.h`;
4. preencha as credenciais de Wi-Fi e MQTT;
5. abra o arquivo `.ino`;
6. selecione a placa ESP32 correspondente;
7. compile o projeto;
8. envie o firmware ao ESP32.


## Node-RED

O fluxo utilizado pelo sistema está disponível em:

[Fluxo do Node-RED](https://github.com/lzmeister/Sistemas-Embarcados/tree/main/node-red)

Para utilizá-lo:

1. instale o Node-RED;
2. instale os pacotes necessários para o dashboard;
3. instale o pacote de integração com o MySQL;
4. importe o arquivo `flows.json`;
5. configure as credenciais do broker MQTT;
6. configure a conexão com o banco MySQL;
7. revise os endereços e portas utilizados;
8. realize o deploy.

O Node-RED é responsável por:

- receber as leituras publicadas pelo ESP32;
- exibir as informações em tempo real;
- apresentar gráficos;
- consultar o histórico armazenado;
- enviar comandos para os LEDs;
- selecionar o modo manual ou automático;
- acompanhar os estados confirmados pelo ESP32.

## Banco de dados

O script de criação da estrutura está disponível em:

[Estrutura do banco de dados](https://github.com/lzmeister/Sistemas-Embarcados/blob/main/database/smokeguard_schema.sql)

O banco de dados utilizado pelo sistema é:

```text
sistema_iot
```

A tabela `leituras` armazena:

- identificador da leitura;
- nível de fumaça;
- temperatura;
- umidade;
- data e hora da medição.

## Aplicação web

Os arquivos da aplicação PHP estão localizados em:

[Aplicação Web](https://github.com/lzmeister/Sistemas-Embarcados/tree/main/web)

A aplicação contém:

- uma página web para visualização das informações;
- uma API PHP para recebimento das leituras;
- um arquivo de exemplo para configuração do MySQL.

O arquivo:

```text
web/smokeguard_db.example.php
```

contém somente valores de exemplo.


## Links do sistema

- [Página web Apache](https://embarcados.lzmeister.uk/)
- [API da aplicação](https://embarcados.lzmeister.uk/api.php)
- [Node-RED](https://nodered.lzmeister.uk/)
- [Dashboard](https://nodered.lzmeister.uk/ui)

## Segurança

Este repositório não inclui:

- senhas de Wi-Fi;
- credenciais MQTT;
- credenciais do MySQL;
- arquivos de credenciais do Node-RED;
- tokens privados;
- chaves privadas;
- certificados privados.

Os arquivos `.gitignore`, `secrets.example.h` e `smokeguard_db.example.php` foram preparados para permitir a configuração do sistema sem publicar credenciais reais.

## Documentação complementar

A pasta [docs](https://github.com/lzmeister/Sistemas-Embarcados/tree/main/docs) reúne a documentação técnica e operacional do projeto.

Ela receberá:

- diagramas detalhados da arquitetura;
- esquemas de ligação dos sensores;
- tabela de pinagem do ESP32;
- esquema de ligação dos LEDs e botões;
- instruções de instalação do servidor;
- configuração do Mosquitto;
- instruções de importação do Node-RED;
- manual de operação;
- registros de testes;
- imagens do protótipo.

## Instituição

Projeto desenvolvido para a disciplina de **Sistemas Embarcados** no **Instituto Federal de Educação, Ciência e Tecnologia de São Paulo — Campus Catanduva**.
