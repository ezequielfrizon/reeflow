# Fase 5 - Sistema de Reles

## Escopo

Implementar o modulo funcional de controle dos quatro reles oficiais do REEFLOW: Recalque no GPIO16, Aquecedor no GPIO17, ATO no GPIO18 e Reserva no GPIO19. A fase deve fornecer controle manual local pelo firmware, estado seguro no boot, atualizacao do bloco `relays` do System State, registro da origem da mudanca e eventos locais `RELAY_ON` e `RELAY_OFF`.

Esta fase e puramente de desenvolvimento e nao exige ESP32 conectado, modulo de rele conectado, carga AC/DC real, bomba de recalque, aquecedor, bomba ATO, multimetro, osciloscopio, monitor Serial real, Wi-Fi, MQTT, cloud ou aplicativo.

Mocks e fakes devem ser usados para validacoes automatizadas das implementacoes, seguindo o padrao das fases anteriores. Mocks e fakes devem ficar restritos a `firmware/test/` ou a configuracoes de teste, sem se tornarem dependencia do build real do firmware destinado ao hardware.

O firmware real deve ficar preparado para usar o modulo de rele fisico futuramente, mas a conclusao desta fase nao pode depender de teste fisico. Qualquer verificacao com rele real, carga real, tomada, bomba, aquecedor, corrente, ruido eletrico, resposta fisica, Serial real ou medicao eletrica deve ficar registrada como `Pendente para Hardware Validation` e fora dos criterios de conclusao desta fase.

O contrato eletrico oficial dos reles nesta fase e `ACTIVE_HIGH`: `OFF = LOW` e `ON = HIGH`. O contrato deve vir de `firmware/include/contracts/relay_contract.h` e o pinout deve vir de `firmware/include/contracts/hardware_pins.h`, sem duplicar numeros de GPIO em modulos funcionais.

Na inicializacao do runtime da Fase 5, todos os reles devem ser comandados fisicamente para OFF antes de qualquer comando manual local. A excecao futura "quando um modo exigir restauracao" pertence as Fases 7 e 9 e nao deve ser implementada nesta fase. O boot seguro nao deve emitir `RELAY_OFF` quando o System State ja estiver no estado default OFF.

O controle manual desta fase e local ao firmware. O runtime da Fase 5 deve usar somente `LOCAL` para comandos manuais locais e `FAILSAFE` para desligamentos seguros locais quando aplicavel. Os valores `MQTT`, `APP` e `AUTOMATION` podem permanecer como valores canonicos ja previstos no System State, mas seus transportes, comandos remotos e automacoes nao devem ser implementados nesta fase.

Comandos idempotentes devem ter comportamento fechado: se o rele alvo ja estiver no estado desejado, o servico nao deve atualizar `lastChanged`, nao deve alterar `source` e nao deve emitir `RELAY_ON` ou `RELAY_OFF`.

O Rele 3 deve ser controlavel manualmente como rele ATO, mas nao deve executar automacao de reposicao, nao deve ler `waterLevel`, nao deve alterar o bloco `ato`, nao deve gerar eventos `ATO_*`, nao deve aplicar timeout de reposicao e nao deve aplicar cooldown. Essas responsabilidades pertencem a Fase 6.

Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` deve ser alterado.

## Estrutura Recomendada

```text
firmware/
|-- include/
|   |-- modules/
|   |   `-- relays/
|   |       |-- relay_types.h
|   |       |-- relay_controller.h
|   |       |-- relay_service.h
|   |       `-- relay_events.h
|   |-- core/
|   |-- config/
|   `-- contracts/
|-- src/
|   |-- modules/
|   |   `-- relays/
|   |       |-- relay_service.cpp
|   |       `-- relay_events.cpp
|   |-- drivers/
|   |   `-- relays/
|   |       |-- gpio_relay_controller.h
|   |       `-- gpio_relay_controller.cpp
|   |-- app/
|   `-- core/
`-- test/
    |-- fakes/
    |   |-- fake_gpio_port.h
    |   `-- fake_relay_controller.h
    |-- unit/
    `-- integration/
```

`firmware/test/hardware/` fica reservado para validacoes futuras e nao e criterio de conclusao da Fase 5.

