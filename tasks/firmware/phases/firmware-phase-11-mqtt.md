# Fase 11 - MQTT

## Escopo

Implementar a comunicacao MQTT em tempo real do firmware REEFLOW com broker Mosquitto, cobrindo conexao, reconexao automatica, publicacao de telemetria derivada exclusivamente do System State, recebimento de comandos pelo ESP32, QoS, heartbeat MQTT e status `network.mqttConnected`.

Esta fase depende da Fase 10, pois Wi-Fi, status de rede, NTP, heartbeat local e configuracoes de rede ja devem existir. Tambem depende da Fase 9 para configuracoes MQTT restauraveis localmente, incluindo broker, porta, credenciais quando houver, client id e parametros de conexao.

MQTT e uma interface externa do ESP32. Ele nao pode substituir automacoes criticas locais, nao pode ser pre-requisito para sensores, relays, ATO, modos, iluminacao, persistencia local, watchdog ou fail-safe, e nao pode tornar cloud ou aplicativo autoridades do aquario.

Toda telemetria publicada deve ser derivada do System State. Nenhuma publicacao deve ler diretamente sensores, drivers, relays, PWM, ATO, modos ou services funcionais como fonte primaria. O fluxo correto e: modulo local atualiza System State, eventos locais sinalizam mudancas, MQTT publica um snapshot ou delta derivado do estado oficial.

Todo comando externo recebido por MQTT deve ser tratado como solicitacao ao ESP32. O firmware deve validar topico, payload, origem, permissao funcional, estado atual e limites locais antes de encaminhar qualquer acao para services ja existentes. MQTT nao deve escrever diretamente em GPIO, PWM, relays, System State, NVS ou estruturas internas de modulos.

QoS 0 deve ser usado para telemetria periodica e snapshots nao criticos. QoS 1 deve ser usado para eventos importantes, confirmacoes de comando, status de conexao e publicacoes em que perda ocasional nao seja aceitavel dentro do contrato MQTT da fase.

Esta fase nao deve implementar cloud bridge, Supabase, historico remoto, push notification, autenticacao de usuarios, app, UI, dashboard, Alert Manager da Fase 12, testes de resiliencia da Fase 13, provisioning Wi-Fi, captive portal, BLE, OTA ou comandos que nao existam nos services locais ja implementados.

A conclusao desta fase nao pode depender de broker Mosquitto real, Internet real, Wi-Fi real, ESP32 conectado, monitor Serial real, reboot fisico, medicao eletrica, aquario, bancada ou qualquer teste fisico. Validacoes reais com broker Mosquitto, rede local, credenciais reais, queda real de Wi-Fi, latencia real, ESP32, antena, Serial, caixa eletrica ou aquario devem ficar registradas como `Pendente para Hardware Validation` e fora dos criterios de conclusao.

As tarefas desta fase nao devem criar testes unitarios. A implementacao deve ser feita com contratos claros, boundaries substituiveis, validacao defensiva, build PlatformIO, revisao automatizada de escopo quando aplicavel e checks de integracao ou smoke que nao dependam de hardware real. Nao criar novos arquivos em `firmware/test/unit/`.

Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` deve ser alterado.

## Estrutura Recomendada

```text
firmware/
|-- include/
|   |-- mqtt/
|   |   |-- mqtt_types.h
|   |   |-- mqtt_config.h
|   |   |-- mqtt_client.h
|   |   |-- mqtt_topics.h
|   |   |-- mqtt_payloads.h
|   |   |-- mqtt_events.h
|   |   |-- mqtt_service.h
|   |   |-- mqtt_telemetry_publisher.h
|   |   |-- mqtt_command_handler.h
|   |   `-- mqtt_outbox.h
|   |-- core/
|   |-- config/
|   `-- network/
|-- src/
|   |-- mqtt/
|   |   |-- arduino_mqtt_client.h
|   |   |-- arduino_mqtt_client.cpp
|   |   |-- mqtt_topics.cpp
|   |   |-- mqtt_payloads.cpp
|   |   |-- mqtt_events.cpp
|   |   |-- mqtt_service.cpp
|   |   |-- mqtt_telemetry_publisher.cpp
|   |   |-- mqtt_command_handler.cpp
|   |   `-- mqtt_outbox.cpp
|   |-- app/
|   `-- network/
`-- test/
    |-- fakes/
    |   `-- fake_mqtt_client.h
    |-- integration/
    |   |-- test_mqtt_boot_scheduler_integration.cpp
    |   |-- test_mqtt_scope_contract.cpp
    |   |-- run_phase11_integration_tests.sh
    |   `-- run_phase11_scope_review.sh
    `-- hardware/
