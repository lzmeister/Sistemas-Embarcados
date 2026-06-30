#include <WiFi.h>
#include <esp_crt_bundle.h>
#include <DHT.h>
#include "mqtt_client.h"
#include "secrets.h"

// ============================================================
// CONFIGURACAO DE REDE
// ============================================================

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// API PHP executada no servidor físico local
const IPAddress IP_SERVIDOR_API(192, 168, 1, 150);
const uint16_t PORTA_SERVIDOR_API = 80;
const char* CAMINHO_API_PHP = "/api.php";

// Tempo inicial para estabilização do Wi-Fi e da pilha TCP/IP
const unsigned long ESPERA_INICIAL_REDE_MS = 10000;

// Tentativas feitas somente antes do envio do GET.
// Depois que o GET é transmitido, ele não é repetido automaticamente,
// evitando a possibilidade de gravar a mesma leitura duas vezes.
const uint8_t MAX_TENTATIVAS_CONEXAO_API = 5;
const unsigned long INTERVALO_TENTATIVAS_API_MS = 1000;
const unsigned long TIMEOUT_RESPOSTA_API_MS = 5000;

// ============================================================
// SENSORES
// ============================================================

#define MQ_PIN 34
#define DHT_PIN 4
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

// ============================================================
// DOIS LEDS RGB DE CATODO COMUM
// ============================================================

// LED RGB 1
#define LED1_VERMELHO 25
#define LED1_VERDE    26
#define LED1_AZUL     27

// LED RGB 2
#define LED2_VERMELHO 18
#define LED2_VERDE    19
#define LED2_AZUL     21

// ============================================================
// CINCO BOTOES FISICOS
// Ligacao: GPIO -> botao -> GND
// Leitura com INPUT_PULLUP: solto = HIGH, pressionado = LOW
// ============================================================

#define BOTAO_NORMAL              22
#define BOTAO_ATENCAO             23
#define BOTAO_SPRINKLERS          32
#define BOTAO_ALERTA_SPRINKLERS   33
#define BOTAO_MODO                13

// ============================================================
// TOPICOS MQTT
// ============================================================

const char* TOPICO_SENSORES = "smokeguard/sensores";
const char* TOPICO_COMANDOS_LEDS = "smokeguard/comandos/leds";
const char* TOPICO_STATUS_LEDS = "smokeguard/status/leds";
const char* TOPICO_COMANDOS_MODO = "smokeguard/comandos/modo";
const char* TOPICO_STATUS_MODO = "smokeguard/status/modo";

// ============================================================
// MQTT
// ============================================================

esp_mqtt_client_handle_t client = nullptr;
bool mqtt_conectado = false;

// ============================================================
// PERÍODO DE LEITURA DOS SENSORES
// ============================================================

const unsigned long INTERVALO_LEITURA_SENSORES = 3000;

// ============================================================
// ESTADOS VISUAIS DOS LEDS
// ============================================================

enum EstadoLeds {
    ESTADO_NORMAL,
    ESTADO_ATENCAO,
    ESTADO_SPRINKLERS,
    ESTADO_ALERTA_SPRINKLERS
};

EstadoLeds estadoAtualLeds = ESTADO_NORMAL;

unsigned long ultimaAlternanciaLeds = 0;
bool faseAlternanciaLeds = false;

// Identificadores das tarefas FreeRTOS
TaskHandle_t tarefaLedsHandle = nullptr;
TaskHandle_t tarefaSensoresHandle = nullptr;
TaskHandle_t tarefaBotoesHandle = nullptr;

// ============================================================
// MODO DE CONTROLE
// O modo automatico e o padrao de seguranca no inicio.
// O ultimo modo retido no MQTT substitui esse valor ao conectar.
// ============================================================

enum ModoControle {
    MODO_MANUAL,
    MODO_AUTOMATICO
};

ModoControle modoAtual = MODO_AUTOMATICO;

// ============================================================
// ESTRUTURA DE DEBOUNCE DOS BOTOES
// ============================================================

struct Botao {
    uint8_t pino;
    bool leituraAnterior;
    bool estadoEstavel;
    unsigned long instanteUltimaMudanca;
};

const unsigned long TEMPO_DEBOUNCE = 50;

