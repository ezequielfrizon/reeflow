# Fase 9 - Persistencia Local

## Escopo

Implementar persistencia local do firmware REEFLOW usando NVS Preferences para salvar e restaurar configuracoes persistiveis apos reboot. A fase deve cobrir Wi-Fi, MQTT, curvas e configuracoes da luminaria, configuracoes do ATO, limites de temperatura, timers, modo atual e calibracoes, mantendo o ESP32 como autoridade soberana e o System State como fonte unica de verdade em runtime.

Esta fase depende dos modulos que ja possuem configuracoes persistiveis e deve usar os contratos existentes de System State, Event Bus, Config Manager, Scheduler, Logger, Watchdog, Modes e Lighting. A persistencia deve restaurar configuracoes para os modulos locais sem depender de Internet, MQTT, cloud, aplicativo, Serial real, ESP32 conectado, NVS fisica validada em bancada, sensores reais, reles reais, luminaria real ou qualquer teste fisico.

Mocks e fakes devem ser usados para validacoes automatizadas das implementacoes, seguindo o padrao das fases anteriores. Mocks e fakes devem ficar restritos a `firmware/test/` ou a configuracoes de teste, sem se tornarem dependencia do build real do firmware destinado ao hardware.

O firmware real deve ficar preparado para usar NVS Preferences no ESP32, mas a conclusao desta fase nao pode depender de reboot fisico real, flash real, ciclo de energia real, monitor Serial real, medicao eletrica ou bancada. Qualquer validacao com ESP32, NVS real, reboot fisico, energia real, desgaste real de flash, Wi-Fi real, broker MQTT real, sensores, reles, luminaria ou aquario deve ficar registrada como `Pendente para Hardware Validation` e fora dos criterios de conclusao desta fase.

Nao implementar Wi-Fi da Fase 10, MQTT runtime da Fase 11, Alert Manager da Fase 12, testes de resiliencia da Fase 13, cloud, aplicativo, historico, notificacoes, comandos remotos, backup remoto, migracao cloud, provisionamento Wi-Fi, reconexao, NTP, heartbeat ou funcionalidades de fases posteriores.

Wi-Fi e MQTT nesta fase significam apenas configuracoes persistidas e restauradas no Config Manager. A fase nao deve conectar rede, validar credenciais contra access point, abrir broker, publicar telemetria, receber comandos ou alterar `network` como se houvesse conectividade real.

Persistir somente dados configuraveis e estado explicitamente persistivel. Nao persistir leituras volateis como `temperature.currentTemperature`, `waterLevel.currentLevel`, status runtime de sensores, `relays.*.enabled`, `lighting.*.currentPWM`, `ato.pumpRunning`, `network.wifiConnected`, `network.mqttConnected`, `alerts.activeAlerts`, `systemHealth.uptime`, heap, CPU ou qualquer dado derivado de hardware em execucao.

A politica de escrita deve salvar apenas quando houver mudanca efetiva, evitar gravacoes excessivas, permitir coalescencia ou flush controlado pelo scheduler e nunca gravar a cada ciclo de loop. Falhas simuladas de leitura, escrita, namespace ausente, dados corrompidos, schema invalido ou reset devem ser tratadas sem corromper a configuracao atual em memoria.

Eventos oficiais da fase sao `CONFIG_SAVED`, `CONFIG_RESTORED` e `CONFIG_RESET`, conforme `specs/storage-spec.md`. Esses eventos devem permanecer locais ao firmware nesta fase. Eles nao devem publicar MQTT, criar alertas centralizados, persistir historico, enviar notificacoes ou sincronizar cloud.

Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` deve ser alterado.

## Estrutura Recomendada

```text
firmware/
|-- include/
|   |-- storage/
|   |   |-- storage_types.h
|   |   |-- storage_schema.h
|   |   |-- storage_backend.h
|   |   |-- storage_service.h
|   |   |-- storage_events.h
|   |   `-- storage_write_policy.h
|   |-- config/
|   |-- core/
|   `-- modules/
|       |-- lighting/
|       `-- modes/
|-- src/
|   |-- storage/
|   |   |-- preferences_storage_backend.h
|   |   |-- preferences_storage_backend.cpp
|   |   |-- storage_service.cpp
|   |   |-- storage_events.cpp
|   |   `-- storage_write_policy.cpp
|   |-- app/
|   |-- config/
|   `-- modules/
|       |-- lighting/
|       `-- modes/
`-- test/
    |-- fakes/
    |   `-- fake_storage_backend.h
    |-- unit/
    |   |-- test_storage_contracts.cpp
    |   |-- test_storage_backend.cpp
    |   |-- test_storage_write_policy.cpp
    |   |-- test_storage_service.cpp
    |   `-- test_storage_domains.cpp
    |-- integration/
    |   |-- test_storage_boot_restore_integration.cpp
    |   |-- test_storage_scheduler_integration.cpp
    |   |-- run_phase9_integration_tests.sh
    |   `-- run_phase9_scope_review.sh
    `-- hardware/
```

`firmware/test/hardware/` fica reservado para validacoes futuras e nao e criterio de conclusao da Fase 9.

## Tarefas

### ID

FW-P09-000

### Titulo

Definir contratos de storage, schema persistivel, eventos e fake de backend

### Objetivo

Criar os contratos publicos do modulo de persistencia local, delimitando dominios persistiveis, schema, versionamento local, eventos oficiais e backend substituivel para validacao automatizada sem NVS real.

### Dependencias

Fase 2 concluida.

Fase 7 concluida.

Fase 8 concluida.

### Descricao

Definir o modulo `storage` com um boundary `StorageBackend` ou equivalente para leitura, escrita, remocao, existencia de chave e abertura de namespaces logicos. O contrato deve permitir uma implementacao real com NVS Preferences e um fake de teste em memoria.

Definir o schema persistivel da Fase 9 com dominios separados para Wi-Fi, MQTT, temperatura, ATO, timers, modo atual, calibracoes e iluminacao. O schema deve registrar uma versao local do formato persistido e deve permitir rejeitar dados com schema ausente, schema invalido ou payload corrompido sem alterar arquitetura ou specs.

