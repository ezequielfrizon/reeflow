# Fase 3 - Sensor de Temperatura

## Escopo

Implementar o modulo funcional de monitoramento de temperatura do REEFLOW usando o DS18B20 no GPIO4, com leitura periodica, atualizacao do bloco `temperature` do System State, estados de temperatura, deteccao de sensor offline, recuperacao e eventos locais.

Esta fase e puramente de desenvolvimento e nao exige ESP32 conectado, DS18B20 conectado, monitor Serial real, Wi-Fi, MQTT, cloud, aplicativo, rele do aquecedor, termostato fisico, multimetro, osciloscopio ou medicoes reais.

Mocks e fakes devem ser usados para validacoes automatizadas das implementacoes, seguindo o padrao da Fase 1. Mocks e fakes devem ficar restritos a `firmware/test/` ou a configuracoes de teste, sem se tornarem dependencia do build real do firmware destinado ao hardware.

O firmware real deve ficar preparado para usar o DS18B20 fisico futuramente, mas a conclusao desta fase nao pode depender de teste fisico. Qualquer verificacao com sensor real, leitura real em bancada, estabilidade fisica, cabo, pull-up, Serial real ou comportamento termico real deve ficar fora dos criterios de conclusao desta fase.

Nao implementar sensor de nivel, ATO, controle de reles, controle do aquecedor, modos operacionais, iluminacao, persistencia NVS definitiva, Wi-Fi, MQTT, Alert Manager completo, cloud, aplicativo ou testes de resiliencia de fases posteriores.

Os limites de temperatura devem usar a configuracao em memoria existente da Fase 2. Persistencia NVS dos limites fica reservada para a Fase 9. Publicacao MQTT fica reservada para a Fase 11. Centralizacao de alertas com cooldown fica reservada para a Fase 12.

O nome canonico dos estados no firmware deve seguir `specs/system-state-spec.md`: `NORMAL`, `HIGH`, `LOW` e `SENSOR_OFFLINE`. A nomenclatura da spec de temperatura `ALTA` e `BAIXA` deve ser tratada como equivalente conceitual de `HIGH` e `LOW`, sem alterar specs.

Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` deve ser alterado.

## Estrutura Recomendada

```text
firmware/
|-- include/
|   |-- modules/
|   |   `-- temperature/
|   |       |-- temperature_sensor.h
|   |       |-- temperature_service.h
|   |       |-- temperature_state_evaluator.h
|   |       `-- temperature_events.h
|   |-- core/
|   |-- config/
|   `-- contracts/
|-- src/
|   |-- modules/
|   |   `-- temperature/
|   |       |-- temperature_service.cpp
|   |       |-- temperature_state_evaluator.cpp
|   |       `-- temperature_events.cpp
|   |-- drivers/
|   |   `-- sensors/
|   |       `-- ds18b20/
|   |           |-- ds18b20_temperature_sensor.h
|   |           `-- ds18b20_temperature_sensor.cpp
|   |-- app/
|   `-- core/
`-- test/
    |-- fakes/
    |   `-- fake_temperature_sensor.h
    |-- unit/
    `-- integration/