Botao botaoNormal = {
    BOTAO_NORMAL, HIGH, HIGH, 0
};

Botao botaoAtencao = {
    BOTAO_ATENCAO, HIGH, HIGH, 0
};

Botao botaoSprinklers = {
    BOTAO_SPRINKLERS, HIGH, HIGH, 0
};

Botao botaoAlertaSprinklers = {
    BOTAO_ALERTA_SPRINKLERS, HIGH, HIGH, 0
};

Botao botaoModo = {
    BOTAO_MODO, HIGH, HIGH, 0
};

// ============================================================
// CONTROLE INDIVIDUAL DOS LEDS
// ============================================================

void definirLed1(bool vermelho, bool verde, bool azul) {
    digitalWrite(LED1_VERMELHO, vermelho ? HIGH : LOW);
    digitalWrite(LED1_VERDE, verde ? HIGH : LOW);
    digitalWrite(LED1_AZUL, azul ? HIGH : LOW);
}

void definirLed2(bool vermelho, bool verde, bool azul) {
    digitalWrite(LED2_VERMELHO, vermelho ? HIGH : LOW);
    digitalWrite(LED2_VERDE, verde ? HIGH : LOW);
    digitalWrite(LED2_AZUL, azul ? HIGH : LOW);
}

void apagarLeds() {
    definirLed1(false, false, false);
    definirLed2(false, false, false);
}

// ============================================================
// PADROES VISUAIS DOS LEDS
// ============================================================

void aplicarPadraoLeds() {
    switch (estadoAtualLeds) {
        case ESTADO_NORMAL:
            definirLed1(false, true, false);
            definirLed2(false, true, false);
            break;

        case ESTADO_ATENCAO:
            if (!faseAlternanciaLeds) {
                definirLed1(true, false, false);
                definirLed2(false, true, false);
            } else {
                definirLed1(false, true, false);
                definirLed2(true, false, false);
            }
            break;

        case ESTADO_SPRINKLERS:
            if (!faseAlternanciaLeds) {
                definirLed1(false, false, true);
                definirLed2(false, false, true);
            } else {
                apagarLeds();
            }
            break;

        case ESTADO_ALERTA_SPRINKLERS:
            if (!faseAlternanciaLeds) {
                definirLed1(true, false, false);
                definirLed2(false, false, true);
            } else {
                definirLed1(false, false, true);
                definirLed2(true, false, false);
            }
            break;
    }
}

void alterarEstadoLeds(EstadoLeds novoEstado) {
    estadoAtualLeds = novoEstado;
    faseAlternanciaLeds = false;
    ultimaAlternanciaLeds = millis();
    aplicarPadraoLeds();
}

void atualizarLeds() {
    const unsigned long agora = millis();
    unsigned long intervaloAlternancia = 0;

    switch (estadoAtualLeds) {
        case ESTADO_NORMAL:
            return;

        case ESTADO_ATENCAO:
            intervaloAlternancia = 250;
            break;

        case ESTADO_SPRINKLERS:
            intervaloAlternancia = 350;
            break;

        case ESTADO_ALERTA_SPRINKLERS:
            intervaloAlternancia = 250;
            break;
    }

    if (agora - ultimaAlternanciaLeds >= intervaloAlternancia) {
        ultimaAlternanciaLeds = agora;
        faseAlternanciaLeds = !faseAlternanciaLeds;
        aplicarPadraoLeds();
    }
}

// ============================================================
// TAREFA FREERTOS — CONTROLE DOS LEDS
// Executada periodicamente a cada 20 ms
// ============================================================

void tarefaControleLeds(void* parametro) {
    (void)parametro;

    TickType_t ultimoDespertar = xTaskGetTickCount();

    Serial.println("Tarefa FreeRTOS dos LEDs iniciada.");

    for (;;) {
        atualizarLeds();

        vTaskDelayUntil(
            &ultimoDespertar,
            pdMS_TO_TICKS(20)
        );
    }
}

// ============================================================
// CONVERSOES DE ESTADO
// ============================================================

const char* obterNomeEstadoLeds(EstadoLeds estado) {
    switch (estado) {
        case ESTADO_NORMAL:
            return "NORMAL";

        case ESTADO_ATENCAO:
            return "ATENCAO";

        case ESTADO_SPRINKLERS:
            return "SPRINKLERS";

        case ESTADO_ALERTA_SPRINKLERS:
            return "ALERTA_SPRINKLERS";

        default:
            return "DESCONHECIDO";
    }
}