Declarar os eventos locais `CONFIG_SAVED`, `CONFIG_RESTORED` e `CONFIG_RESET`, mapeados para o Event Bus local. Os eventos devem carregar dominio afetado, resultado, motivo e instante quando essas informacoes ja existirem nos contratos de core, sem publicar MQTT, sem criar Alert Manager e sem persistir historico.

Definir tipos de resultado para operacoes de storage: sucesso, chave ausente, namespace ausente, payload invalido, schema incompativel, falha de leitura, falha de escrita, sem mudanca, reset aplicado e operacao ignorada por politica de escrita.

Criar fake ou mock de backend restrito aos testes. O fake deve permitir simular armazenamento por namespace/chave, ausencia de dados, dados corrompidos, falha de leitura, falha de escrita, contagem de gravacoes, ultimo payload salvo e reset de namespace.

Esta tarefa nao deve implementar NVS Preferences real, restauracao de boot, serializacao completa de dominios, politica de coalescencia, scheduler, Wi-Fi, MQTT runtime, cloud, app ou testes fisicos.

### Arquivos afetados

- `firmware/include/storage/storage_types.h`
- `firmware/include/storage/storage_schema.h`
- `firmware/include/storage/storage_backend.h`
- `firmware/include/storage/storage_service.h`
- `firmware/include/storage/storage_events.h`
- `firmware/include/storage/storage_write_policy.h`
- `firmware/src/storage/storage_events.cpp`
- `firmware/include/core/events/event_bus.h`, somente se necessario para registrar eventos `CONFIG_*`
- `firmware/src/core/events/event_bus.cpp`, somente se necessario para registrar eventos `CONFIG_*`
- `firmware/test/fakes/fake_storage_backend.h`
- `firmware/test/unit/test_storage_contracts.cpp`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe modulo `storage` com contratos publicos da Fase 9.
- Existe boundary de backend substituivel para storage local.
- O boundary permite leitura por namespace e chave.
- O boundary permite escrita por namespace e chave.
- O boundary permite remover chave ou resetar dominio quando suportado pela tarefa.
- O boundary representa ausencia de namespace.
- O boundary representa ausencia de chave.
- O boundary representa falha de leitura.
- O boundary representa falha de escrita.
- O schema persistivel possui versao local.
- O schema separa dominios de Wi-Fi, MQTT, temperatura, ATO, timers, modo atual, calibracoes e iluminacao.
- O schema nao inclui leituras volateis de sensores.
- O schema nao inclui estados runtime de relays.
- O schema nao inclui `lighting.currentPWM` como dado persistivel.
- O schema nao inclui `ato.pumpRunning` como dado persistivel.
- O schema nao inclui status runtime de rede.
- O schema nao inclui alertas ativos nem saude runtime do sistema.
- Existem eventos locais `CONFIG_SAVED`, `CONFIG_RESTORED` e `CONFIG_RESET`.
- Eventos `CONFIG_*` sao mapeados para o Event Bus local.
- Eventos `CONFIG_*` nao publicam MQTT.
- Eventos `CONFIG_*` nao criam Alert Manager.
- Eventos `CONFIG_*` nao persistem historico.
- Existe fake ou mock de storage restrito a testes.
- O fake permite simular dados ausentes.
- O fake permite simular dados corrompidos.
- O fake permite simular falha de leitura.
- O fake permite simular falha de escrita.
- O fake permite contar gravacoes por chave ou dominio.
- O build real do firmware nao depende de fakes ou mocks.
- Nenhuma implementacao real de Preferences e criada nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario confirmando os dominios persistiveis da Fase 9.
- Executar teste unitario confirmando que dados volateis nao fazem parte do schema persistivel.
- Executar teste unitario confirmando versionamento local do schema.
- Executar teste unitario confirmando eventos `CONFIG_*` no Event Bus local.
- Executar teste unitario confirmando que eventos `CONFIG_*` nao publicam MQTT.
- Executar teste unitario confirmando leitura valida pelo fake storage.
- Executar teste unitario confirmando ausencia de chave pelo fake storage.
- Executar teste unitario confirmando payload corrompido pelo fake storage.
- Executar teste unitario confirmando falha simulada de leitura.
- Executar teste unitario confirmando falha simulada de escrita.
- Executar teste unitario confirmando contagem de gravacoes no fake.
- Confirmar que nenhum teste exige ESP32, NVS real, reboot fisico, Wi-Fi, MQTT, cloud, app ou Serial real.
- Confirmar que o build real nao inclui o fake ou mock de teste.

---

### ID

FW-P09-001

### Titulo

Implementar backend NVS Preferences e politica de escrita anti-desgaste

### Objetivo

Criar a implementacao real do backend de persistencia usando NVS Preferences e uma politica de escrita que salve apenas mudancas efetivas, evitando gravacoes excessivas.

### Dependencias

FW-P09-000.

FW-P02-000.

FW-P02-004.

### Descricao

Implementar `PreferencesStorageBackend` ou equivalente em `src/storage/`, separado do `StorageService`. O backend real deve implementar o boundary definido em `FW-P09-000`, usando NVS Preferences somente no build destinado ao firmware real. Testes unitarios devem usar fake ou adapter substituivel; a fase nao deve depender de flash fisica ou ESP32 conectado.

Definir namespaces e chaves deterministicas para os dominios persistiveis da Fase 9. O backend deve abrir e fechar namespaces de forma controlada, retornar erros funcionais quando leitura ou escrita falhar e nao mascarar dados ausentes como configuracao valida.

Implementar uma politica de escrita anti-desgaste em `storage_write_policy`. A politica deve comparar o valor novo com o ultimo valor conhecido ou salvo, marcar dominios como sujos somente quando houver mudanca efetiva, permitir flush explicito, permitir coalescencia de multiplas mudancas do mesmo dominio e impedir gravacao repetida em avaliacoes idempotentes.

A politica nao deve usar `delay`, polling fisico, tempo real obrigatorio ou loop bloqueante. Quando usar tempo, deve depender de Time Source injetavel ou Scheduler ja existente. A politica tambem deve permitir flush imediato em eventos criticos de configuracao quando explicitamente solicitado por chamadas locais, sem transformar todo update em escrita imediata obrigatoria.

