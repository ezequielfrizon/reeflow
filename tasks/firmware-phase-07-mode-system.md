# Fase 7 - Sistema de Modos

## Escopo

Implementar o modulo funcional de modos operacionais do REEFLOW: `NORMAL`, `FEEDING`, `TPA` e `MAINTENANCE`. A fase deve permitir troca local de modo, aplicar comportamento operacional previsto sobre reles e automacoes locais ja existentes, atualizar o bloco `modes` do System State, emitir eventos locais `MODE_*`, retornar automaticamente de `FEEDING` para `NORMAL` e preparar persistencia do modo atual por boundary substituivel validado com fake.

Esta fase depende das fases anteriores do firmware funcional local. Ela deve operar sem Internet, MQTT, cloud, aplicativo, monitor Serial real, ESP32 conectado, modulo de rele conectado, bomba real, sensor real, agua real, luminaria real ou qualquer validacao fisica.

Mocks e fakes devem ser usados para validacoes automatizadas das implementacoes, seguindo o padrao das fases anteriores. Mocks e fakes devem ficar restritos a `firmware/test/` ou a configuracoes de teste, sem se tornarem dependencia do build real do firmware destinado ao hardware.

O firmware real deve ficar preparado para operar os modos com hardware futuro, mas a conclusao desta fase nao pode depender de teste fisico. Qualquer verificacao com reles reais, bomba real, resposta hidraulica, alimentacao real, TPA real, manutencao real, comportamento fisico do aquario, Serial real ou seguranca de bancada deve ficar registrada como `Pendente para Hardware Validation` e fora dos criterios de conclusao desta fase.

Nao implementar iluminacao da Fase 8, NVS Preferences definitivo da Fase 9, Wi-Fi, MQTT, Alert Manager, cloud, aplicativo, historico, notificacoes, comandos remotos, resiliencia integrada ou funcionalidades de fases posteriores.

A persistencia desta fase deve ficar limitada ao contrato funcional do modo atual e a validacao com fake ou backend substituivel. A implementacao definitiva de NVS Preferences, politica de gravacoes e restauracao completa de configuracoes fica reservada para a Fase 9. O objetivo da Fase 7 e garantir que o modulo de modos solicite salvamento/restauracao por um boundary claro, sem acoplar o servico de modos a NVS real.

O bloqueio de automacoes por modo deve ser representado por um boundary funcional explicito do modulo de modos. Modulos locais que executam automacoes, especialmente o ATO, devem conseguir consultar esse gate antes de atuar. Desligar a bomba uma vez ao entrar em um modo bloqueante nao e suficiente se o scheduler do ATO puder religar a bomba no ciclo seguinte.

O modo `NORMAL` representa operacao padrao e deve liberar os bloqueios locais de modo, mantendo automacoes habilitadas conforme suas proprias configuracoes. `NORMAL` nao deve ligar reles automaticamente apenas por entrar no modo.

O modo `FEEDING` deve desligar o Rele Recalque temporariamente, suspender automacoes locais configuradas para nao atuar durante alimentacao e retornar automaticamente para `NORMAL` ao final do tempo configurado. O tempo deve vir de `ConfigManager::timers().feedingDurationSeconds`, com default existente de configuracao em memoria.

O modo `TPA` deve desligar os equipamentos configurados para troca parcial de agua, bloquear automacoes configuradas enquanto o modo estiver ativo e retornar somente por comando manual local para outro modo. Na ausencia de configuracao persistida especifica, a Fase 7 deve definir defaults locais testaveis para desligar Recalque e manter ATO seguro/desligado, sem criar UI, MQTT, app ou NVS.

O modo `MAINTENANCE` deve permitir desligamentos manuais e suspender regras nao criticas configuradas enquanto estiver ativo. Ele deve retornar somente por comando manual local para outro modo. O modo nao deve criar automacoes novas, nao deve comandar iluminacao da Fase 8 e nao deve substituir fail-safes criticos ja existentes.

Eventos `MODE_CHANGED`, `MODE_STARTED` e `MODE_FINISHED` devem ser eventos locais do firmware nesta fase. Eles podem servir como materia-prima futura para MQTT, historico, alertas, cloud ou aplicativo, mas nao devem publicar MQTT, persistir historico, criar alertas centralizados ou enviar notificacoes nesta fase.

Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` deve ser alterado.

## Estrutura Recomendada

```text
firmware/
|-- include/
|   |-- modules/
|   |   |-- modes/
|   |   |   |-- mode_types.h
|   |   |   |-- mode_config.h
|   |   |   |-- mode_events.h
|   |   |   |-- mode_policy.h
|   |   |   |-- mode_effects.h
|   |   |   |-- mode_automation_gate.h
|   |   |   |-- mode_store.h
|   |   |   `-- mode_service.h
|   |   |-- relays/
|   |   `-- ato/
|   |-- core/
|   `-- config/
|-- src/
|   |-- modules/
|   |   |-- modes/
|   |   |   |-- mode_events.cpp
|   |   |   |-- mode_policy.cpp
|   |   |   |-- mode_effects.cpp
|   |   |   |-- mode_automation_gate.cpp
|   |   |   `-- mode_service.cpp
|   |   |-- relays/
|   |   `-- ato/
|   |-- app/
|   `-- core/
`-- test/
    |-- fakes/
    |   |-- fake_mode_store.h
    |   |-- fake_mode_effects.h
    |   `-- fake_mode_automation_gate.h
    |-- unit/
    |   |-- test_mode_contracts.cpp
    |   |-- test_mode_policy.cpp
    |   `-- test_mode_service.cpp
    |-- integration/
    |   |-- test_mode_scheduler_integration.cpp
    |   |-- run_phase7_integration_tests.sh
    |   `-- run_phase7_scope_review.sh
    `-- hardware/
```

`firmware/test/hardware/` fica reservado para validacoes futuras e nao e criterio de conclusao da Fase 7.

O modulo funcional em `modules/modes` nao deve depender de tipos, resultados ou rotinas de diagnostico da Fase 1. Ele deve consumir somente contratos funcionais ja existentes de System State, Config Manager, Event Bus, Time Source, relays e ATO.

## Tarefas

### ID

FW-P07-000

### Titulo

Definir contratos funcionais, eventos, configuracao local e fakes de modos

### Objetivo

Criar os contratos minimos do modulo de modos, incluindo tipos, comandos, eventos locais, configuracao de comportamento, boundary de persistencia do modo atual e fakes de teste, permitindo validacao automatizada sem hardware e sem NVS definitivo.

### Dependencias

Fase 2 concluida.

Fase 5 concluida.

Fase 6 concluida.

### Descricao

Definir o modulo `modules/modes` com os quatro modos canonicos do System State: `NORMAL`, `FEEDING`, `TPA` e `MAINTENANCE`. Os tipos publicos devem ser compativeis com `core::state::OperationalMode` e nao devem criar nomes alternativos para os modos.

