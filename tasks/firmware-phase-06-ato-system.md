# Fase 6 - Sistema ATO

## Escopo

Implementar o modulo funcional de ATO do REEFLOW para automatizar a reposicao de agua doce usando o nivel de agua ja exposto por `waterLevel` e a bomba ATO controlada pelo Rele 3. A fase deve aplicar acionamento automatico, parada por nivel maximo, timeout, cooldown, protecoes contra rearmes indevidos, protecao contra sensor invalido, fail-safe de bomba, atualizacao do bloco `ato` do System State e eventos locais `ATO_*`.

Esta fase depende da Fase 4 e da Fase 5. O ATO deve consumir `waterLevel.currentLevel`, `waterLevel.minimumLevel`, `waterLevel.maximumLevel` e `waterLevel.status` como fonte de verdade do sensor de nivel, e deve acionar a bomba ATO exclusivamente pelo modulo funcional de reles, sem escrever GPIO diretamente.

Esta fase e puramente de desenvolvimento e nao exige ESP32 conectado, VL6180X conectado, modulo de rele conectado, bomba ATO real, reservatorio de agua doce, sump real, vazao real, sensor em bancada, monitor Serial real, Wi-Fi, MQTT, cloud, aplicativo, multimetro, osciloscopio ou medicoes reais.

Mocks e fakes devem ser usados para validacoes automatizadas das implementacoes, seguindo o padrao das fases anteriores. Mocks e fakes devem ficar restritos a `firmware/test/` ou a configuracoes de teste, sem se tornarem dependencia do build real do firmware destinado ao hardware.

O firmware real deve ficar preparado para usar o sensor de nivel fisico e a bomba ATO fisica futuramente, mas a conclusao desta fase nao pode depender de teste fisico. Qualquer verificacao com agua real, evaporacao real, vazao da bomba, altura real do sump, resposta fisica do sensor, comportamento eletrico do rele, corrente da bomba, ruido eletrico, Serial real ou seguranca de bancada deve ficar registrada como `Pendente para Hardware Validation` e fora dos criterios de conclusao desta fase.

Nao implementar modos operacionais, persistencia NVS definitiva, Wi-Fi, MQTT, Alert Manager completo, cloud, aplicativo, historico, notificacoes, comandos remotos, resiliencia integrada ou funcionalidades de fases posteriores.

Os parametros de ATO devem vir da configuracao em memoria existente da Fase 2: `enabled`, `minimumLevel`, `maximumLevel`, `timeoutMillis` e `cooldownMillis`. A persistencia desses parametros fica reservada para a Fase 9. Publicacao MQTT fica reservada para a Fase 11. Centralizacao de alertas com cooldown fica reservada para a Fase 12. Coordenacao com modos NORMAL, FEEDING, TPA e MAINTENANCE fica reservada para a Fase 7.

O nome canonico dos estados no firmware deve seguir `specs/system-state-spec.md`: `NORMAL`, `REFILLING`, `TIMEOUT`, `SENSOR_OFFLINE` e `DISABLED`. O ATO nao deve criar estados novos no System State.

Os eventos `ATO_START`, `ATO_STOP`, `ATO_TIMEOUT`, `ATO_SENSOR_OFFLINE` e `ATO_RECOVERED` devem ser eventos locais do firmware nesta fase. Eles podem servir como materia-prima futura para MQTT, historico, Alert Manager, cloud ou aplicativo, mas nao devem publicar MQTT, persistir historico, criar alertas centralizados ou enviar notificacoes nesta fase.

O Rele 3 deve ser comandado pela automacao ATO com origem `AUTOMATION` quando a bomba for ligada ou parada por fluxo normal, e com origem `FAILSAFE` quando a bomba for desligada por timeout, sensor invalido, configuracao invalida, divergencia entre estado ATO e estado do rele, ou desabilitacao segura. A extensao do modulo de reles para aceitar `AUTOMATION` como origem local da Fase 6 esta em escopo. Entradas `MQTT` e `APP` continuam fora do escopo desta fase.

O modulo ATO nao possui driver proprio de bomba. A bomba ATO e representada somente pelo Rele 3 via modulo funcional de reles. Fakes de teste devem simular o `RelayService` ou o boundary funcional de reles, nunca criar um driver de bomba ATO separado.

Durante controle automatico bem sucedido, o invariant esperado e `ato.pumpRunning == relays.atoPump.enabled`. Qualquer divergencia detectada entre o bloco `ato` e o estado do Rele 3 deve ser tratada como condicao de reconciliacao segura, priorizando bomba desligada por fail-safe quando houver duvida.

Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` deve ser alterado.

## Estrutura Recomendada

```text
firmware/
|-- include/
|   |-- modules/
|   |   |-- ato/
|   |   |   |-- ato_config.h
|   |   |   |-- ato_events.h
|   |   |   |-- ato_types.h
|   |   |   |-- ato_policy.h
|   |   |   `-- ato_service.h
|   |   |-- relays/
|   |   `-- water_level/
|   |-- core/
|   |-- config/
|   `-- contracts/
|-- src/
|   |-- modules/
|   |   |-- ato/
|   |   |   |-- ato_events.cpp
|   |   |   |-- ato_policy.cpp
|   |   |   `-- ato_service.cpp
|   |   |-- relays/
|   |   `-- water_level/
|   |-- app/
|   `-- core/
`-- test/
    |-- fakes/
    |-- unit/
    |   |-- test_ato_contracts.cpp
    |   |-- test_ato_policy.cpp
    |   `-- test_ato_service.cpp
    |-- integration/
    |   |-- test_ato_scheduler_integration.cpp
    |   |-- run_phase6_integration_tests.sh
    |   `-- run_phase6_scope_review.sh
    `-- hardware/