const char* obterNomeModo(ModoControle modo) {
    return modo == MODO_MANUAL ? "MANUAL" : "AUTOMATICO";
}

// ============================================================
// PUBLICACAO DOS STATUS
// ============================================================

void publicarStatusLeds(const char* origem) {
    if (!mqtt_conectado || client == nullptr) {
        return;
    }

    String payload =
        "{\"estado\":\"" +
        String(obterNomeEstadoLeds(estadoAtualLeds)) +
        "\",\"origem\":\"" +
        String(origem) +
        "\",\"confirmado\":true}";

    const int idMensagem = esp_mqtt_client_publish(
        client,
        TOPICO_STATUS_LEDS,
        payload.c_str(),
        0,
        1,
        1
    );

    Serial.print("Status dos LEDs publicado: ");
    Serial.println(payload);
    Serial.print("ID da mensagem de status: ");
    Serial.println(idMensagem);
}

void publicarStatusModo(const char* origem) {
    if (!mqtt_conectado || client == nullptr) {
        return;
    }

    String payload =
        "{\"modo\":\"" +
        String(obterNomeModo(modoAtual)) +
        "\",\"origem\":\"" +
        String(origem) +
        "\",\"confirmado\":true}";

    const int idMensagem = esp_mqtt_client_publish(
        client,
        TOPICO_STATUS_MODO,
        payload.c_str(),
        0,
        1,
        1
    );

    Serial.print("Status do modo publicado: ");
    Serial.println(payload);
    Serial.print("ID da mensagem de modo: ");
    Serial.println(idMensagem);
}

// ============================================================
// PROCESSAMENTO DOS COMANDOS DOS LEDS
// ============================================================

bool processarComandoLeds(String comando, const char* origem) {
    comando.trim();
    comando.toUpperCase();

    if (comando == "NORMAL") {
        alterarEstadoLeds(ESTADO_NORMAL);
    }
    else if (comando == "ATENCAO") {
        alterarEstadoLeds(ESTADO_ATENCAO);
    }
    else if (comando == "SPRINKLERS" || comando == "SPRINKLER") {
        alterarEstadoLeds(ESTADO_SPRINKLERS);
    }
    else if (
        comando == "ALERTA_SPRINKLERS" ||
        comando == "ALERTA"
    ) {
        alterarEstadoLeds(ESTADO_ALERTA_SPRINKLERS);
    }
    else {
        Serial.print("Comando de LEDs nao reconhecido: ");
        Serial.println(comando);
        return false;
    }

    Serial.print("Estado dos LEDs aplicado: ");
    Serial.println(obterNomeEstadoLeds(estadoAtualLeds));
    Serial.print("Origem do comando: ");
    Serial.println(origem);

    publicarStatusLeds(origem);
    return true;
}

// ============================================================
// PROCESSAMENTO DOS COMANDOS DE MODO
// ============================================================

bool processarComandoModo(String comando, const char* origem) {
    comando.trim();
    comando.toUpperCase();

    if (comando == "MANUAL") {
        modoAtual = MODO_MANUAL;
    }
    else if (comando == "AUTOMATICO" || comando == "AUTOMÁTICO") {
        modoAtual = MODO_AUTOMATICO;
    }
    else {
        Serial.print("Comando de modo nao reconhecido: ");
        Serial.println(comando);
        return false;
    }

    Serial.print("Modo de controle aplicado: ");
    Serial.println(obterNomeModo(modoAtual));
    Serial.print("Origem do modo: ");
    Serial.println(origem);

    publicarStatusModo(origem);
    return true;
}

// ============================================================
// LEITURA COM DEBOUNCE
// Retorna true apenas uma vez no instante do pressionamento.
// ============================================================

bool foiPressionado(Botao& botao) {
    const bool leituraAtual = digitalRead(botao.pino);
    const unsigned long agora = millis();

    if (leituraAtual != botao.leituraAnterior) {
        botao.leituraAnterior = leituraAtual;
        botao.instanteUltimaMudanca = agora;
    }

    if (
        agora - botao.instanteUltimaMudanca >= TEMPO_DEBOUNCE &&
        leituraAtual != botao.estadoEstavel
    ) {
        botao.estadoEstavel = leituraAtual;

        if (botao.estadoEstavel == LOW) {
            return true;
        }
    }

    return false;
}

