# Auto Top Off Specification

## Objetivo

Manter o nível de água do sump automaticamente.

## Sensor

VL6180X

I2C

GPIO21 SDA

GPIO22 SCL

## Atuador

Bomba ATO

Relé 3

## Estados

NORMAL

LOW_LEVEL

REFILLING

TIMEOUT

SENSOR_OFFLINE

## Funcionamento

Quando o nível atingir o limite mínimo:

Iniciar reposição.

Quando o nível atingir o limite máximo:

Parar reposição.

## Configurações

Nível mínimo.

Nível máximo.

Timeout máximo.

Cooldown entre acionamentos.

## Proteções

Timeout.

Proteção contra ciclos excessivos.

Proteção contra sensor inválido.

Proteção contra leitura impossível.

## Falhas

Se o nível não subir durante a reposição:

Gerar TIMEOUT.

Desligar bomba.

## Eventos

ATO_START

ATO_STOP

ATO_TIMEOUT

ATO_SENSOR_OFFLINE

ATO_RECOVERED

## MQTT

Publicar estado.

Publicar nível atual.

## Alertas

ATO travado.

Sensor offline.
