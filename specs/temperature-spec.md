# Temperature Module Specification

## Objetivo

Monitorar continuamente a temperatura do aquário.

## Sensor

DS18B20

GPIO4

## Frequência de Leitura

A cada 5 segundos.

## Estados

NORMAL

ALTA

BAIXA

SENSOR_OFFLINE

## Faixas Padrão

Temperatura ideal:
26.5°C

Temperatura mínima:
25.0°C

Temperatura máxima:
28.0°C

Os limites devem ser configuráveis.

## Falhas

Se não houver leitura válida por mais de 30 segundos:

Estado:
SENSOR_OFFLINE

## Eventos

TEMPERATURE_UPDATED

TEMPERATURE_HIGH

TEMPERATURE_LOW

TEMPERATURE_SENSOR_OFFLINE

TEMPERATURE_SENSOR_RECOVERED

## Persistência

Salvar limites configurados.

## MQTT

Publicar temperatura atual.

Publicar mudanças de estado.

## Alertas

Temperatura alta.

Temperatura baixa.

Sensor offline.
