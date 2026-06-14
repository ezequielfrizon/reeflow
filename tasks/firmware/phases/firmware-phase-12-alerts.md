# Fase 12 - Sistema de Alertas

## Escopo

Implementar o sistema centralizado de alertas locais do firmware REEFLOW, cobrindo categorias `INFO`, `WARNING` e `CRITICAL`, cooldown configuravel, priorizacao, eventos de recuperacao, estado `alerts` no System State, historico local consultavel e publicacao de alertas por MQTT quando a Fase 11 estiver disponivel.

Esta fase depende dos eventos e estados dos modulos locais ja implementados. Alertas devem ser derivados do System State e do Event Bus local, sem ler sensores, GPIO, PWM, relays, Wi-Fi, MQTT ou hardware diretamente como fonte primaria.

O ESP32 continua sendo a autoridade soberana do controlador. O sistema de alertas deve observar e registrar condicoes anormais, mas nao deve substituir automacoes criticas locais, nao deve comandar relays, ATO, modos, iluminacao ou rede diretamente, e nao deve transformar MQTT, cloud ou aplicativo em pre-requisito para seguranca local.

Alertas cobertos pela Fase 12:

- Temperatura alta.
- Temperatura baixa.
- Sensor temperatura offline.
- Sensor ATO offline.
- ATO timeout.
- ESP32 offline, representado localmente por heartbeat ausente ou marcador equivalente de saude do firmware quando aplicavel.
- Wi-Fi offline.
- MQTT offline.
- Reboot inesperado.

Todo alerta ativo deve possuir evento de recuperacao quando a condicao voltar ao normal. Exemplos canonicos: `TEMPERATURE_HIGH` deve recuperar por `TEMPERATURE_RECOVERED`; `ATO_TIMEOUT` deve recuperar por `ATO_RECOVERED`.

O historico desta fase e local ao firmware. Ele deve permitir consulta futura por outros componentes do firmware e deve ficar preparado para persistencia local quando houver contrato existente apropriado, mas nao deve criar banco remoto, Supabase, Edge Functions, push notification, aplicativo, UI, dashboard, cloud sync direto ou historico remoto.

Publicacao MQTT nesta fase significa publicar alertas e recuperacoes usando os contratos ja existentes da Fase 11, quando MQTT estiver conectado. Perda de MQTT nao pode impedir deteccao, cooldown, recuperacao, historico local ou atualizacao do System State. Sincronizacao cloud deve permanecer responsabilidade externa ao firmware.

A conclusao desta fase nao pode depender de ESP32 conectado, sensores reais, relays reais, bomba ATO real, luminaria real, roteador real, Internet real, broker Mosquitto real, cloud, aplicativo, monitor Serial real, reboot fisico, medicao eletrica, aquario, bancada ou qualquer teste fisico. Validacoes reais com hardware, rede real, broker real, reboot real, aquario ou bancada devem ficar registradas como `Pendente para Hardware Validation` e fora dos criterios de conclusao.

As tarefas desta fase nao devem criar testes unitarios. A implementacao deve ser feita com contratos claros, boundaries substituiveis, validacao defensiva, build PlatformIO, checks de integracao ou smoke sem hardware e revisao automatizada de escopo quando aplicavel. Nao criar novos arquivos em `firmware/test/unit/`.

Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` deve ser alterado.

## Estrutura Recomendada

```text
firmware/
|-- include/
|   |-- alerts/
|   |   |-- alert_types.h
|   |   |-- alert_config.h
|   |   |-- alert_events.h
|   |   |-- alert_registry.h
|   |   |-- alert_cooldown.h
|   |   |-- alert_history.h
|   |   |-- alert_manager.h
|   |   |-- alert_detector.h
|   |   `-- alert_mqtt_publisher.h
|   |-- core/
|   |-- mqtt/
|   `-- storage/
|-- src/
|   |-- alerts/
|   |   |-- alert_events.cpp
|   |   |-- alert_registry.cpp
|   |   |-- alert_cooldown.cpp
|   |   |-- alert_history.cpp
|   |   |-- alert_manager.cpp
|   |   |-- alert_detector.cpp
|   |   `-- alert_mqtt_publisher.cpp
|   `-- app/
`-- test/
    |-- fakes/
    |   |-- fake_alert_history_store.h
    |   `-- fake_alert_mqtt_sink.h
    |-- integration/
    |   |-- test_alerts_manager_integration.cpp
    |   |-- test_alerts_eventbus_integration.cpp
    |   |-- test_alerts_mqtt_publication_smoke.cpp
    |   |-- run_phase12_integration_tests.sh
    |   `-- run_phase12_scope_review.sh
    `-- hardware/