// ============================================================
// CONTROLE PELOS BOTOES FISICOS
// ============================================================

void tratarBotoesFisicos() {
    // O botao de modo funciona tanto no manual quanto no automatico.
    if (foiPressionado(botaoModo)) {
        modoAtual = modoAtual == MODO_MANUAL
            ? MODO_AUTOMATICO
            : MODO_MANUAL;

        Serial.print("Botao MODO: ");
        Serial.println(obterNomeModo(modoAtual));

        publicarStatusModo("botao_local");
    }

    const bool normalPressionado = foiPressionado(botaoNormal);
    const bool atencaoPressionada = foiPressionado(botaoAtencao);
    const bool sprinklersPressionado = foiPressionado(botaoSprinklers);
    const bool alertaPressionado = foiPressionado(botaoAlertaSprinklers);

    // Os quatro botoes de estado atuam somente no modo manual.
    if (modoAtual != MODO_MANUAL) {
        if (
            normalPressionado ||
            atencaoPressionada ||
            sprinklersPressionado ||
            alertaPressionado
        ) {
            Serial.println(
                "Comando local ignorado: sistema em modo AUTOMATICO."
            );
        }

        return;
    }

    if (normalPressionado) {
        processarComandoLeds("NORMAL", "botao_local");
    }
    else if (atencaoPressionada) {
        processarComandoLeds("ATENCAO", "botao_local");
    }
    else if (sprinklersPressionado) {
        processarComandoLeds("SPRINKLERS", "botao_local");
    }
    else if (alertaPressionado) {
        processarComandoLeds(
            "ALERTA_SPRINKLERS",
            "botao_local"
        );
    }
}

// ============================================================
// TAREFA FREERTOS — LEITURA DOS BOTOES FISICOS
// Executada periodicamente a cada 20 ms
// ============================================================

void tarefaLeituraBotoes(void* parametro) {
    (void)parametro;

    TickType_t ultimoDespertar = xTaskGetTickCount();

    Serial.println("Tarefa FreeRTOS dos botoes iniciada.");

    for (;;) {
        tratarBotoesFisicos();

        vTaskDelayUntil(
            &ultimoDespertar,
            pdMS_TO_TICKS(20)
        );
    }
}

// ============================================================
// COMANDOS TEMPORARIOS PELO MONITOR SERIAL
// ============================================================

void tratarComandoSerial() {
    if (!Serial.available()) {
        return;
    }

    String comando = Serial.readStringUntil('\n');
    comando.trim();
    comando.toUpperCase();

    if (comando == "AJUDA") {
        Serial.println("Comandos disponiveis:");
        Serial.println("NORMAL");
        Serial.println("ATENCAO");
        Serial.println("SPRINKLERS");
        Serial.println("ALERTA_SPRINKLERS");
        Serial.println("MANUAL");
        Serial.println("AUTOMATICO");
        return;
    }

    if (comando == "MANUAL" || comando == "AUTOMATICO") {
        processarComandoModo(comando, "serial");
        return;
    }

    if (!processarComandoLeds(comando, "serial")) {
        Serial.println("Use AJUDA para listar os comandos.");
    }
}

// ============================================================
// FUNCAO AUXILIAR PARA CONVERTER DADOS MQTT EM STRING
// ============================================================

String criarStringComTamanho(const char* dados, int tamanho) {
    String resultado;
    resultado.reserve(tamanho);

    for (int i = 0; i < tamanho; i++) {
        resultado += dados[i];
    }

    return resultado;
}

// ============================================================
// CALLBACK DO MQTT
// ============================================================

