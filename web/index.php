<?php

declare(strict_types=1);

mysqli_report(MYSQLI_REPORT_OFF);

// Credenciais armazenadas fora da pasta pública
$config = require '/var/www/smokeguard_db.php';

$conn = new mysqli(
    $config['host'],
    $config['usuario'],
    $config['senha'],
    $config['banco']
);

if ($conn->connect_error) {
    http_response_code(500);

    if (
        isset($_GET['formato']) &&
        $_GET['formato'] === 'json'
    ) {
        header('Content-Type: application/json; charset=utf-8');

        echo json_encode(
            [
                'status' => 'erro',
                'mensagem' => 'Não foi possível conectar ao banco de dados.'
            ],
            JSON_UNESCAPED_UNICODE
        );

        exit;
    }

    die('Não foi possível conectar ao banco de dados.');
}

$conn->set_charset('utf8mb4');

// ============================================================
// SAÍDA EM JSON
// ============================================================

if (
    isset($_GET['formato']) &&
    $_GET['formato'] === 'json'
) {
    header('Content-Type: application/json; charset=utf-8');

    $sql = '
        SELECT
            id,
            co2,
            temperatura,
            umidade,
            data_hora
        FROM leituras
        ORDER BY data_hora DESC
        LIMIT 50
    ';

    $resultadoJson = $conn->query($sql);

    if (!$resultadoJson) {
        http_response_code(500);

        echo json_encode(
            [
                'status' => 'erro',
                'mensagem' => 'Não foi possível consultar os registros.'
            ],
            JSON_UNESCAPED_UNICODE
        );

        $conn->close();
        exit;
    }

    $dados = [];

    while ($linha = $resultadoJson->fetch_assoc()) {
        $dados[] = [
            'id' => (int) $linha['id'],
            'fumaca_adc' => (float) $linha['co2'],
            'temperatura' => (float) $linha['temperatura'],
            'umidade' => (float) $linha['umidade'],
            'data_hora' => $linha['data_hora']
        ];
    }

    echo json_encode(
        $dados,
        JSON_UNESCAPED_UNICODE |
        JSON_UNESCAPED_SLASHES
    );

    $conn->close();
    exit;
}

// ============================================================
// PESQUISA VISUAL
// ============================================================

$termoBusca = trim(
    isset($_GET['busca'])
        ? (string) $_GET['busca']
        : ''
);

if ($termoBusca !== '') {
    $sql = '
        SELECT
            id,
            co2,
            temperatura,
            umidade,
            data_hora
        FROM leituras
        WHERE
            CAST(co2 AS CHAR) LIKE ?
            OR CAST(temperatura AS CHAR) LIKE ?
            OR CAST(umidade AS CHAR) LIKE ?
            OR CAST(data_hora AS CHAR) LIKE ?
        ORDER BY data_hora DESC
    ';

    $stmt = $conn->prepare($sql);

    if (!$stmt) {
        http_response_code(500);
        die('Não foi possível preparar a pesquisa.');
    }

    $termoSql = '%' . $termoBusca . '%';

    $stmt->bind_param(
        'ssss',
        $termoSql,
        $termoSql,
        $termoSql,
        $termoSql
    );

    $stmt->execute();
    $resultado = $stmt->get_result();
} else {
    $sql = '
        SELECT
            id,
            co2,
            temperatura,
            umidade,
            data_hora
        FROM leituras
        ORDER BY data_hora DESC
        LIMIT 20
    ';

    $resultado = $conn->query($sql);

    if (!$resultado) {
        http_response_code(500);
        die('Não foi possível consultar os registros.');
    }
}
?>