Implementar operacao local de reset de configuracao persistida para defaults de firmware, restrita ao boundary local e testes. O reset deve emitir `CONFIG_RESET` quando aplicado com sucesso, mas nao deve criar comando remoto, UI, endpoint, MQTT ou acao de aplicativo.

Esta tarefa nao deve serializar todos os dominios funcionais, nao deve restaurar boot completo, nao deve inicializar Wi-Fi, MQTT, cloud, app, Alert Manager ou testes de resiliencia.

### Arquivos afetados

- `firmware/src/storage/preferences_storage_backend.h`
- `firmware/src/storage/preferences_storage_backend.cpp`
- `firmware/include/storage/storage_backend.h`
- `firmware/include/storage/storage_types.h`
- `firmware/include/storage/storage_schema.h`
- `firmware/include/storage/storage_write_policy.h`
- `firmware/src/storage/storage_write_policy.cpp`
- `firmware/include/storage/storage_events.h`
- `firmware/src/storage/storage_events.cpp`
- `firmware/include/core/platform/`, somente se necessario para Time Source injetavel existente
- `firmware/test/fakes/fake_storage_backend.h`
- `firmware/test/unit/test_storage_backend.cpp`
- `firmware/test/unit/test_storage_write_policy.cpp`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe backend real para NVS Preferences.
- O backend real implementa o boundary de storage da Fase 9.
- O backend real fica separado do `StorageService`.
- O backend real compila para o firmware destinado ao hardware.
- Testes automatizados nao dependem de NVS real.
- Namespaces da Fase 9 sao deterministicas.
- Chaves da Fase 9 sao deterministicas.
- Ausencia de namespace e retornada como ausencia de dados, nao como configuracao valida.
- Ausencia de chave e retornada como ausencia de dados, nao como configuracao valida.
- Falha de leitura e reportada ao chamador.
- Falha de escrita e reportada ao chamador.
- Payload invalido nao e aceito como configuracao restaurada.
- Existe politica de escrita anti-desgaste.
- Mudanca efetiva marca dominio como sujo.
- Valor identico nao marca dominio como sujo novamente.
- Flush salva somente dominios sujos.
- Flush de dominio limpo nao grava novamente.
- Multiplas mudancas do mesmo dominio podem ser coalescidas antes do flush.
- Escritas repetidas em ciclos idempotentes sao evitadas.
- A politica nao usa `delay`.
- A politica nao bloqueia o loop principal.
- Tempo usado pela politica e injetavel ou vem de boundary existente.
- Existe operacao local de reset de configuracao persistida.
- Reset bem sucedido emite `CONFIG_RESET`.
- Reset nao cria comando remoto, endpoint, MQTT, UI ou app.
- Nenhum Wi-Fi runtime, MQTT runtime, Alert Manager, cloud ou app e implementado.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario do backend usando fake ou adapter substituivel sem NVS real.
- Testar leitura de chave existente.
- Testar leitura de chave ausente.
- Testar namespace ausente.
- Testar falha simulada de leitura.
- Testar falha simulada de escrita.
- Testar escrita bem sucedida.
- Testar reset de dominio persistido.
- Testar emissao local de `CONFIG_RESET`.
- Testar que mudanca efetiva marca dominio como sujo.
- Testar que valor identico nao gera nova escrita.
- Testar coalescencia de multiplas mudancas antes do flush.
- Testar flush de dominio sujo gerando uma escrita esperada.
- Testar flush repetido sem mudanca nao gerando escrita adicional.
- Testar que a politica nao depende de delay real.
- Confirmar que os testes nao exigem ESP32, flash real, reboot fisico, Serial real, Wi-Fi, MQTT, cloud ou app.
- Confirmar que o build real nao inclui fakes ou mocks.

---

### ID

FW-P09-002

### Titulo

Persistir e restaurar configuracoes gerais do Config Manager

### Objetivo

Implementar salvamento e restauracao dos dominios gerais do Config Manager: Wi-Fi, MQTT, temperatura, ATO, timers e calibracoes, com validacao, defaults seguros e eventos locais.

### Dependencias

FW-P09-001.

FW-P02-003.

FW-P03-002.

FW-P04-002.

FW-P06-001.

### Descricao

Implementar no `StorageService` a serializacao, desserializacao, validacao e aplicacao dos dominios gerais persistiveis do Config Manager: Wi-Fi, MQTT, limites de temperatura, parametros do ATO, timers e calibracoes.

Wi-Fi deve persistir somente dados de configuracao, como SSID e credenciais ou estrutura equivalente ja prevista no Config Manager. A Fase 9 nao deve conectar ao access point, validar senha real, atualizar `network.wifiConnected`, iniciar reconexao ou depender de Internet.

MQTT deve persistir somente dados de configuracao, como host, porta, credenciais, identificacao do dispositivo e parametros configuraveis ja previstos no Config Manager. A Fase 9 nao deve abrir broker, publicar telemetria, receber comandos, testar QoS, iniciar heartbeat ou atualizar `network.mqttConnected`.

Temperatura deve persistir somente limites configurados, como `minTemperature` e `maxTemperature`. A restauracao deve atualizar configuracao em memoria e refletir limites no bloco `temperature` quando o contrato existente assim exigir, sem persistir `currentTemperature`, `status` ou `lastUpdate` como leitura real.

ATO deve persistir somente parametros configuraveis, como `enabled`, `minimumLevel`, `maximumLevel`, `timeoutMillis` e `cooldownMillis`. A restauracao nao deve iniciar bomba, nao deve alterar `ato.pumpRunning`, nao deve escrever reles e nao deve tratar restauracao como ciclo ATO real.

Timers devem persistir duracoes configuraveis ja existentes, especialmente `feedingDurationSeconds` usado por modos. Calibracoes devem persistir parametros configuraveis de sensores ou nivel ja previstos no Config Manager, sem executar leitura fisica ou recalibracao automatica.