O modulo funcional em `modules/relays` nao deve depender de tipos, resultados ou rotinas de diagnostico da Fase 1. Diagnosticos de bring-up e modulo funcional podem reutilizar contratos de pinout, contrato eletrico e boundary de GPIO quando adequado, mas nao devem compartilhar enums de diagnostico como contrato funcional.

O boundary GPIO atual pode ser usado para registrar modo e escrita de pino com fakes de teste. Como o boundary GPIO existente nao retorna status de escrita, a Fase 5 nao deve inventar falha fisica de GPIO no controlador real. Falhas de comando devem ser representadas no boundary funcional de reles e validadas com fake de controlador nos testes do servico.

## Tarefas

### ID

FW-P05-000

### Titulo

Definir contratos funcionais e fakes do modulo de reles

### Objetivo

Criar os contratos minimos do modulo funcional de reles, incluindo tipos canonicos, identificadores dos quatro reles, comandos ON/OFF, origem da mudanca, resultado de comando e interface substituivel de controle, permitindo testes automatizados sem hardware e sem acoplamento aos diagnosticos da Fase 1.

### Dependencias

Fase 1 concluida.

Fase 2 concluida.

### Descricao

Definir os identificadores funcionais `RECALQUE`, `HEATER`, `ATO_PUMP` e `RESERVE`, mapeados aos campos `relays.recalque`, `relays.heater`, `relays.atoPump` e `relays.reserve` do System State.

Definir um contrato funcional de comando de rele contendo rele alvo, estado desejado (`ON` ou `OFF`) e origem da mudanca. A origem deve ser compativel com `RelaySource` do System State. Nesta fase, apenas `LOCAL` e `FAILSAFE` devem ser usados pelo runtime.

Definir uma interface funcional de controlador de reles que permita aplicar ON/OFF em um rele individual e aplicar OFF em todos os reles. A interface deve retornar resultado proprio do dominio, no minimo: sucesso, rele desconhecido, falha do controlador e contrato invalido. Esse resultado pertence ao boundary funcional de reles e nao deve expor Arduino, `digitalWrite`, Serial real ou diagnosticos de bring-up.

Criar fake ou mock de controlador de reles restrito aos testes automatizados. O fake deve permitir simular escrita bem sucedida, falha em rele especifico, sequencias de comandos, chamada de `allOff`, estado inicial e leitura dos comandos recebidos para validacao.

Esta tarefa nao deve implementar driver GPIO real, nao deve atualizar System State, nao deve emitir eventos e nao deve modificar rotinas de diagnostico da Fase 1.

### Arquivos afetados

- `firmware/include/modules/relays/relay_types.h`
- `firmware/include/modules/relays/relay_controller.h`
- `firmware/include/modules/relays/relay_service.h`, somente se necessario para declarar o contrato de comando
- `firmware/test/fakes/fake_relay_controller.h`
- `firmware/test/unit/`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existem identificadores funcionais para Recalque, Aquecedor, ATO e Reserva.
- Cada identificador funcional possui mapeamento claro para o campo correspondente em `system.relays`.
- Existe contrato de comando com rele alvo, estado desejado e origem da mudanca.
- O contrato permite `ON` e `OFF`.
- O runtime desta fase fica limitado a origens `LOCAL` e `FAILSAFE`.
- `MQTT`, `APP` e `AUTOMATION` nao sao implementados como entrada de comando nesta fase.
- Existe uma interface de controlador de reles substituivel em testes.
- A interface permite ligar um rele individual.
- A interface permite desligar um rele individual.
- A interface permite desligar todos os reles.
- A interface representa sucesso e falha do controlador.
- A interface representa rele desconhecido ou invalido.
- A interface nao expoe tipos de diagnostico da Fase 1.
- A interface nao depende de Arduino, Serial real, Wi-Fi, MQTT, cloud, aplicativo, NVS, ATO ou modos.
- O fake ou mock permite controlar resultados e inspecionar comandos recebidos em testes sem hardware.
- O build real do firmware nao depende de fakes ou mocks.
- Nenhum comportamento funcional de acionamento e iniciado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario confirmando os quatro identificadores funcionais.
- Executar teste unitario confirmando mapeamento dos identificadores para nomes/campos esperados.
- Executar teste unitario confirmando que o fake registra comando ON.
- Executar teste unitario confirmando que o fake registra comando OFF.
- Executar teste unitario confirmando que o fake registra `allOff`.
- Executar teste unitario confirmando falha simulada em rele especifico.
- Executar teste unitario confirmando rejeicao de rele invalido.
- Executar teste unitario confirmando que comandos remotos nao sao expostos como entrada implementada nesta fase.
- Confirmar que nenhum teste exige ESP32 conectado, modulo de rele conectado, carga real ou Serial real.
- Confirmar que o build real nao inclui o fake ou mock de teste.

