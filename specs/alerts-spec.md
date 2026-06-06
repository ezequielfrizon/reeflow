# Alerts Specification

## Objetivo

Detectar situações anormais.

## Categorias

INFO

WARNING

CRITICAL

## Alertas

Temperatura alta.

Temperatura baixa.

Sensor temperatura offline.

Sensor ATO offline.

ATO timeout.

ESP32 offline.

Wi-Fi offline.

MQTT offline.

Reboot inesperado.

## Cooldown

Alertas repetidos devem possuir cooldown configurável.

## Recuperação

Todo alerta deve possuir evento de recuperação.

Exemplos:

TEMPERATURE_HIGH

TEMPERATURE_RECOVERED

ATO_TIMEOUT

ATO_RECOVERED

## MQTT

Publicar todos os alertas.

## Cloud

Sincronizar todos os alertas.

## Histórico

Todos os alertas devem ser armazenados para consulta futura.
