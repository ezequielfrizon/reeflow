# Lighting Module Specification

## Objetivo

Controlar a luminária LED do aquário.

## Canais

Branco

Azul

Royal Blue

UV

## GPIO

Branco:
GPIO25

Azul:
GPIO26

Royal:
GPIO27

uv:
GPIO14

## Controle

PWM

Fade suave obrigatório.

## Modos

MANUAL

AUTOMÁTICO

ACLIMATAÇÃO

## Funcionalidades

Sunrise

Sunset

Moonlight

Curvas personalizadas

Limite máximo de intensidade

Aclimatação gradual

## Configurações

Horário de início.

Horário de término.

Curvas por canal.

Intensidade máxima.

Duração da aclimatação.

## Persistência

Salvar todas as curvas.

Salvar horários.

Salvar configurações.

## Eventos

LIGHTING_PROFILE_CHANGED

LIGHTING_STARTED

LIGHTING_STOPPED

## MQTT

Sincronizar estado.

Receber alterações remotas.