```

`firmware/test/hardware/` fica reservado para validacoes futuras e nao e criterio de conclusao da Fase 3.

O modulo funcional em `modules/temperature` nao deve depender de tipos, resultados ou rotinas de diagnostico da Fase 1. Diagnosticos de bring-up e modulo funcional podem reutilizar contratos de pinout e infraestrutura de baixo nivel quando adequado, mas nao devem compartilhar enums de diagnostico como contrato funcional.

## Tarefas

### ID

FW-P03-000

### Titulo

Criar boundary funcional e fake do sensor de temperatura

### Objetivo

Criar a interface minima que isola o modulo de temperatura da leitura fisica do DS18B20, permitindo testes automatizados sem hardware e sem acoplamento aos diagnosticos da Fase 1.

### Dependencias

Fase 1 concluida.

Fase 2 concluida.

### Descricao

Definir um boundary funcional de leitura de temperatura para a Fase 3. A interface deve representar leitura valida em Celsius, sensor ausente ou nao encontrado, erro de leitura e metadados minimos necessarios para que o modulo decida estados sem conhecer detalhes de OneWire, Arduino ou diagnosticos de bring-up.

Criar fake ou mock de sensor restrito aos testes automatizados. O fake deve permitir simular leitura valida, temperatura alta, temperatura baixa, erro de leitura, sensor ausente, sequencias de falha, ausencia desde o boot e recuperacao.

Esta tarefa nao deve implementar leitura real periodica, nao deve atualizar System State, nao deve emitir eventos de temperatura e nao deve modificar rotinas de diagnostico da Fase 1.

### Arquivos afetados

- `firmware/include/modules/temperature/temperature_sensor.h`
- `firmware/test/fakes/fake_temperature_sensor.h`
- `firmware/test/unit/`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe uma interface de sensor de temperatura substituivel em testes.
- A interface representa leitura valida com valor em Celsius.
- A interface representa sensor ausente ou nao encontrado.
- A interface representa erro de leitura.
- A interface nao expoe tipos de diagnostico da Fase 1.
- A interface nao depende de Arduino, OneWire concreto, Serial real, Wi-Fi, MQTT, cloud, aplicativo, NVS, reles ou ATO.
- O fake ou mock permite controlar resultados e sequencias de resultados em testes sem hardware.
- O build real do firmware nao depende de fakes ou mocks.
- Nenhum comportamento funcional de leitura periodica e iniciado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario confirmando que o fake retorna leitura valida configurada.
- Executar teste unitario confirmando que o fake retorna sensor ausente.
- Executar teste unitario confirmando que o fake retorna erro de leitura.
- Executar teste unitario confirmando sequencia de falha e recuperacao.
- Executar teste unitario confirmando falhas desde o boot sem leitura valida.
- Confirmar que nenhum teste exige ESP32 conectado, DS18B20 conectado ou Serial real.
- Confirmar que o build real nao inclui o fake ou mock de teste.

---

### ID

FW-P03-001

### Titulo

Implementar adapter real DS18B20 isolado de diagnosticos

### Objetivo

Criar a implementacao real do boundary de temperatura usando DS18B20 no GPIO4, preparada para hardware futuro e validavel por build sem sensor conectado.

### Dependencias

FW-P03-000.

### Descricao

Implementar o adapter real do sensor de temperatura usando o contrato de pinout do hardware V1, onde o DS18B20 esta no GPIO4. O adapter deve ficar em subarea propria `drivers/sensors/ds18b20/` e deve implementar o boundary funcional definido em `FW-P03-000`.

O adapter pode reutilizar infraestrutura OneWire ou bibliotecas preparadas anteriormente, mas nao deve acoplar o modulo funcional aos diagnosticos da Fase 1. Resultados de diagnostico como `FOUND`, `NOT_FOUND` e `READ_ERROR` nao devem ser usados como contrato publico do modulo funcional.

Se a implementacao do DS18B20 exigir espera de conversao, o adapter deve ser projetado para nao forcar bloqueio longo no loop principal da aplicacao. A validacao fisica da leitura real do DS18B20 fica fora dos criterios de conclusao desta fase.

### Arquivos afetados

- `firmware/src/drivers/sensors/ds18b20/`
- `firmware/src/drivers/onewire/`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/modules/temperature/temperature_sensor.h`
- `firmware/test/unit/`
- `firmware/platformio.ini`, somente se necessario para dependencias de compilacao

### Criterios de aceitacao