Definir um contrato de comando de modo contendo modo solicitado, origem local da solicitacao, instante da solicitacao e motivo funcional. Nesta fase, a entrada implementada deve ser local ao firmware. Valores futuros de MQTT ou app podem existir no estado global como conceitos canonicos, mas transporte remoto e comandos remotos nao devem ser implementados.

Declarar os eventos locais `MODE_CHANGED`, `MODE_STARTED` e `MODE_FINISHED`, mapeados para o Event Bus local. Os eventos devem carregar, quando necessario, modo anterior, modo novo, instante de inicio, tempo restante e motivo de finalizacao, sem publicar MQTT, sem criar alertas centralizados e sem persistir historico.

Definir `mode_config.h` com configuracao local em memoria para comportamento de modos. O tempo de `FEEDING` deve vir de `ConfigManager::timers().feedingDurationSeconds`. `TPA` e `MAINTENANCE` devem ter retorno manual nesta fase, seguindo `specs/modes-spec.md`. Qualquer campo de timer existente para TPA ou maintenance deve permanecer disponivel no Config Manager, mas nao deve ser usado para retorno automatico enquanto a spec de modos exigir retorno manual.

Definir defaults locais testaveis para os efeitos de modo: `FEEDING` desliga Recalque; `TPA` desliga Recalque e mantem ATO seguro/desligado; `MAINTENANCE` suspende automacoes nao criticas e preserva controle manual; `NORMAL` libera bloqueios de modo sem ligar equipamentos automaticamente.

Definir um boundary de persistencia do modo atual, por exemplo `ModeStore`, com operacoes minimas para salvar e carregar o modo atual. O contrato deve permitir fake de teste e backend real futuro, mas a tarefa nao deve implementar NVS Preferences definitivo.

Criar fake ou mock de persistencia de modo restrito aos testes automatizados. O fake deve permitir simular carregamento valido, carregamento ausente, modo invalido, falha de salvamento, ultimo valor salvo e contagem de gravacoes.

Criar fake ou mock de efeitos de modo restrito aos testes quando necessario, permitindo inspecionar comandos esperados para reles e automacoes sem hardware.

Esta tarefa nao deve implementar a politica completa de transicao, nao deve atualizar System State, nao deve comandar reles, nao deve alterar ATO e nao deve registrar tarefa periodica.

### Arquivos afetados

- `firmware/include/modules/modes/mode_types.h`
- `firmware/include/modules/modes/mode_config.h`
- `firmware/include/modules/modes/mode_events.h`
- `firmware/include/modules/modes/mode_policy.h`
- `firmware/include/modules/modes/mode_effects.h`
- `firmware/include/modules/modes/mode_automation_gate.h`
- `firmware/include/modules/modes/mode_store.h`
- `firmware/include/modules/modes/mode_service.h`
- `firmware/src/modules/modes/mode_events.cpp`
- `firmware/include/config/config_manager.h`, somente se ajuste de validacao de timers/modo ja previsto for necessario
- `firmware/src/config/config_manager.cpp`, somente se ajuste de validacao de timers/modo ja previsto for necessario
- `firmware/test/fakes/fake_mode_store.h`
- `firmware/test/fakes/fake_mode_effects.h`
- `firmware/test/fakes/fake_mode_automation_gate.h`
- `firmware/test/unit/test_mode_contracts.cpp`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe modulo `modules/modes` com contratos publicos da Fase 7.
- Existem tipos funcionais para `NORMAL`, `FEEDING`, `TPA` e `MAINTENANCE`.
- Os tipos funcionais sao compativeis com `core::state::OperationalMode`.
- Nenhum modo novo e criado.
- Existe contrato de comando local de modo.
- O contrato de comando nao implementa MQTT, app, cloud ou Serial command shell.
- Existem eventos locais `MODE_CHANGED`, `MODE_STARTED` e `MODE_FINISHED`.
- Eventos de modo sao mapeados para o Event Bus local.
- Eventos de modo nao publicam MQTT.
- Eventos de modo nao criam Alert Manager.
- Eventos de modo nao persistem historico.
- Existe configuracao local de comportamento dos modos.
- `FEEDING` usa `ConfigManager::timers().feedingDurationSeconds`.
- `TPA` e `MAINTENANCE` sao definidos como retorno manual nesta fase.
- Existe default local para desligar Recalque em `FEEDING`.
- Existe default local para desligar Recalque em `TPA`.
- Existe default local para manter ATO seguro/desligado em `TPA`.
- Existe default local para suspender automacoes nao criticas em `MAINTENANCE`.
- `NORMAL` e definido como liberacao de bloqueios, sem ligar equipamentos automaticamente.
- Existe boundary de persistencia de modo atual.
- O boundary de persistencia nao depende de NVS Preferences real nesta fase.
- Existe boundary funcional para aplicar efeitos de modo sem expor GPIO.
- Existe boundary funcional para consultar permissoes de automacao por modo.
- O gate de automacao permite bloquear ATO em modos configurados.
- O gate de automacao permite liberar ATO em `NORMAL` quando a configuracao propria do ATO permitir.
- Existe fake ou mock de persistencia restrito a testes.
- O fake permite simular modo salvo valido.
- O fake permite simular ausencia de modo salvo.
- O fake permite simular falha de salvamento.
- O fake permite inspecionar o ultimo modo salvo.
- O build real do firmware nao depende de fakes ou mocks.
- Nenhum comportamento funcional de troca de modo e iniciado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario confirmando os quatro modos canonicos.
- Executar teste unitario confirmando mapeamento para `core::state::OperationalMode`.
- Executar teste unitario confirmando eventos `MODE_*` no Event Bus local.
- Executar teste unitario confirmando que eventos de modo nao publicam MQTT.
- Executar teste unitario confirmando default de `FEEDING` para desligar Recalque.
- Executar teste unitario confirmando default de `TPA` para desligar Recalque.
- Executar teste unitario confirmando default de `TPA` para manter ATO seguro/desligado.
- Executar teste unitario confirmando default de `MAINTENANCE` para suspender automacoes nao criticas.
- Executar teste unitario confirmando que `NORMAL` nao liga reles automaticamente.
- Executar teste unitario confirmando contrato do gate de automacoes para ATO permitido e bloqueado.
- Executar teste unitario confirmando leitura valida do fake `ModeStore`.
- Executar teste unitario confirmando ausencia de modo salvo no fake `ModeStore`.
- Executar teste unitario confirmando falha simulada de salvamento.
- Confirmar que nenhum teste exige ESP32, reles reais, sensores reais, agua real, Wi-Fi, MQTT, cloud, app ou Serial real.
- Confirmar que o build real nao inclui o fake ou mock de teste.

---

### ID

FW-P07-001

### Titulo

Implementar politica pura de transicao e efeitos de modos

