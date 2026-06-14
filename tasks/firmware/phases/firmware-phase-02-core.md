# Fase 2 - Core do Firmware

## Escopo

Implementar a fundacao modular do firmware REEFLOW: System State, Event Bus, Config Manager em memoria, Task Scheduler, Logger, Watchdog e os adapters minimos necessarios para testar o core sem hardware fisico.

Esta fase deve criar a base comum que os modulos funcionais usarao nas fases seguintes. O ESP32 permanece como autoridade soberana do sistema, e o estado global definido em `specs/system-state-spec.md` deve ser a unica fonte de verdade.

Esta fase e puramente de desenvolvimento e nao exige ESP32 conectado, sensores conectados, modulo de reles conectado, luminaria conectada, Wi-Fi, MQTT, cloud, aplicativo, monitor Serial real, watchdog fisico real, multimetro, osciloscopio ou medicoes reais.

Mocks e fakes devem ser usados apenas para testes automatizados ou simulados. Apos os testes, mocks e fakes nao devem permanecer como dependencia do firmware final destinado ao hardware real. O build real do firmware deve usar adapters reais ou implementacoes neutras, nunca mocks de teste.

Nao implementar leitura real de sensores, automacao ATO, controle funcional de reles, modos operacionais, curvas de iluminacao, Wi-Fi, MQTT, cloud, aplicativo, Alert Manager completo, persistencia NVS definitiva ou testes de resiliencia das fases posteriores.

O Config Manager desta fase deve operar com defaults, validacao estrutural e armazenamento em memoria. A persistencia em NVS Preferences fica reservada para a Fase 9.

Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` deve ser alterado.

## Estrutura Recomendada

```text
firmware/
|-- include/
|   |-- contracts/
|   |-- core/
|   |-- config/
|   `-- diagnostics/
|-- src/
|   |-- app/
|   |-- core/
|   |   |-- state/
|   |   |-- events/
|   |   |-- scheduler/
|   |   |-- logging/
|   |   |-- watchdog/
|   |   `-- platform/
|   |-- config/
|   |-- diagnostics/
|   |-- drivers/
|   `-- modules/
`-- test/
    |-- unit/
    |-- integration/
    |-- fakes/
    `-- hardware/
```

`firmware/test/hardware/` fica reservado para validacoes futuras e nao e criterio de conclusao da Fase 2.

## Tarefas

### ID

FW-P02-000

### Titulo

Definir boundaries e adapters mockaveis do core

### Objetivo

Criar as interfaces minimas que isolam o core de dependencias diretas de Arduino, ESP32, Serial, tempo real e watchdog fisico, permitindo testes automatizados sem hardware.

### Dependencias

Fase 1 concluida.

### Descricao

Definir boundaries para tempo, saida de log, runtime da plataforma e backend de watchdog. Criar adapters reais ou neutros para o build de firmware e fakes restritos aos testes automatizados.

Esta tarefa tambem deve preparar a estrutura `src/core/platform/` e `test/fakes/`, sem implementar System State, Event Bus, Config Manager, Scheduler, Logger ou Watchdog completos. `main.cpp` deve continuar como orquestrador minimo e nao deve acumular logica de core.

### Arquivos afetados

- `firmware/include/core/`
- `firmware/src/core/platform/`
- `firmware/src/app/`
- `firmware/src/main.cpp`
- `firmware/test/fakes/`
- `firmware/test/unit/`
- `firmware/test/integration/`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe boundary para fonte de tempo testavel.
- Existe boundary para saida de log substituivel.
- Existe boundary para backend de watchdog substituivel.
- Existe boundary ou adapter para dependencias minimas da plataforma.
- Fakes e mocks ficam restritos a `firmware/test/` ou configuracao de teste.
- O build real do firmware nao depende de fakes ou mocks.
- `main.cpp` permanece como orquestrador minimo.
- Nenhum modulo funcional das fases 3 a 16 e implementado.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste sem hardware confirmando que os fakes podem substituir tempo, log e watchdog.
- Confirmar que o build real nao inclui fakes ou mocks.
- Confirmar que nenhum teste exige ESP32 conectado, Serial real, sensores, reles, luminaria, Wi-Fi ou MQTT.
- Conferir que `main.cpp` nao contem logica interna dos componentes de core.

