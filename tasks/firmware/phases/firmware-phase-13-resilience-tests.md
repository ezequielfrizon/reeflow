# Fase 13 - Testes de Resiliencia

## Escopo

Validar a resiliencia local do firmware REEFLOW diante de falhas simuladas e recuperacoes controladas, cobrindo reboot inesperado, falha de energia representada por ciclo de boot simulado, perda de Wi-Fi, perda de MQTT, sensor offline e ATO travado.

Esta fase depende dos sistemas criticos locais ja implementados: core, System State, Event Bus, Config Manager, Task Scheduler, Logger, Watchdog, sensores, relays, ATO, modos, iluminacao, persistencia local, rede, MQTT e alertas. A Fase 13 nao deve criar novas funcionalidades operacionais; deve apenas compor cenarios de validacao e pequenos pontos de instrumentacao quando forem indispensaveis para observar comportamento ja existente.

O ESP32 continua sendo a autoridade soberana do controlador. Falhas de rede, MQTT, cloud ou aplicativo nao podem bloquear sensores, relays, ATO, modos, iluminacao, alertas locais, watchdog ou fail-safe.

A conclusao desta fase nao pode depender de ESP32 conectado, reboot fisico real, queda real de energia, sensores reais, relays reais, bomba ATO real, luminaria real, roteador real, Internet real, broker Mosquitto real, cloud, aplicativo, monitor Serial real, medicao eletrica, aquario, bancada ou qualquer teste fisico. Validacoes reais com hardware, rede real, broker real, energia real, aquario ou bancada devem ficar registradas como `Pendente para Hardware Validation` e fora dos criterios de conclusao.

As tarefas desta fase nao devem criar testes unitarios. A implementacao deve garantir o funcionamento por meio de build PlatformIO, checks de integracao, smoke tests, harnesses com fakes ja isolados em `firmware/test/`, revisao automatizada de escopo e verificacoes deterministicas sem hardware real. Nao criar novos arquivos em `firmware/test/unit/`.

Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` deve ser alterado.

## Estrutura Recomendada

```text
firmware/
|-- include/
|   |-- resilience/
|   |   |-- resilience_types.h
|   |   |-- resilience_scenarios.h
|   |   `-- resilience_report.h
|   `-- core/
|-- src/
|   |-- resilience/
|   |   |-- resilience_scenarios.cpp
|   |   `-- resilience_report.cpp
|   `-- app/
`-- test/
    |-- fakes/
    |   |-- fake_power_cycle_context.h
    |   |-- fake_resilience_network.h
    |   |-- fake_resilience_mqtt.h
    |   |-- fake_resilience_sensors.h
    |   `-- fake_resilience_ato.h
    |-- integration/
    |   |-- test_resilience_boot_power_integration.cpp
    |   |-- test_resilience_network_mqtt_integration.cpp
    |   |-- test_resilience_sensor_offline_integration.cpp
    |   |-- test_resilience_ato_stuck_integration.cpp
    |   |-- test_resilience_end_to_end_smoke.cpp
    |   |-- run_phase13_integration_tests.sh
    |   `-- run_phase13_scope_review.sh
    `-- hardware/
