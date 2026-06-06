# Storage Module Specification

## Objetivo

Persistir configurações localmente.

## Tecnologia

NVS Preferences

## Dados Persistidos

Wi-Fi

MQTT

Curvas da luminária

Configurações do ATO

Configurações de temperatura

Timers

Modo atual

Calibrações

## Regras

Salvar apenas quando houver mudança.

Evitar gravações excessivas.

## Recuperação

Após reboot:

Restaurar todas as configurações.

## Eventos

CONFIG_SAVED

CONFIG_RESTORED

CONFIG_RESET