Quando um dominio estiver ausente, o Config Manager deve manter defaults deterministicos. Quando um payload estiver invalido ou fora das validacoes existentes, o dominio deve ser rejeitado e o default ou configuracao atual deve ser preservado. Restauracao bem sucedida deve emitir `CONFIG_RESTORED`; salvamento bem sucedido deve emitir `CONFIG_SAVED`; falhas devem ser reportadas por resultado funcional e log local quando disponivel.

Esta tarefa nao deve implementar persistencia de iluminacao, modo atual, scheduler de flush, boot completo, Wi-Fi runtime, MQTT runtime, Alert Manager, cloud ou app.

### Arquivos afetados

- `firmware/include/storage/storage_service.h`
- `firmware/src/storage/storage_service.cpp`
- `firmware/include/storage/storage_schema.h`
- `firmware/include/storage/storage_types.h`
- `firmware/include/config/config_manager.h`
- `firmware/src/config/config_manager.cpp`
- `firmware/include/core/state/system_state.h`, somente se necessario para refletir limites restaurados
- `firmware/src/core/state/system_state.cpp`, somente se necessario para refletir limites restaurados
- `firmware/test/fakes/fake_storage_backend.h`
- `firmware/test/unit/test_storage_service.cpp`
- `firmware/test/unit/test_storage_domains.cpp`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- `StorageService` salva configuracao Wi-Fi.
- `StorageService` restaura configuracao Wi-Fi.
- Restauracao de Wi-Fi nao conecta rede.
- Restauracao de Wi-Fi nao altera `network.wifiConnected`.
- `StorageService` salva configuracao MQTT.
- `StorageService` restaura configuracao MQTT.
- Restauracao de MQTT nao conecta broker.
- Restauracao de MQTT nao publica telemetria.
- Restauracao de MQTT nao altera `network.mqttConnected`.
- `StorageService` salva limites de temperatura configurados.
- `StorageService` restaura limites de temperatura configurados.
- Restauracao de temperatura nao persiste nem inventa `currentTemperature`.
- Restauracao de temperatura nao altera leitura runtime do sensor.
- `StorageService` salva parametros configuraveis do ATO.
- `StorageService` restaura parametros configuraveis do ATO.
- Restauracao de ATO nao liga bomba.
- Restauracao de ATO nao altera `ato.pumpRunning`.
- Restauracao de ATO nao escreve reles.
- `StorageService` salva timers configuraveis.
- `StorageService` restaura timers configuraveis.
- `feedingDurationSeconds` restaurado fica disponivel ao modulo de modos via Config Manager.
- `StorageService` salva calibracoes configuraveis.
- `StorageService` restaura calibracoes configuraveis.
- Restauracao de calibracoes nao executa leitura fisica nem recalibracao automatica.
- Dominio ausente mantem default deterministico do Config Manager.
- Payload invalido e rejeitado sem corromper configuracao atual.
- Valores fora das validacoes existentes sao rejeitados.
- Restauracao bem sucedida emite `CONFIG_RESTORED` local.
- Salvamento bem sucedido emite `CONFIG_SAVED` local.
- Eventos `CONFIG_*` nao publicam MQTT, nao criam alertas e nao persistem historico.
- Nenhuma persistencia de iluminacao e implementada nesta tarefa.
- Nenhuma persistencia de modo atual e implementada nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar salvamento e restauracao de Wi-Fi com fake storage.
- Testar que restauracao de Wi-Fi nao altera status runtime de rede.
- Testar salvamento e restauracao de MQTT com fake storage.
- Testar que restauracao de MQTT nao inicia broker nem publica evento remoto.
- Testar salvamento e restauracao de limites de temperatura.
- Testar rejeicao de limites de temperatura invalidos.
- Testar que `currentTemperature` nao e persistido.
- Testar salvamento e restauracao de parametros ATO.
- Testar rejeicao de `minimumLevel >= maximumLevel`.
- Testar rejeicao de timeout ATO invalido.
- Testar que restauracao ATO nao comanda rele.
- Testar salvamento e restauracao de timers.
- Testar restauracao de `feedingDurationSeconds` usado por modos.
- Testar salvamento e restauracao de calibracoes.
- Testar dominio ausente mantendo default.
- Testar payload corrompido mantendo configuracao atual.
- Testar emissao de `CONFIG_SAVED` e `CONFIG_RESTORED` locais.
- Confirmar que os testes usam fake storage, Config Manager em memoria e estados simulados, sem ESP32, NVS real, sensores, reles, Wi-Fi, MQTT, cloud ou app.

---

### ID

FW-P09-003

### Titulo

Persistir e restaurar configuracoes, perfis e curvas da iluminacao

### Objetivo

Implementar persistencia local dos dados configuraveis da luminaria, incluindo modo de iluminacao, flags, perfil atual, horarios, limites, aclimatacao e curvas por canal, sem comandar PWM durante restauracao.

### Dependencias

FW-P09-001.

FW-P08-000.

FW-P08-003.

### Descricao

Implementar no `StorageService` a serializacao, desserializacao, validacao e aplicacao das configuracoes persistiveis de iluminacao definidas na Fase 8. O escopo inclui perfis, nomes de perfis, horario de inicio, horario de termino, flags de sunrise, sunset e aclimatacao, limite maximo global, limites por canal, duracao de aclimatacao, curvas por canal e perfil atual.

As curvas devem ser restauradas somente se respeitarem os limites contratuais da Fase 8: quantidade maxima de perfis, quantidade maxima de pontos por curva, tamanho de nome, minuto do dia dentro de `0..1439`, ordem crescente dos pontos, intensidade percentual dentro de `0..100` e duty dentro da faixa funcional declarada. Payload invalido deve ser rejeitado preservando defaults ou configuracao anterior.

Restaurar iluminacao significa atualizar configuracao em memoria e campos configuraveis do bloco `lighting` quando previsto pelos contratos existentes. A restauracao nao deve escrever PWM, nao deve ligar canais, nao deve chamar controlador LEDC, nao deve alterar `currentPWM` como se o duty fisico tivesse sido aplicado e nao deve emitir eventos `LIGHTING_STARTED` ou `LIGHTING_STOPPED` por si so.