### Objetivo

Criar a regra deterministica que decide transicoes, duracao, retorno automatico de alimentacao, efeitos locais sobre reles e automacoes, e rejeicoes de comandos invalidos usando apenas estado, configuracao e tempo injetado.

### Dependencias

FW-P07-000.

FW-P02-001.

FW-P02-003.

### Descricao

Implementar `ModePolicy` como regra pura sem dependencias de Arduino, GPIO, relays concretos, ATO concreto, scheduler, NVS, MQTT ou hardware. A politica deve receber uma visao imutavel de `system.modes`, configuracao de timers/modos em memoria, instante atual e comando opcional de troca, retornando uma decisao funcional.

A politica deve aceitar transicoes explicitas locais entre `NORMAL`, `FEEDING`, `TPA` e `MAINTENANCE`. Comandos para o modo ja ativo devem ser idempotentes: nao devem reiniciar `startedAt`, nao devem alterar `remainingTime`, nao devem gerar efeitos de rele duplicados e nao devem exigir novo salvamento.

Comandos manuais locais para sair de `FEEDING`, `TPA` ou `MAINTENANCE` para `NORMAL` devem ser aceitos de forma deterministica. A saida manual para `NORMAL` deve liberar bloqueios locais de modo, mas nao deve religar equipamentos automaticamente.

Ao entrar em `NORMAL`, a politica deve retornar decisao para liberar bloqueios locais de modo e concluir o modo anterior quando aplicavel. `NORMAL` nao deve ligar Recalque, ATO Pump, Aquecedor ou Reserva automaticamente.

Ao entrar em `FEEDING`, a politica deve calcular `startedAt` pelo tempo atual, `remainingTime` a partir de `feedingDurationSeconds`, efeito de desligar Recalque e efeito de suspender automacoes configuradas para alimentacao. Se `feedingDurationSeconds` for zero, a configuracao deve ser rejeitada ou tratada como invalida sem entrar em um modo temporario sem duracao.

Enquanto `FEEDING` estiver ativo, a politica deve reduzir `remainingTime` conforme o tempo atual. Antes do vencimento, deve manter o modo ativo. Quando o tempo vencer ou passar do limite, deve decidir retorno automatico para `NORMAL` e gerar motivo de finalizacao por tempo.

Ao entrar em `TPA`, a politica deve definir `startedAt`, `remainingTime = 0`, efeito de desligar equipamentos configurados e efeito de bloquear automacoes configuradas. `TPA` nao deve retornar automaticamente nesta fase.

Ao entrar em `MAINTENANCE`, a politica deve definir `startedAt`, `remainingTime = 0`, efeito de suspender regras nao criticas e preservar a possibilidade de comandos manuais locais aos reles. `MAINTENANCE` nao deve retornar automaticamente nesta fase.

A politica deve diferenciar efeitos de entrada, permanencia e saida para que o servico aplique comandos de rele e bloqueios de automacao somente quando necessario. Avaliacoes repetidas sem transicao relevante nao devem gerar eventos repetidos.

A politica deve priorizar seguranca local: se um modo exigir ATO seguro/desligado, a decisao deve pedir bloqueio de automacao ATO e desligamento seguro da bomba quando a bomba estiver ligada. A politica nao deve criar novos estados de ATO e nao deve atualizar `ato` diretamente.

### Arquivos afetados

- `firmware/include/modules/modes/mode_policy.h`
- `firmware/include/modules/modes/mode_types.h`
- `firmware/include/modules/modes/mode_config.h`
- `firmware/src/modules/modes/mode_policy.cpp`
- `firmware/test/unit/test_mode_policy.cpp`

### Criterios de aceitacao

- A politica e uma regra pura testavel sem hardware.
- A politica usa `system.modes` como entrada de estado atual.
- A politica usa `ConfigManager::timers()` ou visao equivalente de configuracao em memoria.
- A politica usa tempo injetado.
- A politica nao le GPIO.
- A politica nao comanda reles diretamente.
- A politica nao chama ATO diretamente.
- A politica nao acessa NVS.
- A politica nao acessa MQTT, Wi-Fi, cloud ou app.
- Transicao local para `NORMAL` e aceita.
- Transicao local para `FEEDING` e aceita.
- Transicao local para `TPA` e aceita.
- Transicao local para `MAINTENANCE` e aceita.
- Saida manual local para `NORMAL` a partir de qualquer modo e aceita.
- Saida manual local para `NORMAL` nao liga equipamentos automaticamente.
- Comando para modo ja ativo e idempotente.
- Comando idempotente nao reinicia `startedAt`.
- Comando idempotente nao altera `remainingTime`.
- Comando idempotente nao pede eventos ou efeitos duplicados.
- Entrada em `NORMAL` libera bloqueios locais de modo.
- Entrada em `NORMAL` nao liga equipamentos automaticamente.
- Entrada em `FEEDING` define `startedAt`.
- Entrada em `FEEDING` define `remainingTime` pelo timer de alimentacao.
- Entrada em `FEEDING` pede desligamento de Recalque.
- Entrada em `FEEDING` pede suspensao das automacoes configuradas.
- Timer de `FEEDING` igual a zero e rejeitado ou bloqueado de forma deterministica.
- `FEEDING` antes do vencimento permanece ativo.
- `FEEDING` vencido decide retorno automatico para `NORMAL`.
- Entrada em `TPA` define `startedAt`.
- Entrada em `TPA` define `remainingTime = 0`.
- Entrada em `TPA` pede desligamento dos equipamentos configurados.
- Entrada em `TPA` pede bloqueio das automacoes configuradas.
- `TPA` nao retorna automaticamente nesta fase.
- Entrada em `MAINTENANCE` define `startedAt`.
- Entrada em `MAINTENANCE` define `remainingTime = 0`.
- Entrada em `MAINTENANCE` pede suspensao de regras nao criticas.
- `MAINTENANCE` nao retorna automaticamente nesta fase.
- Efeitos de entrada, permanencia e saida sao diferenciados.
- A politica nao cria novos estados em `system.modes`.
- A politica nao cria novos estados em `system.ato`.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar transicao `NORMAL -> FEEDING`.
- Testar transicao `NORMAL -> TPA`.
- Testar transicao `NORMAL -> MAINTENANCE`.
- Testar transicao `FEEDING -> NORMAL` por comando manual local.
- Testar transicao `TPA -> NORMAL` por comando manual local.
- Testar transicao `MAINTENANCE -> NORMAL` por comando manual local.
- Testar comando idempotente para cada modo.
- Testar que entrada em `NORMAL` nao pede ligamento de reles.
- Testar que entrada em `FEEDING` pede desligamento de Recalque.
- Testar `remainingTime` de `FEEDING` logo apos entrada.
- Testar `remainingTime` de `FEEDING` antes do vencimento.
- Testar retorno automatico de `FEEDING` exatamente no vencimento.
- Testar retorno automatico de `FEEDING` apos o vencimento.
- Testar rejeicao ou bloqueio deterministico de `feedingDurationSeconds = 0`.
- Testar que `TPA` nao retorna automaticamente com passagem de tempo.
- Testar que `MAINTENANCE` nao retorna automaticamente com passagem de tempo.
- Testar que efeitos repetidos nao sao solicitados sem transicao relevante.
- Testar que bloqueio de ATO e solicitado nos modos configurados.
- Confirmar que os testes usam fake de tempo e estados simulados, sem hardware fisico.

