-- SmokeGuard
-- Projeto desenvolvido no IFSP - Campus Catanduva
-- Estrutura do banco de dados sem registros e sem credenciais

CREATE DATABASE IF NOT EXISTS `sistema_iot`
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

USE `sistema_iot`;

DROP TABLE IF EXISTS `leituras`;

CREATE TABLE `leituras` (
  `id` int NOT NULL AUTO_INCREMENT,
  `co2` float NOT NULL,
  `temperatura` float NOT NULL,
  `data_hora` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `umidade` float DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_leituras_data_hora` (`data_hora`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

