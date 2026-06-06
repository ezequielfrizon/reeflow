# MQTT Specification

## Objetivo

Permitir comunicação em tempo real.

## Broker

Mosquitto

## Reconexão

Automática.

Tentativas infinitas.

Backoff progressivo.

## Heartbeat

Publicar a cada 30 segundos.

## Telemetria

Temperatura

Nível

Estado dos relés

Modo atual

PWM atual

Status do Wi-Fi

Status do sistema

## Comandos

Troca de modo.

Controle de relés.

Controle da luminária.

Configurações.

## QoS

QoS 1 para eventos importantes.

QoS 0 para telemetria.

## Eventos

MQTT_CONNECTED

MQTT_DISCONNECTED

MQTT_RECONNECTED