---

### ID

FW-P05-001

### Titulo

Implementar controlador GPIO real dos reles com contrato ACTIVE_HIGH

### Objetivo

Criar a implementacao real do controlador funcional de reles usando o boundary GPIO existente, o pinout oficial e o contrato eletrico `ACTIVE_HIGH`, preparada para hardware futuro e validavel por fake GPIO sem modulo de rele conectado.

### Dependencias

FW-P05-000.

FW-P01-005B.

### Descricao

Implementar o controlador real dos reles em `drivers/relays/`, separado do servico funcional. O controlador deve implementar a interface definida em `FW-P05-000` e usar `GpioPort` ou boundary equivalente para configurar GPIOs como saida e escrever niveis logicos.

O controlador deve buscar os GPIOs oficiais em `hardware_pins.h`: Rele 1 Recalque no GPIO16, Rele 2 Aquecedor no GPIO17, Rele 3 ATO no GPIO18 e Rele 4 Reserva no GPIO19. Numeros de GPIO nao devem ser duplicados em modulo funcional.

O controlador deve aplicar o contrato `ACTIVE_HIGH` de `relay_contract.h`: comando OFF escreve nivel fisico LOW e comando ON escreve nivel fisico HIGH. Se o contrato mudar futuramente, a traducao deve continuar centralizada no contrato eletrico, nao espalhada pelo modulo.

A operacao `allOff` deve configurar todos os pinos de rele como saida quando necessario e escrever OFF em todos os reles. A ordem deve ser deterministica e testavel.

Como o boundary GPIO atual nao retorna status, esta tarefa nao deve exigir propagacao de falha de escrita fisica do `GpioPort`. Falhas de controlador devem ser simuladas e validadas no fake do boundary funcional de reles usado pelos testes do servico.

O controlador pode reutilizar boundary de GPIO e contratos da Fase 1, mas nao deve acoplar o modulo funcional aos diagnosticos de bring-up. Esta tarefa nao deve atualizar System State, nao deve emitir eventos, nao deve acionar ATO automatico e nao deve implementar comandos remotos.

### Arquivos afetados

