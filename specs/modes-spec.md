# Operational Modes Specification

## Objetivo

Permitir mudanças rápidas no comportamento do aquário.

## Modos

NORMAL

FEEDING

TPA

MAINTENANCE

## NORMAL

Operação padrão.

Todas as automações ativas.

## FEEDING

Desligar recalque.

Tempo configurável.

Retorno automático.

## TPA

Desligar equipamentos configurados.

Bloquear automações configuradas.

Retorno manual.

## MAINTENANCE

Permitir desligamentos manuais.

Suspender regras não críticas.

Retorno manual.

## Persistência

Modo atual deve sobreviver ao reboot.

## Eventos

MODE_CHANGED

MODE_STARTED

MODE_FINISHED

## MQTT

Publicar modo atual.

Receber alteração remota.