---

### ID

FW-P07-002A

### Titulo

Implementar executor de efeitos e gate de automacoes por modo

### Objetivo

Criar o boundary que aplica os efeitos operacionais decididos pela politica de modos e o gate consultavel por automacoes locais, garantindo que reles e ATO sejam coordenados sem GPIO direto, sem driver novo e sem antecipar fases futuras.

### Dependencias

FW-P07-001.

FW-P05-002.

FW-P06-003.

### Descricao

Implementar `ModeEffects` ou boundary equivalente para aplicar efeitos de entrada, permanencia e saida de modo retornados pela `ModePolicy`. Esse executor deve ser o unico ponto do modulo de modos que conversa com `RelayService` e com o gate de automacoes. O `ModeService` deve orquestrar a transicao, mas nao concentrar a logica detalhada de desligamento de equipamentos ou permissao de automacao.

Efeitos sobre reles devem ser aplicados exclusivamente pelo modulo funcional de reles. O executor de efeitos nao deve escrever GPIO diretamente, nao deve acessar `GpioPort`, nao deve instanciar driver de rele e nao deve usar diagnosticos da Fase 1.

Quando `FEEDING` exigir desligamento de Recalque, o executor deve comandar o Rele Recalque pelo `RelayService` com origem `AUTOMATION`. Quando `TPA` exigir desligamento de equipamentos configurados, o executor deve comandar esses reles pelo `RelayService` com origem `AUTOMATION`, exceto quando o desligamento for explicitamente de seguranca, caso em que a origem deve ser `FAILSAFE`.

Implementar `ModeAutomationGate` ou boundary equivalente para representar permissoes de automacao derivadas do modo atual. O gate deve permitir consultar, no minimo, se a automacao ATO pode atuar no modo atual. `NORMAL` deve liberar automacoes conforme suas configuracoes proprias. `FEEDING`, `TPA` e `MAINTENANCE` devem bloquear ou permitir automacoes de acordo com a configuracao local definida para a Fase 7.

Integrar o ATO ao gate de modo de forma minima e local: antes de iniciar uma reposicao ou manter uma reposicao automatica, o ATO deve conseguir consultar se a automacao ATO esta permitida. Se o modo atual bloquear ATO e a bomba estiver ligada, o caminho funcional deve solicitar desligamento seguro pelo modulo de reles e convergir para estado seguro sem criar estado novo em `system.ato` e sem alterar `waterLevel`.

O gate de automacao nao deve alterar configuracao persistivel de ATO. Bloquear ATO por modo nao equivale a mudar `ConfigManager::ato().enabled`; e uma inibicao operacional temporaria derivada do modo atual.

Criar fakes ou mocks restritos aos testes para `ModeEffects`, `ModeAutomationGate`, `RelayService` e integracao minima com ATO quando necessario. Os fakes devem permitir validar comandos esperados, bloqueios ativos, liberacao em `NORMAL`, falhas simuladas de relays e ausencia de GPIO direto.

Esta tarefa nao deve atualizar `system.modes`, nao deve emitir eventos `MODE_*`, nao deve salvar modo atual, nao deve registrar scheduler e nao deve implementar NVS, MQTT, iluminacao, Alert Manager, cloud ou aplicativo.

### Arquivos afetados

- `firmware/include/modules/modes/mode_effects.h`
- `firmware/include/modules/modes/mode_automation_gate.h`
- `firmware/include/modules/modes/mode_types.h`
- `firmware/include/modules/modes/mode_config.h`
- `firmware/src/modules/modes/mode_effects.cpp`
- `firmware/src/modules/modes/mode_automation_gate.cpp`
- `firmware/include/modules/ato/ato_service.h`, somente para consultar o gate de automacao quando necessario
- `firmware/src/modules/ato/ato_service.cpp`, somente para consultar o gate de automacao quando necessario
- `firmware/include/modules/relays/relay_service.h`
- `firmware/src/modules/relays/relay_service.cpp`
- `firmware/test/fakes/fake_mode_effects.h`
- `firmware/test/fakes/fake_mode_automation_gate.h`
- `firmware/test/unit/test_mode_effects.cpp`
- `firmware/test/unit/test_ato_service.cpp`, somente para validar ATO bloqueado por modo
- `firmware/test/integration/`

### Criterios de aceitacao

- Existe boundary `ModeEffects` ou equivalente.
- Existe boundary `ModeAutomationGate` ou equivalente.
- Efeitos de modo sao aplicados sem GPIO direto.
- Efeitos de modo sao aplicados sem drivers novos de atuador.
- Efeitos sobre reles passam pelo `RelayService`.
- `FEEDING` desliga Recalque com origem `AUTOMATION`.
- `TPA` desliga equipamentos configurados com origem adequada.
- Desligamentos explicitamente de seguranca usam origem `FAILSAFE`.
- `NORMAL` libera bloqueios de automacao sem ligar equipamentos automaticamente.
- O gate permite consultar se ATO pode atuar.
- O gate bloqueia ATO nos modos configurados.
- O gate libera ATO em `NORMAL` quando a configuracao propria do ATO permitir.
- Bloqueio de ATO por modo nao altera `ConfigManager::ato().enabled`.
- Bloqueio de ATO por modo nao cria estado novo em `system.ato`.
- Bloqueio de ATO por modo nao altera `waterLevel`.
- ATO consulta o gate antes de iniciar reposicao automatica.
- ATO consulta o gate antes de manter reposicao automatica quando aplicavel.
- Se ATO estiver bloqueado por modo e a bomba estiver ligada, ha desligamento seguro por caminho funcional existente.
- Falha simulada de relay e propagada ao chamador sem fingir sucesso.
- Fakes de efeitos e gate ficam restritos a `firmware/test/`.
- O build real do firmware nao depende de fakes ou mocks.
- A tarefa nao atualiza `system.modes`.
- A tarefa nao emite eventos `MODE_*`.
- A tarefa nao salva modo atual.
- A tarefa nao registra scheduler.
- Nenhuma funcionalidade de iluminacao, NVS real, MQTT, Alert Manager, cloud ou aplicativo e implementada.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar `FEEDING` desligando Recalque via fake relay service.
- Testar que `FEEDING` usa origem `AUTOMATION`.
- Testar `TPA` desligando Recalque conforme default local.
- Testar desligamento de seguranca usando origem `FAILSAFE` quando aplicavel.
- Testar que `NORMAL` libera bloqueios sem ligar reles.
- Testar gate permitindo ATO em `NORMAL`.
- Testar gate bloqueando ATO em `FEEDING` quando configurado.
- Testar gate bloqueando ATO em `TPA`.
- Testar gate bloqueando ATO em `MAINTENANCE` quando configurado.
- Testar que ATO nao inicia reposicao quando o gate bloqueia ATO.
- Testar que ATO bloqueado por modo desliga bomba ligada por caminho seguro.
- Testar que bloqueio de ATO nao altera `waterLevel`.
- Testar que bloqueio de ATO nao altera configuracao ATO em memoria.
- Testar falha simulada no relay service durante efeito obrigatorio.
- Confirmar que nenhum teste exige ESP32, rele real, bomba real, sensor real, agua real, NVS real, MQTT, cloud, app ou Serial real.