O primeiro ciclo funcional de iluminacao apos boot continua sendo responsabilidade do `LightingService` e do scheduler da Fase 8, respeitando fade suave e estado seguro de duty zero. A Fase 9 deve apenas disponibilizar a configuracao restaurada para esse ciclo.

Esta tarefa nao deve implementar comandos remotos de iluminacao, MQTT, app, cloud, UI, historico, Alert Manager, alteracoes de GPIO, novo canal PWM, uso funcional do GPIO13 ou testes fisicos com luminaria.

### Arquivos afetados

- `firmware/include/storage/storage_service.h`
- `firmware/src/storage/storage_service.cpp`
- `firmware/include/storage/storage_schema.h`
- `firmware/include/modules/lighting/lighting_profile.h`
- `firmware/include/modules/lighting/lighting_config.h`
- `firmware/include/modules/lighting/lighting_service.h`, somente se necessario para aplicar configuracao restaurada sem PWM imediato
- `firmware/src/modules/lighting/lighting_service.cpp`, somente se necessario para aplicar configuracao restaurada sem PWM imediato
- `firmware/include/config/config_manager.h`
- `firmware/src/config/config_manager.cpp`
- `firmware/include/core/state/system_state.h`, somente se necessario para campos configuraveis de `lighting`
- `firmware/src/core/state/system_state.cpp`, somente se necessario para campos configuraveis de `lighting`
- `firmware/test/fakes/fake_storage_backend.h`
- `firmware/test/fakes/fake_lighting_pwm_controller.h`
- `firmware/test/unit/test_storage_domains.cpp`
- `firmware/test/unit/test_storage_service.cpp`
- `firmware/test/integration/test_storage_boot_restore_integration.cpp`

### Criterios de aceitacao

- `StorageService` salva configuracao de iluminacao.
- `StorageService` restaura configuracao de iluminacao.
- Perfis de iluminacao sao persistidos.
- Nome de perfil e persistido respeitando tamanho maximo.
- Horario de inicio e persistido.
- Horario de termino e persistido.
- Flags de sunrise, sunset e aclimatacao sao persistidas.
- Limite maximo global e persistido.
- Limites por canal sao persistidos.
- Duracao de aclimatacao e persistida.
- Curvas por canal sao persistidas.
- Perfil atual e persistido.
- Quantidade maxima de perfis e respeitada na restauracao.
- Quantidade maxima de pontos por curva e respeitada na restauracao.
- Minuto do dia fora de `0..1439` e rejeitado.
- Pontos fora de ordem sao rejeitados.
- Intensidade fora de `0..100` e rejeitada.
- Duty fora da faixa funcional e rejeitado ou normalizado conforme contrato da Fase 8.
- Payload de iluminacao invalido nao corrompe configuracao atual.
- Dominio de iluminacao ausente mantem defaults deterministicos.
- Restauracao de iluminacao nao escreve PWM.
- Restauracao de iluminacao nao chama controlador LEDC.
- Restauracao de iluminacao nao liga canais.
- Restauracao de iluminacao nao altera `currentPWM` como duty aplicado.
- Restauracao de iluminacao nao emite `LIGHTING_STARTED`.
- Restauracao de iluminacao nao emite `LIGHTING_STOPPED`.
- O primeiro ciclo funcional apos boot permanece sob responsabilidade do `LightingService`.
- GPIO13 nao e usado como canal funcional.
- Nenhum MQTT, app, cloud, Alert Manager, historico ou comando remoto e implementado.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar salvamento e restauracao de perfil de iluminacao completo.
- Testar salvamento e restauracao de multiplos perfis dentro do limite.
- Testar rejeicao de quantidade de perfis acima do limite.
- Testar rejeicao de nome de perfil acima do limite.
- Testar rejeicao de horario invalido.
- Testar salvamento e restauracao de flags de sunrise, sunset e aclimatacao.
- Testar salvamento e restauracao de limite maximo global.
- Testar salvamento e restauracao de limites por canal.
- Testar salvamento e restauracao de curvas dos canais Branco, Azul, Royal Blue e UV.
- Testar rejeicao de minuto do dia fora de `0..1439`.
- Testar rejeicao de pontos fora de ordem.
- Testar rejeicao de intensidade fora de `0..100`.
- Testar dominio de iluminacao ausente mantendo defaults.
- Testar payload corrompido mantendo configuracao anterior.
- Testar que restauracao nao chama fake PWM controller.
- Testar que restauracao nao altera `currentPWM` dos canais.
- Testar que restauracao nao emite `LIGHTING_STARTED` ou `LIGHTING_STOPPED`.
- Confirmar que os testes usam fake storage, fake PWM quando necessario e estados simulados, sem luminaria real, ESP32, NVS real, Wi-Fi, MQTT, cloud ou app.

---

### ID

FW-P09-004

### Titulo

Persistir e restaurar modo atual com aplicacao segura no boot

### Objetivo

Implementar o backend definitivo de persistencia do modo atual usando o storage da Fase 9, integrando o boundary de modos existente sem criar novos comportamentos operacionais.

### Dependencias

FW-P09-001.

FW-P07-003.

FW-P07-004.

### Descricao

Implementar a persistencia definitiva do modo atual usando o backend de storage da Fase 9 e o boundary de modo existente da Fase 7, como `ModeStore` ou equivalente. A implementacao deve salvar e carregar somente `NORMAL`, `FEEDING`, `TPA` e `MAINTENANCE`, sem criar modos novos e sem depender de NVS real nos testes.

Quando houver mudanca efetiva de modo, o servico de modos deve solicitar salvamento pelo boundary definitivo. Comando idempotente para o modo ja ativo nao deve gerar nova escrita. Falha simulada de salvamento deve ser reportada sem corromper o modo operacional ja aplicado no System State.

Na restauracao de boot, ausencia de modo salvo deve manter `NORMAL`. Modo invalido, payload corrompido ou schema incompativel deve ser rejeitado e manter `NORMAL` seguro. `NORMAL` salvo deve restaurar `system.modes.currentMode = NORMAL` sem ligar equipamentos automaticamente.