- Existe implementacao real do boundary de temperatura para o DS18B20.
- O adapter real fica em `firmware/src/drivers/sensors/ds18b20/`.
- O GPIO4 vem do contrato oficial de pinout, sem numero duplicado no modulo funcional.
- O adapter real fica separado do servico de temperatura.
- O adapter traduz leitura valida para temperatura em Celsius.
- O adapter traduz sensor ausente para resultado proprio do boundary funcional.
- O adapter traduz erro de leitura para resultado proprio do boundary funcional.
- O adapter nao expoe enums ou tipos de diagnostico da Fase 1 como contrato funcional.
- O firmware compila sem exigir DS18B20 fisicamente conectado.
- A tarefa nao atualiza System State, nao agenda leitura periodica e nao emite eventos.
- Nenhum teste fisico e necessario para concluir a tarefa.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario do adapter usando fake de OneWire ou fake de backend equivalente.
- Testar caso de leitura valida simulada.
- Testar caso de sensor ausente simulado.
- Testar caso de erro de leitura simulado.
- Confirmar que o adapter usa o GPIO4 do contrato oficial.
- Confirmar que o adapter nao exige diagnostico de bring-up para ser usado pelo modulo funcional.
- Confirmar que o teste nao depende de sensor real, Serial real ou ESP32 conectado.
- Confirmar que mocks usados no teste nao permanecem como dependencia do build real.

---

### ID

FW-P03-002

### Titulo

Implementar avaliador e maquina de estado de temperatura

### Objetivo

Criar a regra pura que classifica a temperatura como NORMAL, HIGH, LOW ou SENSOR_OFFLINE usando limites configuraveis, tempo de monitoramento e ultima leitura valida.

### Dependencias

FW-P03-000.

FW-P02-003.

### Descricao

Implementar um avaliador de estado termico sem dependencias de Arduino ou hardware. O avaliador deve receber resultado do boundary de sensor, configuracao de temperatura, instante atual, instante de inicio do monitoramento e instante da ultima leitura valida, retornando o status correto para o bloco `temperature`.

Os limites padrao devem seguir a spec: temperatura ideal 26.5 C, minima 25.0 C e maxima 28.0 C. O intervalo de leitura padrao deve ser 5000 ms e o estado `SENSOR_OFFLINE` deve ocorrer apos mais de 30000 ms sem leitura valida, conforme configuracao em memoria da Fase 2.

Quando nunca houver leitura valida desde o inicio do monitoramento, o timeout offline deve contar a partir de `monitorStartedAt`. Quando ja houver leitura valida, o timeout offline deve contar a partir da ultima leitura valida. A fronteira de tempo deve ser deterministica: antes do timeout nao e offline, exatamente no timeout ainda nao e offline quando a regra exigir "mais de 30 segundos", e apos o timeout e offline.

Esta tarefa tambem deve declarar os contratos locais de eventos de temperatura em `modules/temperature`, sem implementar MQTT, Alert Manager, historico ou notificacoes remotas.

### Arquivos afetados

- `firmware/include/modules/temperature/temperature_state_evaluator.h`
- `firmware/include/modules/temperature/temperature_events.h`
- `firmware/src/modules/temperature/temperature_state_evaluator.cpp`
- `firmware/src/modules/temperature/temperature_events.cpp`
- `firmware/include/config/config_manager.h`
- `firmware/src/config/config_manager.cpp`, somente se ajuste de defaults ja previstos for necessario
- `firmware/test/unit/`

### Criterios de aceitacao

- O avaliador retorna NORMAL para temperatura dentro dos limites.
- O avaliador retorna HIGH para temperatura acima do limite maximo.
- O avaliador retorna LOW para temperatura abaixo do limite minimo.
- O avaliador retorna SENSOR_OFFLINE apos mais do que o timeout configurado sem leitura valida.
- O avaliador trata ausencia de leitura valida desde o boot usando o instante de inicio do monitoramento.
- O avaliador usa limites vindos da configuracao de temperatura em memoria.
- O avaliador respeita intervalo de leitura e timeout offline configuraveis.
- `ALTA` e `BAIXA` da spec de temperatura ficam mapeados conceitualmente para os estados canonicos HIGH e LOW do firmware.
- Existem contratos locais para `TEMPERATURE_UPDATED`, `TEMPERATURE_HIGH`, `TEMPERATURE_LOW`, `TEMPERATURE_SENSOR_OFFLINE` e `TEMPERATURE_SENSOR_RECOVERED`.
- Eventos de temperatura nao publicam MQTT, nao criam alertas centralizados com cooldown e nao persistem historico.
- A regra e deterministica e testavel sem hardware.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar classificacao NORMAL com temperatura simulada dentro dos limites.
- Testar classificacao HIGH com temperatura simulada acima do limite maximo.
- Testar classificacao LOW com temperatura simulada abaixo do limite minimo.
- Testar que falhas antes do timeout configurado nao geram SENSOR_OFFLINE.
- Testar que falhas exatamente no timeout configurado nao geram SENSOR_OFFLINE quando a regra exigir mais que o timeout.
- Testar que falhas apos o timeout configurado geram SENSOR_OFFLINE.
- Testar SENSOR_OFFLINE quando nunca houve leitura valida desde o inicio do monitoramento.
- Testar recuperacao conceitual apos leitura valida posterior ao offline.
- Testar limites customizados usando configuracao em memoria.
- Confirmar que os testes usam fake de tempo ou valores simulados, sem hardware fisico.