---

### ID

FW-P02-001

### Titulo

Implementar System State minimo completo

### Objetivo

Representar a estrutura completa do estado global oficial e criar a API unica de leitura e atualizacao controlada do System State.

### Dependencias

FW-P02-000.

### Descricao

Criar os tipos, enums, estruturas, defaults seguros e API central do System State para `system`, `temperature`, `waterLevel`, `lighting`, `relays`, `modes`, `ato`, `network`, `alerts` e `systemHealth`.

A API deve permitir leitura do estado global atual e atualizacoes controladas por bloco, preservando consistencia, origem quando aplicavel, timestamp ou versao de alteracao quando aplicavel. Esta tarefa deve criar somente estado e acesso controlado, sem publicar eventos, MQTT, historico, alertas completos, sensores, reles funcionais, automacoes ou persistencia NVS.

### Arquivos afetados

- `firmware/include/core/`
- `firmware/src/core/state/`
- `firmware/test/unit/`
- `firmware/test/integration/`

### Criterios de aceitacao

- Todos os blocos principais de `specs/system-state-spec.md` estao representados.
- Todos os campos listados em `specs/system-state-spec.md` estao representados.
- Os enums de status, modo, prioridade e origem usam nomes canonicos coerentes com as specs.
- O estado inicial e seguro, deterministico e testavel sem hardware.
- Relays iniciam desligados no estado global.
- Network inicia desconectado no estado global.
- Existe ponto unico para obter o estado global atual.
- Existe API controlada para atualizar blocos do estado.
- Atualizacoes preservam blocos nao relacionados.
- A API nao depende de MQTT, cloud, aplicativo, NVS, sensores ou hardware fisico.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar construcao do estado global default.
- Testar valores iniciais de todos os blocos do estado.
- Testar leitura do estado global atual.
- Testar atualizacao isolada de cada bloco principal.
- Testar que uma atualizacao nao corrompe blocos nao relacionados.
- Testar timestamps, versao ou metadados de alteracao quando existirem.
- Comparar manualmente os modelos com `specs/system-state-spec.md`.

---

### ID

FW-P02-002

### Titulo

Implementar Event Bus integrado ao System State

### Objetivo

Criar o barramento local de eventos e garantir que mudancas relevantes no System State gerem eventos locais derivados do estado global.

### Dependencias

FW-P02-001.

### Descricao

Implementar um Event Bus local com publicacao, assinatura, cancelamento de assinatura e entrega deterministica de eventos. Integrar o System State ao Event Bus para que atualizacoes reais de blocos do estado gerem eventos locais de mudanca.

O fluxo deve seguir o principio oficial: modulo atualiza estado global, estado global gera evento, consumidores reagem ao evento. Nesta fase, consumidores devem ser apenas componentes locais de core ou testes. Eventos especificos de sensores, ATO, relays, modos, iluminacao, MQTT e alertas podem ser reservados apenas como contratos quando necessario, sem disparar automacoes reais.

### Arquivos afetados

- `firmware/include/core/`
- `firmware/src/core/events/`
- `firmware/src/core/state/`
- `firmware/test/unit/`
- `firmware/test/integration/`

### Criterios de aceitacao