---

### ID

FW-P07-002

### Titulo

Implementar servico de modos com System State, efeitos, persistencia e eventos locais

### Objetivo

Criar o servico funcional que aplica decisoes de modo, comanda efeitos necessarios via modulos existentes, atualiza `system.modes` como fonte unica de verdade, solicita persistencia do modo atual e emite eventos locais oficiais.

### Dependencias

FW-P07-002A.

FW-P02-002.

### Descricao

Implementar `ModeService`. Para cada comando local de troca ou avaliacao periodica, o servico deve ler `currentSystemState().modes`, configuracao em memoria e tempo atual, executar `ModePolicy` e solicitar ao executor de efeitos da `FW-P07-002A` que aplique os efeitos obrigatorios da decisao.

O servico deve atualizar `modes.currentMode`, `modes.startedAt` e `modes.remainingTime` somente depois que efeitos obrigatorios de entrada forem aplicados com sucesso. Quando um efeito obrigatorio falhar, como falha simulada ao desligar Recalque para `FEEDING`, o servico deve reportar falha e preservar o modo anterior no System State.

O servico nao deve concentrar a logica detalhada de efeitos sobre reles ou ATO. Ele deve depender de boundary substituivel de efeitos para validar, em testes, que uma decisao foi aplicada ou rejeitada. Escrita direta em GPIO, acesso a drivers de rele e chamada direta a diagnosticos continuam proibidos.

O servico deve tratar o bloqueio de automacoes como parte da decisao aplicada pelo executor/gate da `FW-P07-002A`. Ele deve observar o resultado desse boundary para decidir se a transicao pode ser concluida, sem alterar configuracao persistivel de ATO e sem criar estados novos em `system.ato`.

O servico deve emitir `MODE_STARTED` quando um modo diferente de `NORMAL` inicia, `MODE_FINISHED` quando `FEEDING`, `TPA` ou `MAINTENANCE` termina, e `MODE_CHANGED` quando o modo atual muda. Eventos nao devem ser duplicados em avaliacoes repetidas sem transicao.

O servico deve solicitar salvamento do modo atual pelo boundary `ModeStore` quando houver mudanca efetiva de modo. Falha de salvamento deve ser reportada, mas nao deve desfazer uma transicao operacional ja aplicada com sucesso, desde que o System State reflita o modo real em execucao.

Ao retornar de `FEEDING` para `NORMAL` por tempo, o servico deve solicitar liberacao de bloqueios ao executor/gate, atualizar `modes.currentMode = NORMAL`, `remainingTime = 0`, emitir eventos de finalizacao/mudanca e salvar `NORMAL` pelo boundary de persistencia. O retorno nao deve religar Recalque automaticamente; qualquer religamento deve ser comando manual local ou responsabilidade futura explicitamente definida.

O servico nao deve alterar `temperature`, `waterLevel`, `lighting`, `network`, `alerts` ou `systemHealth`. Ele deve interagir com `relays` e `ato` somente por meio do executor de efeitos e do gate de automacoes definidos para a Fase 7.

### Arquivos afetados

- `firmware/include/modules/modes/mode_service.h`
- `firmware/include/modules/modes/mode_events.h`
- `firmware/include/modules/modes/mode_policy.h`
- `firmware/include/modules/modes/mode_effects.h`
- `firmware/include/modules/modes/mode_automation_gate.h`
- `firmware/include/modules/modes/mode_store.h`
- `firmware/include/modules/modes/mode_types.h`
- `firmware/src/modules/modes/mode_service.cpp`
- `firmware/src/modules/modes/mode_events.cpp`
- `firmware/src/modules/modes/mode_policy.cpp`
- `firmware/src/modules/modes/mode_effects.cpp`
- `firmware/src/modules/modes/mode_automation_gate.cpp`
- `firmware/include/core/state/system_state.h`
- `firmware/src/core/state/system_state.cpp`
- `firmware/include/core/events/event_bus.h`
- `firmware/src/core/events/event_bus.cpp`
- `firmware/test/fakes/`
- `firmware/test/unit/test_mode_service.cpp`
- `firmware/test/integration/`

### Criterios de aceitacao

