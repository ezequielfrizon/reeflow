# REEFLOW Firmware Master Plan

## Objetivo

Definir o plano mestre exclusivo do firmware ESP32 do REEFLOW.

O firmware deve transformar o ESP32 na autoridade soberana do controlador de aquario marinho. Toda automacao critica deve existir localmente e continuar operacional sem Internet, MQTT, cloud ou aplicativo.

Cloud e aplicativo sao extensoes externas: refletem o estado oficial do ESP32 ou solicitam alteracoes por interfaces expostas, mas nao substituem responsabilidades criticas do firmware.

## Escopo do Dominio Firmware

Pertence ao firmware:

- Hardware bring-up do ESP32 e perifericos V1.
- System State como fonte unica de verdade.
- Event Bus, Config Manager, Task Scheduler, Logger e Watchdog.
- Sensores DS18B20 e VL6180X.
- Relays, ATO, modos operacionais e iluminacao.
- Persistencia local em NVS Preferences.
- Wi-Fi, NTP, MQTT e heartbeat executados pelo ESP32.
- Alertas locais, cooldown e eventos de recuperacao.
- Testes de resiliencia do controlador local.

Nao pertence ao firmware:

- Banco Supabase.
- Edge Functions.
- MQTT Bridge cloud/backend.
- Historico remoto.
- Autenticacao de usuarios.
- Push notifications.
- Aplicativo React Native.
- UI, dashboard, telas, navegacao ou biometria.

## Regras Arquiteturais

- O firmware deve ser desenvolvivel integralmente sem mobile.
- O firmware deve ser desenvolvivel integralmente sem cloud.
- MQTT e rede nao podem ser pre-requisito para automacoes criticas.
- Cloud e app nunca podem assumir controle critico do aquario.
- Toda telemetria publicada deve ser derivada do System State.
- Todo comando externo futuro deve ser tratado como solicitacao ao ESP32, nao como substituicao da logica local.
- Validacoes fisicas pendentes devem ser registradas como `Pendente para Hardware Validation`.

## Ordem Oficial de Implementacao do Firmware

### Fase 0 - Estrutura do Projeto

Preparar estrutura de pastas, documentos de arquitetura, specs funcionais, PlatformIO e padroes de codigo.

### Fase 1 - Hardware Bring-Up

Preparar contratos, drivers minimos, diagnosticos, pinout, relays, PWM e sensores do hardware V1.

Arquivo: `phases/firmware-phase-01-hardware-bringup.md`

### Fase 2 - Core do Firmware

Implementar System State, Event Bus, Config Manager em memoria, Task Scheduler, Logger e Watchdog.

Arquivo: `phases/firmware-phase-02-core.md`

### Fase 3 - Sensor de Temperatura

Implementar monitoramento DS18B20, estados de temperatura, sensor offline e eventos locais.

Arquivo: `phases/firmware-phase-03-temperature-sensor.md`

### Fase 4 - Sensor de Nivel

Implementar leitura VL6180X, calibracao logica, estados de nivel, falhas e eventos locais.

Arquivo: `phases/firmware-phase-04-nivel-sensor.md`

### Fase 5 - Sistema de Relays

Controlar Recalque, Aquecedor, ATO e Reserva com boot seguro, origem de comando e eventos locais.

Arquivo: `phases/firmware-phase-05-relay-system.md`

### Fase 6 - Sistema ATO

Automatizar reposicao de agua doce localmente usando `waterLevel` e Rele 3, com timeout, cooldown, fail-safe e eventos locais.

Arquivo: `phases/firmware-phase-06-ato-system.md`

### Fase 7 - Sistema de Modos

Implementar NORMAL, FEEDING, TPA e MAINTENANCE, efeitos locais, gate de automacoes e persistencia por boundary substituivel.

Arquivo: `phases/firmware-phase-07-mode-system.md`

### Fase 8 - Sistema de Iluminacao

Controlar luminaria por PWM com Branco, Azul, Royal Blue e UV, curvas, sunrise, sunset, moonlight como perfil, aclimatacao, limite maximo e fade suave.

Arquivo: `phases/firmware-phase-08-light-system.md`

### Fase 9 - Persistencia Local

Salvar e restaurar configuracoes persistiveis em NVS Preferences sem iniciar Wi-Fi, MQTT, cloud ou aplicativo.

Arquivo: `phases/firmware-phase-09-local-persistence.md`

### Fase 10 - Rede

Implementar Wi-Fi, reconexao automatica, NTP, heartbeat e status de rede no System State, sem afetar automacoes criticas locais.

### Fase 11 - MQTT

Implementar comunicacao em tempo real com Mosquitto, publicacao de telemetria derivada do System State, recebimento de comandos pelo ESP32, QoS e reconexao.

### Fase 12 - Sistema de Alertas

Centralizar alertas locais com categorias, cooldown, priorizacao, recuperacao e publicacao futura quando MQTT estiver disponivel.

### Fase 13 - Testes de Resiliencia

Validar recuperacao do firmware em reboot inesperado, perda de Wi-Fi, perda de MQTT, sensor offline, falha de energia e ATO travado.

## Dependencias entre Fases de Firmware

- Fase 0 e pre-requisito para todas as demais.
- Fase 1 prepara os contratos de hardware.
- Fase 2 e pre-requisito para todos os modulos funcionais.
- Fases 3, 4, 5 e 8 dependem das bases de Fase 1 e Fase 2.
- Fase 6 depende de Fase 4 e Fase 5.
- Fase 7 depende de Fase 5 e Fase 6.
- Fase 9 depende dos modulos com configuracoes persistiveis.
- Fase 10 depende de Fase 2 e de configuracoes locais restauraveis.
- Fase 11 depende de Fase 10 e publica somente dados derivados do System State.
- Fase 12 depende dos eventos e estados dos modulos locais.
- Fase 13 depende dos sistemas criticos locais ja implementados.

## Marcos de Integracao do Firmware

- Hardware V1 preparado para validacao futura.
- Core comum operando com System State e Event Bus.
- Sensores atualizando `temperature` e `waterLevel`.
- Atuadores atualizando `relays` e `lighting`.
- Automacoes locais de ATO e modos operando sem dependencia externa.
- Configuracoes persistiveis restauradas localmente.
- Rede e MQTT integrados sem assumir responsabilidade critica.
- Alertas locais centralizados.
- Resiliencia local validada.

## Limite do Plano

Este plano termina no firmware autonomo do ESP32.

Cloud/backend e mobile possuem planos mestres proprios em:

- `../cloud/cloud-master-plan.md`
- `../mobile/mobile-master-plan.md`