Para `FEEDING`, a restauracao deve seguir a decisao segura ja definida na Fase 7: se nao houver tempo restante confiavel persistido e validado, o runtime deve cair para `NORMAL` ou finalizar `FEEDING` imediatamente, sem religar Recalque automaticamente e sem bloquear o boot. A Fase 9 nao deve inventar nova semantica de retorno automatico alem da spec de modos.

Para `TPA` e `MAINTENANCE`, a restauracao deve reaplicar os bloqueios e efeitos seguros ja definidos pelo modulo de modos, usando os boundaries de efeitos e gate existentes. Falha simulada em efeito obrigatorio deve ser reportada e manter ou retornar para estado seguro conforme o servico de modos, sem ligar reles indevidamente.

Esta tarefa nao deve implementar novos efeitos de modo, alterar regras de ATO, alterar regras de reles, comandar iluminacao, implementar Wi-Fi, MQTT, Alert Manager, cloud, app ou testes de resiliencia.

### Arquivos afetados

- `firmware/include/modules/modes/mode_store.h`
- `firmware/src/modules/modes/mode_service.cpp`
- `firmware/include/storage/storage_service.h`
- `firmware/src/storage/storage_service.cpp`
- `firmware/include/storage/storage_schema.h`
- `firmware/include/storage/storage_types.h`
- `firmware/src/app/core_app.h`
- `firmware/src/app/core_app.cpp`
- `firmware/src/app/core_app_arduino.cpp`
- `firmware/test/fakes/fake_storage_backend.h`
- `firmware/test/fakes/fake_mode_effects.h`
- `firmware/test/fakes/fake_mode_automation_gate.h`
- `firmware/test/unit/test_storage_domains.cpp`
- `firmware/test/unit/test_mode_service.cpp`
- `firmware/test/integration/test_storage_boot_restore_integration.cpp`

### Criterios de aceitacao

- Modo atual e salvo pelo backend de storage da Fase 9.
- Modo atual e restaurado pelo backend de storage da Fase 9.
- Somente `NORMAL`, `FEEDING`, `TPA` e `MAINTENANCE` sao aceitos.
- Modo invalido e rejeitado.
- Payload corrompido e rejeitado.
- Schema incompativel e rejeitado.
- Ausencia de modo salvo mantem `NORMAL`.
- `NORMAL` salvo restaura `system.modes.currentMode = NORMAL`.
- Restauracao de `NORMAL` nao liga equipamentos automaticamente.
- `FEEDING` salvo sem tempo restante confiavel nao prende o sistema em modo temporario.
- Restauracao de `FEEDING` cai para `NORMAL` ou finaliza de forma segura conforme contrato da Fase 7.
- Restauracao de `FEEDING` nao religa Recalque automaticamente.
- `TPA` salvo solicita reaplicacao de bloqueios e efeitos seguros existentes.
- `MAINTENANCE` salvo solicita reaplicacao de suspensao de regras nao criticas existente.
- Falha simulada em efeito obrigatorio durante restauracao e reportada.
- Falha simulada em efeito obrigatorio nao liga reles indevidamente.
- Mudanca efetiva de modo solicita salvamento.
- Comando idempotente de modo nao solicita nova escrita.
- Falha simulada de salvamento nao corrompe `system.modes`.
- Eventos `CONFIG_SAVED` e `CONFIG_RESTORED` sao emitidos para o dominio de modo quando aplicavel.
- Eventos de modo continuam locais e nao publicam MQTT.
- Nenhum novo comportamento operacional de modo e criado.
- Nenhuma regra de ATO, rele ou iluminacao e alterada fora do necessario para restauracao segura existente.
- Nenhuma funcionalidade de Wi-Fi, MQTT, Alert Manager, cloud, app ou resiliencia futura e implementada.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar salvamento de `NORMAL`.
- Testar salvamento de `FEEDING`.
- Testar salvamento de `TPA`.
- Testar salvamento de `MAINTENANCE`.
- Testar que comando idempotente nao grava novamente.
- Testar falha simulada de salvamento.
- Testar boot sem modo salvo.
- Testar boot com modo invalido.
- Testar boot com payload corrompido.
- Testar boot com `NORMAL` salvo.
- Testar boot com `FEEDING` salvo sem tempo restante confiavel.
- Testar que `FEEDING` restaurado retorna ou finaliza de forma segura.
- Testar boot com `TPA` salvo reaplicando bloqueios por fake effects e fake gate.
- Testar boot com `MAINTENANCE` salvo reaplicando suspensao por fake effects e fake gate.
- Testar falha simulada de efeito obrigatorio durante restauracao.
- Testar emissao local de `CONFIG_SAVED` e `CONFIG_RESTORED` para modo.
- Confirmar que os testes usam fake storage, fake effects, fake automation gate e fake tempo, sem ESP32, NVS real, reles reais, sensores, Wi-Fi, MQTT, cloud ou app.

---

### ID

FW-P09-005

### Titulo

Integrar restauracao no boot e flush de persistencia ao runtime

### Objetivo

Conectar o Storage Service ao runtime do firmware para restaurar configuracoes em ordem segura durante boot e salvar mudancas persistiveis por flush controlado sem bloquear o loop principal.

### Dependencias

FW-P09-002.

FW-P09-003.

FW-P09-004.

FW-P02-004.

FW-P02-007.

FW-P08-004.

### Descricao

Integrar o `StorageService` ao `CoreApp` depois da inicializacao de System State, Event Bus, Config Manager, Logger, Scheduler e Watchdog, e antes de modulos que precisem consumir configuracoes restauradas em sua primeira avaliacao funcional.

Durante `setup`, o runtime deve restaurar configuracoes persistiveis para o Config Manager e, quando previsto pelos contratos existentes, refletir campos configuraveis no System State. A restauracao deve ocorrer antes de ciclos funcionais de ATO, modos e iluminacao usarem configuracoes finais. Falhas de restauracao devem ser reportadas e defaults em memoria devem permanecer validos.