---

### ID

FW-P03-003

### Titulo

Implementar servico de temperatura com System State e eventos locais

### Objetivo

Criar o servico funcional que executa uma leitura de temperatura, avalia o estado, atualiza o bloco `temperature` como fonte unica de verdade e emite eventos locais oficiais.

### Dependencias

FW-P03-001.

FW-P03-002.

FW-P02-001.

FW-P02-002.

### Descricao

Implementar o servico do modulo de temperatura. Em cada ciclo, o servico deve consultar o boundary de sensor, aplicar a configuracao de temperatura em memoria, avaliar o estado termico e atualizar `currentTemperature`, `minTemperature`, `maxTemperature`, `status` e `lastUpdate` em `temperature`.

`temperature.lastUpdate` deve representar a ultima leitura valida do sensor. Falhas de leitura, sensor ausente e transicoes para `SENSOR_OFFLINE` nao devem sobrescrever `lastUpdate` quando ja houver leitura valida anterior. Quando nunca houver leitura valida, o servico deve manter metadados internos suficientes para avaliar offline a partir do inicio do monitoramento sem inventar uma leitura valida.

Toda leitura valida que atualizar o estado deve gerar `TEMPERATURE_UPDATED`. Transicoes para HIGH, LOW e SENSOR_OFFLINE devem gerar seus eventos especificos. Quando o sensor retornar de SENSOR_OFFLINE para um estado valido, deve ser gerado `TEMPERATURE_SENSOR_RECOVERED`.

Eventos repetidos nao devem ser emitidos indefinidamente quando nao houver mudanca relevante de estado, exceto `TEMPERATURE_UPDATED` quando houver nova leitura valida conforme o intervalo de leitura. O servico deve usar o System State como unica fonte de verdade para o estado exposto do modulo.

Esta tarefa nao deve acionar reles, controlar aquecedor, executar fail-safe de atuadores, publicar MQTT, persistir NVS, criar alertas centralizados ou depender de hardware fisico.

### Arquivos afetados

- `firmware/include/modules/temperature/temperature_service.h`
- `firmware/include/modules/temperature/temperature_events.h`
- `firmware/src/modules/temperature/temperature_service.cpp`
- `firmware/src/modules/temperature/temperature_events.cpp`
- `firmware/include/core/state/system_state.h`
- `firmware/src/core/state/system_state.cpp`
- `firmware/include/core/events/`
- `firmware/src/core/events/`
- `firmware/include/config/config_manager.h`
- `firmware/src/config/config_manager.cpp`
- `firmware/test/fakes/`
- `firmware/test/unit/`
- `firmware/test/integration/`

### Criterios de aceitacao