<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">

    <meta
        name="viewport"
        content="width=device-width, initial-scale=1.0"
    >

    <title>SmokeGuard</title>

    <style>
        * {
            box-sizing: border-box;
        }

        body {
            margin: 0;
            padding: 20px;
            font-family: Arial, sans-serif;
            background-color: #f4f4f9;
            color: #212529;
        }

        .container {
            width: 100%;
            max-width: 1000px;
            margin: auto;
            padding: 24px;
            background-color: #ffffff;
            border-radius: 10px;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.10);
        }

        h1 {
            margin-top: 0;
            margin-bottom: 5px;
            text-align: center;
            color: #b71c1c;
        }

        .subtitulo {
            margin-top: 0;
            margin-bottom: 24px;
            text-align: center;
            color: #616161;
        }

        .search-box {
            margin-bottom: 20px;
        }

        .search-box form {
            display: flex;
            justify-content: center;
            gap: 8px;
            flex-wrap: wrap;
        }

        input[type="text"] {
            width: min(100%, 500px);
            padding: 11px;
            border: 1px solid #bdbdbd;
            border-radius: 5px;
            font-size: 15px;
        }

        button,
        .btn {
            display: inline-block;
            padding: 11px 16px;
            border: none;
            border-radius: 5px;
            color: #ffffff;
            text-decoration: none;
            cursor: pointer;
            font-size: 14px;
        }

        .btn-buscar {
            background-color: #2e7d32;
        }

        .btn-limpar {
            background-color: #616161;
        }

        .btn-json {
            margin-top: 20px;
            background-color: #0277bd;
        }

        .tabela-container {
            overflow-x: auto;
        }

        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 10px;
        }

        th,
        td {
            padding: 12px;
            border: 1px solid #dddddd;
            text-align: center;
            white-space: nowrap;
        }

        th {
            background-color: #b71c1c;
            color: #ffffff;
        }

        tbody tr:nth-child(even) {
            background-color: #f5f5f5;
        }

        tbody tr:hover {
            background-color: #ffebee;
        }

        .acoes {
            text-align: center;
        }

        @media (max-width: 600px) {
            body {
                padding: 10px;
            }

            .container {
                padding: 15px;
            }
        }
    </style>
</head>

<body>

<div class="container">
    <h1>SmokeGuard</h1>

    <p class="subtitulo">
        Consulta de temperatura, umidade e nível relativo de fumaça
    </p>

    <div class="search-box">
        <form method="GET" action="index.php">
            <input
                type="text"
                name="busca"
                placeholder="Buscar valor ou data, por exemplo: 2026-06-28"
                value="<?php
                    echo htmlspecialchars(
                        $termoBusca,
                        ENT_QUOTES,
                        'UTF-8'
                    );
                ?>"
            >

            <button
                type="submit"
                class="btn-buscar"
            >
                Buscar
            </button>

            <a
                href="index.php"
                class="btn btn-limpar"
            >
                Limpar
            </a>
        </form>
    </div>

    <div class="tabela-container">
        <table>
            <thead>
                <tr>
                    <th>ID</th>
                    <th>Fumaça (ADC)</th>
                    <th>Temperatura (°C)</th>
                    <th>Umidade (%)</th>
                    <th>Data e hora</th>
                </tr>
            </thead>

            <tbody>
                <?php if ($resultado->num_rows > 0): ?>

                    <?php while ($linha = $resultado->fetch_assoc()): ?>
                        <tr>
                            <td>
                                <?php
                                    echo (int) $linha['id'];
                                ?>
                            </td>

                            <td>
                                <?php
                                    echo htmlspecialchars(
                                        (string) $linha['co2'],
                                        ENT_QUOTES,
                                        'UTF-8'
                                    );
                                ?>
                            </td>

                            <td>
                                <?php
                                    echo htmlspecialchars(
                                        (string) $linha['temperatura'],
                                        ENT_QUOTES,
                                        'UTF-8'
                                    );
                                ?>
                            </td>

                            <td>
                                <?php
                                    echo htmlspecialchars(
                                        (string) $linha['umidade'],
                                        ENT_QUOTES,
                                        'UTF-8'
                                    );
                                ?>
                            </td>

                            <td>
                                <?php
                                    echo date(
                                        'd/m/Y H:i:s',
                                        strtotime($linha['data_hora'])
                                    );
                                ?>
                            </td>
                        </tr>
                    <?php endwhile; ?>

                <?php else: ?>
                    <tr>
                        <td colspan="5">
                            Nenhum dado encontrado.
                        </td>
                    </tr>
                <?php endif; ?>
            </tbody>
        </table>
    </div>

    <div class="acoes">
        <a
            href="index.php?formato=json"
            target="_blank"
            class="btn btn-json"
        >
            Consultar registros em JSON
        </a>
    </div>
</div>

</body>
</html>

<?php
if (isset($stmt) && $stmt instanceof mysqli_stmt) {
    $stmt->close();
}

$conn->close();
?>