- Event Bus permite registrar assinantes por tipo de evento.
- Event Bus permite publicar eventos com payload ou metadados minimos.
- Event Bus permite remover assinantes.
- Event Bus entrega eventos em ordem previsivel.
- Falha de um assinante nao trava permanentemente os demais assinantes.
- Atualizacoes reais do System State emitem evento local de mudanca.
- Atualizacoes sem mudanca efetiva nao geram evento duplicado quando houver comparacao de igualdade.
- O evento identifica a area do estado alterada.
- O Event Bus nao publica MQTT, nao persiste historico e nao cria alertas completos.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar publicacao e recebimento de evento simples.
- Testar multiplos assinantes para o mesmo evento.
- Testar cancelamento de assinatura.
- Testar ordem de entrega.
- Testar comportamento quando nao ha assinantes.
- Testar que atualizar um bloco do estado gera evento esperado.
- Testar que um assinante consegue ler o estado atualizado durante ou apos o evento.
- Confirmar que nenhum teste exige hardware fisico.

---

### ID

FW-P02-003

### Titulo

Implementar Config Manager em memoria

### Objetivo

Criar gerenciamento central de configuracoes com defaults, schemas e validacao estrutural, sem persistencia NVS definitiva e sem regras funcionais profundas dos modulos futuros.

### Dependencias

FW-P02-002.

### Descricao

Implementar um Config Manager em memoria para os dominios previstos nas specs: temperatura, ATO, iluminacao, timers, modo atual, Wi-Fi, MQTT e calibracoes.

O componente deve carregar defaults determinicos, validar estrutura, tipo, presenca e faixas basicas, permitir leitura e atualizacao controlada e emitir evento local de configuracao alterada. Esta tarefa nao deve implementar `Preferences`, gravacao em NVS, restauracao real pos-reboot, Wi-Fi, MQTT ou aplicacao das configuracoes em modulos funcionais.

Validacoes de dominio profundo, como regras de ATO, curvas reais de iluminacao, comportamento de modos, reconexao MQTT ou restauracao apos reboot, pertencem as fases posteriores.

### Arquivos afetados

- `firmware/include/config/`
- `firmware/src/config/`
- `firmware/include/core/`
- `firmware/src/core/events/`
- `firmware/test/unit/`
- `firmware/test/integration/`

### Criterios de aceitacao

- Config Manager possui defaults determinicos para configuracoes previstas nas specs.
- Config Manager permite leitura centralizada por dominio.
- Config Manager valida estrutura, tipo, presenca e faixas basicas antes de aceitar alteracoes.
- Alteracoes aceitas emitem evento local de configuracao quando aplicavel.
- Alteracoes invalidas sao rejeitadas sem corromper a configuracao atual.
- Nenhuma gravacao em NVS Preferences e implementada.
- Nenhuma restauracao real pos-reboot e implementada.
- Nenhum modulo funcional aplica essas configuracoes nesta fase.
- Config Manager nao depende de Wi-Fi, MQTT, cloud, aplicativo, sensores, relays ou hardware fisico.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar carregamento de defaults.
- Testar leitura de configuracoes por dominio.
- Testar atualizacao valida.
- Testar rejeicao de atualizacao invalida.
- Testar preservacao da configuracao anterior apos rejeicao.
- Testar emissao de evento local quando configuracao aceita mudar.
- Confirmar que nao ha dependencia de NVS Preferences no teste desta fase.
- Confirmar que nenhum teste exige hardware fisico.

---

### ID

FW-P02-004

### Titulo

Implementar Task Scheduler cooperativo

### Objetivo

Criar um agendador local para executar tarefas periodicas e recorrentes do firmware sem bloquear o loop principal e sem depender de tempo real durante testes.

### Dependencias

FW-P02-000, FW-P02-002.

### Descricao

Implementar um Task Scheduler cooperativo com registro de tarefas, intervalo de execucao, habilitacao, desabilitacao e execucao por clock injetavel.

O scheduler deve suportar ciclos futuros de sensores, heartbeat, watchdog e automacoes, mas nesta fase deve executar apenas tarefas de core ou tarefas falsas de teste. Nao implementar leitura periodica real de DS18B20, VL6180X, MQTT heartbeat, ATO automatico, modos ou iluminacao.

### Arquivos afetados