- O servico executa uma iteracao de leitura por chamada explicita.
- Leitura valida atualiza `temperature.currentTemperature`.
- Limite minimo configurado atualiza `temperature.minTemperature`.
- Limite maximo configurado atualiza `temperature.maxTemperature`.
- Status calculado atualiza `temperature.status`.
- `temperature.lastUpdate` representa a ultima leitura valida do sensor.
- Falha isolada de leitura nao corrompe o ultimo valor valido.
- Falhas apos mais de 30000 ms sem leitura valida mudam o status para SENSOR_OFFLINE.
- Ausencia de leitura valida desde o boot pode levar a SENSOR_OFFLINE apos o timeout configurado.
- Leitura valida gera `TEMPERATURE_UPDATED`.
- Transicao para HIGH gera `TEMPERATURE_HIGH`.
- Transicao para LOW gera `TEMPERATURE_LOW`.
- Transicao para SENSOR_OFFLINE gera `TEMPERATURE_SENSOR_OFFLINE`.
- Recuperacao a partir de SENSOR_OFFLINE gera `TEMPERATURE_SENSOR_RECOVERED`.
- Eventos de transicao nao sao duplicados sem mudanca relevante.
- O System State permanece a fonte unica de verdade do modulo.
- O servico nao aciona reles, nao controla aquecedor e nao executa fail-safe de atuadores.
- Nenhum evento publica MQTT ou cria alerta centralizado nesta fase.
- O servico nao depende de hardware fisico para ser testado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar com fake sensor uma leitura valida e confirmar atualizacao de `temperature`.
- Testar com fake sensor temperatura alta e confirmar status HIGH no System State.
- Testar com fake sensor temperatura baixa e confirmar status LOW no System State.
- Testar com fake sensor falha isolada e confirmar preservacao do ultimo valor valido e de `lastUpdate`.
- Testar falhas antes, exatamente no limite e apos o timeout offline configurado.
- Testar falhas desde o boot sem leitura valida ate SENSOR_OFFLINE.
- Testar que limites configurados em memoria aparecem no bloco `temperature`.
- Testar que blocos nao relacionados do System State nao sao corrompidos.
- Testar que leitura valida publica `TEMPERATURE_UPDATED`.
- Testar que NORMAL para HIGH publica `TEMPERATURE_HIGH`.
- Testar que NORMAL para LOW publica `TEMPERATURE_LOW`.
- Testar que falhas ate timeout publicam `TEMPERATURE_SENSOR_OFFLINE`.
- Testar que recuperacao apos offline publica `TEMPERATURE_SENSOR_RECOVERED`.
- Testar que chamadas repetidas sem transicao nao duplicam eventos de estado.
- Confirmar que os testes usam fake sensor, fake tempo ou Event Bus local, sem hardware fisico.

---

### ID

FW-P03-004

### Titulo

Integrar leitura periodica ao scheduler sem bloqueio

### Objetivo

Integrar o servico de temperatura ao Task Scheduler para executar leitura a cada 5 segundos por configuracao, preservando o loop principal, o watchdog e a validacao sem hardware.

### Dependencias

FW-P03-003.

FW-P02-004.

FW-P02-007.

### Descricao

Registrar uma tarefa periodica de temperatura no scheduler do runtime do core. O intervalo padrao deve ser 5000 ms, vindo da configuracao de temperatura em memoria. A tarefa deve chamar uma iteracao do servico de temperatura e retornar sucesso ou falha de forma compativel com o scheduler.

A integracao deve preservar a inicializacao deterministica do core: System State, Event Bus, Config Manager, Logger, Scheduler e Watchdog continuam sendo inicializados antes do modulo funcional de temperatura quando necessario.

O loop principal nao deve bloquear aguardando conversao fisica do DS18B20. Caso a implementacao do sensor exija espera de conversao, o desenho deve usar estado interno, etapas de conversao/leitura ou outro mecanismo compativel com o scheduler, mantendo testes deterministas com fake de tempo.

Esta tarefa nao deve inicializar MQTT, NVS, Alert Manager, reles, ATO, modos, iluminacao ou qualquer funcionalidade de fases posteriores.

### Arquivos afetados

- `firmware/src/app/`
- `firmware/include/modules/temperature/temperature_service.h`
- `firmware/src/modules/temperature/temperature_service.cpp`
- `firmware/include/core/scheduler/task_scheduler.h`
- `firmware/src/core/scheduler/task_scheduler.cpp`
- `firmware/test/fakes/`
- `firmware/test/unit/`
- `firmware/test/integration/`

### Criterios de aceitacao