```

`firmware/test/hardware/` fica reservado para validacoes futuras e nao e criterio de conclusao da Fase 12.

## Tarefas

### ID

FW-P12-000

### Titulo

Definir contratos de alertas, catalogo, cooldown, eventos e historico local

### Objetivo

Criar os contratos publicos do sistema de alertas da Fase 12, delimitando categorias, codigos canonicos, recuperacoes, prioridade, cooldown configuravel, historico local e boundaries substituiveis para validacao sem hardware.

### Dependencias

Fase 2 concluida.

Fase 3 concluida.

Fase 4 concluida.

Fase 6 concluida.

Fase 10 concluida.

Fase 11 concluida para contratos MQTT, quando a publicacao MQTT for usada por tarefas posteriores.

### Descricao

Definir o modulo `alerts` com tipos funcionais para codigo de alerta, categoria, prioridade efetiva, estado ativo, estado recuperado, origem local, motivo, timestamp, contador de repeticoes, resultado de processamento e resultado de publicacao.

Definir o catalogo canonico da Fase 12 para os alertas `TEMPERATURE_HIGH`, `TEMPERATURE_LOW`, `TEMPERATURE_SENSOR_OFFLINE`, `ATO_SENSOR_OFFLINE`, `ATO_TIMEOUT`, `ESP32_OFFLINE`, `WIFI_OFFLINE`, `MQTT_OFFLINE` e `UNEXPECTED_REBOOT`.

Definir os eventos de recuperacao correspondentes, incluindo `TEMPERATURE_RECOVERED`, `ATO_RECOVERED`, `ESP32_RECOVERED`, `WIFI_RECOVERED`, `MQTT_RECOVERED` e recuperacoes especificas ou mapeadas para sensores quando necessario. O mapeamento deve ser deterministico e nao depender de strings soltas espalhadas pelo runtime.

Declarar eventos locais de alerta para o Event Bus, como `ALERT_RAISED`, `ALERT_RECOVERED`, `ALERT_COOLDOWN_SUPPRESSED` e `ALERT_HISTORY_RECORDED`, ajustando a nomenclatura ao padrao de enums existente quando necessario.

Definir configuracao local de cooldown por alerta e cooldown default. A configuracao deve aceitar valores restaurados localmente quando existirem, usar defaults seguros em memoria e rejeitar valores invalidos sem travar o firmware.

Definir um historico local com capacidade finita e comportamento deterministico quando cheio, por exemplo sobrescrita circular ou rejeicao controlada de entradas antigas. Cada entrada deve registrar codigo, categoria, estado levantado ou recuperado, timestamp quando disponivel, origem, prioridade e motivo resumido.

Definir um boundary opcional para armazenamento local do historico quando houver storage apropriado ja existente. A Fase 12 nao deve criar backend remoto, cloud sync, Supabase, banco externo, UI ou API de consulta.

Criar fakes restritos a `firmware/test/fakes/` para historico local e sink MQTT de alertas. Os fakes devem permitir simular sucesso, falha, capacidade cheia, contagem de registros, ultima entrada gravada e publicacao indisponivel.

Esta tarefa nao deve implementar o Alert Manager runtime, deteccao de condicoes, publicacao MQTT real, scheduler, cloud, aplicativo, notificacoes, historico remoto, testes unitarios ou testes fisicos.

### Arquivos afetados

- `firmware/include/alerts/alert_types.h`
- `firmware/include/alerts/alert_config.h`
- `firmware/include/alerts/alert_events.h`
- `firmware/include/alerts/alert_registry.h`
- `firmware/include/alerts/alert_cooldown.h`
- `firmware/include/alerts/alert_history.h`
- `firmware/include/alerts/alert_manager.h`
- `firmware/include/alerts/alert_detector.h`
- `firmware/include/alerts/alert_mqtt_publisher.h`
- `firmware/src/alerts/alert_events.cpp`
- `firmware/src/alerts/alert_registry.cpp`
- `firmware/include/core/events/event_bus.h`, somente se necessario para registrar eventos `ALERT_*`
- `firmware/src/core/events/event_bus.cpp`, somente se necessario para registrar eventos `ALERT_*`
- `firmware/include/core/state/system_state.h`, somente se necessario para usar campos `alerts` ja previstos
- `firmware/src/core/state/system_state.cpp`, somente se necessario para usar campos `alerts` ja previstos
- `firmware/include/config/config_manager.h`, somente se necessario para expor configuracao de cooldown em memoria
- `firmware/src/config/config_manager.cpp`, somente se necessario para expor configuracao de cooldown em memoria
- `firmware/test/fakes/fake_alert_history_store.h`
- `firmware/test/fakes/fake_alert_mqtt_sink.h`
- `firmware/platformio.ini`, somente se necessario para ambiente de build ou smoke

### Criterios de aceitacao

- Existe modulo `alerts` com contratos publicos da Fase 12.
- Existem categorias `INFO`, `WARNING` e `CRITICAL`.
- Existe catalogo canonico para todos os alertas da Fase 12.
- Cada alerta possui codigo deterministico.
- Cada alerta possui categoria default.
- Cada alerta possui evento de recuperacao definido.
- Alertas e recuperacoes nao dependem de strings soltas como contrato principal.
- Existem eventos locais `ALERT_RAISED`, `ALERT_RECOVERED`, `ALERT_COOLDOWN_SUPPRESSED` e `ALERT_HISTORY_RECORDED`, ou equivalentes no padrao local.
- Eventos `ALERT_*` sao mapeados para o Event Bus local.
- Existe configuracao de cooldown default.
- Existe configuracao de cooldown por alerta.
- Cooldown invalido e rejeitado ou normalizado de forma deterministica.
- Existe contrato de historico local com capacidade finita.
- Historico local registra alertas levantados.
- Historico local registra recuperacoes.
- Historico local registra categoria, origem, prioridade e timestamp quando disponivel.
- Historico local nao cria banco remoto.
- Historico local nao chama Supabase, Edge Functions, cloud ou app.
- Existe boundary substituivel para historico local quando armazenamento for usado.
- Existem fakes restritos a `firmware/test/fakes/`.
- O build real do firmware nao depende dos fakes.
- Nenhum Alert Manager runtime, detector, publicacao MQTT real, cloud, app, notificacao, UI ou teste fisico e implementado nesta tarefa.
- Nenhum teste unitario e criado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar check de compilacao dos contratos de alertas no build real.
- Executar check de integracao ou smoke confirmando que todos os alertas da Fase 12 existem no catalogo.
- Executar check de integracao ou smoke confirmando que cada alerta possui recuperacao mapeada.
- Executar check de integracao ou smoke confirmando cooldown default e por alerta.
- Executar check de integracao ou smoke confirmando historico local com capacidade finita.
- Executar check de integracao ou smoke confirmando que fakes de alertas nao entram no build real.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para esta tarefa.
- Confirmar que nenhum teste exige ESP32, sensores reais, relays reais, Wi-Fi real, MQTT real, broker real, cloud, app, Serial real ou qualquer validacao fisica.

---

### ID

FW-P12-001

### Titulo

Implementar Alert Manager local com priorizacao, cooldown e recuperacao

### Objetivo

Implementar o nucleo local do sistema de alertas para levantar, suprimir por cooldown, recuperar, priorizar, atualizar `system.alerts` e registrar historico sem depender de MQTT, cloud, aplicativo ou hardware real.

### Dependencias

FW-P12-000.

Fase 2 concluida.

### Descricao

Implementar `AlertManager` ou equivalente como service central da Fase 12. O service deve receber solicitacoes de alerta e recuperacao por chamadas locais ou eventos processados por tarefas posteriores, aplicar cooldown configuravel, atualizar o bloco `alerts` do System State e publicar eventos locais `ALERT_*`.

Quando um alerta for levantado pela primeira vez, o manager deve inserir o alerta em `system.alerts.activeAlerts`, atualizar `lastAlert`, calcular a prioridade efetiva e registrar entrada no historico local.

Quando um alerta repetido chegar dentro do cooldown, o manager deve preservar o estado ativo, nao duplicar indevidamente o alerta ativo, registrar ou sinalizar supressao conforme contrato local e emitir `ALERT_COOLDOWN_SUPPRESSED` quando aplicavel.

Quando uma recuperacao chegar, o manager deve remover ou marcar como inativo o alerta correspondente, atualizar `lastRecovery`, recalcular a prioridade geral, registrar a recuperacao no historico local e emitir `ALERT_RECOVERED`.

Quando multiplos alertas estiverem ativos, a prioridade geral de `system.alerts.priority` deve refletir o alerta ativo mais severo. A ordenacao ou substituicao dentro de `activeAlerts` deve ser deterministica quando a capacidade maxima for atingida.

O manager deve tratar entradas invalidas, alerta desconhecido, recuperacao sem alerta ativo, historico cheio, timestamp indisponivel e falha de sink local sem travar o firmware.

Esta tarefa nao deve detectar condicoes a partir de temperatura, ATO, rede ou MQTT; nao deve publicar MQTT real; nao deve criar comandos remotos; nao deve alterar automacoes criticas; nao deve criar cloud sync, app, notificacoes, UI, testes unitarios ou testes fisicos.

### Arquivos afetados

- `firmware/include/alerts/alert_manager.h`
- `firmware/include/alerts/alert_types.h`
- `firmware/include/alerts/alert_config.h`
- `firmware/include/alerts/alert_cooldown.h`
- `firmware/include/alerts/alert_history.h`
- `firmware/include/alerts/alert_events.h`
- `firmware/src/alerts/alert_manager.cpp`
- `firmware/src/alerts/alert_cooldown.cpp`
- `firmware/src/alerts/alert_history.cpp`
- `firmware/src/alerts/alert_events.cpp`
- `firmware/include/core/state/system_state.h`, somente se necessario para atualizacao do bloco `alerts`
- `firmware/src/core/state/system_state.cpp`, somente se necessario para atualizacao do bloco `alerts`
- `firmware/include/core/events/event_bus.h`, somente se necessario para registrar eventos `ALERT_*`
- `firmware/src/core/events/event_bus.cpp`, somente se necessario para registrar eventos `ALERT_*`
- `firmware/src/app/core_app.h`, somente se necessario para compor o service
- `firmware/src/app/core_app.cpp`, somente se necessario para compor o service
- `firmware/test/integration/test_alerts_manager_integration.cpp`
- `firmware/test/integration/run_phase12_integration_tests.sh`

### Criterios de aceitacao

- Existe `AlertManager` ou service equivalente.
- O manager levanta alerta valido.
- O manager rejeita alerta desconhecido de forma deterministica.
- O manager aplica cooldown configuravel.
- Alerta repetido dentro do cooldown nao duplica alerta ativo.
- Alerta repetido fora do cooldown produz novo registro observavel conforme contrato local.
- Recuperacao remove ou inativa o alerta correspondente.
- Recuperacao sem alerta ativo nao corrompe o estado.
- `system.alerts.activeAlerts` reflete os alertas ativos.
- `system.alerts.activeAlertCount` permanece dentro do limite definido.
- `system.alerts.lastAlert` e atualizado ao levantar alerta.
- `system.alerts.lastRecovery` e atualizado ao recuperar alerta.
- `system.alerts.priority` reflete a maior severidade ativa.
- Capacidade cheia de alertas ativos tem comportamento deterministico.
- Historico local registra alerta levantado.
- Historico local registra recuperacao.
- Falha de historico local nao trava o firmware.
- Eventos `ALERT_RAISED` e `ALERT_RECOVERED` sao emitidos localmente.
- Evento ou resultado de cooldown suprimido e observavel.
- O manager nao le sensores, GPIO, PWM, relays, Wi-Fi ou MQTT diretamente.
- O manager nao comanda ATO, relays, modos, iluminacao ou rede.
- O manager nao depende de MQTT, Internet, cloud ou aplicativo.
- Nenhuma publicacao MQTT real, detector de condicoes, comando remoto, cloud sync, app, UI ou teste fisico e implementado nesta tarefa.
- Nenhum teste unitario e criado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar check de integracao ou smoke levantando `TEMPERATURE_HIGH`.
- Executar check de integracao ou smoke recuperando `TEMPERATURE_HIGH` por `TEMPERATURE_RECOVERED`.
- Executar check de integracao ou smoke levantando `ATO_TIMEOUT`.
- Executar check de integracao ou smoke recuperando `ATO_TIMEOUT` por `ATO_RECOVERED`.
- Executar check de integracao ou smoke confirmando supressao por cooldown.
- Executar check de integracao ou smoke confirmando prioridade geral com multiplos alertas ativos.
- Executar check de integracao ou smoke confirmando limite de `activeAlerts`.
- Executar check de integracao ou smoke confirmando registro de historico local.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para esta tarefa.
- Confirmar que nenhum teste exige ESP32, sensores reais, relays reais, Wi-Fi real, MQTT real, broker real, cloud, app, Serial real ou qualquer validacao fisica.

---

### ID

FW-P12-002

### Titulo

Integrar detectores de alertas aos eventos e estados locais existentes

### Objetivo

Conectar o sistema de alertas aos eventos e blocos do System State ja existentes para detectar temperatura, ATO, rede, MQTT, reboot inesperado e saude do ESP32 sem acessar hardware diretamente.

### Dependencias

FW-P12-001.

Fase 3 concluida.

Fase 4 concluida.

Fase 6 concluida.

Fase 10 concluida.

Fase 11 concluida.

### Descricao

Implementar `AlertDetector` ou equivalente para observar eventos e snapshots do System State. O detector deve converter condicoes locais em chamadas ao `AlertManager`, mantendo o manager como ponto unico de aplicacao de cooldown, recuperacao, prioridade e historico.

Mapear eventos e estados de temperatura: `TEMPERATURE_HIGH` para temperatura alta, `TEMPERATURE_LOW` para temperatura baixa, `TEMPERATURE_SENSOR_OFFLINE` para sensor offline e `TEMPERATURE_SENSOR_RECOVERED` ou estado normal para recuperacao correspondente.

Mapear eventos e estados de ATO: `ATO_SENSOR_OFFLINE` para sensor ATO offline, `ATO_TIMEOUT` para timeout e `ATO_RECOVERED` ou estado normal para recuperacao correspondente.

Mapear eventos e estados de rede: Wi-Fi desconectado ou falha de reconexao para `WIFI_OFFLINE`, Wi-Fi conectado para `WIFI_RECOVERED`, MQTT desconectado apos ter sido configurado ou previamente conectado para `MQTT_OFFLINE` e MQTT conectado ou reconectado para `MQTT_RECOVERED`.

Mapear saude do sistema para `UNEXPECTED_REBOOT` quando os campos de `systemHealth` indicarem reboot inesperado ou watchdog triggered conforme contratos existentes. A recuperacao deve ser registrada quando o firmware estabilizar o boot localmente, sem exigir reboot fisico em teste.

Representar `ESP32_OFFLINE` somente por sinais locais observaveis pelo proprio firmware, como heartbeat local ausente no contexto de integracao com watchdog ou marcador equivalente ja existente. A tarefa nao deve criar dependencia de cloud, app ou monitor externo para provar offline do ESP32.

Integrar o detector ao Event Bus e ao scheduler sem bloquear o loop principal. A avaliacao deve ser idempotente para o mesmo snapshot, nao deve gerar alertas em cascata a cada ciclo e deve respeitar cooldown do manager.

Esta tarefa nao deve alterar a logica funcional de temperatura, ATO, rede, MQTT, relays, modos ou iluminacao. Tambem nao deve criar resiliencia da Fase 13, testes de queda real, reboot real, broker real, cloud, app, notificacoes, comandos remotos, testes unitarios ou testes fisicos.

### Arquivos afetados

- `firmware/include/alerts/alert_detector.h`
- `firmware/include/alerts/alert_manager.h`
- `firmware/include/alerts/alert_types.h`
- `firmware/include/alerts/alert_events.h`
- `firmware/src/alerts/alert_detector.cpp`
- `firmware/src/alerts/alert_manager.cpp`
- `firmware/include/core/events/event_bus.h`, somente se necessario para eventos `ALERT_*`
- `firmware/src/core/events/event_bus.cpp`, somente se necessario para eventos `ALERT_*`
- `firmware/include/core/state/system_state.h`, somente se necessario para leitura segura dos blocos existentes
- `firmware/src/app/core_app.h`
- `firmware/src/app/core_app.cpp`
- `firmware/src/app/core_app_arduino.cpp`
- `firmware/test/integration/test_alerts_eventbus_integration.cpp`
- `firmware/test/integration/run_phase12_integration_tests.sh`

### Criterios de aceitacao

- Existe detector central de alertas.
- Detector observa Event Bus local.
- Detector usa System State como fonte oficial de estado.
- Detector nao le sensores fisicos diretamente.
- Detector nao chama GPIO, PWM, relays, Wi-Fi ou MQTT diretamente.
- Temperatura alta gera `TEMPERATURE_HIGH`.
- Temperatura baixa gera `TEMPERATURE_LOW`.
- Sensor de temperatura offline gera `TEMPERATURE_SENSOR_OFFLINE`.
- Recuperacao de temperatura gera `TEMPERATURE_RECOVERED`.
- Sensor ATO offline gera `ATO_SENSOR_OFFLINE`.
- ATO timeout gera `ATO_TIMEOUT`.
- Recuperacao de ATO gera `ATO_RECOVERED`.
- Wi-Fi offline gera `WIFI_OFFLINE`.
- Recuperacao de Wi-Fi gera `WIFI_RECOVERED`.
- MQTT offline gera `MQTT_OFFLINE` quando MQTT for configurado ou tiver estado anterior relevante.
- Recuperacao de MQTT gera `MQTT_RECOVERED`.
- Reboot inesperado gera `UNEXPECTED_REBOOT`.
- ESP32 offline e representado apenas por sinal local disponivel ao firmware.
- Detector e idempotente para snapshots repetidos.
- Detector nao gera duplicacoes fora das regras de cooldown.
- Detector nao bloqueia scheduler, watchdog ou automacoes locais.
- Perda de Wi-Fi ou MQTT nao impede alertas locais.
- Nenhuma logica funcional de temperatura, ATO, rede, MQTT, relays, modos ou iluminacao e alterada indevidamente.
- Nenhuma resiliencia da Fase 13, cloud, app, notificacao, comando remoto, broker real ou teste fisico e implementado nesta tarefa.
- Nenhum teste unitario e criado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar check de integracao ou smoke simulando eventos de temperatura pelo Event Bus.
- Executar check de integracao ou smoke simulando eventos de ATO pelo Event Bus.
- Executar check de integracao ou smoke simulando Wi-Fi offline e recuperacao por estado de rede.
- Executar check de integracao ou smoke simulando MQTT offline e recuperacao por estado de rede.
- Executar check de integracao ou smoke simulando reboot inesperado por `systemHealth`.
- Executar check de integracao ou smoke confirmando idempotencia para snapshots repetidos.
- Executar check de integracao ou smoke confirmando que automacoes locais continuam independentes de MQTT.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para esta tarefa.
- Confirmar que nenhum teste exige ESP32, sensores reais, relays reais, Wi-Fi real, MQTT real, broker real, cloud, app, Serial real ou qualquer validacao fisica.

---

### ID

FW-P12-003

### Titulo

Publicar alertas por MQTT existente e fechar validacao de escopo da fase

### Objetivo

Publicar alertas e recuperacoes pelo MQTT ja implementado, sem tornar MQTT obrigatorio para alertas locais, e adicionar checks de integracao e revisao de escopo que comprovem a conclusao da Fase 12 sem testes unitarios ou hardware.

### Dependencias

FW-P12-002.

Fase 11 concluida.

### Descricao

Implementar `AlertMqttPublisher` ou equivalente usando exclusivamente os contratos MQTT existentes da Fase 11. O publisher deve receber eventos de alerta e recuperacao do Alert Manager ou Event Bus, montar payloads deterministicos e publicar em topicos oficiais de alertas sem ler sensores, services funcionais ou hardware diretamente.

Publicar todos os alertas levantados e todas as recuperacoes quando MQTT estiver conectado. Quando MQTT estiver desconectado, o firmware deve manter o alerta local, atualizar System State, aplicar cooldown e registrar historico local. A ausencia de MQTT deve produzir resultado observavel, mas nao deve bloquear o loop principal nem tentar reconectar MQTT por conta propria.

Os payloads devem incluir codigo do alerta, categoria, prioridade, estado levantado ou recuperado, timestamp quando disponivel, origem local, contador ou sequencia quando existir, e motivo resumido. Payloads devem respeitar os limites de tamanho definidos pelos contratos MQTT existentes.

QoS deve seguir a politica da Fase 11 para eventos importantes. Publicacoes de alerta e recuperacao devem usar QoS 1 quando suportado pelo boundary MQTT existente.

Adicionar checks de integracao ou smoke da Fase 12 para manager, detector, historico, cooldown, publicacao MQTT com fake e comportamento sem MQTT. Esses checks nao sao testes unitarios e devem ficar em `firmware/test/integration/`.

Adicionar um script de revisao de escopo da Fase 12 para confirmar ausencia de arquivos em `firmware/test/unit/`, ausencia de cloud/app/Supabase/push notification, ausencia de dependencia de hardware real, e ausencia de alteracoes em `architecture/`, `specs/` e `tasks/firmware/firmware-master-plan.md`.

Registrar em comentarios de task, checklist ou saida de script que validacoes reais com hardware, rede real, broker Mosquitto real, reboot fisico, aquario ou bancada sao `Pendente para Hardware Validation`, sem tornar isso criterio de conclusao.

Esta tarefa nao deve implementar cloud sync direto, historico remoto, Supabase, Edge Functions, push notification, aplicativo, UI, comandos remotos, reconexao MQTT propria, resiliencia da Fase 13, testes unitarios ou testes fisicos.

### Arquivos afetados

- `firmware/include/alerts/alert_mqtt_publisher.h`
- `firmware/include/alerts/alert_types.h`
- `firmware/include/alerts/alert_events.h`
- `firmware/include/mqtt/mqtt_topics.h`, somente se necessario para topicos oficiais de alertas
- `firmware/include/mqtt/mqtt_payloads.h`, somente se necessario para payloads de alerta
- `firmware/src/alerts/alert_mqtt_publisher.cpp`
- `firmware/src/mqtt/mqtt_topics.cpp`, somente se necessario para topicos oficiais de alertas
- `firmware/src/mqtt/mqtt_payloads.cpp`, somente se necessario para payloads de alerta
- `firmware/src/app/core_app.h`
- `firmware/src/app/core_app.cpp`
- `firmware/src/app/core_app_arduino.cpp`
- `firmware/test/fakes/fake_alert_mqtt_sink.h`
- `firmware/test/integration/test_alerts_mqtt_publication_smoke.cpp`
- `firmware/test/integration/run_phase12_integration_tests.sh`
- `firmware/test/integration/run_phase12_scope_review.sh`
- `firmware/platformio.ini`, somente se necessario para ambiente de smoke/integracao

### Criterios de aceitacao

- Existe publisher MQTT de alertas atras dos contratos da Fase 11.
- Publisher recebe alertas por contrato local ou Event Bus.
- Publisher nao le sensores ou hardware diretamente.
- Alerta levantado e publicado quando MQTT esta conectado.
- Recuperacao e publicada quando MQTT esta conectado.
- Publicacao usa topico oficial deterministico.
- Payload de alerta contem codigo, categoria, prioridade e estado.
- Payload de alerta contem timestamp quando disponivel.
- Payload de alerta respeita limite de tamanho MQTT existente.
- Publicacao de alerta usa QoS 1 quando suportado pelo boundary existente.
- Falha de publish e representada como resultado observavel.
- MQTT desconectado nao impede alerta local.
- MQTT desconectado nao impede recuperacao local.
- MQTT desconectado nao impede historico local.
- Publisher nao tenta reconectar MQTT por conta propria.
- Publisher nao chama cloud, Supabase, Edge Functions, push notification ou app.
- Checks de integracao ou smoke da Fase 12 existem.
- Script de revisao de escopo da Fase 12 existe.
- Revisao de escopo confirma ausencia de arquivos em `firmware/test/unit/`.
- Revisao de escopo confirma ausencia de cloud/app/historico remoto.
- Revisao de escopo confirma ausencia de dependencia de hardware real nos criterios automatizados.
- Validacoes reais ficam marcadas como `Pendente para Hardware Validation`.
- Nenhum teste unitario e criado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhuma cloud sync direta, historico remoto, app, UI, comando remoto, resiliencia da Fase 13 ou teste fisico e implementado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar `firmware/test/integration/run_phase12_integration_tests.sh` com sucesso.
- Executar `firmware/test/integration/run_phase12_scope_review.sh` com sucesso.
- Executar check de integracao ou smoke confirmando publicacao de alerta com fake MQTT conectado.
- Executar check de integracao ou smoke confirmando publicacao de recuperacao com fake MQTT conectado.
- Executar check de integracao ou smoke confirmando comportamento local quando MQTT esta desconectado.
- Executar check de integracao ou smoke confirmando falha de publish sem travar o firmware.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para esta fase.
- Confirmar que nenhum teste exige ESP32, sensores reais, relays reais, Wi-Fi real, MQTT real, broker Mosquitto real, Internet real, cloud, app, Serial real, reboot fisico ou qualquer validacao fisica.