- `firmware/include/core/`
- `firmware/src/core/scheduler/`
- `firmware/src/core/platform/`
- `firmware/test/fakes/`
- `firmware/test/unit/`
- `firmware/test/integration/`

### Criterios de aceitacao

- Scheduler permite registrar tarefas com nome ou identificador.
- Scheduler executa tarefas vencidas sem bloquear o loop principal.
- Scheduler permite habilitar e desabilitar tarefas.
- Scheduler usa fonte de tempo testavel.
- Scheduler registra ou emite evento local quando tarefa falha, se houver suporte a falhas.
- Nenhuma rotina funcional de fases futuras e agendada nesta fase.
- O build real nao depende do clock fake usado em testes.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar registro de tarefa periodica.
- Testar que tarefa executa quando o intervalo vence usando clock fake.
- Testar que tarefa nao executa antes do intervalo.
- Testar desabilitacao e reabilitacao de tarefa.
- Testar multiplas tarefas com intervalos diferentes.
- Confirmar que o build real nao inclui clock fake.
- Confirmar que nenhum teste exige hardware fisico.

---

### ID

FW-P02-005

### Titulo

Implementar Logger local do firmware

### Objetivo

Criar um logger central para mensagens de diagnostico e operacao local do firmware, com saida substituivel em testes e sem dependencias de rede ou persistencia.

### Dependencias

FW-P02-000, FW-P02-002.

### Descricao

Implementar um Logger com niveis, origem da mensagem e sink de saida substituivel. O logger deve suportar sink real ou neutro no build de firmware e fake de saida para testes.

O logger pode consumir eventos locais do core para registrar inicializacao, mudancas de estado, mudancas de configuracao e falhas do scheduler. Ele nao deve publicar logs em MQTT, cloud, banco de dados ou aplicativo. Logs tambem nao substituem eventos oficiais do Event Bus.

### Arquivos afetados

- `firmware/include/core/`
- `firmware/src/core/logging/`
- `firmware/src/core/platform/`
- `firmware/test/fakes/`
- `firmware/test/unit/`
- `firmware/test/integration/`

### Criterios de aceitacao

- Logger possui niveis minimos como DEBUG, INFO, WARNING e ERROR.
- Logger permite identificar origem ou componente da mensagem.
- Logger possui sink de saida substituivel em testes.
- Logger pode ser usado pelos componentes de core sem criar dependencia circular.
- Logger nao depende de MQTT, Wi-Fi, cloud, app, banco de dados, NVS ou hardware fisico.
- Mensagens de log nao substituem eventos oficiais do Event Bus.
- O build real nao depende do sink fake usado em testes.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar emissao de mensagem por nivel.
- Testar filtragem de nivel quando configurada.
- Testar saida por sink fake.
- Testar uso do logger a partir de ao menos um componente de core.
- Testar ausencia de dependencia circular entre Logger, Event Bus e System State.
- Confirmar que o build real nao inclui sink fake.
- Confirmar que nenhum teste exige hardware fisico.

---

### ID

FW-P02-006

### Titulo

Implementar Watchdog service do core

### Objetivo

Criar o servico local de watchdog com backend substituivel e alimentacao controlada, refletindo informacoes basicas em `systemHealth` sem antecipar testes de resiliencia.

### Dependencias

FW-P02-001, FW-P02-004, FW-P02-005.

### Descricao

Implementar um Watchdog service com inicializacao, alimentacao periodica, backend real ou neutro para o build de firmware e backend fake para testes. O componente deve atualizar ou preparar atualizacao de campos de `systemHealth`, como `watchdogTriggered`, `lastReboot` e `rebootReason`, conforme informacoes disponiveis.

Esta tarefa nao deve implementar testes de resiliencia completos, recuperacao apos reboot inesperado, recuperacao Wi-Fi, recuperacao MQTT, sensor offline, ATO travado ou fail-safe de modulos funcionais.

### Arquivos afetados