- O servico aceita comando local para entrar em `NORMAL`.
- O servico aceita comando local para entrar em `FEEDING`.
- O servico aceita comando local para entrar em `TPA`.
- O servico aceita comando local para entrar em `MAINTENANCE`.
- O servico rejeita modo invalido sem alterar System State.
- O servico executa avaliacao periodica para retorno automatico quando aplicavel.
- O servico usa `ModePolicy` para decidir transicoes.
- O servico atualiza `system.modes.currentMode`.
- O servico atualiza `system.modes.startedAt`.
- O servico atualiza `system.modes.remainingTime`.
- Falha em efeito obrigatorio preserva o modo anterior.
- Entrada em `FEEDING` solicita efeitos obrigatorios ao executor de efeitos.
- Entrada em `TPA` solicita efeitos obrigatorios ao executor de efeitos.
- Entrada em `MAINTENANCE` solicita suspensao de regras nao criticas ao gate/executor.
- Entrada em `NORMAL` solicita liberacao de bloqueios ao gate/executor.
- Falha retornada pelo executor de efeitos impede conclusao da transicao.
- O servico nao escreve GPIO diretamente.
- O servico nao acessa drivers de rele diretamente.
- O servico nao altera `ConfigManager::ato().enabled`.
- O servico nao cria estado novo em `system.ato`.
- O servico nao altera `waterLevel`.
- `MODE_STARTED` e emitido ao iniciar `FEEDING`, `TPA` ou `MAINTENANCE`.
- `MODE_FINISHED` e emitido ao finalizar `FEEDING`, `TPA` ou `MAINTENANCE`.
- `MODE_CHANGED` e emitido quando o modo muda.
- Eventos de modo nao sao duplicados sem transicao.
- Mudanca efetiva de modo solicita salvamento ao `ModeStore`.
- Comando idempotente nao solicita novo salvamento.
- Falha simulada de salvamento e reportada.
- Falha simulada de salvamento nao corrompe o modo operacional ja aplicado.
- Retorno automatico de `FEEDING` atualiza modo para `NORMAL`.
- Retorno automatico de `FEEDING` nao religa Recalque automaticamente.
- O servico nao implementa iluminacao, NVS real, MQTT, Alert Manager, cloud, app ou historico.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar entrada em `FEEDING` com fake executor de efeitos.
- Testar que `FEEDING` solicita efeitos obrigatorios.
- Testar que falha simulada no executor preserva modo anterior.
- Testar entrada em `TPA` com equipamentos default configurados.
- Testar que `TPA` solicita efeitos obrigatorios.
- Testar que `TPA` solicita bloqueio de automacoes configuradas.
- Testar entrada em `MAINTENANCE`.
- Testar que `MAINTENANCE` solicita bloqueio de automacoes nao criticas.
- Testar entrada em `NORMAL` liberando bloqueios locais.
- Testar que `NORMAL` nao liga reles automaticamente.
- Testar retorno automatico de `FEEDING` para `NORMAL`.
- Testar que retorno de `FEEDING` nao religa Recalque automaticamente.
- Testar `MODE_STARTED`.
- Testar `MODE_FINISHED`.
- Testar `MODE_CHANGED`.
- Testar ausencia de eventos duplicados em avaliacao repetida.
- Testar salvamento do modo atual em transicao efetiva.
- Testar ausencia de salvamento em comando idempotente.
- Testar falha simulada de salvamento.
- Testar que blocos nao relacionados do System State nao sao corrompidos.
- Testar que `waterLevel` nao e alterado pelo servico de modos.
- Testar que eventos de modo nao geram MQTT, alertas centralizados ou historico.
- Confirmar que os testes usam fakes de effects, gate, store, tempo e Event Bus local, sem hardware fisico.

---

### ID

FW-P07-003

### Titulo

Integrar restauracao de modo atual ao boot por boundary substituivel

### Objetivo

Integrar o carregamento do modo atual salvo ao fluxo de inicializacao, validando restauracao com fake de persistencia sem implementar NVS Preferences definitivo.

### Dependencias

FW-P07-002.

FW-P02-003.

FW-P02-007.

### Descricao

Integrar o boundary `ModeStore` ao runtime do core de forma que o modo atual possa ser carregado durante `setup`, depois da inicializacao de System State, Event Bus, Config Manager, Logger, Scheduler, Watchdog, relays, gate de automacoes e executor de efeitos necessarios para aplicar efeitos de modo com seguranca.

Quando o store retornar ausencia de modo salvo, o runtime deve manter `NORMAL` como default seguro. Quando o store retornar modo invalido, o runtime deve rejeitar o valor, manter `NORMAL`, reportar a falha de forma compativel com o logger/core e nao aplicar efeitos de modo invalido.

Quando o store retornar `NORMAL`, o runtime deve atualizar `system.modes` de forma deterministica sem ligar equipamentos automaticamente. Quando retornar `FEEDING`, a restauracao deve tratar o modo temporario de forma segura: se nao houver informacao confiavel de tempo restante persistida nesta fase, o runtime deve cair para `NORMAL` ou finalizar `FEEDING` imediatamente, sem religar equipamentos automaticamente e sem bloquear o boot.

Quando o store retornar `TPA` ou `MAINTENANCE`, a restauracao deve reaplicar os bloqueios e efeitos seguros do modo manual por meio do executor de efeitos e do gate de automacoes, desde que os efeitos obrigatorios consigam ser aplicados. Falha simulada em efeito obrigatorio deve manter ou retornar para `NORMAL` seguro conforme decisao do servico, sem ligar reles indevidamente.

A restauracao desta fase deve ser validada por fake de `ModeStore`. NVS Preferences real, politica de gravacao excessiva, restauracao completa de configuracoes e sobrevivencia fisica ao reboot real ficam reservadas para a Fase 9.

Esta tarefa nao deve inicializar MQTT, Wi-Fi, Alert Manager, iluminacao, cloud, app ou resiliencia integrada.

### Arquivos afetados

- `firmware/src/app/core_app.h`
- `firmware/src/app/core_app.cpp`
- `firmware/src/app/core_app_arduino.cpp`
- `firmware/include/modules/modes/mode_store.h`
- `firmware/include/modules/modes/mode_service.h`
- `firmware/include/modules/modes/mode_effects.h`
- `firmware/include/modules/modes/mode_automation_gate.h`
- `firmware/src/modules/modes/mode_service.cpp`
- `firmware/include/config/config_manager.h`
- `firmware/src/config/config_manager.cpp`
- `firmware/test/fakes/fake_mode_store.h`
- `firmware/test/fakes/fake_mode_effects.h`
- `firmware/test/fakes/fake_mode_automation_gate.h`
- `firmware/test/integration/test_mode_scheduler_integration.cpp`
- `firmware/test/unit/test_mode_service.cpp`

### Criterios de aceitacao

- Runtime possui caminho para carregar modo atual por `ModeStore`.
- Carregamento ocorre depois da inicializacao do core necessario.
- Ausencia de modo salvo mantem `NORMAL`.
- Modo salvo invalido e rejeitado.
- Modo salvo invalido nao aplica efeitos.
- Modo salvo invalido nao liga reles.
- `NORMAL` salvo restaura `system.modes.currentMode = NORMAL`.
- Restauracao de `NORMAL` nao liga equipamentos automaticamente.
- `FEEDING` salvo sem tempo restante confiavel nao deixa o sistema preso em modo temporario.
- Restauracao de `FEEDING` finaliza ou cai para `NORMAL` de forma segura.
- Restauracao de `FEEDING` nao religa Recalque automaticamente.
- `TPA` salvo reaplica bloqueios seguros do modo manual.
- `MAINTENANCE` salvo reaplica suspensao de regras nao criticas.
- Falha em efeito obrigatorio durante restauracao e reportada.
- Falha em efeito obrigatorio durante restauracao nao liga reles indevidamente.
- Restauracao e validada com fake de `ModeStore`.
- Nenhuma NVS Preferences real e implementada nesta tarefa.
- Nenhuma politica de gravacoes em flash e implementada nesta tarefa.
- Nenhuma funcionalidade de MQTT, Wi-Fi, Alert Manager, iluminacao, cloud ou app e inicializada.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar boot com store sem modo salvo.
- Testar boot com store retornando modo invalido.
- Testar boot com `NORMAL` salvo.
- Testar que `NORMAL` salvo nao liga reles.
- Testar boot com `FEEDING` salvo sem tempo restante confiavel.
- Testar que `FEEDING` salvo retorna ou finaliza para estado seguro.
- Testar boot com `TPA` salvo.
- Testar que `TPA` salvo solicita ao executor/gate o bloqueio de ATO e desligamento de Recalque conforme default.
- Testar boot com `MAINTENANCE` salvo.
- Testar que `MAINTENANCE` salvo suspende regras nao criticas.
- Testar falha simulada de efeito obrigatorio durante restauracao.
- Testar que restauracao nao bloqueia `setup`.
- Confirmar que testes usam fake store, fake effects, fake automation gate e fake tempo, sem reboot fisico real.
- Confirmar que nenhum teste exige ESP32, NVS real, reles reais, sensores reais, Wi-Fi, MQTT, cloud ou app.