- O runtime registra uma tarefa periodica para o modulo de temperatura.
- O intervalo padrao da tarefa e 5000 ms.
- O intervalo vem da configuracao de temperatura em memoria quando disponivel.
- A tarefa executa uma iteracao do servico de temperatura.
- A tarefa nao bloqueia o loop principal aguardando conversao fisica do DS18B20.
- Falha da tarefa e reportada de forma compativel com o scheduler.
- O Watchdog continua sendo alimentado conforme a Fase 2.
- O build real nao depende de fake sensor ou fake tempo.
- Nenhuma funcionalidade de MQTT, NVS, Alert Manager, reles, ATO, modos ou iluminacao e inicializada.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar com fake time source que a tarefa nao executa antes de 5000 ms.
- Testar com fake time source que a tarefa executa quando o intervalo vence.
- Testar que uma execucao da tarefa chama o servico de temperatura uma vez.
- Testar que multiplos ciclos respeitam o intervalo configurado.
- Testar que falha simulada no servico e refletida como falha de tarefa quando aplicavel.
- Testar que uma simulacao de conversao pendente nao bloqueia o loop principal.
- Executar teste de integracao do runtime com fake sensor e fake tempo.
- Confirmar que nenhum teste exige DS18B20 real, ESP32 conectado ou Serial real.

---

### ID

FW-P03-005

### Titulo

Consolidar validacao automatizada da Fase 3

### Objetivo

Criar o gate final da Fase 3, garantindo que o modulo de temperatura esteja integrado, testado por mocks e fakes, e limitado ao escopo definido.

### Dependencias

FW-P03-004.

### Descricao

Consolidar testes unitarios e de integracao da Fase 3 em comandos ou scripts de validacao. O gate deve provar que o DS18B20 funcional esta preparado para o build real, que o modulo opera com fake sensor sem hardware, que o System State e atualizado, que eventos locais sao emitidos e que o scheduler executa leituras periodicas sem bloquear o loop principal.

Tambem deve haver uma revisao de escopo confirmando que a Fase 3 nao implementou sensor de nivel, ATO, reles, controle de aquecedor, modos, iluminacao, persistencia NVS, Wi-Fi, MQTT, Alert Manager, cloud, app ou testes de resiliencia de fases posteriores.

### Arquivos afetados

- `firmware/test/unit/`
- `firmware/test/integration/`
- `firmware/test/fakes/`
- `firmware/include/modules/temperature/`
- `firmware/src/modules/temperature/`
- `firmware/src/drivers/sensors/ds18b20/`
- `firmware/src/app/`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste
- `tasks/firmware-phase-03-temperature-sensor.md`

### Criterios de aceitacao

- Existe comando ou script para executar os testes unitarios da Fase 3.
- Existe comando ou script para executar os testes de integracao da Fase 3.
- Todos os testes da Fase 3 usam mocks, fakes ou valores simulados.
- O build real do firmware compila sem depender de mocks ou fakes.
- O modulo funcional nao depende de tipos ou rotinas de diagnostico da Fase 1.
- O bloco `temperature` do System State e atualizado conforme a spec.
- `temperature.lastUpdate` representa a ultima leitura valida do sensor.
- Eventos locais de temperatura sao gerados conforme a spec.
- Sensor offline apos mais de 30000 ms sem leitura valida e validado por teste automatizado.
- Ausencia de leitura valida desde o boot e validada por teste automatizado.
- Recuperacao de sensor offline e validada por teste automatizado.
- Leitura periodica de 5000 ms e validada por fake de tempo.
- Nao bloqueio do loop principal durante leitura/conversao e validado por teste automatizado ou revisao objetiva.
- Nenhum teste fisico e criterio de conclusao da Fase 3.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar todos os testes unitarios da Fase 3 com sucesso.
- Executar todos os testes de integracao da Fase 3 com sucesso.
- Executar testes existentes da Fase 2 impactados por Event Bus, System State, Config Manager e Scheduler.
- Confirmar que o build real nao inclui fakes ou mocks.
- Confirmar que os testes nao exigem ESP32, DS18B20, Wi-Fi, MQTT, cloud, app ou qualquer validacao fisica.
- Confirmar por revisao que funcionalidades de fases posteriores nao foram iniciadas.
