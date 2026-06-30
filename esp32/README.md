# Firmware do SmokeGuard

Firmware do sistema **SmokeGuard**, desenvolvido para a disciplina de **Sistemas Embarcados** no **IFSP — Campus Catanduva**.

O firmware é executado em um ESP32 e realiza a leitura dos sensores, o controle dos LEDs RGB, o processamento dos botões físicos e a comunicação com o servidor por MQTT e HTTP.

## Acesso rápido

- [README principal](https://github.com/lzmeister/Sistemas-Embarcados/blob/main/Readme.md)
- [Documentação técnica](https://github.com/lzmeister/Sistemas-Embarcados/blob/main/documentacao/README.md)
- [Código principal do ESP32](https://github.com/lzmeister/Sistemas-Embarcados/blob/main/esp32/smokeguard_freertos_tcp_direto/smokeguard_freertos_tcp_direto.ino)
- [Exemplo de credenciais](https://github.com/lzmeister/Sistemas-Embarcados/blob/main/esp32/smokeguard_freertos_tcp_direto/secrets.example.h)
- [Fluxo do Node-RED](https://github.com/lzmeister/Sistemas-Embarcados/blob/main/node-red/flows.json)
- [API PHP](https://github.com/lzmeister/Sistemas-Embarcados/blob/main/web/api.php)

## Recursos principais

- aquisição de temperatura e umidade pelo sensor DHT22;
- aquisição analógica do nível de fumaça pelo sensor MQ-2;
- execução das tarefas utilizando FreeRTOS;
- comunicação MQTT por WebSockets seguros;
- envio das leituras para uma API PHP por HTTP;
- armazenamento das medições no MySQL;
- controle de dois LEDs RGB;
- leitura de cinco botões físicos;
- operação nos modos manual e automático;
- confirmação dos estados aplicados pelo ESP32;
- reconexão automática ao Wi-Fi e ao broker MQTT.

## Estrutura dos arquivos

```text
esp32/
├── README.md
├── SHA256SUMS
└── smokeguard_freertos_tcp_direto/
    ├── secrets.example.h
    └── smokeguard_freertos_tcp_direto.ino
```

## Arquivo principal

O código principal está localizado em:

[Código Principal](https://github.com/lzmeister/Sistemas-Embarcados/blob/main/esp32/smokeguard_freertos_tcp_direto/smokeguard_freertos_tcp_direto.ino)

## Hardware utilizado

| Componente | Função |
|---|---|
| ESP32 | Processamento e comunicação |
| DHT22 | Medição de temperatura e umidade |
| MQ-2 | Detecção do nível de fumaça |
| Dois LEDs RGB | Sinalização visual |
| Cinco botões | Controle físico do sistema |

## Pinagem do ESP32

### Sensores

| Dispositivo | Sinal | GPIO |
|---|---|---:|
| DHT22 | Dados | GPIO 4 |
| MQ-2 | Saída analógica | GPIO 34 |

### LED RGB 1

| Cor | GPIO |
|---|---:|
| Vermelho | GPIO 25 |
| Verde | GPIO 26 |
| Azul | GPIO 27 |

### LED RGB 2

| Cor | GPIO |
|---|---:|
| Vermelho | GPIO 18 |
| Verde | GPIO 19 |
| Azul | GPIO 21 |

### Botões físicos

| Botão | Função | GPIO |
|---|---|---:|
| Normal | Estado normal | GPIO 22 |
| Atenção | Estado de atenção | GPIO 23 |
| Sprinklers | Acionamento dos sprinklers | GPIO 32 |
| Alerta com sprinklers | Estado de emergência | GPIO 33 |
| Modo | Manual ↔ Automático | GPIO 13 |

Os botões utilizam `INPUT_PULLUP`:

```text
GPIO do ESP32 ── Botão ── GND
```

- botão solto: `HIGH`;
- botão pressionado: `LOW`.

## Modos de operação

### Modo automático

O ESP32 analisa as leituras dos sensores e altera automaticamente a sinalização conforme as condições detectadas.

São monitoradas situações como:

- temperatura acima do limite;
- umidade abaixo do limite;
- presença de fumaça;
- possível situação de incêndio.

### Modo manual

O operador pode controlar a sinalização por meio:

- dos botões físicos;
- do dashboard do Node-RED;
- dos comandos recebidos por MQTT.

O ESP32 confirma o estado aplicado por meio dos tópicos de status.

## Sinalização luminosa

## Sinalização luminosa

| Sinalização | Condição |
|---|---|
| **Verde contínuo** | Operação normal, sem condições de alerta |
| **Verde e vermelho alternantes** | Temperatura elevada, umidade baixa ou presença de fumaça |
| **Vermelho e azul alternantes** | Fumaça combinada com temperatura elevada ou umidade baixa, indicando possível incêndio e acionamento dos sprinklers |
| **Azul piscante** | Sprinklers acionados manualmente |

## Comunicação MQTT

O ESP32 utiliza MQTT por WebSockets seguros para trocar informações com o Mosquitto e o Node-RED.

### Tópicos utilizados

| Tópico | Direção | Função |
|---|---|---|
| `smokeguard/sensores` | ESP32 → Node-RED | Publicação das leituras |
| `smokeguard/comandos/leds` | Node-RED → ESP32 | Comandos dos LEDs |
| `smokeguard/status/leds` | ESP32 → Node-RED | Estado confirmado dos LEDs |
| `smokeguard/comandos/modo` | Node-RED → ESP32 | Alteração do modo |
| `smokeguard/status/modo` | ESP32 → Node-RED | Confirmação do modo atual |

## Comunicação HTTP

Além do MQTT, o ESP32 envia as leituras para a API PHP:

```text
ESP32 ── HTTP GET ──► API PHP ──► MySQL
```

Os dados enviados incluem:

- temperatura;
- umidade;
- nível de fumaça.

## Dependências

O firmware utiliza:

```cpp
#include <WiFi.h>
#include <esp_crt_bundle.h>
#include <DHT.h>
#include "mqtt_client.h"
#include "secrets.h"
```

Antes da compilação, verifique:

- suporte à placa ESP32 instalado na Arduino IDE;
- biblioteca utilizada pelo sensor DHT instalada;
- placa ESP32 correta selecionada;
- porta serial correta selecionada.

## Configuração das credenciais

Para configurar o firmware:

1. abra a pasta `smokeguard_freertos_tcp_direto`;
2. copie o arquivo `secrets.example.h`;
3. renomeie a cópia para `secrets.h`;
4. preencha as credenciais de Wi-Fi e MQTT;
5. mantenha `secrets.h` fora do controle de versão.

Exemplo:

```cpp
#define WIFI_SSID "SEU_SSID_WIFI"
#define WIFI_PASSWORD "SUA_SENHA_WIFI"

#define MQTT_USERNAME "SEU_USUARIO_MQTT"
#define MQTT_PASSWORD "SUA_SENHA_MQTT"
```

> Nunca coloque credenciais reais dentro de `secrets.example.h`.

## Compilação e envio

1. abra `smokeguard_freertos_tcp_direto.ino`;
2. verifique se `secrets.h` está na mesma pasta do arquivo `.ino`;
3. selecione a placa ESP32 correspondente;
4. selecione a porta serial;
5. clique em **Verificar**;
6. corrija eventuais erros de biblioteca ou configuração;
7. clique em **Carregar**;
8. abra o monitor serial para acompanhar a inicialização.

## Verificação dos arquivos

O arquivo `SHA256SUMS` permite conferir a integridade dos arquivos do firmware.

No Linux:

```bash
cd esp32
sha256sum -c SHA256SUMS
```

O resultado esperado deve indicar:

```text
README.md: OK
smokeguard_freertos_tcp_direto/secrets.example.h: OK
smokeguard_freertos_tcp_direto/smokeguard_freertos_tcp_direto.ino: OK
```

> Após alterar qualquer um desses arquivos, o conteúdo de `SHA256SUMS` deverá ser recalculado.

## Funcionamento esperado

Após a inicialização, o ESP32 deve:

1. conectar-se à rede Wi-Fi;
2. conectar-se ao broker MQTT;
3. assinar os tópicos de comandos;
4. publicar o modo atual;
5. publicar o estado dos LEDs;
6. iniciar a leitura periódica dos sensores;
7. enviar os dados ao Node-RED;
8. enviar as medições para a API PHP.

## Solução de problemas

### ESP32 não conecta ao Wi-Fi

- verifique `WIFI_SSID`;
- verifique `WIFI_PASSWORD`;
- confirme se a rede está disponível;
- confirme se o ESP32 possui sinal suficiente.

### MQTT não conecta

- verifique o usuário e a senha MQTT;
- confira o endereço do broker;
- confira a porta configurada;
- verifique se o Mosquitto está ativo;
- confira as permissões do usuário na ACL.

### DHT22 não apresenta valores

- confira a ligação no GPIO 4;
- verifique a alimentação;
- confirme a biblioteca instalada;
- verifique se o sensor necessita de resistor pull-up.

### MQ-2 apresenta valores incorretos

- aguarde o aquecimento do sensor;
- confira a ligação da saída analógica;
- confirme que a tensão aplicada ao GPIO 34 não supera 3,3 V;
- ajuste os limites utilizados no modo automático.

## Observação

O SmokeGuard é um protótipo acadêmico e não substitui detectores, alarmes ou sistemas de combate a incêndio certificados.

## Instituição

Projeto desenvolvido para a disciplina de **Sistemas Embarcados** no **Instituto Federal de Educação, Ciência e Tecnologia de São Paulo — IFSP, Campus Catanduva**.