---

### ID

FW-P07-004

### Titulo

Integrar modos ao runtime com scheduler sem bloqueio

### Objetivo

Registrar o servico de modos no runtime do core para avaliacao periodica local, garantindo retorno automatico de `FEEDING`, ordem segura de inicializacao, loop nao bloqueante, watchdog preservado e validacao sem hardware.

### Dependencias

FW-P07-003.

FW-P02-004.

FW-P02-007.

FW-P05-003.

FW-P06-004.

### Descricao

Integrar o `ModeService` ao `CoreApp` depois da inicializacao de System State, Event Bus, Config Manager, Logger, Scheduler, Watchdog, relays, ATO, gate de automacoes e executor de efeitos. A integracao deve respeitar o boot seguro dos reles antes de aplicar qualquer efeito de modo que possa desligar equipamentos.

Registrar uma tarefa periodica de modos no scheduler para atualizar `remainingTime`, finalizar `FEEDING` quando o tempo vencer e manter bloqueios locais de modo sem bloquear o loop principal. O intervalo deve vir de contrato local do modulo de modos, por exemplo `1000 ms`, sem NVS e sem alterar specs.

Garantir na integracao que o ATO consulta o gate de automacoes antes de iniciar ou manter atuacao automatica. Essa validacao deve usar fake de tempo, fake de gate e fixtures de System State, sem sensor real, bomba real ou agua real.

A tarefa de modos deve chamar uma iteracao curta do `ModeService` e retornar sucesso ou falha de forma compativel com o scheduler. Ela nao deve usar delay real, polling fisico, Serial real, MQTT, Wi-Fi, cloud ou app.

Expor API local por chamada direta para solicitar troca de modo em testes de integracao e futuros modulos locais. Essa API nao deve abrir shell Serial, nao deve implementar MQTT, nao deve aceitar comando remoto e nao deve depender de app.

O Watchdog deve continuar sendo alimentado conforme a Fase 2. Falhas de avaliacao de modo devem ser reportadas sem travar o loop e sem ligar equipamentos automaticamente.

Esta tarefa nao deve inicializar iluminacao da Fase 8, NVS definitivo da Fase 9, rede, MQTT, Alert Manager, cloud, aplicativo ou testes de resiliencia de fases posteriores.

### Arquivos afetados

- `firmware/src/app/core_app.h`
- `firmware/src/app/core_app.cpp`
- `firmware/src/app/core_app_arduino.cpp`
- `firmware/include/modules/modes/mode_service.h`
- `firmware/include/modules/modes/mode_config.h`
- `firmware/include/modules/modes/mode_effects.h`
- `firmware/include/modules/modes/mode_automation_gate.h`
- `firmware/src/modules/modes/mode_service.cpp`
- `firmware/include/core/scheduler/task_scheduler.h`
- `firmware/src/core/scheduler/task_scheduler.cpp`
- `firmware/test/fakes/`
- `firmware/test/unit/`
- `firmware/test/integration/test_mode_scheduler_integration.cpp`

### Criterios de aceitacao

- O runtime cria ou recebe o servico funcional de modos.
- O runtime inicializa modos apos System State.
- O runtime inicializa modos apos Event Bus.
- O runtime inicializa modos apos Config Manager.
- O runtime inicializa modos apos boot seguro dos reles.
- O runtime inicializa modos apos servicos necessarios de ATO quando o guard de ATO for usado.
- O runtime disponibiliza o gate de automacoes usado pelo ATO.
- O runtime registra tarefa periodica de modos.
- O intervalo padrao da tarefa de modos e definido por contrato local.
- A tarefa de modos executa uma iteracao do servico.
- A tarefa de modos nao executa antes do intervalo configurado.
- A tarefa de modos atualiza `remainingTime` de `FEEDING`.
- A tarefa de modos finaliza `FEEDING` quando o tempo vence.
- A tarefa de modos nao finaliza `TPA` automaticamente.
- A tarefa de modos nao finaliza `MAINTENANCE` automaticamente.
- A tarefa de modos nao bloqueia o loop principal.
- ATO consulta o gate de automacoes antes de iniciar atuacao automatica.
- ATO nao inicia reposicao quando o gate bloqueia ATO por modo.
- Existe API local por chamada direta para solicitar modo.
- API local permite solicitar `NORMAL`.
- API local permite solicitar `FEEDING`.
- API local permite solicitar `TPA`.
- API local permite solicitar `MAINTENANCE`.
- API local nao implementa MQTT, app, cloud ou Serial command shell.
- O Watchdog continua sendo alimentado conforme a Fase 2.
- O build real nao depende de fake store, fake effects, fake automation gate ou fake tempo.
- Nenhuma funcionalidade de iluminacao, NVS real, Wi-Fi, MQTT, Alert Manager, cloud ou aplicativo e inicializada.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste de integracao do runtime com fake `ModeStore`, fake effects, fake automation gate e fake tempo.
- Testar que `setup` registra a tarefa de modos.
- Testar que a tarefa de modos nao executa antes do intervalo.
- Testar que a tarefa de modos executa quando o intervalo vence.
- Testar que uma execucao da tarefa chama o servico uma vez.
- Testar que multiplos ciclos respeitam o intervalo configurado.
- Testar API local para entrar em `FEEDING`.
- Testar API local para entrar em `TPA`.
- Testar API local para entrar em `MAINTENANCE`.
- Testar API local para voltar para `NORMAL`.
- Testar que `remainingTime` de `FEEDING` diminui por fake de tempo.
- Testar que `FEEDING` termina automaticamente pelo scheduler.
- Testar que `TPA` permanece ativo apos passagem de tempo.
- Testar que `MAINTENANCE` permanece ativo apos passagem de tempo.
- Testar que ATO nao inicia reposicao quando o gate de automacoes bloqueia ATO por modo.
- Testar que `loopOnce` permanece nao bloqueante.
- Testar que falha simulada no servico e refletida como falha de tarefa quando aplicavel.
- Confirmar que nenhum teste exige ESP32, rele real, bomba real, sensor real, agua real, NVS real, Wi-Fi, MQTT, cloud, app ou Serial real.

---

### ID

FW-P07-005

### Titulo

Consolidar validacao automatizada da Fase 7