- `firmware/include/core/`
- `firmware/src/core/watchdog/`
- `firmware/src/core/platform/`
- `firmware/src/core/state/`
- `firmware/test/fakes/`
- `firmware/test/unit/`
- `firmware/test/integration/`

### Criterios de aceitacao

- Watchdog possui API de inicializacao e alimentacao.
- Watchdog pode ser alimentado por scheduler ou aplicacao de core.
- Existe backend real ou neutro para o build de firmware.
- Existe backend fake restrito aos testes automatizados.
- `systemHealth` recebe informacoes basicas de saude relacionadas ao watchdog quando possivel.
- Falhas simuladas podem gerar log ou evento local quando aplicavel.
- Nenhuma rotina de resiliencia das fases posteriores e implementada.
- O build real nao depende do backend fake usado em testes.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar inicializacao do watchdog com backend fake.
- Testar alimentacao periodica via scheduler ou chamada direta.
- Testar atualizacao esperada de `systemHealth` em cenario simulado.
- Testar que falhas simuladas geram log ou evento local quando aplicavel.
- Confirmar que o build real nao inclui backend fake.
- Confirmar que nenhum teste exige hardware fisico ou watchdog fisico real.

---

### ID

FW-P02-007

### Titulo

Integrar runtime do core e validar contratos

### Objetivo

Inicializar System State, Event Bus, Config Manager, Task Scheduler, Logger e Watchdog em ordem deterministica dentro da aplicacao de core e validar a arquitetura final da Fase 2 sem hardware fisico.

### Dependencias

FW-P02-003, FW-P02-006.

### Descricao

Conectar os componentes implementados na Fase 2 dentro do shell de aplicacao. A ordem de inicializacao deve garantir que Logger esteja disponivel para diagnosticos, System State esteja inicializado antes dos consumidores, Event Bus esteja pronto para eventos locais, Config Manager carregue defaults em memoria, Scheduler execute tarefas do core e Watchdog seja alimentado de forma controlada.

Esta tarefa tambem deve atuar como gate final de contrato da Fase 2, consolidando testes unitarios e de integracao ja criados nas tarefas anteriores. O gate deve confirmar que a base modular funciona sem ESP32 conectado e que nenhuma funcionalidade das fases futuras foi iniciada.

### Arquivos afetados

- `firmware/src/app/`
- `firmware/src/core/`
- `firmware/src/config/`
- `firmware/src/main.cpp`
- `firmware/test/unit/`
- `firmware/test/integration/`
- `firmware/test/fakes/`

### Criterios de aceitacao

- Todos os componentes da Fase 2 sao inicializados em ordem clara.
- O loop principal da aplicacao executa scheduler e manutencao do core.
- Logger registra inicializacao basica do core.
- Watchdog e integrado sem bloquear testes automatizados.
- System State permanece a fonte unica de verdade.
- Event Bus recebe eventos derivados de mudancas reais do estado.
- Config Manager opera apenas em memoria.
- Cada task da Fase 2 possui cobertura automatizada propria.
- Existe ao menos um teste de integracao da aplicacao de core.
- O build real do firmware nao depende de fakes ou mocks.
- Nenhum sensor, rele funcional, ATO, modo, iluminacao, Wi-Fi, MQTT, cloud, app, NVS ou resiliencia futura e inicializado.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar todos os testes unitarios da Fase 2 com sucesso.
- Executar teste de integracao da aplicacao de core com sucesso.
- Executar build PlatformIO com sucesso.
- Testar inicializacao completa da aplicacao de core com fakes quando necessario.
- Testar uma iteracao de loop sem hardware conectado.
- Testar que eventos de estado e configuracao percorrem Event Bus durante integracao.
- Verificar que o scheduler executa apenas tarefas de core nesta fase.
- Confirmar por revisao que nenhum teste depende de hardware real.
- Confirmar por revisao que nenhum teste implementa comportamento funcional das fases 3 a 16.
