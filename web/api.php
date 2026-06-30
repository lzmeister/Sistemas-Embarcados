<?php

declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');

// Credenciais armazenadas fora da pasta pública do Apache
$config = require '/var/www/smokeguard_db.php';

// Conexão com o banco de dados
$conn = new mysqli(
    $config['host'],
    $config['usuario'],
    $config['senha'],
    $config['banco']
);

if ($conn->connect_error) {
    http_response_code(500);

    echo json_encode(
        [
            'status' => 'erro',
            'mensagem' => 'Não foi possível conectar ao banco de dados.'
        ],
        JSON_UNESCAPED_UNICODE
    );

    exit;
}

$conn->set_charset('utf8mb4');

// Verifica se os dados foram enviados pelo método GET
if (
    isset($_GET['co2']) &&
    isset($_GET['temperatura']) &&
    isset($_GET['umidade'])
) {
    $co2 = filter_var($_GET['co2'], FILTER_VALIDATE_FLOAT);
    $temperatura = filter_var(
        $_GET['temperatura'],
        FILTER_VALIDATE_FLOAT
    );
    $umidade = filter_var(
        $_GET['umidade'],
        FILTER_VALIDATE_FLOAT
    );

    if (
        $co2 === false ||
        $temperatura === false ||
        $umidade === false
    ) {
        http_response_code(400);

        echo json_encode(
            [
                'status' => 'erro',
                'mensagem' => 'Os parâmetros devem conter valores numéricos válidos.'
            ],
            JSON_UNESCAPED_UNICODE
        );

        $conn->close();
        exit;
    }

    $stmt = $conn->prepare(
        'INSERT INTO leituras
        (co2, temperatura, umidade)
        VALUES (?, ?, ?)'
    );

    if (!$stmt) {
        http_response_code(500);

        echo json_encode(
            [
                'status' => 'erro',
                'mensagem' => 'Não foi possível preparar a gravação.'
            ],
            JSON_UNESCAPED_UNICODE
        );

        $conn->close();
        exit;
    }

    $stmt->bind_param(
        'ddd',
        $co2,
        $temperatura,
        $umidade
    );

    if ($stmt->execute()) {
        http_response_code(201);

        echo json_encode(
            [
                'status' => 'sucesso',
                'mensagem' => 'Dados gravados no MySQL com sucesso.'
            ],
            JSON_UNESCAPED_UNICODE
        );
    } else {
        http_response_code(500);

        echo json_encode(
            [
                'status' => 'erro',
                'mensagem' => 'Erro ao gravar os dados.'
            ],
            JSON_UNESCAPED_UNICODE
        );
    }

    $stmt->close();
} else {
    http_response_code(200);

    echo json_encode(
        [
            'status' => 'aviso',
            'mensagem' => 'Aguardando envio de dados: co2, temperatura e umidade.'
        ],
        JSON_UNESCAPED_UNICODE
    );
}

$conn->close();