Registrar uma tarefa periodica ou mecanismo equivalente de flush de storage no scheduler. O flush deve processar apenas dominios sujos, respeitar a politica anti-desgaste da `FW-P09-001`, executar uma iteracao curta e retornar sucesso ou falha de forma compativel com o scheduler.

Integrar o Config Manager aos eventos ou callbacks de mudanca para marcar dominios persistiveis como sujos quando configuracoes aceitas mudarem. Mudancas rejeitadas pelo Config Manager nao devem gerar escrita. Mudancas idempotentes nao devem gerar escrita adicional.

Garantir que a restauracao de boot nao comande GPIO, nao ligue reles, nao escreva PWM, nao inicie bomba ATO, nao conecte Wi-Fi, nao conecte MQTT e nao bloqueie o loop principal. O Watchdog deve continuar sendo alimentado conforme a Fase 2.

Esta tarefa nao deve implementar Wi-Fi runtime, MQTT runtime, Alert Manager, testes de resiliencia, cloud, aplicativo, historico, notificacoes ou comandos remotos.

### Arquivos afetados

- `firmware/src/app/core_app.h`
- `firmware/src/app/core_app.cpp`
- `firmware/src/app/core_app_arduino.cpp`
- `firmware/include/storage/storage_service.h`
- `firmware/src/storage/storage_service.cpp`
- `firmware/include/storage/storage_write_policy.h`
- `firmware/src/storage/storage_write_policy.cpp`
- `firmware/include/config/config_manager.h`
- `firmware/src/config/config_manager.cpp`
- `firmware/include/core/scheduler/task_scheduler.h`, somente se necessario para registrar flush
- `firmware/src/core/scheduler/task_scheduler.cpp`, somente se necessario para registrar flush
- `firmware/test/fakes/fake_storage_backend.h`
- `firmware/test/unit/test_storage_service.cpp`
- `firmware/test/integration/test_storage_boot_restore_integration.cpp`
- `firmware/test/integration/test_storage_scheduler_integration.cpp`

### Criterios de aceitacao

- Runtime cria ou recebe `StorageService`.
- Storage e inicializado apos System State.
- Storage e inicializado apos Event Bus.
- Storage e inicializado apos Config Manager.
- Storage e inicializado apos Logger.
- Storage e inicializado apos Scheduler.
- Storage e inicializado apos Watchdog.
- Restauracao ocorre antes da primeira avaliacao funcional de ATO quando parametros ATO restaurados existirem.
- Restauracao ocorre antes da primeira avaliacao funcional de modos quando modo atual restaurado existir.
- Restauracao ocorre antes da primeira avaliacao funcional de iluminacao quando perfis restaurados existirem.
- Restauracao bem sucedida atualiza Config Manager.
- Restauracao bem sucedida reflete campos configuraveis no System State quando previsto pelos contratos existentes.
- Falha de restauracao preserva defaults validos em memoria.
- Payload invalido nao bloqueia `setup`.
- Runtime registra tarefa ou mecanismo de flush de storage.
- Flush executa apenas dominios sujos.
- Flush de dominio limpo nao grava.
- Flush retorna sucesso ou falha ao scheduler.
- Flush nao bloqueia o loop principal.
- Mudanca aceita no Config Manager marca dominio persistivel como sujo.
- Mudanca rejeitada no Config Manager nao marca dominio como sujo.
- Mudanca idempotente nao gera escrita adicional.
- Restauracao de boot nao comanda GPIO.
- Restauracao de boot nao liga reles.
- Restauracao de boot nao escreve PWM.
- Restauracao de boot nao inicia bomba ATO.
- Restauracao de boot nao conecta Wi-Fi.
- Restauracao de boot nao conecta MQTT.
- Watchdog continua sendo alimentado conforme a Fase 2.
- O build real nao depende de fake storage, fake tempo ou mocks.
- Nenhuma funcionalidade de Wi-Fi runtime, MQTT runtime, Alert Manager, cloud, app ou resiliencia futura e inicializada.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste de integracao de boot com fake storage preenchido.
- Testar que `setup` restaura Config Manager antes dos modulos funcionais consumirem configuracao.
- Testar boot com storage vazio mantendo defaults.
- Testar boot com payload invalido mantendo defaults e sem travar.
- Testar que restauracao de ATO nao comanda fake relay.
- Testar que restauracao de iluminacao nao comanda fake PWM.
- Testar que restauracao de Wi-Fi nao altera status runtime de rede.
- Testar que restauracao de MQTT nao publica ou conecta.
- Testar que tarefa de flush e registrada.
- Testar que tarefa de flush nao executa antes do intervalo configurado quando houver intervalo.
- Testar que flush executa quando ha dominio sujo.
- Testar que flush nao escreve dominio limpo.
- Testar que multiplas mudancas aceitas sao coalescidas.
- Testar que mudanca rejeitada nao gera escrita.
- Testar que `loopOnce` permanece nao bloqueante.
- Testar que falha simulada de escrita e reportada sem corromper configuracao em memoria.
- Confirmar que nenhum teste exige ESP32, NVS real, reboot fisico, reles, sensores, luminaria, Wi-Fi, MQTT, cloud, app ou Serial real.

---

### ID

FW-P09-006

### Titulo

Consolidar validacao automatizada da Fase 9

### Objetivo

Criar o gate final da Fase 9, garantindo que a persistencia local esteja integrada, testada por mocks e fakes, limitada ao escopo definido e preparada para validacao fisica futura sem depender dela.

### Dependencias

FW-P09-005.

### Descricao

Consolidar testes unitarios e de integracao da Fase 9 em comandos ou scripts de validacao. O gate deve provar que contratos de storage operam sem hardware, que o backend NVS Preferences compila para o firmware real sem depender de fakes, que a politica anti-desgaste evita gravacoes excessivas, que os dominios persistiveis oficiais sao salvos e restaurados, que dados volateis nao sao persistidos, que eventos `CONFIG_*` sao emitidos localmente e que o runtime restaura configuracoes e executa flush sem bloquear o loop principal.