```

`firmware/test/hardware/` fica reservado para validacoes futuras e nao e criterio de conclusao da Fase 13.

## Tarefas

### ID

FW-P13-000

### Titulo

Definir contratos, cenarios e harness de resiliencia sem hardware

### Objetivo

Criar a base de validacao da Fase 13 com cenarios canonicos, relatorio deterministico, fakes de falha e harness de integracao sem ESP32 real, sem criar testes unitarios ou funcionalidades operacionais novas.

### Dependencias

Fase 2 concluida.

Fase 9 concluida.

Fase 10 concluida.

Fase 11 concluida.

Fase 12 concluida.

### Descricao

Definir o modulo `resilience` com tipos para cenario, etapa, falha simulada, recuperacao esperada, resultado, severidade, timestamp local quando disponivel, snapshots de System State e resumo de eventos observados.

Definir os cenarios canonicos da Fase 13: reboot inesperado, falha de energia representada por boot simulado, perda de Wi-Fi, perda de MQTT, sensor de temperatura offline, sensor de nivel offline e ATO travado.

Criar um harness de integracao que consiga iniciar o firmware em memoria, injetar fakes existentes ou novos fakes restritos a `firmware/test/fakes/`, executar ciclos de scheduler, simular passagem de tempo, capturar eventos locais e inspecionar System State sem depender de hardware real.

Criar relatorio local de resiliencia para os cenarios executados em testes de integracao ou smoke. O relatorio deve ser deterministico e conter somente dados locais necessarios para aprovar ou reprovar a validacao; nao deve criar historico remoto, cloud sync, dashboard, app, UI ou persistencia nova.

Quando for necessario adicionar instrumentacao minima ao firmware real para permitir observabilidade, ela deve ser passiva, local e sem alterar comportamento critico. Preferir boundaries ja existentes, fakes em `firmware/test/` e snapshots do System State.

Esta tarefa nao deve implementar os cenarios completos, alterar automacoes criticas, criar watchdog novo, criar storage novo, criar Alert Manager novo, alterar rede/MQTT, criar testes unitarios, criar testes fisicos, cloud, app, UI ou funcionalidades futuras.

### Arquivos afetados

- `firmware/include/resilience/resilience_types.h`
- `firmware/include/resilience/resilience_scenarios.h`
- `firmware/include/resilience/resilience_report.h`
- `firmware/src/resilience/resilience_scenarios.cpp`
- `firmware/src/resilience/resilience_report.cpp`
- `firmware/test/fakes/fake_power_cycle_context.h`
- `firmware/test/fakes/fake_resilience_network.h`
- `firmware/test/fakes/fake_resilience_mqtt.h`
- `firmware/test/fakes/fake_resilience_sensors.h`
- `firmware/test/fakes/fake_resilience_ato.h`
- `firmware/test/integration/test_resilience_end_to_end_smoke.cpp`
- `firmware/test/integration/run_phase13_integration_tests.sh`
- `firmware/test/integration/run_phase13_scope_review.sh`
- `firmware/platformio.ini`, somente se necessario para ambiente de integracao ou smoke

### Criterios de aceitacao

- Existe modulo `resilience` com contratos publicos da Fase 13.
- Existem cenarios canonicos para reboot inesperado, falha de energia simulada, perda de Wi-Fi, perda de MQTT, sensor offline e ATO travado.
- O harness executa sem ESP32 real.
- O harness executa sem sensores reais.
- O harness executa sem relays reais.
- O harness executa sem bomba ATO real.
- O harness executa sem luminaria real.
- O harness executa sem roteador real.
- O harness executa sem broker MQTT real.
- O harness usa System State como fonte de verificacao.
- O harness observa eventos locais pelo Event Bus quando aplicavel.
- Fakes ficam restritos a `firmware/test/`.
- O build real do firmware nao depende dos fakes.
- O relatorio de resiliencia e deterministico.
- O relatorio de resiliencia nao cria historico remoto.
- Nenhum teste unitario e criado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhuma funcionalidade operacional nova, cloud, app, UI ou teste fisico e implementado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar check de compilacao dos contratos de resiliencia no build real.
- Executar check de integracao ou smoke confirmando que o harness inicia sem hardware real.
- Executar check de integracao ou smoke confirmando captura de snapshot do System State.
- Executar check de integracao ou smoke confirmando captura de eventos locais.
- Executar check de integracao ou smoke confirmando geracao deterministica do relatorio.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para esta tarefa.
- Confirmar que nenhum teste exige ESP32, reboot fisico, energia real, sensores reais, relays reais, Wi-Fi real, MQTT real, broker real, cloud, app, Serial real ou qualquer validacao fisica.

---

### ID

FW-P13-001

### Titulo

Validar reboot inesperado e falha de energia por ciclo de boot simulado

### Objetivo

Implementar cenarios de resiliencia para reboot inesperado e falha de energia simulada, comprovando restauracao local segura, boot em estado conhecido e preservacao das automacoes criticas sem depender de reboot fisico real.

### Dependencias

FW-P13-000.

Fase 2 concluida.

Fase 5 concluida.

Fase 6 concluida.

Fase 7 concluida.

Fase 8 concluida.

Fase 9 concluida.

Fase 12 concluida.

### Descricao

Implementar cenario de reboot inesperado usando harness de integracao. O cenario deve preparar um estado local com configuracoes restauraveis, executar ciclos de scheduler, simular marca de reboot inesperado ou watchdog triggered conforme contratos existentes, reinicializar o contexto em memoria e validar o estado resultante.

Implementar cenario de falha de energia simulada como desligamento abrupto do contexto de teste seguido de novo boot em memoria. A simulacao deve validar que configuracoes persistiveis ja cobertas pela Fase 9 sao restauradas, enquanto leituras volateis e estados runtime nao persistiveis nao sao reaproveitados indevidamente.

Validar que relays iniciam em estado seguro conforme contratos existentes, ATO nao retorna com bomba ligada por estado antigo, modos retornam conforme regra persistivel existente, iluminacao nao reaplica `currentPWM` volatil como configuracao persistida e alertas locais registram reboot inesperado quando a Fase 12 ja oferecer esse contrato.

Garantir que Scheduler, Watchdog, Config Manager, Event Bus e System State continuam inicializados apos o boot simulado. Qualquer falha deve ser reportada pelo relatorio de resiliencia sem travar o processo de validacao.

Esta tarefa nao deve criar persistencia nova, alterar politica de boot, alterar pinout, alterar relays, criar comandos remotos, criar testes unitarios, criar testes fisicos, cloud, app ou UI.

### Arquivos afetados

- `firmware/include/resilience/resilience_scenarios.h`
- `firmware/src/resilience/resilience_scenarios.cpp`
- `firmware/src/resilience/resilience_report.cpp`
- `firmware/test/fakes/fake_power_cycle_context.h`
- `firmware/test/integration/test_resilience_boot_power_integration.cpp`
- `firmware/test/integration/run_phase13_integration_tests.sh`
- `firmware/test/integration/run_phase13_scope_review.sh`
- `firmware/src/app/core_app.cpp`, somente se instrumentacao passiva for indispensavel
- `firmware/src/app/core_app.h`, somente se instrumentacao passiva for indispensavel

### Criterios de aceitacao

- Reboot inesperado e simulado sem reboot fisico real.
- Falha de energia e simulada sem queda real de energia.
- Configuracoes persistiveis restauram pelo fluxo local existente.
- Leituras volateis de sensores nao sao tratadas como persistidas.
- Estados runtime de relays nao sao restaurados como comando ativo.
- ATO nao retorna com bomba ligada por estado antigo.
- Iluminacao nao restaura `currentPWM` volatil como configuracao.
- Relays iniciam em estado seguro conforme contratos existentes.
- System State fica inicializado apos boot simulado.
- Event Bus fica operacional apos boot simulado.
- Scheduler fica operacional apos boot simulado.
- Watchdog fica operacional apos boot simulado.
- Reboot inesperado gera alerta ou evento local quando contrato existente permitir.
- O cenario nao depende de Wi-Fi, MQTT, cloud ou app.
- Nenhum teste unitario e criado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhuma persistencia nova, politica nova de boot, comando remoto, cloud, app, UI ou teste fisico e implementado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar `firmware/test/integration/run_phase13_integration_tests.sh` com sucesso para os cenarios de reboot e energia simulada.
- Executar check de integracao confirmando boot seguro apos reboot inesperado simulado.
- Executar check de integracao confirmando boot seguro apos falha de energia simulada.
- Executar check de integracao confirmando que dados volateis nao sao restaurados como persistiveis.
- Executar check de integracao confirmando que ATO nao reinicia com bomba ligada por estado antigo.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para esta tarefa.
- Confirmar que nenhum teste exige reboot fisico, queda real de energia, ESP32, sensores reais, relays reais, Wi-Fi real, MQTT real, cloud, app ou qualquer validacao fisica.

---

### ID

FW-P13-002

### Titulo

Validar perda e recuperacao de Wi-Fi e MQTT sem afetar automacoes locais

### Objetivo

Implementar cenarios de resiliencia para perda de Wi-Fi e perda de MQTT, comprovando que automacoes criticas locais continuam operando e que os status, eventos e alertas de conectividade se recuperam de forma deterministica.

### Dependencias

FW-P13-000.

Fase 6 concluida.

Fase 7 concluida.

Fase 8 concluida.

Fase 10 concluida.

Fase 11 concluida.

Fase 12 concluida.

### Descricao

Implementar cenario de perda de Wi-Fi usando fake de rede ou boundary existente. O cenario deve simular Wi-Fi conectado, queda de conexao, periodo offline, tentativa de reconexao por backoff e recuperacao, sem usar roteador real ou Internet real.

Implementar cenario de perda de MQTT usando fake de MQTT ou boundary existente. O cenario deve simular MQTT conectado, broker indisponivel, falha de publish, periodo desconectado, reconexao e retomada de status, sem usar broker Mosquitto real.

Durante as falhas, validar que sensores simulados continuam atualizando System State, ATO continua respeitando gate de modos e fail-safe, relays nao recebem comandos indevidos, iluminacao continua avaliando perfil local, watchdog continua alimentado e alertas locais funcionam sem publicar obrigatoriamente MQTT.

Validar que `network.wifiConnected`, `network.mqttConnected`, heartbeat local, eventos `WIFI_*`, eventos `MQTT_*` e alertas `WIFI_OFFLINE`/`MQTT_OFFLINE` ou recuperacoes correspondentes refletem o estado simulado conforme contratos existentes.

Esta tarefa nao deve alterar a logica de reconexao, implementar MQTT novo, criar publicacao nova, criar Alert Manager novo, criar comandos remotos, criar testes unitarios, criar testes fisicos, cloud, app ou UI.

### Arquivos afetados

- `firmware/include/resilience/resilience_scenarios.h`
- `firmware/src/resilience/resilience_scenarios.cpp`
- `firmware/src/resilience/resilience_report.cpp`
- `firmware/test/fakes/fake_resilience_network.h`
- `firmware/test/fakes/fake_resilience_mqtt.h`
- `firmware/test/integration/test_resilience_network_mqtt_integration.cpp`
- `firmware/test/integration/run_phase13_integration_tests.sh`
- `firmware/test/integration/run_phase13_scope_review.sh`

### Criterios de aceitacao

- Perda de Wi-Fi e simulada sem roteador real.
- Recuperacao de Wi-Fi e simulada sem Internet real.
- Perda de MQTT e simulada sem broker real.
- Recuperacao de MQTT e simulada sem broker real.
- `network.wifiConnected` reflete queda e recuperacao simuladas.
- `network.mqttConnected` reflete queda e recuperacao simuladas.
- Eventos locais de Wi-Fi sao observados quando contratos existentes permitirem.
- Eventos locais de MQTT sao observados quando contratos existentes permitirem.
- Alertas de Wi-Fi offline e recuperacao sao observados quando contratos existentes permitirem.
- Alertas de MQTT offline e recuperacao sao observados quando contratos existentes permitirem.
- Sensores simulados continuam atualizando System State durante falha de rede.
- ATO local continua respeitando fail-safe durante falha de rede.
- Modos continuam governando automacoes durante falha de rede.
- Relays nao recebem comando indevido por queda de rede ou MQTT.
- Iluminacao continua avaliando localmente durante falha de rede.
- Watchdog continua sendo alimentado durante falha de rede.
- Nenhum teste unitario e criado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhuma nova logica de reconexao, publicacao MQTT, comando remoto, cloud, app, UI ou teste fisico e implementado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar `firmware/test/integration/run_phase13_integration_tests.sh` com sucesso para os cenarios de Wi-Fi e MQTT.
- Executar check de integracao confirmando automacoes locais durante Wi-Fi offline.
- Executar check de integracao confirmando automacoes locais durante MQTT offline.
- Executar check de integracao confirmando recuperacao de status de Wi-Fi.
- Executar check de integracao confirmando recuperacao de status de MQTT.
- Executar check de integracao confirmando que alertas locais nao dependem de publicacao MQTT.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para esta tarefa.
- Confirmar que nenhum teste exige roteador real, Internet real, broker real, ESP32, cloud, app, Serial real ou qualquer validacao fisica.

---

### ID

FW-P13-003

### Titulo

Validar sensores offline e recuperacao sem acionar atuadores indevidos

### Objetivo

Implementar cenarios de resiliencia para sensor de temperatura offline e sensor de nivel offline, comprovando que estados, eventos, alertas e recuperacoes sao tratados sem comandar atuadores indevidamente.

### Dependencias

FW-P13-000.

Fase 3 concluida.

Fase 4 concluida.

Fase 5 concluida.

Fase 6 concluida.

Fase 12 concluida.

### Descricao

Implementar cenario de sensor de temperatura offline usando fake de sensor ou boundary existente. O cenario deve simular leitura valida, ausencia de leitura ate timeout offline, periodo offline e leitura valida de recuperacao.

Implementar cenario de sensor de nivel offline usando fake de VL6180X ou boundary existente. O cenario deve simular nivel valido, ausencia de leitura ate timeout offline, periodo offline e leitura valida de recuperacao.

Validar que o System State representa offline e recuperacao de forma deterministica, que eventos locais correspondentes sao emitidos quando existirem, que alertas `TEMPERATURE_SENSOR_OFFLINE` e `ATO_SENSOR_OFFLINE` sao levantados quando a Fase 12 oferecer esse contrato, e que recuperacoes sao registradas.

Validar que sensor offline nao liga relays diretamente, nao inicia ATO por leitura ausente, nao altera modo operacional por conta propria, nao altera PWM de iluminacao e nao depende de Wi-Fi, MQTT, cloud ou aplicativo.

Esta tarefa nao deve alterar drivers de sensores, alterar thresholds funcionais, criar politica nova de ATO, criar testes unitarios, criar testes fisicos, cloud, app ou UI.

### Arquivos afetados

- `firmware/include/resilience/resilience_scenarios.h`
- `firmware/src/resilience/resilience_scenarios.cpp`
- `firmware/src/resilience/resilience_report.cpp`
- `firmware/test/fakes/fake_resilience_sensors.h`
- `firmware/test/integration/test_resilience_sensor_offline_integration.cpp`
- `firmware/test/integration/run_phase13_integration_tests.sh`
- `firmware/test/integration/run_phase13_scope_review.sh`

### Criterios de aceitacao

- Sensor de temperatura offline e simulado sem DS18B20 real.
- Sensor de nivel offline e simulado sem VL6180X real.
- Recuperacao do sensor de temperatura e simulada sem hardware real.
- Recuperacao do sensor de nivel e simulada sem hardware real.
- System State representa temperatura offline.
- System State representa nivel offline.
- System State representa recuperacao de temperatura.
- System State representa recuperacao de nivel.
- Eventos locais de sensor offline sao observados quando contratos existentes permitirem.
- Alertas de sensor offline sao levantados quando contratos existentes permitirem.
- Recuperacoes de alertas de sensores sao registradas quando contratos existentes permitirem.
- Sensor offline nao liga relays diretamente.
- Sensor de nivel offline nao inicia ATO.
- Sensor offline nao altera modo operacional por conta propria.
- Sensor offline nao altera PWM de iluminacao.
- O cenario nao depende de Wi-Fi, MQTT, cloud ou app.
- Nenhum teste unitario e criado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhum driver, threshold funcional, politica nova de ATO, cloud, app, UI ou teste fisico e implementado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar `firmware/test/integration/run_phase13_integration_tests.sh` com sucesso para os cenarios de sensores offline.
- Executar check de integracao confirmando timeout offline de temperatura.
- Executar check de integracao confirmando recuperacao de temperatura.
- Executar check de integracao confirmando timeout offline de nivel.
- Executar check de integracao confirmando recuperacao de nivel.
- Executar check de integracao confirmando que sensor offline nao aciona atuadores indevidos.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para esta tarefa.
- Confirmar que nenhum teste exige DS18B20 real, VL6180X real, relays reais, bomba real, ESP32, cloud, app ou qualquer validacao fisica.

---

### ID

FW-P13-004

### Titulo

Validar ATO travado, timeout, cooldown e fail-safe local

### Objetivo

Implementar cenario de resiliencia para ATO travado, comprovando que timeout, cooldown, relay seguro, eventos e alertas locais funcionam sem depender de bomba real, sensor real ou rede.

### Dependencias

FW-P13-000.

Fase 4 concluida.

Fase 5 concluida.

Fase 6 concluida.

Fase 7 concluida.

Fase 12 concluida.

### Descricao

Implementar cenario de ATO travado com fake de nivel e fake de relay. O cenario deve simular condicao que solicita reposicao, acionamento local permitido, nivel que nao recupera dentro do timeout, desligamento por timeout, entrada em cooldown e tentativa bloqueada durante cooldown.

Validar que o relay da bomba ATO e desligado no timeout, que o estado `ato` representa timeout/cooldown conforme contratos existentes, que eventos locais de ATO sao emitidos quando existirem e que alerta `ATO_TIMEOUT` e recuperacao correspondente sao observados quando a Fase 12 oferecer esse contrato.

Validar que modos que bloqueiam ATO continuam bloqueando a automacao mesmo durante o cenario de falha. Em `FEEDING`, `TPA` ou `MAINTENANCE`, o cenario nao deve permitir que o ATO seja acionado por tentativa de recuperacao automatica.

Validar que falha de Wi-Fi ou MQTT simultanea, quando simulada de forma simples dentro do cenario, nao altera a decisao local de fail-safe do ATO e nao impede o desligamento da bomba simulada.

Esta tarefa nao deve alterar politica de ATO, alterar semantica de modos, alterar relays, criar testes unitarios, criar testes fisicos, cloud, app, UI ou comando remoto.

### Arquivos afetados

- `firmware/include/resilience/resilience_scenarios.h`
- `firmware/src/resilience/resilience_scenarios.cpp`
- `firmware/src/resilience/resilience_report.cpp`
- `firmware/test/fakes/fake_resilience_ato.h`
- `firmware/test/fakes/fake_resilience_sensors.h`
- `firmware/test/integration/test_resilience_ato_stuck_integration.cpp`
- `firmware/test/integration/run_phase13_integration_tests.sh`
- `firmware/test/integration/run_phase13_scope_review.sh`

### Criterios de aceitacao

- ATO travado e simulado sem bomba real.
- ATO travado e simulado sem sensor real.
- ATO inicia somente quando gate de modos permite.
- ATO nao inicia quando modo atual bloqueia automacao.
- Timeout de ATO e observado de forma deterministica.
- Relay da bomba ATO e desligado no timeout.
- Estado de cooldown e observado apos timeout.
- Tentativa durante cooldown permanece bloqueada.
- Eventos locais de ATO sao observados quando contratos existentes permitirem.
- Alerta `ATO_TIMEOUT` e levantado quando contrato existente permitir.
- Recuperacao de ATO e registrada quando contrato existente permitir.
- Falha simultanea simulada de Wi-Fi nao impede fail-safe local.
- Falha simultanea simulada de MQTT nao impede fail-safe local.
- O cenario nao depende de Wi-Fi real, MQTT real, cloud ou app.
- Nenhum teste unitario e criado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhuma politica nova de ATO, semantica nova de modos, alteracao funcional de relays, cloud, app, UI ou teste fisico e implementado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar `firmware/test/integration/run_phase13_integration_tests.sh` com sucesso para o cenario de ATO travado.
- Executar check de integracao confirmando desligamento por timeout.
- Executar check de integracao confirmando cooldown apos timeout.
- Executar check de integracao confirmando bloqueio por modo operacional.
- Executar check de integracao confirmando fail-safe local com Wi-Fi offline simulado.
- Executar check de integracao confirmando fail-safe local com MQTT offline simulado.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para esta tarefa.
- Confirmar que nenhum teste exige bomba real, sensor real, relay real, ESP32, Wi-Fi real, MQTT real, cloud, app ou qualquer validacao fisica.

---

### ID

FW-P13-005

### Titulo

Consolidar suite de resiliencia, revisao de escopo e pendencias de hardware

### Objetivo

Consolidar a validacao final da Fase 13 com runner unico, revisao automatizada de escopo, relatorio de cenarios e registro claro das validacoes reais pendentes como `Pendente para Hardware Validation`.

### Dependencias

FW-P13-001.

FW-P13-002.

FW-P13-003.

FW-P13-004.

### Descricao

Consolidar `run_phase13_integration_tests.sh` para executar todos os cenarios da Fase 13 em ambiente sem hardware real. O runner deve falhar quando qualquer cenario canonico nao for executado, quando o relatorio indicar falha ou quando houver dependencia indevida de hardware, rede real, broker real, cloud ou app.

Consolidar `run_phase13_scope_review.sh` para verificar limites de escopo: nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` alterado; nenhum arquivo novo em `firmware/test/unit/`; nenhum uso de cloud, Supabase, Edge Functions, app, UI, dashboard, push notification, OTA, BLE, provisioning, banco remoto ou teste fisico como criterio de conclusao.