```

`firmware/test/hardware/` fica reservado para validacoes futuras e nao e criterio de conclusao da Fase 11.

## Tarefas

### ID

FW-P11-000

### Titulo

Definir contratos MQTT, topicos, payloads, eventos e boundary substituivel

### Objetivo

Criar os contratos publicos da camada MQTT da Fase 11, delimitando conexao Mosquitto, topicos oficiais, payloads, QoS, eventos locais e um boundary substituivel para permitir implementacao real sem acoplar testes ou services ao broker real.

### Dependencias

Fase 9 concluida.

Fase 10 concluida.

### Descricao

Definir o modulo `mqtt` com tipos funcionais para status de conexao, motivo de desconexao, resultado de publish, resultado de subscribe, resultado de comando recebido, QoS, retain flag, sessao limpa, backoff, heartbeat MQTT e estado de outbox.

Definir um boundary `MqttClient` ou equivalente para encapsular operacoes de MQTT: configurar broker, conectar, desconectar, verificar conexao, publicar, assinar topicos, processar loop de rede e entregar mensagens recebidas ao firmware. O boundary deve permitir uma implementacao real usando uma biblioteca MQTT compativel com ESP32 e um fake restrito a validacoes de integracao ou smoke.

Definir topicos oficiais da Fase 11 para telemetria, eventos, heartbeat, status de conexao, comandos recebidos e respostas de comando. Os nomes devem ser deterministicos e derivados de um prefixo local configuravel e de identificador do dispositivo quando disponivel.

Definir payloads serializaveis para telemetria derivada do System State: temperatura, nivel, relays, modo atual, PWM atual, status de Wi-Fi, status MQTT e status do sistema. Os payloads devem carregar timestamp quando ja houver Time Source/NTP disponivel, ou marcador local deterministico quando tempo absoluto nao estiver sincronizado.

Declarar eventos locais `MQTT_CONNECTED`, `MQTT_DISCONNECTED` e `MQTT_RECONNECTED`, mapeados para o Event Bus local. Eventos MQTT nao devem criar Alert Manager, persistir historico remoto, chamar cloud, disparar app ou alterar diretamente automacoes criticas.

Criar configuracao local MQTT com broker, porta, client id, credenciais opcionais, keepalive, timeout, intervalo de heartbeat MQTT, limites de payload, limites de outbox, backoff inicial e backoff maximo. A configuracao deve vir do Config Manager quando ja restaurada pela Fase 9 e deve ter defaults seguros para ambiente sem MQTT configurado.

Criar fake ou mock de cliente MQTT apenas fora do build real, em `firmware/test/fakes/`, para smoke/integracao. O fake deve simular conexao, desconexao, publish, subscribe, entrega de mensagem, falha de publish, falha de subscribe e contagem de chamadas.

Esta tarefa nao deve implementar cliente MQTT real, reconexao runtime, publicacao periodica, comandos remotos, outbox funcional, cloud bridge, Alert Manager, testes unitarios ou testes fisicos.

### Arquivos afetados

- `firmware/include/mqtt/mqtt_types.h`
- `firmware/include/mqtt/mqtt_config.h`
- `firmware/include/mqtt/mqtt_client.h`
- `firmware/include/mqtt/mqtt_topics.h`
- `firmware/include/mqtt/mqtt_payloads.h`
- `firmware/include/mqtt/mqtt_events.h`
- `firmware/include/mqtt/mqtt_service.h`
- `firmware/include/mqtt/mqtt_telemetry_publisher.h`
- `firmware/include/mqtt/mqtt_command_handler.h`
- `firmware/include/mqtt/mqtt_outbox.h`
- `firmware/src/mqtt/mqtt_topics.cpp`
- `firmware/src/mqtt/mqtt_payloads.cpp`
- `firmware/src/mqtt/mqtt_events.cpp`
- `firmware/include/core/events/event_bus.h`, somente se necessario para registrar eventos `MQTT_*`
- `firmware/src/core/events/event_bus.cpp`, somente se necessario para registrar eventos `MQTT_*`
- `firmware/include/config/config_manager.h`, somente se necessario para expor configuracao MQTT ja restaurada
- `firmware/src/config/config_manager.cpp`, somente se necessario para expor configuracao MQTT ja restaurada
- `firmware/test/fakes/fake_mqtt_client.h`
- `firmware/platformio.ini`, somente se necessario para dependencias de build

### Criterios de aceitacao

- Existe modulo `mqtt` com contratos publicos da Fase 11.
- Existe boundary substituivel para cliente MQTT.
- O boundary representa conexao bem sucedida.
- O boundary representa broker indisponivel.
- O boundary representa credencial ausente quando obrigatoria.
- O boundary representa falha de autenticacao.
- O boundary representa desconexao durante runtime.
- O boundary representa falha de publish.
- O boundary representa falha de subscribe.
- Topicos oficiais da Fase 11 sao deterministicos.
- Topicos usam prefixo local configuravel.
- Topicos podem incluir identificador do dispositivo quando disponivel.
- Payloads de telemetria sao derivados do System State.
- Payloads nao leem sensores ou drivers diretamente.
- QoS 0 esta definido para telemetria periodica.
- QoS 1 esta definido para eventos importantes e respostas de comando.
- Existem eventos locais `MQTT_CONNECTED`, `MQTT_DISCONNECTED` e `MQTT_RECONNECTED`.
- Eventos MQTT sao mapeados para o Event Bus local.
- Eventos MQTT nao criam Alert Manager.
- Eventos MQTT nao chamam cloud ou app.
- Configuracao local MQTT cobre broker, porta, client id, credenciais opcionais, keepalive, timeout, heartbeat e backoff.
- Existe fake de cliente MQTT restrito a validacoes sem broker real.
- O build real do firmware nao depende do fake.
- Nenhum teste unitario e criado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhum cliente MQTT real, telemetria periodica, comando remoto, cloud bridge, app ou teste fisico e implementado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar check de compilacao dos contratos MQTT no build real.
- Executar check de integracao ou smoke confirmando que o fake MQTT nao entra no build real.
- Executar check de integracao ou smoke confirmando que os topicos oficiais sao gerados de forma deterministica.
- Executar check de integracao ou smoke confirmando que payloads usam dados recebidos do System State.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para esta tarefa.
- Confirmar que nenhum teste exige ESP32, broker Mosquitto real, Wi-Fi real, Internet real, cloud, app, Serial real ou qualquer validacao fisica.

---

### ID

FW-P11-001

### Titulo

Implementar cliente MQTT real, conexao Mosquitto e status no System State

### Objetivo

Implementar a conexao MQTT real com Mosquitto atras do boundary da Fase 11, usando configuracao local restaurada, rede da Fase 10 e atualizacao segura de `network.mqttConnected`.

### Dependencias

FW-P11-000.

Fase 10 concluida.

### Descricao

Implementar `ArduinoMqttClient` ou equivalente em `src/mqtt/`, isolado atras do boundary `MqttClient`. A implementacao real deve compilar para ESP32 e usar uma biblioteca MQTT adequada ao ambiente PlatformIO do firmware.

Implementar `MqttService` ou equivalente para iniciar conexao somente quando Wi-Fi estiver conectado, broker estiver configurado e o firmware tiver configuracao MQTT valida em memoria. Ausencia de broker ou client id deve manter MQTT desabilitado ou nao configurado, sem logs repetitivos e sem loop agressivo.

Atualizar `system.network.mqttConnected` para verdadeiro somente depois de conexao MQTT confirmada. Em desconexao, timeout, falha de autenticacao ou broker indisponivel, atualizar `mqttConnected` para falso sem alterar `wifiConnected`, sensores, relays, ATO, modos, iluminacao ou storage.

Assinar somente topicos de comando oficiais da Fase 11. A assinatura deve acontecer apos conexao bem sucedida e deve falhar de forma observavel sem executar comandos parciais.

Emitir `MQTT_CONNECTED`, `MQTT_DISCONNECTED` e `MQTT_RECONNECTED` quando aplicavel. Reconexao deve usar backoff progressivo configuravel, nao bloqueante, sem `delay`, busy wait ou loop que impeça Scheduler, Watchdog e automacoes locais de continuar operando.

Esta tarefa nao deve publicar telemetria periodica, implementar parsing de comandos, executar comandos remotos, criar outbox persistente, criar Alert Manager, chamar cloud, app, Supabase, historico remoto, testes unitarios ou testes fisicos.

### Arquivos afetados

- `firmware/src/mqtt/arduino_mqtt_client.h`
- `firmware/src/mqtt/arduino_mqtt_client.cpp`
- `firmware/include/mqtt/mqtt_client.h`
- `firmware/include/mqtt/mqtt_service.h`
- `firmware/src/mqtt/mqtt_service.cpp`
- `firmware/include/mqtt/mqtt_events.h`
- `firmware/src/mqtt/mqtt_events.cpp`
- `firmware/include/mqtt/mqtt_config.h`
- `firmware/include/core/state/system_state.h`, somente se necessario para atualizar campos `network` ja existentes
- `firmware/src/core/state/system_state.cpp`, somente se necessario para atualizar campos `network` ja existentes
- `firmware/src/app/core_app.h`
- `firmware/src/app/core_app.cpp`
- `firmware/src/app/core_app_arduino.cpp`
- `firmware/platformio.ini`, somente se necessario para biblioteca MQTT real

### Criterios de aceitacao

- Existe implementacao real de cliente MQTT para ESP32 atras do boundary.
- Existe `MqttService` ou equivalente.
- O service consome configuracao MQTT local restaurada.
- MQTT nao tenta conectar quando Wi-Fi esta desconectado.
- MQTT nao tenta conectar agressivamente quando broker nao esta configurado.
- Conexao bem sucedida marca `network.mqttConnected` como verdadeiro.
- Desconexao marca `network.mqttConnected` como falso.
- Falha de autenticacao e representada como resultado testavel ou log funcional.
- Broker indisponivel e representado como resultado testavel ou log funcional.
- Timeout de conexao e representado como resultado testavel ou log funcional.
- Reconexao usa backoff progressivo configuravel.
- Reconexao nao usa `delay`.
- Reconexao nao bloqueia o loop principal.
- Scheduler continua podendo executar outras tarefas durante tentativas MQTT.
- Watchdog continua podendo ser alimentado durante tentativas MQTT.
- Topicos oficiais de comando sao assinados apos conexao.
- Falha de assinatura nao executa comando remoto.
- Eventos `MQTT_*` sao emitidos localmente.
- Perda de MQTT nao altera sensores, relays, ATO, modos ou iluminacao.
- MQTT nao assume responsabilidade critica do aquario.
- Nenhuma telemetria periodica, parsing de comando, outbox persistente, Alert Manager, cloud, app ou teste fisico e implementado nesta tarefa.
- Nenhum teste unitario e criado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar check de integracao ou smoke com fake MQTT confirmando conexao bem sucedida.
- Executar check de integracao ou smoke com fake MQTT confirmando broker ausente.
- Executar check de integracao ou smoke com fake MQTT confirmando falha de autenticacao.
- Executar check de integracao ou smoke com fake MQTT confirmando desconexao runtime.
- Executar check de integracao ou smoke confirmando atualizacao de `network.mqttConnected`.
- Executar check de integracao ou smoke confirmando que perda de MQTT nao altera ATO, relays, modos ou iluminacao.
- Executar busca automatizada confirmando ausencia de `delay` na reconexao MQTT.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para esta tarefa.
- Confirmar que nenhum teste exige ESP32, broker Mosquitto real, Wi-Fi real, Internet real, cloud, app, Serial real ou qualquer validacao fisica.

---

### ID

FW-P11-002

### Titulo

Publicar telemetria e heartbeat MQTT derivados do System State

### Objetivo

Implementar publicacao MQTT de telemetria, eventos importantes e heartbeat MQTT usando exclusivamente snapshots ou deltas derivados do System State.

### Dependencias

FW-P11-001.

### Descricao

Implementar `MqttTelemetryPublisher` ou equivalente para publicar telemetria periodica com QoS 0: temperatura, nivel, estado dos relays, modo atual, PWM atual, status de Wi-Fi, status MQTT e status do sistema. A publicacao deve serializar somente dados existentes no System State.

Implementar publicacao de eventos importantes com QoS 1 quando houver mudancas relevantes ja sinalizadas pelo Event Bus local, incluindo eventos MQTT, relay, modo, iluminacao e status critico ja existente. Esta tarefa nao deve criar Alert Manager nem inventar categorias de alerta da Fase 12.

Implementar heartbeat MQTT a cada 30 segundos, conforme `specs/mqtt-spec.md`, sem substituir o heartbeat local de rede da Fase 10. O heartbeat MQTT deve indicar que a sessao MQTT esta viva e deve ser publicado apenas quando MQTT estiver conectado.

Evitar publicacoes redundantes: telemetria periodica pode publicar snapshot no intervalo configurado, enquanto eventos importantes devem publicar quando houver mudanca ou evento local. O service deve respeitar limite de payload e falhas de publish sem bloquear automacoes locais.

Falha de publish deve ser tratada como estado funcional do MQTT. A falha nao deve alterar diretamente sensores, relays, ATO, modos, iluminacao ou Config Manager. Quando necessario, a falha pode alimentar outbox volatil definida na tarefa posterior, sem persistencia obrigatoria nesta tarefa.

Esta tarefa nao deve receber comandos, executar comandos remotos, criar historico remoto, chamar cloud bridge, criar Alert Manager, criar testes unitarios ou depender de broker real.

### Arquivos afetados

- `firmware/include/mqtt/mqtt_telemetry_publisher.h`
- `firmware/src/mqtt/mqtt_telemetry_publisher.cpp`
- `firmware/include/mqtt/mqtt_payloads.h`
- `firmware/src/mqtt/mqtt_payloads.cpp`
- `firmware/include/mqtt/mqtt_topics.h`
- `firmware/src/mqtt/mqtt_topics.cpp`
- `firmware/include/mqtt/mqtt_service.h`
- `firmware/src/mqtt/mqtt_service.cpp`
- `firmware/include/mqtt/mqtt_config.h`
- `firmware/src/app/core_app.h`
- `firmware/src/app/core_app.cpp`
- `firmware/test/integration/test_mqtt_boot_scheduler_integration.cpp`, somente se ja houver padrao de integracao sem broker real
- `firmware/test/integration/run_phase11_integration_tests.sh`, somente se usado como smoke sem unit tests

### Criterios de aceitacao

- Telemetria de temperatura e publicada a partir de `system.temperature`.
- Telemetria de nivel e publicada a partir de `system.waterLevel`.
- Telemetria de relays e publicada a partir de `system.relays`.
- Telemetria de modo atual e publicada a partir de `system.currentMode`.
- Telemetria de PWM atual e publicada a partir de `system.lighting`.
- Telemetria de Wi-Fi e publicada a partir de `system.network`.
- Telemetria de status MQTT e publicada a partir de `system.network.mqttConnected`.
- Telemetria de status do sistema e publicada a partir de `system.systemHealth` ou campo equivalente ja existente.
- Nenhuma telemetria le diretamente sensor, driver, GPIO, PWM, relay controller, ATO service, mode service ou lighting service como fonte primaria.
- Telemetria periodica usa QoS 0.
- Eventos importantes usam QoS 1.
- Heartbeat MQTT publica a cada 30 segundos quando MQTT esta conectado.
- Heartbeat MQTT nao substitui heartbeat local da Fase 10.
- MQTT desconectado nao tenta publicar telemetria.
- Falha de publish nao bloqueia automacoes locais.
- Falha de publish nao altera estados criticos.
- Payload respeita limite configuravel.
- Publicacoes redundantes sao limitadas por intervalo, mudanca de estado ou evento local.
- Nenhum comando remoto, Alert Manager, cloud bridge, historico remoto ou teste fisico e implementado.
- Nenhum teste unitario e criado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar check de integracao ou smoke confirmando publish de snapshot derivado do System State.
- Executar check de integracao ou smoke confirmando QoS 0 para telemetria.
- Executar check de integracao ou smoke confirmando QoS 1 para evento importante.
- Executar check de integracao ou smoke confirmando heartbeat MQTT em intervalo de 30 segundos simulado.
- Executar check de integracao ou smoke confirmando que MQTT desconectado nao publica.
- Executar busca automatizada confirmando que o publisher nao inclui drivers de sensores, GPIO, PWM ou relay controller como fonte primaria.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para esta tarefa.
- Confirmar que nenhum teste exige ESP32, broker Mosquitto real, Wi-Fi real, Internet real, cloud, app, Serial real ou qualquer validacao fisica.

---

### ID

FW-P11-003

### Titulo

Receber comandos MQTT e encaminhar solicitacoes validadas aos services locais

### Objetivo

Implementar recebimento de comandos MQTT para troca de modo, controle de relays, controle de iluminacao e configuracoes permitidas, garantindo que o ESP32 valide e decida localmente antes de executar qualquer acao.

### Dependencias

FW-P11-001.

FW-P11-002.

### Descricao

Implementar `MqttCommandHandler` ou equivalente para receber mensagens dos topicos oficiais de comando, validar topico, tamanho, formato, versao de payload, tipo de comando, correlacao opcional e campos obrigatorios.

Comandos MQTT devem ser convertidos em solicitacoes para services locais existentes. Troca de modo deve chamar a API local de modos. Controle de relays deve chamar a API local de relays respeitando origem MQTT quando o contrato local permitir, ou um adapter de solicitacao externa validada. Controle de iluminacao deve chamar a API local de iluminacao respeitando limites, canais e modos ja existentes. Configuracoes devem passar pelo Config Manager e regras locais ja existentes.

O handler deve rejeitar payload invalido, comando desconhecido, topico nao permitido, valor fora de faixa, canal inexistente, relay inexistente, modo invalido, comando que conflite com fail-safe local, comando recebido com MQTT desconectado ou comando para funcionalidade ainda nao implementada.

Respostas de comando devem ser publicadas em topico oficial de resposta com QoS 1, contendo resultado aceito, rejeitado, invalido, nao suportado, conflito local ou erro interno. A resposta deve carregar correlacao quando fornecida e nunca mascarar rejeicao como sucesso.

MQTT nao deve escrever diretamente no System State. O estado deve mudar apenas por meio dos services locais e seus fluxos normais. MQTT tambem nao deve acionar diretamente GPIO, PWM, relay controller, ATO pump, storage backend ou hardware.

Esta tarefa nao deve criar novos comandos de automacao critica, nao deve implementar UI/app/cloud, nao deve criar Alert Manager, nao deve criar testes unitarios e nao deve depender de broker real.

### Arquivos afetados

- `firmware/include/mqtt/mqtt_command_handler.h`
- `firmware/src/mqtt/mqtt_command_handler.cpp`
- `firmware/include/mqtt/mqtt_payloads.h`
- `firmware/src/mqtt/mqtt_payloads.cpp`
- `firmware/include/mqtt/mqtt_topics.h`
- `firmware/src/mqtt/mqtt_topics.cpp`
- `firmware/include/mqtt/mqtt_service.h`
- `firmware/src/mqtt/mqtt_service.cpp`
- `firmware/src/app/core_app.h`
- `firmware/src/app/core_app.cpp`
- `firmware/include/modules/modes/mode_service.h`, somente se necessario para aceitar origem MQTT validada sem quebrar origem local
- `firmware/include/modules/relays/relay_service.h`, somente se necessario para aceitar origem MQTT validada sem acesso direto a hardware
- `firmware/include/modules/lighting/lighting_service.h`, somente se necessario para aceitar origem MQTT validada sem acesso direto a PWM
- `firmware/include/config/config_manager.h`, somente se necessario para configuracoes permitidas por comando MQTT
- `firmware/src/config/config_manager.cpp`, somente se necessario para configuracoes permitidas por comando MQTT
- `firmware/test/integration/test_mqtt_scope_contract.cpp`, somente se usado como check de integracao sem unit tests

### Criterios de aceitacao

- Comandos MQTT sao aceitos somente em topicos oficiais.
- Payload de comando possui validacao de tamanho.
- Payload de comando possui validacao de formato.
- Payload de comando possui validacao de tipo.
- Payload de comando possui validacao de campos obrigatorios.
- Comando desconhecido e rejeitado.
- Topico nao permitido e rejeitado.
- Valor fora de faixa e rejeitado.
- Relay inexistente e rejeitado.
- Canal de iluminacao inexistente e rejeitado.
- Modo invalido e rejeitado.
- Comando para funcionalidade futura e rejeitado como nao suportado.
- Troca de modo passa pelo service local de modos.
- Controle de relay passa pelo service local de relays.
- Controle de iluminacao passa pelo service local de iluminacao.
- Configuracoes permitidas passam pelo Config Manager.
- MQTT nao escreve diretamente no System State.
- MQTT nao aciona GPIO diretamente.
- MQTT nao aciona PWM diretamente.
- MQTT nao aciona relay controller diretamente.
- MQTT nao aciona ATO pump diretamente.
- MQTT nao escreve direto no backend de storage.
- Fail-safe local prevalece sobre comando MQTT.
- Resposta de comando usa QoS 1.
- Resposta de comando diferencia aceito, rejeitado, invalido, nao suportado, conflito local e erro interno.
- Correlacao de comando e preservada quando fornecida.
- Nenhum app, cloud bridge, Alert Manager, comando futuro, teste unitario ou teste fisico e implementado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar check de integracao ou smoke confirmando comando valido de modo encaminhado ao service local.
- Executar check de integracao ou smoke confirmando comando valido de relay encaminhado ao service local.
- Executar check de integracao ou smoke confirmando comando valido de iluminacao encaminhado ao service local.
- Executar check de integracao ou smoke confirmando rejeicao de payload invalido.
- Executar check de integracao ou smoke confirmando rejeicao de topico nao permitido.
- Executar check de integracao ou smoke confirmando resposta de comando com QoS 1.
- Executar busca automatizada confirmando que o command handler nao chama GPIO, PWM, relay controller, ATO pump ou storage backend diretamente.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para esta tarefa.
- Confirmar que nenhum teste exige ESP32, broker Mosquitto real, Wi-Fi real, Internet real, cloud, app, Serial real ou qualquer validacao fisica.

---

### ID

FW-P11-004

### Titulo

Implementar QoS, outbox volatil e recuperacao de publicacoes importantes

### Objetivo

Implementar tratamento funcional de QoS, retentativas e outbox volatil para eventos importantes e respostas de comando, mantendo telemetria periodica simples e nao bloqueante.

### Dependencias

FW-P11-002.

FW-P11-003.

### Descricao

Implementar `MqttOutbox` ou equivalente em memoria para mensagens QoS 1 importantes que falharem temporariamente por perda de conexao, broker indisponivel ou falha transitora de publish. A outbox deve ser volatil nesta fase e nao deve usar NVS, storage persistente, arquivo, cloud ou historico remoto.

Definir politica de retentativa com limite de tamanho da fila, limite de idade da mensagem, numero maximo de tentativas quando aplicavel e descarte explicito quando a mensagem expirar ou a fila estiver cheia. O descarte deve ser observavel por log ou resultado funcional, sem criar Alert Manager.

Telemetria periodica QoS 0 nao deve inflar a outbox. Snapshots periodicos podem ser descartados quando MQTT estiver offline, pois novo snapshot sera publicado quando conexao voltar.

Eventos importantes e respostas de comando QoS 1 podem ser enfileirados enquanto MQTT estiver temporariamente offline, respeitando limites de memoria. Ao reconectar, a outbox deve drenar de forma nao bloqueante e sem impedir Scheduler, Watchdog e automacoes locais.

Evitar duplicidade indevida: mensagens com correlacao de comando devem manter correlacao, e publicacoes repetidas por retry devem ser aceitaveis dentro da semantica QoS 1. A implementacao nao deve prometer exatamente-once.

Esta tarefa nao deve implementar persistencia da outbox, historico remoto, Alert Manager, resiliencia ampla da Fase 13, cloud bridge, app, testes unitarios ou testes fisicos.

### Arquivos afetados

- `firmware/include/mqtt/mqtt_outbox.h`
- `firmware/src/mqtt/mqtt_outbox.cpp`
- `firmware/include/mqtt/mqtt_service.h`
- `firmware/src/mqtt/mqtt_service.cpp`
- `firmware/include/mqtt/mqtt_telemetry_publisher.h`
- `firmware/src/mqtt/mqtt_telemetry_publisher.cpp`
- `firmware/include/mqtt/mqtt_command_handler.h`
- `firmware/src/mqtt/mqtt_command_handler.cpp`
- `firmware/include/mqtt/mqtt_config.h`
- `firmware/test/integration/test_mqtt_boot_scheduler_integration.cpp`, somente se usado como check de integracao sem unit tests

### Criterios de aceitacao

- Existe outbox volatil em memoria para MQTT.
- Outbox nao usa NVS.
- Outbox nao usa storage persistente.
- Outbox nao chama cloud.
- Outbox nao cria historico remoto.
- Mensagens QoS 1 importantes podem ser enfileiradas quando MQTT esta temporariamente offline.
- Respostas de comando QoS 1 podem ser enfileiradas quando MQTT esta temporariamente offline.
- Telemetria QoS 0 periodica nao entra na outbox.
- Fila possui tamanho maximo configuravel.
- Mensagens possuem idade maxima configuravel ou criterio equivalente de descarte.
- Tentativas de reenvio sao limitadas ou observaveis por politica clara.
- Fila cheia descarta de forma explicita e observavel.
- Mensagem expirada descarta de forma explicita e observavel.
- Drenagem da outbox apos reconexao e nao bloqueante.
- Drenagem nao impede Scheduler.
- Drenagem nao impede Watchdog.
- Drenagem nao altera automacoes locais.
- Correlacao de comando e preservada nas respostas reenviadas.
- Implementacao nao promete exactly-once.
- Nenhum Alert Manager, persistencia de outbox, historico remoto, cloud bridge, app, teste unitario ou teste fisico e implementado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar check de integracao ou smoke confirmando enfileiramento de evento QoS 1 com MQTT offline.
- Executar check de integracao ou smoke confirmando que telemetria QoS 0 offline e descartada.
- Executar check de integracao ou smoke confirmando drenagem apos reconexao simulada.
- Executar check de integracao ou smoke confirmando limite de tamanho da outbox.
- Executar check de integracao ou smoke confirmando descarte observavel por expiracao ou fila cheia.
- Executar check de integracao ou smoke confirmando preservacao de correlacao de comando.
- Executar busca automatizada confirmando que outbox nao usa NVS, storage backend, cloud ou historico remoto.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para esta tarefa.
- Confirmar que nenhum teste exige ESP32, broker Mosquitto real, Wi-Fi real, Internet real, cloud, app, Serial real ou qualquer validacao fisica.

---

### ID

FW-P11-005

### Titulo

Integrar MQTT ao runtime do firmware e fechar validacao de escopo da Fase 11

### Objetivo

Integrar os services MQTT ao `CoreApp`, Scheduler, Event Bus, Logger e configuracao local, garantindo que a Fase 11 esteja operacional sem ultrapassar seu escopo e sem depender de testes fisicos.

### Dependencias

FW-P11-004.

### Descricao

Integrar `MqttService`, `MqttTelemetryPublisher`, `MqttCommandHandler` e `MqttOutbox` ao runtime principal do firmware. As tarefas MQTT devem ser registradas no Scheduler com intervalos configuraveis, sem bloquear o loop principal e sem interferir em sensores, relays, ATO, modos, iluminacao, storage, rede ou watchdog.

Garantir ordem de inicializacao segura: core local inicia primeiro, configuracoes locais sao restauradas, rede da Fase 10 opera, MQTT so tenta conectar quando a rede estiver pronta e a configuracao MQTT estiver valida. Falha de MQTT deve manter o controlador local funcional.

Consolidar logs funcionais de MQTT para conexao, desconexao, reconexao, publish falho, subscribe falho, comando rejeitado e outbox descartada. Logs nao devem vazar credenciais MQTT.

Criar ou atualizar scripts de validacao de integracao e revisao de escopo da Fase 11 sem criar testes unitarios. A revisao de escopo deve bloquear alteracoes em `architecture/`, `specs/`, cloud, mobile, Alert Manager da Fase 12, resiliencia da Fase 13, testes fisicos obrigatorios e criacao de arquivos em `firmware/test/unit/` para esta fase.

Registrar no proprio arquivo de validacao, quando aplicavel, que testes com broker Mosquitto real, rede local real, ESP32 real, monitor Serial real e aquario ficam como `Pendente para Hardware Validation`.

Esta tarefa nao deve implementar Alert Manager, cloud bridge, Supabase, app, historico remoto, push notification, OTA, testes de resiliencia da Fase 13, novas funcionalidades de automacao ou testes fisicos.

### Arquivos afetados

- `firmware/src/app/core_app.h`
- `firmware/src/app/core_app.cpp`
- `firmware/src/app/core_app_arduino.cpp`
- `firmware/include/mqtt/mqtt_service.h`
- `firmware/src/mqtt/mqtt_service.cpp`
- `firmware/include/mqtt/mqtt_telemetry_publisher.h`
- `firmware/src/mqtt/mqtt_telemetry_publisher.cpp`
- `firmware/include/mqtt/mqtt_command_handler.h`
- `firmware/src/mqtt/mqtt_command_handler.cpp`
- `firmware/include/mqtt/mqtt_outbox.h`
- `firmware/src/mqtt/mqtt_outbox.cpp`
- `firmware/test/integration/test_mqtt_boot_scheduler_integration.cpp`
- `firmware/test/integration/test_mqtt_scope_contract.cpp`
- `firmware/test/integration/run_phase11_integration_tests.sh`
- `firmware/test/integration/run_phase11_scope_review.sh`
- `firmware/test/run_phase11_validation.sh`
- `firmware/README.md`, somente se ja existir secao de validacao por fase e for necessario registrar comandos
- `firmware/platformio.ini`, somente se necessario para targets de integracao sem unit tests

### Criterios de aceitacao

- MQTT e integrado ao runtime principal do firmware.
- Core local continua iniciando sem MQTT configurado.
- Configuracao local e restaurada antes da tentativa MQTT.
- MQTT so tenta conectar quando Wi-Fi esta pronto.
- MQTT so tenta conectar quando configuracao MQTT e valida.
- Falha de MQTT nao impede sensores.
- Falha de MQTT nao impede relays.
- Falha de MQTT nao impede ATO.
- Falha de MQTT nao impede modos.
- Falha de MQTT nao impede iluminacao.
- Falha de MQTT nao impede storage local.
- Falha de MQTT nao impede Watchdog.
- Tarefas MQTT sao registradas no Scheduler com intervalos configuraveis.
- Tarefas MQTT nao usam `delay`.
- Tarefas MQTT nao usam loop bloqueante.
- Logs MQTT nao vazam credenciais.
- Existe validacao de integracao ou smoke da Fase 11 sem broker real obrigatorio.
- Existe revisao automatizada de escopo da Fase 11.
- Revisao de escopo bloqueia alteracao em `architecture/`.
- Revisao de escopo bloqueia alteracao em `specs/`.
- Revisao de escopo bloqueia cloud, mobile, Supabase, app e UI.
- Revisao de escopo bloqueia Alert Manager da Fase 12.
- Revisao de escopo bloqueia resiliencia ampla da Fase 13.
- Revisao de escopo bloqueia testes fisicos como criterio obrigatorio.
- Revisao de escopo bloqueia criacao de arquivos em `firmware/test/unit/` para esta fase.
- Validacoes reais com broker Mosquitto, rede real, ESP32 real e aquario ficam como `Pendente para Hardware Validation`.
- Nenhum teste unitario e criado.
- Nenhum arquivo em `firmware/test/unit/` e criado ou alterado por esta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar `firmware/test/integration/run_phase11_integration_tests.sh`, se criado, sem depender de broker real.
- Executar `firmware/test/integration/run_phase11_scope_review.sh`, se criado.
- Executar `firmware/test/run_phase11_validation.sh`, se criado, incluindo build, integracao sem broker real e revisao de escopo.
- Executar busca automatizada confirmando que nenhum arquivo em `firmware/test/unit/` foi criado para a Fase 11.
- Executar busca automatizada confirmando ausencia de dependencias de cloud, mobile, Supabase, app, UI e Alert Manager.
- Executar busca automatizada confirmando ausencia de `delay` em services MQTT.
- Confirmar que MQTT desconectado nao bloqueia inicializacao do firmware.
- Confirmar que os checks nao exigem ESP32, broker Mosquitto real, Wi-Fi real, Internet real, cloud, app, Serial real ou qualquer validacao fisica.
- Confirmar que validacoes fisicas pendentes estao registradas como `Pendente para Hardware Validation`.