### Objetivo

Criar o gate final da Fase 7, garantindo que o sistema de modos esteja integrado, testado por mocks e fakes, limitado ao escopo definido e preparado para validacao fisica futura sem depender dela.

### Dependencias

FW-P07-004.

### Descricao

Consolidar testes unitarios e de integracao da Fase 7 em comandos ou scripts de validacao. O gate deve provar que contratos de modos operam sem hardware, que a politica pura cobre transicoes e retorno automatico de `FEEDING`, que o executor de efeitos aplica comandos via modulo funcional de reles, que o gate de automacoes e consultado pelo ATO, que o servico atualiza `modes`, que o ATO e bloqueado de forma segura quando o modo exigir, que eventos `MODE_*` sao emitidos corretamente, que o boundary de persistencia e validado com fake e que o scheduler executa a avaliacao periodica sem bloquear o loop principal.

Tambem deve haver uma revisao de escopo confirmando que a Fase 7 nao implementou iluminacao da Fase 8, NVS Preferences definitivo da Fase 9, Wi-Fi, MQTT, Alert Manager, cloud, app, historico, notificacoes, comandos remotos, GPIO direto, drivers novos de atuador, comandos por Serial real ou testes de resiliencia de fases posteriores.

Qualquer validacao real com reles, bomba, sensores, aquario, agua, TPA real, alimentacao real, manutencao real, corrente, ruido eletrico, resposta fisica, Serial real ou seguranca de bancada deve ficar marcada como `Pendente para Hardware Validation` e nao pode bloquear a conclusao de desenvolvimento da Fase 7.

### Arquivos afetados

- `firmware/test/unit/`
- `firmware/test/integration/`
- `firmware/test/fakes/`
- `firmware/test/run_phase7_validation.sh`
- `firmware/test/integration/run_phase7_integration_tests.sh`
- `firmware/test/integration/run_phase7_scope_review.sh`
- `firmware/include/modules/modes/`
- `firmware/src/modules/modes/`
- `firmware/include/modules/relays/`
- `firmware/src/modules/relays/`
- `firmware/include/modules/ato/`
- `firmware/src/modules/ato/`
- `firmware/src/app/`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste
- `tasks/firmware-phase-07-mode-system.md`

### Criterios de aceitacao

- Existe comando ou script para executar os testes unitarios da Fase 7.
- Existe comando ou script para executar os testes de integracao da Fase 7.
- Existe revisao automatizada ou objetiva de escopo da Fase 7.
- Todos os testes da Fase 7 usam mocks, fakes ou valores simulados.
- O build real do firmware compila sem depender de mocks ou fakes.
- Contratos de modos sao validados sem hardware.
- Boundary de persistencia de modo e validado com fake.
- Executor de efeitos de modo e validado com fake.
- Gate de automacoes por modo e validado com fake.
- Politica de modos e validada sem hardware.
- Servico de modos e validado sem hardware.
- Integracao do scheduler de modos e validada sem hardware.
- `NORMAL`, `FEEDING`, `TPA` e `MAINTENANCE` sao suportados.
- `system.modes.currentMode` e atualizado conforme a spec.
- `system.modes.startedAt` e validado por teste automatizado.
- `system.modes.remainingTime` e validado por teste automatizado.
- `FEEDING` desliga Recalque por teste automatizado.
- `FEEDING` retorna automaticamente para `NORMAL` por teste automatizado.
- `TPA` desliga equipamentos configurados por teste automatizado.
- `TPA` nao retorna automaticamente nesta fase.
- `MAINTENANCE` suspende regras nao criticas por teste automatizado.
- `MAINTENANCE` nao retorna automaticamente nesta fase.
- `NORMAL` libera bloqueios locais sem ligar equipamentos automaticamente.
- Efeitos sobre reles passam pelo modulo funcional de reles.
- Modos nao escrevem GPIO diretamente.
- ATO e bloqueado ou desligado com seguranca quando o modo exigir.
- ATO consulta o gate de automacoes antes de iniciar reposicao.
- ATO nao religa bomba em ciclo posterior enquanto o modo bloquear ATO.
- Bloqueio de ATO por modo nao cria estado novo de ATO.
- Bloqueio de ATO por modo nao altera `waterLevel`.
- Eventos locais `MODE_CHANGED`, `MODE_STARTED` e `MODE_FINISHED` sao validados por teste automatizado.
- Eventos de modo nao publicam MQTT, nao criam alertas centralizados e nao persistem historico.
- Retorno automatico de `FEEDING` nao religa Recalque automaticamente.
- Restauracao de modo por fake store e validada.
- Nenhum teste fisico e criterio de conclusao da Fase 7.
- Qualquer validacao real dos modos fica marcada como `Pendente para Hardware Validation`.
- Revisao de escopo confirma que iluminacao da Fase 8 nao foi implementada.
- Revisao de escopo confirma que NVS Preferences definitivo da Fase 9 nao foi implementado.
- Revisao de escopo confirma que Wi-Fi e MQTT nao foram implementados.
- Revisao de escopo confirma que Alert Manager nao foi implementado.
- Revisao de escopo confirma que cloud e app nao foram implementados.
- Revisao de escopo confirma que comandos remotos nao foram implementados.
- Revisao de escopo confirma que nenhum GPIO direto foi usado pelo modulo de modos.
- Revisao de escopo confirma que nenhum driver novo de atuador foi criado para modos.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar todos os testes unitarios da Fase 7 com sucesso.
- Executar todos os testes de integracao da Fase 7 com sucesso.
- Executar testes unitarios do executor de efeitos de modo com sucesso.
- Executar testes unitarios do gate de automacoes por modo com sucesso.
- Executar testes existentes da Fase 2 impactados por Event Bus, System State, Config Manager, Scheduler e Watchdog.
- Executar testes existentes da Fase 5 impactados por relays e origem de comando.
- Executar testes existentes da Fase 6 impactados por ATO e bloqueio de automacao.
- Executar script de validacao final da Fase 7.
- Confirmar que o build real nao inclui fakes ou mocks.
- Confirmar que os testes nao exigem ESP32, rele real, bomba real, sensores reais, agua real, Wi-Fi, MQTT, cloud, app, Serial real ou qualquer validacao fisica.
- Confirmar que eventos `MODE_*` existem somente como eventos locais nesta fase.
- Confirmar que o ATO consulta o gate de automacoes por modo antes de acionar a bomba.
- Confirmar que nenhum teste ou implementacao inicializa iluminacao, NVS real, Wi-Fi, MQTT, Alert Manager, cloud ou app.
- Confirmar que nenhum teste ou implementacao implementa comandos remotos.
- Confirmar que nenhum teste ou implementacao altera `architecture/`, `specs/` ou `tasks/firmware-master-plan.md`.
- Confirmar por revisao que funcionalidades de fases posteriores nao foram iniciadas.