- `firmware/src/drivers/relays/gpio_relay_controller.h`
- `firmware/src/drivers/relays/gpio_relay_controller.cpp`
- `firmware/src/drivers/io/gpio_interface.h`, somente se ajuste ja previsto for inevitavel
- `firmware/src/drivers/io/arduino_hardware_io.h`
- `firmware/src/drivers/io/arduino_hardware_io.cpp`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/contracts/relay_contract.h`
- `firmware/include/modules/relays/relay_controller.h`
- `firmware/include/modules/relays/relay_types.h`
- `firmware/test/fakes/fake_gpio_port.h`
- `firmware/test/unit/`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe implementacao real do controlador funcional de reles.
- O controlador real implementa a interface definida em `FW-P05-000`.
- O controlador real usa boundary GPIO substituivel em testes.
- O controlador real configura GPIOs de rele como saida quando necessario.
- Recalque usa GPIO16 vindo do contrato oficial.
- Aquecedor usa GPIO17 vindo do contrato oficial.
- ATO usa GPIO18 vindo do contrato oficial.
- Reserva usa GPIO19 vindo do contrato oficial.
- O controlador traduz OFF para LOW pelo contrato `ACTIVE_HIGH`.
- O controlador traduz ON para HIGH pelo contrato `ACTIVE_HIGH`.
- `allOff` aplica OFF nos quatro reles em ordem deterministica.
- O controlador nao exige alteracao do boundary GPIO apenas para simular falha fisica.
- O controlador nao expoe enums ou tipos de diagnostico da Fase 1 como contrato funcional.
- O firmware compila sem exigir modulo de rele fisicamente conectado.
- A tarefa nao atualiza System State, nao publica eventos e nao inicializa funcionalidades futuras.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario do controlador usando fake GPIO.
- Testar que Recalque escreve no GPIO16.
- Testar que Aquecedor escreve no GPIO17.
- Testar que ATO escreve no GPIO18.
- Testar que Reserva escreve no GPIO19.
- Testar que comando ON escreve nivel HIGH com contrato `ACTIVE_HIGH`.
- Testar que comando OFF escreve nivel LOW com contrato `ACTIVE_HIGH`.
- Testar que `allOff` escreve LOW nos quatro GPIOs.
- Testar rejeicao de rele invalido.
- Confirmar que falha de controlador e coberta nos testes do servico por fake de `RelayController`, nao por falha fisica inventada no `GpioPort`.
- Confirmar que o teste nao depende de rele real, carga real, Serial real ou ESP32 conectado.
- Confirmar que mocks usados no teste nao permanecem como dependencia do build real.

---

### ID

FW-P05-002

### Titulo

Implementar servico de reles com System State e eventos locais

### Objetivo

Criar o servico funcional que aplica comandos manuais locais, aciona o controlador de rele, atualiza o bloco `relays` como fonte unica de verdade, registra `lastChanged` e `source`, e emite eventos locais oficiais.

### Dependencias

FW-P05-001.

FW-P02-001.

FW-P02-002.

### Descricao

Implementar o servico do modulo de reles. Para cada comando manual local, o servico deve validar o rele alvo, consultar o estado atual de `system.relays`, decidir se ha mudanca efetiva, solicitar a escrita ao controlador funcional quando necessario, atualizar o System State somente apos sucesso da escrita e emitir `RELAY_ON` ou `RELAY_OFF` quando houver mudanca efetiva.

Ao ligar um rele, o campo `enabled` do rele alvo deve virar `true`, `lastChanged` deve receber o instante do comando aplicado com sucesso e `source` deve receber a origem informada. Ao desligar, `enabled` deve virar `false` com a mesma regra de `lastChanged` e `source`.

Comandos idempotentes devem ter comportamento deterministico: se `enabled` ja estiver no estado desejado, o servico nao deve chamar o controlador, nao deve atualizar `lastChanged`, nao deve alterar `source` e nao deve emitir evento local. Essa regra tambem vale quando a origem informada for diferente, porque nao houve mudanca efetiva de estado do rele.

O servico deve usar `system.relays` como fonte unica de verdade para o estado exposto do modulo. Falha do controlador funcional nao deve marcar o rele como ligado ou desligado no System State se o comando nao foi aceito pelo boundary funcional.

O servico deve registrar `lastChanged` usando a fonte de tempo da Fase 2. Nesta fase, comandos manuais locais devem usar `LOCAL` e desligamentos seguros locais devem usar `FAILSAFE`. Valores `MQTT`, `APP` e `AUTOMATION` podem existir no enum do estado para compatibilidade futura, mas seus transportes e automacoes nao devem ser implementados.

Implementar uma operacao funcional `allOff` para desligar os quatro reles, usada por boot seguro e por futuros fail-safes locais. A operacao deve solicitar OFF fisico ao controlador para os quatro reles. No System State, ela deve atualizar e emitir `RELAY_OFF` apenas para reles que estavam efetivamente ligados. Quando todos os reles ja estiverem OFF no estado default, `allOff` nao deve emitir eventos.

Declarar os contratos locais de evento `RELAY_ON` e `RELAY_OFF` no modulo de reles e, se necessario, adicionar os tipos equivalentes ao Event Bus. Os eventos devem ser locais e servir como materia-prima futura para MQTT, historico, alertas, ATO ou modos, sem implementar essas funcionalidades nesta fase.

Esta tarefa nao deve implementar MQTT, app, comandos remotos, ATO automatico, timeout de ATO, cooldown, modos operacionais, persistencia NVS, Alert Manager, cloud ou historico. O controle manual do Rele 3 nao deve alterar `ato.status`, `ato.pumpRunning`, `ato.lastActivation`, `ato.lastCompletion` ou `ato.timeoutCounter`.

### Arquivos afetados

- `firmware/include/modules/relays/relay_service.h`
- `firmware/include/modules/relays/relay_events.h`
- `firmware/include/modules/relays/relay_types.h`
- `firmware/src/modules/relays/relay_service.cpp`
- `firmware/src/modules/relays/relay_events.cpp`
- `firmware/include/core/state/system_state.h`
- `firmware/src/core/state/system_state.cpp`
- `firmware/include/core/events/event_bus.h`
- `firmware/src/core/events/event_bus.cpp`
- `firmware/test/fakes/`
- `firmware/test/unit/`
- `firmware/test/integration/`

### Criterios de aceitacao

- O servico aceita comando manual local para ligar rele individual.
- O servico aceita comando manual local para desligar rele individual.
- O servico rejeita rele invalido sem alterar System State.
- O servico consulta o estado atual antes de chamar o controlador.
- Comando idempotente nao chama o controlador.
- Comando idempotente nao atualiza `lastChanged`.
- Comando idempotente nao altera `source`, mesmo quando a origem informada for diferente.
- Comando idempotente nao emite `RELAY_ON` ou `RELAY_OFF`.
- O servico chama o controlador funcional antes de atualizar o estado quando ha mudanca efetiva.
- Falha do controlador nao altera `system.relays`.
- Sucesso ao ligar atualiza `enabled = true` no rele alvo.
- Sucesso ao desligar atualiza `enabled = false` no rele alvo.
- Sucesso atualiza `lastChanged` com a fonte de tempo da Fase 2.
- Sucesso atualiza `source` com a origem informada.
- `RELAY_ON` e emitido quando um rele muda para ligado.
- `RELAY_OFF` e emitido quando um rele muda para desligado.
- A operacao `allOff` solicita OFF fisico para os quatro reles.
- A operacao `allOff` nao emite eventos quando todos os reles ja estiverem OFF.
- A operacao `allOff` atualiza e emite `RELAY_OFF` apenas para reles previamente ligados.
- O System State permanece a fonte unica de verdade do modulo.
- O servico nao altera blocos `temperature`, `waterLevel`, `lighting`, `modes`, `ato`, `network`, `alerts` ou `systemHealth`.
- O servico nao cria eventos `ATO_*`, `MODE_*`, `MQTT_*` ou alertas centralizados.
- Eventos de rele nao publicam MQTT, nao criam alertas centralizados e nao persistem historico.
- Eventos de rele nao criam eventos `ATO_*`, `MODE_*` ou comandos de iluminacao.
- O servico nao depende de hardware fisico para ser testado.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar com fake controller comando ON e confirmar atualizacao de `system.relays`.
- Testar com fake controller comando OFF e confirmar atualizacao de `system.relays`.
- Testar que apenas o rele alvo muda.
- Testar que falha simulada do controller preserva o estado anterior.
- Testar que rele invalido e rejeitado sem alterar estado.
- Testar `lastChanged` com fake time source.
- Testar `source = LOCAL` em comando manual local.
- Testar `source = FAILSAFE` em desligamento seguro local quando houver mudanca efetiva.
- Testar que `MQTT`, `APP` e `AUTOMATION` nao sao usados pelo runtime desta fase.
- Testar emissao de `RELAY_ON`.
- Testar emissao de `RELAY_OFF`.
- Testar comando idempotente sem chamada ao controlador.
- Testar comando idempotente sem atualizacao de `lastChanged`.
- Testar comando idempotente sem alteracao de `source`.
- Testar comando idempotente sem evento duplicado.
- Testar que origem diferente com mesmo estado desejado continua idempotente e nao altera `source`.
- Testar `allOff` com todos os reles ligados.
- Testar `allOff` com alguns reles ja desligados.
- Testar `allOff` com todos os reles ja desligados e confirmar ausencia de eventos `RELAY_OFF`.
- Testar que o bloco `ato` nao e alterado ao controlar manualmente o Rele 3.
- Testar que blocos nao relacionados do System State nao sao corrompidos.
- Confirmar que os testes usam fake controller, fake tempo ou Event Bus local, sem hardware fisico.

---

### ID

FW-P05-003

### Titulo

Integrar reles ao runtime com boot seguro e API manual local

### Objetivo

Integrar o servico de reles ao runtime do core, garantindo que todos os reles iniciem em OFF, que comandos manuais locais possam ser aplicados por chamada explicita e que o loop principal continue sem bloqueio.

### Dependencias

FW-P05-002.

FW-P02-004.

FW-P02-007.

### Descricao

Registrar o modulo de reles no runtime do core depois da inicializacao de System State, Event Bus, Config Manager, Logger, Scheduler e Watchdog, conforme a fundacao da Fase 2.

Durante `setup`, o runtime deve aplicar `allOff` antes de aceitar qualquer comando manual local. O estado inicial apos boot deve refletir os quatro reles desligados com `enabled = false`. Como o estado default ja e OFF, o boot seguro deve escrever OFF fisico nos quatro reles, mas nao deve emitir eventos `RELAY_OFF` por mudanca inexistente de estado. Restauracao de estado por modo ou persistencia nao deve ser implementada nesta fase.

Expor uma API local simples para que testes de integracao e futuros modulos possam solicitar comando manual de rele por chamada direta ao firmware. Essa API nao deve abrir Serial command shell novo, nao deve implementar MQTT, nao deve depender de app e nao deve iniciar automacoes.

A integracao nao deve registrar tarefa periodica para os reles. O modulo de reles nesta fase deve ser acionado por comando explicito e por boot seguro. O loop principal nao deve bloquear por causa do modulo.

O Watchdog deve continuar funcionando conforme a Fase 2. Falhas de inicializacao do controlador de reles devem ser reportadas de forma compativel com o logger/core sem ligar reles indevidamente.

### Arquivos afetados

- `firmware/src/app/core_app.h`
- `firmware/src/app/core_app.cpp`
- `firmware/src/app/core_app_arduino.cpp`
- `firmware/include/modules/relays/relay_service.h`
- `firmware/src/modules/relays/relay_service.cpp`
- `firmware/src/drivers/relays/gpio_relay_controller.h`
- `firmware/src/drivers/relays/gpio_relay_controller.cpp`
- `firmware/src/drivers/io/arduino_hardware_io.h`
- `firmware/src/drivers/io/arduino_hardware_io.cpp`
- `firmware/test/fakes/`
- `firmware/test/unit/`
- `firmware/test/integration/`

### Criterios de aceitacao

- O runtime cria ou recebe o servico funcional de reles.
- O runtime aplica OFF fisico nos quatro reles durante `setup`.
- O estado global apos `setup` mantem Recalque desligado.
- O estado global apos `setup` mantem Aquecedor desligado.
- O estado global apos `setup` mantem ATO desligado.
- O estado global apos `setup` mantem Reserva desligado.
- Nenhum rele e ligado automaticamente no boot.
- Boot seguro nao emite `RELAY_ON`.
- Boot seguro nao emite `RELAY_OFF` quando todos os reles ja estiverem OFF no estado default.
- Existe API local por chamada direta para comandar rele individual.
- A API local permite ligar e desligar cada rele.
- A API local registra origem `LOCAL` para comandos manuais locais.
- A integracao nao registra tarefa periodica para reles.
- O loop principal nao bloqueia por causa do modulo de reles.
- O Watchdog continua sendo alimentado conforme a Fase 2.
- O build real nao depende de fake controller, fake GPIO ou fake tempo.
- Nenhuma funcionalidade de MQTT, NVS, Alert Manager, ATO automatico, modos, iluminacao, cloud ou aplicativo e inicializada.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste de integracao do runtime com fake relay controller.
- Testar que `setup` chama `allOff`.
- Testar que `setup` escreve OFF fisico nos quatro reles pelo fake controller.
- Testar que apos `setup` os quatro reles estao OFF no System State.
- Testar que nenhum evento `RELAY_ON` e emitido durante boot seguro.
- Testar que nenhum evento `RELAY_OFF` e emitido durante boot seguro quando o estado default ja esta OFF.
- Testar comando local ON para cada rele apos `setup`.
- Testar comando local OFF para cada rele apos `setup`.
- Testar que falha simulada no `allOff` nao liga reles e e reportada como falha de setup quando aplicavel.
- Testar que `loopOnce` permanece nao bloqueante.
- Testar que a integracao nao registra tarefa periodica de reles.
- Confirmar que nenhum teste exige ESP32, rele real, carga real, Wi-Fi, MQTT, cloud, app ou Serial real.

---

### ID

FW-P05-004

### Titulo

Consolidar validacao automatizada da Fase 5

### Objetivo

Criar o gate final da Fase 5, garantindo que o sistema de reles esteja integrado, testado por mocks e fakes, limitado ao escopo definido e preparado para evolucao futura sem depender de validacao fisica.

### Dependencias

FW-P05-003.

### Descricao

Consolidar testes unitarios e de integracao da Fase 5 em comandos ou scripts de validacao. O gate deve provar que o controlador GPIO real esta preparado para o build destinado ao hardware, que o modulo opera com fake GPIO e fake relay controller sem hardware, que o System State e atualizado, que eventos locais sao emitidos somente em mudancas efetivas e que o runtime inicia com todos os reles fisicamente comandados para OFF.

Tambem deve haver uma revisao de escopo confirmando que a Fase 5 nao implementou ATO automatico, eventos `ATO_*`, leitura ou avaliacao de `waterLevel` para reposicao, timeout de reposicao, cooldown, modos operacionais, persistencia NVS, Wi-Fi, MQTT, Alert Manager, cloud, app, historico ou testes de resiliencia de fases posteriores.

Qualquer validacao real com modulo de rele, carga fisica, bomba, aquecedor, tomada, medicao eletrica, resposta fisica, ruido ou seguranca de bancada deve ficar marcada como `Pendente para Hardware Validation` e nao pode bloquear a conclusao de desenvolvimento da Fase 5.

### Arquivos afetados

- `firmware/test/unit/`
- `firmware/test/integration/`
- `firmware/test/fakes/`
- `firmware/include/modules/relays/`
- `firmware/src/modules/relays/`
- `firmware/src/drivers/relays/`
- `firmware/src/app/`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste
- `tasks/firmware/phases/firmware-phase-05-relay-system.md`

### Criterios de aceitacao

- Existe comando ou script para executar os testes unitarios da Fase 5.
- Existe comando ou script para executar os testes de integracao da Fase 5.
- Todos os testes da Fase 5 usam mocks, fakes ou valores simulados.
- O build real do firmware compila sem depender de mocks ou fakes.
- O modulo funcional nao depende de tipos ou rotinas de diagnostico da Fase 1.
- O contrato `ACTIVE_HIGH` e validado por teste automatizado.
- Recalque, Aquecedor, ATO e Reserva sao controlados pelo firmware por interface funcional.
- O runtime inicia os quatro reles fisicamente comandados para OFF.
- O boot seguro nao gera eventos quando o estado default ja esta OFF.
- O bloco `relays` do System State e atualizado conforme a spec.
- `enabled`, `lastChanged` e `source` sao validados por teste automatizado.
- Idempotencia de comandos e validada por teste automatizado.
- Eventos locais `RELAY_ON` e `RELAY_OFF` sao gerados conforme os contratos da Fase 5.
- Falhas simuladas do controlador funcional nao corrompem o System State.
- `allOff` e validado por teste automatizado.
- Controle manual local e validado por teste automatizado.
- Nenhum teste fisico e criterio de conclusao da Fase 5.
- Qualquer validacao real dos reles fica marcada como `Pendente para Hardware Validation`.
- Revisao de escopo confirma que nenhum evento `ATO_*` foi implementado.
- Revisao de escopo confirma que o Rele 3 nao executa automacao de ATO.
- Revisao de escopo confirma que `MQTT`, `APP` e `AUTOMATION` nao foram implementados como entradas de comando desta fase.
- Revisao de escopo confirma que MQTT, NVS, Alert Manager, modos e app nao foram inicializados ou implementados.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar todos os testes unitarios da Fase 5 com sucesso.
- Executar todos os testes de integracao da Fase 5 com sucesso.
- Executar testes existentes da Fase 2 impactados por Event Bus, System State, Config Manager, Scheduler e Watchdog.
- Executar testes existentes da Fase 3 que possam ser impactados pelo runtime.
- Executar testes existentes da Fase 4 que possam ser impactados pelo runtime.
- Confirmar que o build real nao inclui fakes ou mocks.
- Confirmar que os testes nao exigem ESP32, modulo de rele, carga AC/DC real, bomba, aquecedor, Wi-Fi, MQTT, cloud, app ou qualquer validacao fisica.
- Confirmar que nao existem eventos `ATO_START`, `ATO_STOP`, `ATO_TIMEOUT`, `ATO_SENSOR_OFFLINE` ou `ATO_RECOVERED` criados pela Fase 5.
- Confirmar que nenhum teste ou implementacao usa `waterLevel` para acionar Rele 3.
- Confirmar que nenhum teste ou implementacao inicializa MQTT, NVS, Alert Manager, modos, cloud ou app.
- Confirmar que nenhum comando remoto e implementado nesta fase.
- Confirmar por revisao que funcionalidades de fases posteriores nao foram iniciadas.