static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data
) {
    esp_mqtt_event_handle_t event =
        (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED: {
            Serial.println("Conectado ao MQTT via Cloudflare (WSS).");
            mqtt_conectado = true;

            const int idInscricaoLeds = esp_mqtt_client_subscribe(
                client,
                TOPICO_COMANDOS_LEDS,
                1
            );

            const int idInscricaoModo = esp_mqtt_client_subscribe(
                client,
                TOPICO_COMANDOS_MODO,
                1
            );

            Serial.print("Inscrito em comandos/leds. ID: ");
            Serial.println(idInscricaoLeds);
            Serial.print("Inscrito em comandos/modo. ID: ");
            Serial.println(idInscricaoModo);

            publicarStatusLeds("reconexao");
            publicarStatusModo("reconexao");
            break;
        }

        case MQTT_EVENT_DISCONNECTED:
            Serial.println("MQTT desconectado. Tentando reconectar...");
            mqtt_conectado = false;
            break;

        case MQTT_EVENT_SUBSCRIBED:
            Serial.print("Inscricao MQTT confirmada. ID: ");
            Serial.println(event->msg_id);
            break;

        case MQTT_EVENT_DATA: {
            const String topico = criarStringComTamanho(
                event->topic,
                event->topic_len
            );

            String conteudo = criarStringComTamanho(
                event->data,
                event->data_len
            );

            Serial.print("Mensagem MQTT recebida no topico: ");
            Serial.println(topico);
            Serial.print("Conteudo recebido: ");
            Serial.println(conteudo);

            if (topico == TOPICO_COMANDOS_LEDS) {
                processarComandoLeds(conteudo, "mqtt");
            }
            else if (topico == TOPICO_COMANDOS_MODO) {
                processarComandoModo(conteudo, "mqtt");
            }
            break;
        }

        case MQTT_EVENT_ERROR:
            Serial.println("Erro no MQTT.");
            break;

        default:
            break;
    }
}

// ============================================================
// LEITURA E PUBLICACAO DOS SENSORES
// ============================================================

