# Relay Module Specification

## Objetivo

Controlar equipamentos conectados aos relés.

## Relé 1

Nome:
Recalque

GPIO:
16

Tipo:
127V AC

## Relé 2

Nome:
Aquecedor

GPIO:
17

Tipo:
127V AC

## Relé 3

Nome:
ATO

GPIO:
18

Tipo:
5V DC

## Relé 4

Nome:
Reserva

GPIO:
19

Tipo:
Genérico

## Estados

ON

OFF

## Regras

Após boot:

Todos os relés iniciam desligados.

Exceto quando um modo exigir restauração.

## Eventos

RELAY_ON

RELAY_OFF

## MQTT

Publicar mudanças de estado.

Receber comandos remotos.
