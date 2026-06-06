# System State Specification

## Objetivo

Definir o estado global oficial do sistema REEFLOW.

Todo módulo do firmware, MQTT, banco de dados e aplicativo deve utilizar esta estrutura como referência.

O ESP32 é a autoridade do estado.

Cloud e App apenas refletem ou solicitam alterações.

---

# Estado Global

O sistema possui um único estado global.

Este estado representa a situação completa do aquário em um determinado momento.

---

# Estrutura Principal

system

temperature

waterLevel

lighting

relays

modes

ato

network

alerts

systemHealth

---

# Temperature State

Responsável pelo monitoramento térmico.

Campos:

currentTemperature

minTemperature

maxTemperature

status

lastUpdate

---

## Status Possíveis

NORMAL

HIGH

LOW

SENSOR_OFFLINE

---

# Water Level State

Responsável pelo nível de água.

Campos:

currentLevel

minimumLevel

maximumLevel

status

lastUpdate

---

## Status Possíveis

NORMAL

LOW

HIGH

SENSOR_OFFLINE

---

# Lighting State

Responsável pela luminária.

Campos:

mode

sunriseEnabled

sunsetEnabled

acclimationEnabled

currentProfile

---

## Channels

white

blue

royalBlue

UV

---

## Cada Canal Possui

currentPWM

maxPWM

enabled

---

# Relay State

Responsável pelos relés.

Campos:

recalque

heater

atoPump

reserve

---

## Cada Relé Possui

enabled

lastChanged

source

---

## Source

LOCAL

MQTT

APP

AUTOMATION

FAILSAFE

---

# Mode State

Modo operacional atual.

Campos:

currentMode

startedAt

remainingTime

---

## Modos

NORMAL

FEEDING

TPA

MAINTENANCE

---

# ATO State

Responsável pela reposição automática.

Campos:

enabled

status

pumpRunning

lastActivation

lastCompletion

timeoutCounter

---

## Status

NORMAL

REFILLING

TIMEOUT

SENSOR_OFFLINE

DISABLED

---

# Network State

Responsável pelas conexões.

Campos:

wifiConnected

mqttConnected

internetAvailable

ipAddress

rssi

lastHeartbeat

---

# Alerts State

Responsável pelos alertas ativos.

Campos:

activeAlerts

lastAlert

lastRecovery

---

## Prioridades

INFO

WARNING

CRITICAL

---

# System Health State

Responsável pela saúde do controlador.

Campos:

uptime

freeHeap

cpuUsage

lastReboot

rebootReason

watchdogTriggered

---

# Persistência

Devem ser persistidos:

Configurações

Curvas da luminária

Parâmetros do ATO

Limites de temperatura

Modo atual

Wi-Fi

MQTT

Calibrações

---

# MQTT

Toda telemetria publicada deve ser derivada do estado global.

Não publicar dados diretamente dos módulos.

Fluxo correto:

Sensor
↓
Atualiza estado global
↓
Estado global gera evento
↓
MQTT publica

---

# Aplicativo

O aplicativo deve consumir exclusivamente o estado global.

Não deve depender de cálculos próprios.

Toda informação exibida ao usuário deve ser derivada do estado oficial do ESP32.

---

# Banco de Dados

O histórico deve ser construído a partir das alterações do estado global.

Cada mudança relevante gera um evento persistível.

---

# Princípio Fundamental

Existe apenas uma fonte de verdade.

A fonte de verdade é o estado global mantido pelo firmware do ESP32.

Todo o restante do ecossistema deve refletir esse estado.