Tambem deve haver uma revisao de escopo confirmando que a Fase 9 nao implementou Wi-Fi runtime, MQTT runtime, Alert Manager, testes de resiliencia, cloud, aplicativo, historico, notificacoes, comandos remotos, backup remoto, provisionamento, comandos por Serial real, alteracoes de specs, alteracoes de arquitetura ou testes fisicos obrigatorios.

Qualquer validacao real com ESP32, NVS fisica, reboot fisico, ciclo de energia, flash real, Wi-Fi real, broker MQTT real, sensores, reles, luminaria, Serial real, aquario ou seguranca de bancada deve ficar marcada como `Pendente para Hardware Validation` e nao pode bloquear a conclusao de desenvolvimento da Fase 9.

### Arquivos afetados

- `firmware/test/unit/`
- `firmware/test/integration/`
- `firmware/test/fakes/`
- `firmware/test/run_phase9_validation.sh`
- `firmware/test/integration/run_phase9_integration_tests.sh`
- `firmware/test/integration/run_phase9_scope_review.sh`
- `firmware/include/storage/`
- `firmware/src/storage/`
- `firmware/include/config/`
- `firmware/src/config/`
- `firmware/include/modules/modes/`
- `firmware/src/modules/modes/`
- `firmware/include/modules/lighting/`
- `firmware/src/modules/lighting/`
- `firmware/src/app/`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste
- `tasks/firmware-phase-09-local-persistence.md`

### Criterios de aceitacao

- Existe comando ou script para executar os testes unitarios da Fase 9.
- Existe comando ou script para executar os testes de integracao da Fase 9.
- Existe revisao automatizada ou objetiva de escopo da Fase 9.
- Todos os testes da Fase 9 usam mocks, fakes ou valores simulados quando exercitam storage.
- O build real do firmware compila sem depender de mocks ou fakes.
- Contratos de storage sao validados sem hardware.
- Backend NVS Preferences compila para build real.
- Backend NVS Preferences nao e requisito para testes em hardware fisico nesta fase.
- Politica anti-desgaste e validada por teste automatizado.
- Escritas idempotentes nao geram gravacoes repetidas.
- Flush salva somente dominios sujos.
- Wi-Fi configuravel e salvo e restaurado por teste automatizado.
- MQTT configuravel e salvo e restaurado por teste automatizado.
- Limites de temperatura sao salvos e restaurados por teste automatizado.
- Parametros ATO sao salvos e restaurados por teste automatizado.
- Timers sao salvos e restaurados por teste automatizado.
- Calibracoes sao salvas e restauradas por teste automatizado.
- Modo atual e salvo e restaurado por teste automatizado.
- Perfis e curvas de iluminacao sao salvos e restaurados por teste automatizado.
- Dominio ausente mantem defaults deterministicos.
- Payload corrompido e rejeitado sem corromper configuracao em memoria.
- Schema invalido e rejeitado sem corromper configuracao em memoria.
- Dados volateis de sensores nao sao persistidos.
- Estados runtime de relays nao sao persistidos.
- `lighting.currentPWM` nao e persistido como duty aplicado.
- `ato.pumpRunning` nao e persistido.
- Status runtime de rede nao e persistido.
- Alertas ativos nao sao persistidos nesta fase.
- Saude runtime do sistema nao e persistida como configuracao.
- Eventos locais `CONFIG_SAVED`, `CONFIG_RESTORED` e `CONFIG_RESET` sao validados por teste automatizado.
- Eventos `CONFIG_*` nao publicam MQTT, nao criam Alert Manager e nao persistem historico.
- Restauracao de boot e validada sem reboot fisico real.
- Restauracao de boot nao liga reles.
- Restauracao de boot nao escreve PWM.
- Restauracao de boot nao inicia bomba ATO.
- Restauracao de boot nao conecta Wi-Fi.
- Restauracao de boot nao conecta MQTT.
- Integracao do scheduler de flush e validada sem hardware.
- Nenhum teste fisico e criterio de conclusao da Fase 9.
- Qualquer validacao real da persistencia fica marcada como `Pendente para Hardware Validation`.
- Revisao de escopo confirma que Wi-Fi runtime da Fase 10 nao foi implementado.
- Revisao de escopo confirma que MQTT runtime da Fase 11 nao foi implementado.
- Revisao de escopo confirma que Alert Manager da Fase 12 nao foi implementado.
- Revisao de escopo confirma que testes de resiliencia da Fase 13 nao foram implementados.
- Revisao de escopo confirma que cloud e app nao foram implementados.
- Revisao de escopo confirma que comandos remotos nao foram implementados.
- Revisao de escopo confirma que `architecture/`, `specs/` e `tasks/firmware-master-plan.md` nao foram alterados.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar todos os testes unitarios da Fase 9 com sucesso.
- Executar todos os testes de integracao da Fase 9 com sucesso.
- Executar script de validacao final da Fase 9.
- Executar testes existentes da Fase 2 impactados por Config Manager, Event Bus, Scheduler, Logger e Watchdog.
- Executar testes existentes da Fase 6 impactados por parametros ATO, quando aplicavel.
- Executar testes existentes da Fase 7 impactados por `ModeStore` e boot restore, quando aplicavel.
- Executar testes existentes da Fase 8 impactados por perfis e configuracoes de iluminacao, quando aplicavel.
- Confirmar que o build real nao inclui fakes ou mocks.
- Confirmar que os testes nao exigem ESP32, NVS real, reboot fisico, ciclo de energia, sensores, reles, luminaria, Wi-Fi real, broker MQTT real, cloud, app, Serial real ou qualquer validacao fisica.
- Confirmar que nenhum teste ou implementacao inicializa Wi-Fi runtime, MQTT runtime, Alert Manager, cloud ou app.
- Confirmar que nenhum teste ou implementacao publica MQTT, recebe comandos remotos ou persiste historico.
- Confirmar que nenhum teste ou implementacao transforma restauracao em acionamento fisico de rele, bomba ATO ou PWM.
- Confirmar que nenhum teste ou implementacao altera `architecture/`, `specs/` ou `tasks/firmware-master-plan.md`.
- Confirmar por revisao que funcionalidades de fases posteriores nao foram iniciadas.