void lerEPublicarSensores() {
    const int nivel_co2 = analogRead(MQ_PIN);
    const float umidade = dht.readHumidity();
    const float temperatura = dht.readTemperature();

    if (isnan(umidade) || isnan(temperatura)) {
        Serial.println(
            "Falha de leitura no DHT22. "
            "Envios cancelados neste ciclo."
        );
        Serial.println("---------------------------------------------------");
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(
            "Wi-Fi desconectado. Envios MQTT e HTTP cancelados."
        );
        Serial.println("---------------------------------------------------");
        return;
    }

    // ========================================================
    // PUBLICAÇÃO MQTT PARA NODE-RED E DASHBOARD
    // ========================================================

    String payload =
        "{\"co2\":" + String(nivel_co2) +
        ",\"temperatura\":" + String(temperatura, 2) +
        ",\"umidade\":" + String(umidade, 2) +
        "}";

    if (mqtt_conectado) {
        Serial.print(">> Publicando MQTT: ");
        Serial.println(payload);

        const int idPublicacao = esp_mqtt_client_publish(
            client,
            TOPICO_SENSORES,
            payload.c_str(),
            0,
            0,
            0
        );

        if (idPublicacao < 0) {
            Serial.println(
                "Falha ao colocar a mensagem MQTT na fila."
            );
        }
    } else {
        Serial.println(
            "MQTT desconectado. A leitura não foi enviada ao Node-RED."
        );
    }

    // ========================================================
    // ENVIO GET DIRETO POR TCP PARA A API PHP
    //
    // O HTTPClient foi removido desta etapa. A conexão TCP é
    // aberta diretamente, o GET é enviado pela mesma conexão
    // e a primeira linha da resposta HTTP é analisada.
    // ========================================================

    String caminho =
        String(CAMINHO_API_PHP) +
        "?co2=" + String(nivel_co2) +
        "&temperatura=" + String(temperatura, 2) +
        "&umidade=" + String(umidade, 2);

    WiFiClient clienteApi;
    clienteApi.setNoDelay(true);
    clienteApi.setTimeout(TIMEOUT_RESPOSTA_API_MS);

    bool conectadoAoServidor = false;

    for (
        uint8_t tentativa = 1;
        tentativa <= MAX_TENTATIVAS_CONEXAO_API;
        tentativa++
    ) {
        Serial.print(">> Conexão TCP com a API — tentativa ");
        Serial.print(tentativa);
        Serial.print("/");
        Serial.println(MAX_TENTATIVAS_CONEXAO_API);

        if (
            clienteApi.connect(
                IP_SERVIDOR_API,
                PORTA_SERVIDOR_API
            )
        ) {
            conectadoAoServidor = true;
            Serial.println("Conexão TCP estabelecida.");
            break;
        }

        clienteApi.stop();

        Serial.println(
            "Servidor ainda não acessível pela conexão TCP."
        );

        if (tentativa < MAX_TENTATIVAS_CONEXAO_API) {
            vTaskDelay(
                pdMS_TO_TICKS(INTERVALO_TENTATIVAS_API_MS)
            );
        }
    }

    if (!conectadoAoServidor) {
        Serial.println(
            "Não foi possível conectar à API neste ciclo."
        );
        Serial.println(
            "A próxima tentativa ocorrerá no próximo ciclo."
        );
        Serial.println(
            "---------------------------------------------------"
        );
        return;
    }

    Serial.print(">> GET ");
    Serial.println(caminho);

    // Requisição enviada apenas uma vez, depois que a conexão
    // TCP foi confirmada.
    clienteApi.print("GET ");
    clienteApi.print(caminho);
    clienteApi.println(" HTTP/1.1");
    clienteApi.println("Host: 192.168.1.150");
    clienteApi.println("User-Agent: SmokeGuard-ESP32");
    clienteApi.println("Accept: application/json");
    clienteApi.println("Connection: close");
    clienteApi.println();

    const unsigned long inicioEsperaResposta = millis();

    while (
        !clienteApi.available() &&
        clienteApi.connected()
    ) {
        if (
            millis() - inicioEsperaResposta >=
            TIMEOUT_RESPOSTA_API_MS
        ) {
            Serial.println(
                "Tempo limite aguardando a resposta da API."
            );
            clienteApi.stop();
            Serial.println(
                "---------------------------------------------------"
            );
            return;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (!clienteApi.available()) {
        Serial.println(
            "A conexão foi encerrada sem resposta HTTP."
        );
        clienteApi.stop();
        Serial.println(
            "---------------------------------------------------"
        );
        return;
    }

    String linhaStatus = clienteApi.readStringUntil('\n');
    linhaStatus.trim();

    Serial.print("Linha de status recebida: ");
    Serial.println(linhaStatus);

    int codigoHttp = 0;

    if (
        linhaStatus.startsWith("HTTP/1.0 ") ||
        linhaStatus.startsWith("HTTP/1.1 ")
    ) {
        codigoHttp = linhaStatus.substring(9, 12).toInt();
    }

    // Descarta os cabeçalhos HTTP até a linha vazia.
    while (clienteApi.connected() || clienteApi.available()) {
        String linhaCabecalho =
            clienteApi.readStringUntil('\n');

        linhaCabecalho.trim();

        if (linhaCabecalho.length() == 0) {
            break;
        }
    }

    // Lê o corpo JSON enviado pela API.
    String respostaApi;
    const unsigned long inicioLeituraCorpo = millis();

    while (clienteApi.connected() || clienteApi.available()) {
        while (clienteApi.available()) {
            respostaApi += (char)clienteApi.read();
        }

        if (
            millis() - inicioLeituraCorpo >=
            TIMEOUT_RESPOSTA_API_MS
        ) {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    respostaApi.trim();
    clienteApi.stop();

    Serial.print("Código HTTP recebido: ");
    Serial.println(codigoHttp);

    if (respostaApi.length() > 0) {
        Serial.print("Resposta da API: ");
        Serial.println(respostaApi);
    }

    if (codigoHttp == 201) {
        Serial.println(
            "Leitura gravada no MySQL pela API PHP."
        );
    } else {
        Serial.println(
            "A API não confirmou a gravação desta leitura."
        );
    }

    Serial.println("---------------------------------------------------");
}

// ============================================================
// TAREFA FREERTOS — LEITURA E PUBLICAÇÃO DOS SENSORES
// Executada periodicamente a cada 3 segundos
// ============================================================

void tarefaLeituraSensores(void* parametro) {
    (void)parametro;

    // Aguarda a estabilização do DHT22 e da rede local após o reset.
    vTaskDelay(pdMS_TO_TICKS(ESPERA_INICIAL_REDE_MS));

    Serial.println("Tarefa FreeRTOS dos sensores iniciada.");

    for (;;) {
        lerEPublicarSensores();

        // Aguarda 3 segundos após o término completo do ciclo.
        // Diferentemente de vTaskDelayUntil(), não tenta recuperar
        // períodos perdidos após uma falha ou timeout de rede.
        vTaskDelay(
            pdMS_TO_TICKS(INTERVALO_LEITURA_SENSORES)
        );
    }
}

// ============================================================
// SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    Serial.setTimeout(50);

    pinMode(LED1_VERMELHO, OUTPUT);
    pinMode(LED1_VERDE, OUTPUT);
    pinMode(LED1_AZUL, OUTPUT);

    pinMode(LED2_VERMELHO, OUTPUT);
    pinMode(LED2_VERDE, OUTPUT);
    pinMode(LED2_AZUL, OUTPUT);

    pinMode(BOTAO_NORMAL, INPUT_PULLUP);
    pinMode(BOTAO_ATENCAO, INPUT_PULLUP);
    pinMode(BOTAO_SPRINKLERS, INPUT_PULLUP);
    pinMode(BOTAO_ALERTA_SPRINKLERS, INPUT_PULLUP);
    pinMode(BOTAO_MODO, INPUT_PULLUP);

    apagarLeds();
    alterarEstadoLeds(ESTADO_NORMAL);

    dht.begin();

    Serial.print("Conectando ao Wi-Fi");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    Serial.print("\nWi-Fi conectado! Endereco IP: ");
    Serial.println(WiFi.localIP());

    // Evita atrasos e instabilidades em conexões TCP curtas na rede local.
    WiFi.setSleep(false);

    Serial.println(
        "Inicializando arquitetura MQTT sobre WebSockets (WSS)..."
    );

    esp_mqtt_client_config_t mqtt_cfg = {};

    mqtt_cfg.broker.address.uri = "wss://mqtt.lzmeister.uk:443";
    mqtt_cfg.broker.verification.crt_bundle_attach =
        esp_crt_bundle_attach;

    mqtt_cfg.credentials.username = MQTT_USERNAME;
    mqtt_cfg.credentials.authentication.password = MQTT_PASSWORD;

    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(
        client,
        MQTT_EVENT_ANY,
        mqtt_event_handler,
        NULL
    );

    esp_mqtt_client_start(client);

    // ========================================================
    // CRIAÇÃO DA TAREFA FREERTOS DOS LEDS
    // ========================================================

    BaseType_t resultadoTarefaLeds = xTaskCreatePinnedToCore(
        tarefaControleLeds,
        "ControleLeds",
        2048,
        nullptr,
        2,
        &tarefaLedsHandle,
        1
    );

    if (resultadoTarefaLeds != pdPASS) {
        Serial.println(
            "ERRO: não foi possível criar a tarefa dos LEDs."
        );

        while (true) {
            delay(1000);
        }
    }

    // ========================================================
    // CRIAÇÃO DA TAREFA FREERTOS DOS SENSORES
    // ========================================================

    BaseType_t resultadoTarefaSensores = xTaskCreatePinnedToCore(
        tarefaLeituraSensores,
        "LeituraSensores",
        6144,
        nullptr,
        1,
        &tarefaSensoresHandle,
        1
    );

    if (resultadoTarefaSensores != pdPASS) {
        Serial.println(
            "ERRO: não foi possível criar a tarefa dos sensores."
        );

        while (true) {
            delay(1000);
        }
    }

    // ========================================================
    // CRIAÇÃO DA TAREFA FREERTOS DOS BOTOES
    // ========================================================

    BaseType_t resultadoTarefaBotoes = xTaskCreatePinnedToCore(
        tarefaLeituraBotoes,
        "LeituraBotoes",
        4096,
        nullptr,
        3,
        &tarefaBotoesHandle,
        1
    );

    if (resultadoTarefaBotoes != pdPASS) {
        Serial.println(
            "ERRO: não foi possível criar a tarefa dos botoes."
        );

        while (true) {
            delay(1000);
        }
    }

    Serial.println("Sistema iniciado.");
    Serial.print("Modo inicial: ");
    Serial.println(obterNomeModo(modoAtual));
    Serial.println("Digite AJUDA no Monitor Serial.");
}

// ============================================================
// LOOP PRINCIPAL NAO BLOQUEANTE
// ============================================================

void loop() {
    tratarComandoSerial();

    delay(1);
}