```

`firmware/test/hardware/` fica reservado para validacoes futuras e nao e criterio de conclusao da Fase 6.

O modulo funcional em `modules/ato` nao deve depender de tipos, resultados ou rotinas de diagnostico da Fase 1. Ele deve consumir somente os contratos funcionais ja existentes de System State, Config Manager, Event Bus, Time Source, water level e relays.

Nao criar `firmware/src/drivers/ato/`, `firmware/include/drivers/ato/` ou driver especifico de bomba ATO nesta fase. A separacao correta e: sensor de nivel em `modules/water_level`, atuador em `modules/relays` e regra de reposicao em `modules/ato`.

## Tarefas

### ID

FW-P06-000

### Titulo

Definir contratos funcionais, eventos e origem AUTOMATION do ATO

### Objetivo

Criar os contratos minimos do modulo ATO, incluindo tipos, decisoes, eventos locais, resultado de avaliacao e permissao para comando local de automacao pelo modulo de reles.

### Dependencias

Fase 2 concluida.

Fase 4 concluida.

Fase 5 concluida.

### Descricao

Definir o boundary funcional do ATO em `modules/ato`. O contrato deve representar as decisoes possiveis da automacao: nao fazer nada, iniciar reposicao, parar por nivel maximo, parar por timeout, bloquear por cooldown, bloquear por sensor offline, bloquear por configuracao invalida, reconciliar divergencia de bomba, desligar por configuracao desabilitada e recuperar estado apos falha.

Declarar os eventos locais `ATO_START`, `ATO_STOP`, `ATO_TIMEOUT`, `ATO_SENSOR_OFFLINE` e `ATO_RECOVERED`, mapeados para o Event Bus sem publicar MQTT, sem criar Alert Manager e sem persistir historico.

Definir tipos locais em `ato_types.h` para decisao, motivo de decisao, resultado de avaliacao e metadados minimos da politica. Esses tipos devem ser independentes de Arduino, GPIO, relays concretos, diagnosticos da Fase 1, MQTT, NVS, cloud ou app.

Ajustar os contratos do modulo de reles, quando necessario, para permitir que a automacao local do ATO use origem `AUTOMATION`. Essa abertura deve ficar restrita a automacoes locais do firmware. Entradas `MQTT` e `APP` podem continuar existindo como valores canonicos do System State, mas nao devem ser aceitas como entrada implementada pelo runtime desta fase.

Preparar fakes ou mocks restritos aos testes para validar comando da bomba ATO via boundary funcional de reles sem modulo de rele real. O fake deve simular o `RelayService`, `RelayController` ou interface funcional equivalente, nao uma bomba ATO independente.

Esta tarefa nao deve implementar a maquina de estado completa, nao deve validar configuracao, nao deve atualizar System State, nao deve acionar bomba, nao deve registrar tarefa periodica e nao deve modificar rotinas de diagnostico da Fase 1.

### Arquivos afetados

- `firmware/include/modules/ato/ato_types.h`
- `firmware/include/modules/ato/ato_events.h`
- `firmware/include/modules/ato/ato_policy.h`
- `firmware/include/modules/ato/ato_service.h`
- `firmware/src/modules/ato/ato_events.cpp`
- `firmware/include/modules/relays/relay_types.h`, somente para aceitar origem `AUTOMATION` no escopo local da Fase 6
- `firmware/include/modules/relays/relay_service.h`, somente se necessario para comando de automacao local
- `firmware/test/fakes/`
- `firmware/test/unit/test_ato_contracts.cpp`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe modulo `modules/ato` com contratos publicos da Fase 6.
- Existe `ato_types.h` para tipos funcionais do ATO.
- Existe contrato local de eventos `ATO_START`, `ATO_STOP`, `ATO_TIMEOUT`, `ATO_SENSOR_OFFLINE` e `ATO_RECOVERED`.
- Os eventos ATO sao mapeados para o Event Bus local.
- Eventos ATO nao publicam MQTT.
- Eventos ATO nao criam alertas centralizados.
- Eventos ATO nao persistem historico.
- Existe contrato de decisao para iniciar reposicao.
- Existe contrato de decisao para parar por nivel maximo.
- Existe contrato de decisao para parar por timeout.
- Existe contrato de decisao para bloquear por cooldown.
- Existe contrato de decisao para bloquear por sensor offline.
- Existe contrato de decisao para bloquear por configuracao invalida.
- Existe contrato de decisao para reconciliar divergencia de bomba.
- Existe contrato de decisao para recuperar estado apos falha.
- Existe fake ou mock para simular comandos ao modulo de reles sem hardware.
- O fake permite simular sucesso ao ligar o Rele 3 ATO.
- O fake permite simular sucesso ao desligar o Rele 3 ATO.
- O fake permite simular falha ao ligar o Rele 3 ATO.
- O fake permite simular falha ao desligar o Rele 3 ATO.
- O build real do firmware nao depende de fakes ou mocks.
- O modulo de reles aceita origem `AUTOMATION` apenas como comando local de automacao do firmware.
- `MQTT` e `APP` nao sao implementados como entrada de comando nesta fase.
- Nenhum driver de bomba ATO e criado.
- Nenhum comportamento funcional de reposicao e iniciado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario confirmando o mapeamento dos eventos `ATO_*` para o Event Bus.
- Executar teste unitario confirmando os tipos de decisao do ATO.
- Executar teste unitario confirmando que o fake registra comando para ligar o Rele 3 ATO.
- Executar teste unitario confirmando que o fake registra comando para desligar o Rele 3 ATO.
- Executar teste unitario confirmando falha simulada ao ligar o Rele 3 ATO.
- Executar teste unitario confirmando falha simulada ao desligar o Rele 3 ATO.
- Executar teste unitario confirmando que origem `AUTOMATION` e aceita para comando local da Fase 6.
- Executar teste unitario confirmando que entradas `MQTT` e `APP` nao sao usadas pelo runtime desta fase.
- Confirmar que nenhum teste exige ESP32 conectado, VL6180X real, rele real, bomba real, agua real ou Serial real.
- Confirmar que o build real nao inclui o fake ou mock de teste.

---

### ID

FW-P06-001

### Titulo

Validar configuracao ATO em memoria e defaults locais

### Objetivo

Criar o contrato de configuracao local da Fase 6 e validar os parametros ATO em memoria antes que a politica de reposicao os utilize.

### Dependencias

FW-P06-000.

FW-P02-003.

### Descricao

Definir um contrato local de configuracao do modulo ATO em `ato_config.h`, sem persistencia NVS, contendo no minimo intervalo padrao de avaliacao de `1000 ms`. O timeout e o cooldown devem vir de `ConfigManager::ato()`.

Validar a configuracao ATO em memoria antes da politica funcional. A validacao deve aceitar somente `minimumLevel` e `maximumLevel` dentro da faixa canonica `0..100`, `minimumLevel < maximumLevel`, `timeoutMillis > 0` e `cooldownMillis > 0`.

Quando a configuracao for invalida, a Fase 6 deve tratar o ATO como bloqueado de forma segura, sem inventar novo status no System State. O status publico deve permanecer em um dos estados canonicos ja existentes, e a politica ou servico deve garantir bomba desligada por fail-safe quando necessario.

Essa tarefa nao deve implementar NVS Preferences, restauracao pos-reboot, MQTT, Alert Manager, comandos remotos, modos operacionais ou persistencia real de parametros. A persistencia das configuracoes ATO fica reservada para a Fase 9.

### Arquivos afetados

- `firmware/include/modules/ato/ato_config.h`
- `firmware/src/modules/ato/ato_policy.cpp`, somente se helper de validacao ficar junto da politica
- `firmware/include/config/config_manager.h`, somente se ajuste de validacao de ATO ja previsto for necessario
- `firmware/src/config/config_manager.cpp`, somente se ajuste de validacao de ATO ja previsto for necessario
- `firmware/test/unit/test_ato_contracts.cpp`
- `firmware/test/unit/test_ato_policy.cpp`

### Criterios de aceitacao

- Existe contrato local de configuracao do modulo ATO.
- O intervalo padrao de avaliacao do ATO e `1000 ms`.
- `timeoutMillis` vem de `ConfigManager::ato()`.
- `cooldownMillis` vem de `ConfigManager::ato()`.
- `enabled` vem de `ConfigManager::ato()`.
- `minimumLevel` vem de `ConfigManager::ato()`.
- `maximumLevel` vem de `ConfigManager::ato()`.
- `minimumLevel` deve estar dentro de `0..100`.
- `maximumLevel` deve estar dentro de `0..100`.
- `minimumLevel` deve ser menor que `maximumLevel`.
- `timeoutMillis` deve ser maior que zero.
- `cooldownMillis` deve ser maior que zero.
- Configuracao invalida e rejeitada ou bloqueada sem corromper a configuracao atual.
- Configuracao invalida nao vira semanticamente leitura de sensor offline.
- Configuracao invalida nao cria estado novo no System State.
- Configuracao invalida exige bomba desligada quando a bomba estiver ligada.
- Nenhuma persistencia NVS e implementada.
- Nenhuma funcionalidade de MQTT, Alert Manager, modos, cloud ou app e implementada.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar intervalo padrao de avaliacao de `1000 ms`.
- Testar configuracao valida com limites dentro de `0..100`.
- Testar rejeicao ou bloqueio seguro quando `minimumLevel >= maximumLevel`.
- Testar rejeicao ou bloqueio seguro quando `minimumLevel > 100`.
- Testar rejeicao ou bloqueio seguro quando `maximumLevel > 100`.
- Testar rejeicao ou bloqueio seguro quando `timeoutMillis = 0`.
- Testar rejeicao ou bloqueio seguro quando `cooldownMillis = 0`.
- Testar que configuracao invalida nao altera `waterLevel`.
- Testar que configuracao invalida nao cria status fora dos estados canonicos de ATO.
- Confirmar que os testes usam configuracao em memoria, sem NVS, ESP32, sensor, rele, bomba ou agua real.

---

### ID

FW-P06-002

### Titulo

Implementar politica pura e maquina de estado do ATO

### Objetivo

Criar a regra deterministica que decide quando iniciar reposicao, parar a bomba, entrar em timeout, respeitar cooldown, bloquear configuracao invalida, reconciliar divergencias, bloquear sensor invalido e recuperar o ATO usando apenas estado, configuracao e tempo injetado.

### Dependencias

FW-P06-001.

FW-P02-001.

FW-P04-003.

FW-P05-002.

### Descricao

Implementar a politica pura do ATO sem dependencias de Arduino, GPIO, relays concretos, scheduler ou hardware. A politica deve receber uma visao imutavel de `waterLevel`, `ato`, `relays.atoPump`, configuracao ATO em memoria, instante atual e configuracao local do modulo, retornando uma decisao funcional.

A politica deve aplicar uma prioridade deterministica de seguranca:

1. ATO desabilitado.
2. Configuracao ATO invalida.
3. Divergencia entre `ato.pumpRunning` e `relays.atoPump.enabled`.
4. Sensor de nivel invalido ou `SENSOR_OFFLINE`.
5. Timeout de reposicao.
6. Parada por nivel maximo.
7. Inicio por nivel minimo.
8. Manutencao do estado atual.

Quando `ConfigManager::ato().enabled` estiver falso, a politica deve manter ou retornar `DISABLED`. Se a bomba estiver ligada ou se `relays.atoPump.enabled = true`, a decisao deve exigir desligamento fail-safe da bomba.

Quando a configuracao ATO for invalida, a politica deve impedir partida da bomba e exigir desligamento fail-safe caso a bomba esteja ligada. Configuracao invalida nao deve ser representada como `SENSOR_OFFLINE`; ela deve ser uma decisao interna de bloqueio seguro usando somente estados publicos canonicos.

Quando houver divergencia entre `ato.pumpRunning` e `relays.atoPump.enabled`, a politica deve priorizar reconciliacao segura. Se qualquer um dos dois indicar bomba ligada em cenario duvidoso, a decisao deve exigir desligamento fail-safe do Rele 3 e convergencia futura para `pumpRunning = false`.

Quando `waterLevel.status` for `SENSOR_OFFLINE` ou quando `waterLevel.currentLevel` estiver acima de `100`, a politica deve impedir partida da bomba, desligar a bomba se estiver ligada e retornar decisao de `SENSOR_OFFLINE` sem corromper `waterLevel`.

Quando ATO estiver habilitado, configuracao valida, sensor valido e nivel atual estiver abaixo ou igual ao limite minimo, a politica deve iniciar reposicao se a bomba estiver desligada e se o cooldown desde a ultima parada tiver vencido. A partida deve mudar o status para `REFILLING`, marcar `pumpRunning = true` e preparar atualizacao de `lastActivation`.

Quando a bomba estiver em `REFILLING` e o nivel atual estiver maior ou igual ao limite maximo, a politica deve parar a reposicao de forma normal, mudar o status para `NORMAL`, marcar `pumpRunning = false` e preparar atualizacao de `lastCompletion`.

Quando a bomba estiver em `REFILLING` e o tempo desde `lastActivation` atingir ou exceder `timeoutMillis`, a politica deve desligar a bomba por fail-safe, mudar o status para `TIMEOUT`, marcar `pumpRunning = false`, preparar incremento de `timeoutCounter` e preparar atualizacao de `lastCompletion` como ancora de cooldown.

O timeout tambem deve cobrir o caso em que o nivel nao sobe durante a reposicao. Como a Fase 6 nao depende de agua real, essa protecao deve ser validada por sequencias simuladas de `waterLevel.currentLevel` que permanecem abaixo do nivel maximo ate `timeoutMillis`.

O cooldown deve impedir rearmes imediatos: apos uma conclusao normal ou fail-safe, novo `ATO_START` so pode ocorrer quando `cooldownMillis` tiver vencido desde a ultima parada registrada. Nivel baixo durante cooldown nao deve ligar a bomba e nao deve gerar eventos repetidos indefinidamente.

A protecao contra ciclos excessivos desta fase deve ser implementada sem criar novos campos no System State: a politica nao deve permitir multiplos ciclos consecutivos sem passagem por parada normal ou fail-safe, cooldown vencido e nova avaliacao valida. Caso uma tentativa de rearmar viole essa protecao, a bomba deve permanecer desligada.

`ATO_RECOVERED` deve ser possivel quando o estado anterior era `TIMEOUT` ou `SENSOR_OFFLINE`, o ATO esta habilitado, a configuracao e valida, o sensor voltou a ser valido, a bomba esta desligada, `relays.atoPump.enabled = false` e o nivel atual esta dentro da faixa aceitavel. Recuperacao nao deve ligar a bomba automaticamente no mesmo passo.

### Arquivos afetados

- `firmware/include/modules/ato/ato_policy.h`
- `firmware/include/modules/ato/ato_config.h`
- `firmware/include/modules/ato/ato_types.h`
- `firmware/src/modules/ato/ato_policy.cpp`
- `firmware/test/unit/test_ato_policy.cpp`

### Criterios de aceitacao

- A politica e uma regra pura testavel sem hardware.
- A politica usa `waterLevel` como fonte de verdade do nivel.
- A politica usa `ato` como fonte de verdade do estado ATO atual.
- A politica usa `relays.atoPump` para reconciliar estado real comandado do Rele 3.
- A politica usa `ConfigManager::ato()` para parametros funcionais.
- A politica aplica prioridade deterministica de seguranca.
- A politica nao le GPIO e nao aciona rele diretamente.
- A politica retorna `DISABLED` quando ATO esta desabilitado.
- A politica exige bomba desligada quando ATO esta desabilitado e `pumpRunning = true`.
- A politica exige bomba desligada quando ATO esta desabilitado e `relays.atoPump.enabled = true`.
- A politica bloqueia configuracao invalida sem tratar isso como sensor offline.
- A politica reconcilia divergencia entre `ato.pumpRunning` e `relays.atoPump.enabled`.
- A politica retorna bloqueio seguro quando sensor esta `SENSOR_OFFLINE`.
- A politica retorna bloqueio seguro quando nivel atual esta acima de `100`.
- A politica inicia reposicao quando nivel esta abaixo ou igual ao minimo, ATO esta habilitado, configuracao e sensor sao validos e cooldown venceu.
- A politica nao inicia reposicao durante cooldown.
- A politica mantem `REFILLING` enquanto a bomba esta ligada, sensor valido, nivel abaixo do maximo e timeout nao venceu.
- A politica para reposicao normal quando nivel esta maior ou igual ao maximo.
- A politica dispara timeout quando o tempo desde `lastActivation` atinge ou excede `timeoutMillis`.
- A politica dispara timeout em sequencia simulada onde o nivel nao atinge o maximo ate `timeoutMillis`.
- A politica prepara `lastCompletion` em parada normal e em parada por fail-safe.
- A politica incrementa conceitualmente `timeoutCounter` somente em nova transicao para timeout.
- A politica nao duplica timeout em avaliacoes repetidas do mesmo estado.
- A politica permite recuperacao a partir de `TIMEOUT` quando o nivel volta a faixa aceitavel.
- A politica permite recuperacao a partir de `SENSOR_OFFLINE` quando o sensor volta a ser valido.
- A recuperacao nao liga a bomba no mesmo passo.
- A politica nao altera `waterLevel`, `relays`, `modes`, `network`, `alerts` ou `systemHealth`.
- A politica nao implementa MQTT, NVS, Alert Manager, modos, cloud ou aplicativo.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar prioridade deterministica das decisoes de seguranca.
- Testar ATO desabilitado com bomba desligada.
- Testar ATO desabilitado com `ato.pumpRunning = true` e decisao de desligamento fail-safe.
- Testar ATO desabilitado com `relays.atoPump.enabled = true` e decisao de desligamento fail-safe.
- Testar configuracao invalida com bomba desligada.
- Testar configuracao invalida com bomba ligada e decisao de desligamento fail-safe.
- Testar divergencia `pumpRunning = true` e `relays.atoPump.enabled = false`.
- Testar divergencia `pumpRunning = false` e `relays.atoPump.enabled = true`.
- Testar sensor offline com bomba desligada.
- Testar sensor offline com bomba ligada e decisao de desligamento fail-safe.
- Testar nivel atual acima de `100`.
- Testar inicio de reposicao por nivel abaixo do minimo.
- Testar inicio de reposicao por nivel igual ao minimo.
- Testar que nivel normal nao inicia reposicao.
- Testar que nivel alto nao inicia reposicao.
- Testar que cooldown ainda ativo bloqueia partida.
- Testar que cooldown vencido permite partida.
- Testar permanencia em `REFILLING` antes do nivel maximo e antes do timeout.
- Testar parada normal quando nivel atinge o maximo.
- Testar que antes do timeout nao ha `TIMEOUT`.
- Testar que exatamente em `timeoutMillis` ha `TIMEOUT`.
- Testar que depois de `timeoutMillis` ha `TIMEOUT`.
- Testar sequencia simulada de nivel que nao sobe ate timeout.
- Testar que timeout repetido nao incrementa `timeoutCounter` novamente sem nova transicao.
- Testar recuperacao a partir de `TIMEOUT`.
- Testar recuperacao a partir de `SENSOR_OFFLINE`.
- Testar que recuperacao nao liga bomba automaticamente no mesmo passo.
- Confirmar que os testes usam fake de tempo e estados simulados, sem hardware fisico.

---

### ID

FW-P06-003

### Titulo

Implementar servico ATO com System State, reles e eventos locais

### Objetivo

Criar o servico funcional que executa uma avaliacao ATO, aplica a decisao da politica, comanda a bomba pelo Rele 3, atualiza o bloco `ato` como fonte unica de verdade e emite eventos locais oficiais.

### Dependencias

FW-P06-002.

FW-P02-002.

FW-P05-002.

### Descricao

Implementar o `AtoService`. Em cada ciclo, o servico deve ler `currentSystemState().waterLevel`, `currentSystemState().ato`, `currentSystemState().relays.atoPump` e `ConfigManager::ato()`, executar a politica pura da Fase 6 e aplicar a decisao resultante.

O servico deve comandar a bomba ATO somente pelo modulo de reles, usando o identificador funcional `ATO_PUMP` do Rele 3. Partida e parada normal devem usar origem `AUTOMATION`. Desligamentos por timeout, sensor invalido, ATO desabilitado, configuracao invalida ou reconciliacao de divergencia devem usar origem `FAILSAFE`.

O servico deve atualizar `ato.enabled`, `ato.status`, `ato.pumpRunning`, `ato.lastActivation`, `ato.lastCompletion` e `ato.timeoutCounter` conforme a decisao aplicada. O System State deve ser atualizado somente depois que o comando de bomba necessario tiver sucesso. Falha simulada do comando de rele nao deve marcar a bomba como ligada ou desligada no bloco `ato` se o comando correspondente nao foi aceito.

Quando a bomba iniciar por nivel baixo, o servico deve emitir `ATO_START`. Quando a bomba parar por nivel maximo, deve emitir `ATO_STOP`. Quando a bomba for desligada por timeout, deve emitir `ATO_TIMEOUT`. Quando o sensor ficar indisponivel ou invalido para ATO, deve emitir `ATO_SENSOR_OFFLINE`. Quando o ATO recuperar de `TIMEOUT` ou `SENSOR_OFFLINE`, deve emitir `ATO_RECOVERED`.

Eventos de transicao nao devem ser duplicados sem mudanca relevante. Avaliacoes repetidas durante cooldown, durante `DISABLED`, durante `SENSOR_OFFLINE` persistente, durante configuracao invalida persistente ou durante `TIMEOUT` persistente nao devem gerar eventos repetidos indefinidamente.

Em toda partida ou parada aplicada com sucesso, o servico deve manter o invariant `ato.pumpRunning == relays.atoPump.enabled`. Se o comando ao modulo de reles falhar, o servico deve reportar falha sem fingir convergencia do estado.

O servico nao deve atualizar `waterLevel`; esse bloco pertence a Fase 4. O servico nao deve comandar GPIO diretamente; esse boundary pertence a Fase 5. O servico nao deve implementar MQTT, NVS, Alert Manager, modos, cloud, aplicativo ou historico.

### Arquivos afetados

- `firmware/include/modules/ato/ato_service.h`
- `firmware/include/modules/ato/ato_events.h`
- `firmware/include/modules/ato/ato_policy.h`
- `firmware/include/modules/ato/ato_types.h`
- `firmware/src/modules/ato/ato_service.cpp`
- `firmware/src/modules/ato/ato_events.cpp`
- `firmware/src/modules/ato/ato_policy.cpp`
- `firmware/include/modules/relays/relay_service.h`
- `firmware/src/modules/relays/relay_service.cpp`
- `firmware/include/modules/relays/relay_types.h`
- `firmware/include/core/events/event_bus.h`
- `firmware/src/core/events/event_bus.cpp`
- `firmware/test/fakes/`
- `firmware/test/unit/test_ato_service.cpp`
- `firmware/test/integration/`

### Criterios de aceitacao

- O servico executa uma avaliacao ATO por chamada explicita.
- O servico le `waterLevel` do System State.
- O servico le `ato` do System State.
- O servico le `relays.atoPump` do System State.
- O servico le configuracao ATO do Config Manager em memoria.
- O servico comanda a bomba somente pelo modulo de reles.
- O servico usa o identificador funcional do Rele 3 ATO.
- O servico nao escreve GPIO diretamente.
- Partida normal usa origem `AUTOMATION`.
- Parada normal usa origem `AUTOMATION`.
- Parada por fail-safe usa origem `FAILSAFE`.
- Sucesso ao iniciar atualiza `ato.status = REFILLING`.
- Sucesso ao iniciar atualiza `ato.pumpRunning = true`.
- Sucesso ao iniciar atualiza `ato.lastActivation`.
- Sucesso ao iniciar mantem `ato.pumpRunning == relays.atoPump.enabled`.
- Sucesso ao parar por nivel maximo atualiza `ato.status = NORMAL`.
- Sucesso ao parar por nivel maximo atualiza `ato.pumpRunning = false`.
- Sucesso ao parar por nivel maximo atualiza `ato.lastCompletion`.
- Sucesso ao parar por nivel maximo mantem `ato.pumpRunning == relays.atoPump.enabled`.
- Timeout atualiza `ato.status = TIMEOUT`.
- Timeout atualiza `ato.pumpRunning = false`.
- Timeout atualiza `ato.lastCompletion` como ancora de cooldown.
- Timeout incrementa `ato.timeoutCounter` uma vez por nova ocorrencia.
- Sensor offline ou invalido atualiza `ato.status = SENSOR_OFFLINE`.
- ATO desabilitado atualiza `ato.status = DISABLED`.
- Configuracao invalida bloqueia partida e mantem a bomba desligada sem criar status novo.
- `ato.enabled` reflete a configuracao ATO em memoria.
- Falha simulada ao ligar bomba nao marca `pumpRunning = true`.
- Falha simulada ao desligar bomba nao mascara o erro de comando.
- Divergencia entre `ato.pumpRunning` e `relays.atoPump.enabled` e reconciliada por desligamento fail-safe quando aplicavel.
- `ATO_START` e emitido em partida real da bomba.
- `ATO_STOP` e emitido em parada normal por nivel maximo.
- `ATO_TIMEOUT` e emitido em parada por timeout.
- `ATO_SENSOR_OFFLINE` e emitido em transicao para sensor indisponivel ou invalido.
- `ATO_RECOVERED` e emitido em recuperacao de `TIMEOUT` ou `SENSOR_OFFLINE`.
- Eventos ATO nao sao duplicados sem transicao relevante.
- O servico nao altera `waterLevel`.
- O servico nao altera `temperature`, `lighting`, `modes`, `network`, `alerts` ou `systemHealth`.
- O servico nao implementa MQTT, NVS, Alert Manager, modos, cloud, app ou historico.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar chamada unica do servico com ATO desabilitado.
- Testar chamada unica do servico com nivel normal.
- Testar nivel baixo iniciando bomba com fake relay controller.
- Testar que partida atualiza `ato` e `relays.atoPump`.
- Testar que partida mantem `ato.pumpRunning == relays.atoPump.enabled`.
- Testar que partida emite `ATO_START`.
- Testar nivel maximo parando bomba.
- Testar que parada normal atualiza `lastCompletion`.
- Testar que parada normal mantem `ato.pumpRunning == relays.atoPump.enabled`.
- Testar que parada normal emite `ATO_STOP`.
- Testar timeout desligando bomba por fail-safe.
- Testar que timeout atualiza `lastCompletion` como ancora de cooldown.
- Testar que timeout incrementa `timeoutCounter` uma vez.
- Testar que timeout emite `ATO_TIMEOUT`.
- Testar sensor offline com bomba desligada.
- Testar sensor offline com bomba ligada e desligamento fail-safe.
- Testar que sensor offline emite `ATO_SENSOR_OFFLINE`.
- Testar configuracao invalida com bomba ligada e desligamento fail-safe.
- Testar divergencia entre `ato.pumpRunning` e `relays.atoPump.enabled`.
- Testar recuperacao apos `TIMEOUT`.
- Testar recuperacao apos `SENSOR_OFFLINE`.
- Testar que recuperacao emite `ATO_RECOVERED`.
- Testar que falha simulada ao ligar bomba preserva `ato.pumpRunning = false`.
- Testar que falha simulada ao desligar bomba e reportada.
- Testar que eventos repetidos nao sao duplicados sem transicao.
- Testar que blocos nao relacionados do System State nao sao corrompidos.
- Confirmar que os testes usam fake relay controller, fake tempo e estados simulados, sem hardware fisico.

---

### ID

FW-P06-004

### Titulo

Integrar ATO ao runtime com scheduler sem bloqueio

### Objetivo

Registrar o servico ATO no runtime do core para avaliacao periodica local, garantindo ordem segura de inicializacao, loop nao bloqueante, watchdog preservado e validacao sem hardware.

### Dependencias

FW-P06-003.

FW-P02-004.

FW-P02-007.

FW-P04-004.

FW-P05-003.

### Descricao

Integrar o `AtoService` ao `CoreApp` depois da inicializacao de System State, Event Bus, Config Manager, Logger, Scheduler, Watchdog, water level e relays. O modulo ATO deve ser registrado somente apos o servico de nivel estar disponivel e apos o boot seguro dos reles ter comandado todos os reles para OFF.

Registrar uma tarefa periodica de ATO no scheduler com intervalo padrao de `1000 ms` vindo do contrato local do modulo ATO. A tarefa deve chamar uma iteracao do `AtoService` e retornar sucesso ou falha de forma compativel com o scheduler.

O ATO deve operar inteiramente local no firmware. Ele nao deve depender de Internet, MQTT, cloud ou aplicativo. Tambem nao deve depender de modo operacional da Fase 7; nesta fase, o unico habilitador funcional deve ser `ConfigManager::ato().enabled`.

O runtime deve garantir que, quando o ATO estiver desabilitado por configuracao, a tarefa possa rodar sem ligar bomba e mantendo `ato.status = DISABLED`. Testes de integracao podem habilitar ATO via Config Manager em memoria para validar o fluxo completo sem NVS.

O loop principal nao deve bloquear por causa do ATO. O servico deve avaliar estado ja disponivel e comandar o modulo de reles por chamadas funcionais curtas, sem delays, polling fisico longo ou espera por agua real.

Testes de integracao da Fase 6 devem montar estado de nivel por fixtures do System State ou servicos ja existentes da Fase 4. Eles nao devem criar fake sensor novo como dependencia do ATO nem fazer o ATO chamar diretamente o sensor VL6180X.

Esta tarefa nao deve inicializar MQTT, NVS, Alert Manager, modos, iluminacao, cloud, aplicativo ou testes de resiliencia de fases posteriores.

### Arquivos afetados

- `firmware/src/app/core_app.h`
- `firmware/src/app/core_app.cpp`
- `firmware/src/app/core_app_arduino.cpp`
- `firmware/include/modules/ato/ato_service.h`
- `firmware/include/modules/ato/ato_config.h`
- `firmware/src/modules/ato/ato_service.cpp`
- `firmware/include/core/scheduler/task_scheduler.h`
- `firmware/src/core/scheduler/task_scheduler.cpp`
- `firmware/test/fakes/`
- `firmware/test/unit/`
- `firmware/test/integration/test_ato_scheduler_integration.cpp`

### Criterios de aceitacao

- O runtime cria ou recebe o servico funcional ATO.
- O runtime inicializa ATO apos System State.
- O runtime inicializa ATO apos Event Bus.
- O runtime inicializa ATO apos Config Manager.
- O runtime inicializa ATO apos boot seguro dos reles.
- O runtime inicializa ATO apos registrar o servico de nivel.
- O runtime registra tarefa periodica de ATO.
- O intervalo padrao da tarefa ATO e `1000 ms`.
- A tarefa ATO executa uma iteracao do servico.
- A tarefa ATO nao executa antes do intervalo configurado.
- A tarefa ATO nao bloqueia o loop principal.
- ATO desabilitado por configuracao nao liga bomba.
- ATO habilitado por configuracao em memoria pode iniciar reposicao em teste.
- Testes de integracao usam fixtures do System State ou servicos existentes, sem fake sensor especifico do ATO.
- O Watchdog continua sendo alimentado conforme a Fase 2.
- O build real nao depende de fake relay controller, fake sensor ou fake tempo.
- Nenhuma funcionalidade de MQTT, NVS, Alert Manager, modos, iluminacao, cloud ou aplicativo e inicializada.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste de integracao do runtime com fixture de System State para `waterLevel` e fake relay controller.
- Testar que `setup` registra a tarefa ATO.
- Testar que a tarefa ATO nao executa antes de `1000 ms`.
- Testar que a tarefa ATO executa quando o intervalo vence.
- Testar que uma execucao da tarefa chama o servico ATO uma vez.
- Testar que multiplos ciclos respeitam o intervalo configurado.
- Testar que ATO desabilitado por default nao liga a bomba.
- Testar que ATO habilitado em memoria inicia reposicao quando nivel esta baixo.
- Testar que ATO habilitado em memoria para reposicao quando nivel atinge maximo.
- Testar que timeout simulado e aplicado pelo scheduler sem delay real.
- Testar que cooldown e respeitado entre ciclos do scheduler.
- Testar que `loopOnce` permanece nao bloqueante.
- Testar que falha simulada no servico e refletida como falha de tarefa quando aplicavel.
- Confirmar que nenhum teste exige ESP32, VL6180X real, rele real, bomba real, agua real, Wi-Fi, MQTT, cloud, app ou Serial real.

---

### ID

FW-P06-005

### Titulo

Consolidar validacao automatizada da Fase 6

### Objetivo

Criar o gate final da Fase 6, garantindo que o sistema ATO esteja integrado, testado por mocks e fakes, limitado ao escopo definido e preparado para validacao fisica futura sem depender dela.

### Dependencias

FW-P06-004.

### Descricao

Consolidar testes unitarios e de integracao da Fase 6 em comandos ou scripts de validacao. O gate deve provar que os contratos ATO operam sem hardware, que a validacao de configuracao esta fechada, que a politica ATO opera sem hardware, que o servico atualiza `ato`, que a bomba e comandada pelo modulo de reles, que `waterLevel` e consumido sem ser corrompido, que os eventos `ATO_*` sao emitidos corretamente e que o scheduler executa a avaliacao periodica sem bloquear o loop principal.

Tambem deve haver uma revisao de escopo confirmando que a Fase 6 nao implementou modos operacionais, persistencia NVS, Wi-Fi, MQTT, Alert Manager, cloud, app, historico, notificacoes, comandos remotos, GPIO direto da bomba, driver proprio de bomba ATO, leitura bruta do VL6180X ou testes de resiliencia de fases posteriores.

Qualquer validacao real com sensor, bomba, rele, agua, vazao, sump, reservatorio, evaporacao, tempo de enchimento, corrente, ruido eletrico, resposta fisica, Serial real ou seguranca de bancada deve ficar marcada como `Pendente para Hardware Validation` e nao pode bloquear a conclusao de desenvolvimento da Fase 6.

### Arquivos afetados

- `firmware/test/unit/`
- `firmware/test/integration/`
- `firmware/test/fakes/`
- `firmware/test/run_phase6_validation.sh`
- `firmware/include/modules/ato/`
- `firmware/src/modules/ato/`
- `firmware/include/modules/relays/`
- `firmware/src/modules/relays/`
- `firmware/src/app/`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste
- `tasks/firmware-phase-06-ato-system.md`

### Criterios de aceitacao

- Existe comando ou script para executar os testes unitarios da Fase 6.
- Existe comando ou script para executar os testes de integracao da Fase 6.
- Existe revisao automatizada ou objetiva de escopo da Fase 6.
- Todos os testes da Fase 6 usam mocks, fakes ou valores simulados.
- O build real do firmware compila sem depender de mocks ou fakes.
- Contratos ATO sao validados sem hardware.
- Configuracao ATO em memoria e validada sem hardware.
- A politica ATO e validada sem hardware.
- O servico ATO e validado sem hardware.
- A integracao do scheduler ATO e validada sem hardware.
- O bloco `ato` do System State e atualizado conforme a spec.
- `ato.enabled` reflete a configuracao em memoria.
- `ato.status` usa somente estados canonicos da spec.
- `ato.pumpRunning` reflete a decisao aplicada com sucesso.
- `ato.lastActivation` e validado por teste automatizado.
- `ato.lastCompletion` e validado por teste automatizado em parada normal e fail-safe.
- `ato.timeoutCounter` e validado por teste automatizado.
- A bomba ATO e comandada pelo modulo de reles, nao por GPIO direto.
- Nao existe driver proprio de bomba ATO.
- Origem `AUTOMATION` e validada para partidas e paradas normais.
- Origem `FAILSAFE` e validada para desligamentos de seguranca.
- O invariant `ato.pumpRunning == relays.atoPump.enabled` e validado apos comandos bem sucedidos.
- Divergencia entre `ato.pumpRunning` e `relays.atoPump.enabled` e validada por teste automatizado.
- Inicio por nivel minimo e validado por teste automatizado.
- Parada por nivel maximo e validada por teste automatizado.
- Timeout e validado por teste automatizado.
- Timeout por nivel que nao sobe ate o maximo e validado por teste automatizado.
- Cooldown e validado por teste automatizado.
- Sensor offline e validado por teste automatizado.
- Configuracao invalida e validada por teste automatizado.
- Recuperacao e validada por teste automatizado.
- Eventos locais `ATO_START`, `ATO_STOP`, `ATO_TIMEOUT`, `ATO_SENSOR_OFFLINE` e `ATO_RECOVERED` sao validados por teste automatizado.
- Eventos ATO nao publicam MQTT, nao criam alertas centralizados e nao persistem historico.
- Nenhum teste fisico e criterio de conclusao da Fase 6.
- Qualquer validacao real do ATO fica marcada como `Pendente para Hardware Validation`.
- Revisao de escopo confirma que modos operacionais nao foram implementados.
- Revisao de escopo confirma que NVS nao foi implementado.
- Revisao de escopo confirma que MQTT nao foi implementado.
- Revisao de escopo confirma que Alert Manager nao foi implementado.
- Revisao de escopo confirma que cloud e app nao foram implementados.
- Revisao de escopo confirma que comandos remotos nao foram implementados.
- Revisao de escopo confirma que nao ha leitura bruta do VL6180X no modulo ATO.
- Revisao de escopo confirma que o modulo ATO nao altera `waterLevel`.
- Revisao de escopo confirma que nao ha `firmware/src/drivers/ato/` ou driver equivalente de bomba ATO.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar todos os testes unitarios da Fase 6 com sucesso.
- Executar todos os testes de integracao da Fase 6 com sucesso.
- Executar testes existentes da Fase 2 impactados por Event Bus, System State, Config Manager, Scheduler e Watchdog.
- Executar testes existentes da Fase 4 impactados por `waterLevel`.
- Executar testes existentes da Fase 5 impactados por relays e origem de comando.
- Executar script de validacao final da Fase 6.
- Confirmar que o build real nao inclui fakes ou mocks.
- Confirmar que os testes nao exigem ESP32, VL6180X, modulo de rele, bomba ATO, agua real, Wi-Fi, MQTT, cloud, app ou qualquer validacao fisica.
- Confirmar que eventos `ATO_*` existem somente como eventos locais nesta fase.
- Confirmar que nenhum teste ou implementacao inicializa MQTT, NVS, Alert Manager, modos, cloud ou app.
- Confirmar que nenhum teste ou implementacao usa GPIO direto para a bomba ATO.
- Confirmar que nenhum teste ou implementacao cria driver proprio de bomba ATO.
- Confirmar que nenhum teste ou implementacao altera `architecture/`, `specs/` ou `tasks/firmware-master-plan.md`.
- Confirmar por revisao que funcionalidades de fases posteriores nao foram iniciadas.