Garantir que o relatorio final liste os cenarios executados, resultado de cada cenario, snapshots principais, eventos observados, alertas observados quando aplicavel e limitacoes da validacao sem hardware real.

Registrar validacoes reais futuras sob o rotulo exato `Pendente para Hardware Validation`, incluindo reboot fisico real, queda real de energia, ESP32 real, sensores reais, relays reais, bomba ATO real, luminaria real, roteador real, Internet real, broker Mosquitto real, monitor Serial real, caixa eletrica, bancada e aquario.

Esta tarefa nao deve criar novos cenarios alem dos canonicos da Fase 13, nao deve implementar funcionalidades futuras, nao deve criar testes unitarios, nao deve criar testes fisicos, cloud, app ou UI.

### Arquivos afetados

- `firmware/test/integration/run_phase13_integration_tests.sh`
- `firmware/test/integration/run_phase13_scope_review.sh`
- `firmware/test/integration/test_resilience_end_to_end_smoke.cpp`
- `firmware/src/resilience/resilience_report.cpp`
- `firmware/include/resilience/resilience_report.h`
- `firmware/test/run_phase13_validation.sh`, se o padrao local exigir runner de fase

### Criterios de aceitacao

- Existe runner unico de integracao da Fase 13.
- Existe revisao automatizada de escopo da Fase 13.
- O runner executa todos os cenarios canonicos.
- O runner falha quando cenario canonico fica ausente.
- O runner falha quando o relatorio de resiliencia indica erro.
- A revisao de escopo falha se houver arquivo novo em `firmware/test/unit/`.
- A revisao de escopo falha se houver dependencia de hardware real como criterio de conclusao.
- A revisao de escopo falha se houver cloud, app, UI, Supabase, Edge Functions, push notification, OTA, BLE ou provisioning.
- O relatorio final lista todos os cenarios executados.
- O relatorio final lista resultados por cenario.
- O relatorio final lista eventos observados quando aplicavel.
- O relatorio final lista alertas observados quando aplicavel.
- O relatorio final registra limitacoes sem hardware real.
- Validacoes fisicas futuras ficam registradas com o rotulo exato `Pendente para Hardware Validation`.
- Nenhum teste unitario e criado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhum cenario fora da Fase 13, funcionalidade futura, cloud, app, UI ou teste fisico e implementado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar `firmware/test/integration/run_phase13_integration_tests.sh` com sucesso.
- Executar `firmware/test/integration/run_phase13_scope_review.sh` com sucesso.
- Executar check de integracao confirmando que todos os cenarios canonicos aparecem no relatorio final.
- Executar check automatizado confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para a Fase 13.
- Executar check automatizado confirmando ausencia de dependencias de cloud, app, UI, Supabase, Edge Functions, push notification, OTA, BLE e provisioning.
- Confirmar que as validacoes reais futuras estao marcadas como `Pendente para Hardware Validation`.
- Confirmar que nenhum teste exige ESP32, reboot fisico, queda real de energia, sensores reais, relays reais, bomba real, luminaria real, roteador real, Internet real, broker real, cloud, app, Serial real ou qualquer validacao fisica.
