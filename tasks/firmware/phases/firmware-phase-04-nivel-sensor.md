# Fase 4 - Sensor de Nivel

## Escopo

Implementar o modulo funcional de monitoramento de nivel de agua do REEFLOW usando o VL6180X no barramento I2C oficial, com SDA no GPIO21 e SCL no GPIO22, leitura periodica, calibracao logica em memoria, atualizacao do bloco `waterLevel` do System State, estados do sensor, deteccao de falhas, recuperacao e eventos locais.

Esta fase e puramente de desenvolvimento e nao exige ESP32 conectado, VL6180X conectado, barramento I2C real, monitor Serial real, Wi-Fi, MQTT, cloud, aplicativo, rele da bomba ATO, multimetro, osciloscopio, analisador logico ou medicoes reais.

Mocks e fakes devem ser usados para validacoes automatizadas das implementacoes, seguindo o padrao das fases anteriores. Mocks e fakes devem ficar restritos a `firmware/test/` ou a configuracoes de teste, sem se tornarem dependencia do build real do firmware destinado ao hardware.

O firmware real deve ficar preparado para usar o VL6180X fisico futuramente, mas a conclusao desta fase nao pode depender de teste fisico. Qualquer verificacao com sensor real, leitura real em bancada, endereco I2C real, estabilidade fisica, variacao por distancia, ruido eletrico, Serial real ou comportamento real do nivel de agua deve ficar registrada como `Pendente para Hardware Validation` e fora dos criterios de conclusao desta fase.

Nao implementar ATO automatico, acionamento da bomba ATO, controle de reles, timeout de reposicao, cooldown de reposicao, modos operacionais, iluminacao, persistencia NVS definitiva, Wi-Fi, MQTT, Alert Manager completo, cloud, aplicativo ou testes de resiliencia de fases posteriores.

O nivel canonico exposto por `waterLevel.currentLevel`, `waterLevel.minimumLevel` e `waterLevel.maximumLevel` deve ser um nivel logico normalizado de `0..100`, onde `0` representa nivel minimo logico e `100` representa nivel maximo logico. A leitura bruta do VL6180X deve ficar encapsulada no driver/adapter e nao deve ser publicada diretamente no System State.

Os limites de nivel devem usar a configuracao em memoria existente da Fase 2 para ATO (`minimumLevel` e `maximumLevel`). A calibracao desta fase deve ser logica e minima: leitura bruta ou simulada do sensor, normalizacao para `0..100`, aplicacao de `waterLevelOffset` da configuracao em memoria e clamp para a faixa `0..100`. Calibracao fisica fina baseada em ensaio real do sump, altura real do sensor, geometria da montagem ou curva empirica fica `Pendente para Hardware Validation`.

O intervalo de leitura e o timeout de sensor offline devem ser definidos por contrato local do modulo de nivel em memoria, sem persistencia NVS e sem alterar specs. Os defaults da Fase 4 devem ser `5000 ms` para leitura periodica e `30000 ms` para sensor offline, por simetria com a estrategia de sensores ja usada no firmware. Persistencia de calibracoes fica reservada para a Fase 9. Publicacao MQTT fica reservada para a Fase 11. Centralizacao de alertas com cooldown fica reservada para a Fase 12. Automacao de reposicao fica reservada para a Fase 6.

O nome canonico dos estados no firmware deve seguir `specs/system-state-spec.md`: `NORMAL`, `LOW`, `HIGH` e `SENSOR_OFFLINE`. Estados de ATO como `REFILLING`, `TIMEOUT` e `DISABLED` nao pertencem ao bloco `waterLevel` e nao devem ser implementados nesta fase.

Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` deve ser alterado.

## Estrutura Recomendada

```text
firmware/
|-- include/
|   |-- modules/
|   |   `-- water_level/
|   |       |-- water_level_sensor.h
|   |       |-- water_level_config.h
|   |       |-- water_level_service.h
|   |       |-- water_level_state_evaluator.h
|   |       |-- water_level_calibration.h
|   |       `-- water_level_events.h
|   |-- core/
|   |-- config/
|   `-- contracts/
|-- src/
|   |-- modules/
|   |   `-- water_level/
|   |       |-- water_level_service.cpp
|   |       |-- water_level_state_evaluator.cpp
|   |       |-- water_level_calibration.cpp
|   |       `-- water_level_events.cpp
|   |-- drivers/
|   |   |-- i2c/
|   |   `-- sensors/
|   |       `-- vl6180x/
|   |           |-- vl6180x_level_sensor.h
|   |           `-- vl6180x_level_sensor.cpp
|   |-- app/
|   `-- core/
`-- test/
    |-- fakes/
    |   |-- fake_i2c_bus.h
    |   `-- fake_water_level_sensor.h
    |-- unit/
    `-- integration/
```

`firmware/test/hardware/` fica reservado para validacoes futuras e nao e criterio de conclusao da Fase 4.

O modulo funcional em `modules/water_level` nao deve depender de tipos, resultados ou rotinas de diagnostico da Fase 1. Diagnosticos de bring-up e modulo funcional podem reutilizar contratos de pinout, contrato I2C e infraestrutura de baixo nivel quando adequado, mas nao devem compartilhar enums de diagnostico como contrato funcional.

## Tarefas

### ID

FW-P04-000

### Titulo

Definir contratos funcionais e fake do sensor de nivel

### Objetivo

Criar os contratos minimos do modulo de nivel, incluindo unidade canonica `0..100`, configuracao local em memoria e interface substituivel de sensor, permitindo testes automatizados sem hardware e sem acoplamento aos diagnosticos da Fase 1.

### Dependencias

Fase 1 concluida.

Fase 2 concluida.

### Descricao

Definir um boundary funcional de leitura de nivel para a Fase 4. A interface deve representar leitura valida em nivel logico `0..100`, sensor ausente ou nao encontrado, erro de leitura, leitura fora de faixa e metadados minimos necessarios para que o modulo decida estados sem conhecer detalhes de I2C, TwoWire, Arduino ou diagnosticos de bring-up.

Definir um contrato local de configuracao do modulo de nivel, sem persistencia NVS, contendo no minimo intervalo padrao de leitura de `5000 ms`, timeout padrao de `30000 ms` para sensor offline e limites internos de normalizacao/clamp. Esse contrato deve ser testavel e nao deve alterar `specs/` nem o plano mestre.

Criar fake ou mock de sensor restrito aos testes automatizados. O fake deve permitir simular leitura valida, nivel baixo, nivel alto, erro de leitura, sensor ausente, leitura fora de faixa, sequencias de falha, ausencia desde o boot e recuperacao.

Esta tarefa nao deve implementar driver I2C real, nao deve implementar adapter real do VL6180X, nao deve atualizar System State, nao deve emitir eventos de nivel e nao deve modificar rotinas de diagnostico da Fase 1.

### Arquivos afetados

- `firmware/include/modules/water_level/water_level_sensor.h`
- `firmware/include/modules/water_level/water_level_config.h`
- `firmware/test/fakes/fake_water_level_sensor.h`
- `firmware/test/unit/`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe uma interface de sensor de nivel substituivel em testes.
- A interface representa leitura valida como nivel logico `0..100`.
- A interface representa sensor ausente ou nao encontrado.
- A interface representa erro de leitura.
- A interface representa leitura fora de faixa ou impossivel.
- Existe contrato local de configuracao do modulo de nivel em memoria.
- O contrato local define intervalo padrao de leitura como `5000 ms`.
- O contrato local define timeout padrao de sensor offline como `30000 ms`.
- O contrato local define faixa canonica `0..100`.
- A interface nao expoe tipos de diagnostico da Fase 1.
- A interface nao depende de Arduino, TwoWire concreto, Serial real, Wi-Fi, MQTT, cloud, aplicativo, NVS, reles ou ATO.
- O fake ou mock permite controlar resultados e sequencias de resultados em testes sem hardware.
- O build real do firmware nao depende de fakes ou mocks.
- Nenhum comportamento funcional de leitura periodica e iniciado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario confirmando que o fake retorna leitura valida configurada em `0..100`.
- Executar teste unitario confirmando que o fake retorna sensor ausente.
- Executar teste unitario confirmando que o fake retorna erro de leitura.
- Executar teste unitario confirmando que o fake retorna leitura fora de faixa.
- Executar teste unitario confirmando sequencia de falha e recuperacao.
- Executar teste unitario confirmando falhas desde o boot sem leitura valida.
- Executar teste unitario confirmando intervalo padrao de leitura de `5000 ms`.
- Executar teste unitario confirmando timeout padrao de sensor offline de `30000 ms`.
- Confirmar que nenhum teste exige ESP32 conectado, VL6180X conectado, barramento I2C real ou Serial real.
- Confirmar que o build real nao inclui o fake ou mock de teste.

---

### ID

FW-P04-001

### Titulo

Implementar adapter VL6180X com I2C mockavel

### Objetivo

Criar a abstracao I2C funcional e a implementacao real do boundary de sensor de nivel usando o VL6180X, preparada para hardware futuro e validavel com fake I2C sem sensor conectado.

### Dependencias

FW-P04-000.

### Descricao

Definir um boundary funcional de I2C para inicializacao do barramento, escrita em registrador, leitura de registrador, leitura de bloco quando necessario e verificacao de presenca de dispositivo no endereco esperado.

Implementar o adapter real do sensor de nivel em subarea propria `drivers/sensors/vl6180x/`. O adapter deve implementar o boundary funcional definido em `FW-P04-000` e consumir o boundary I2C desta tarefa.

O adapter deve usar o contrato oficial de pinout com SDA no GPIO21 e SCL no GPIO22, alem do endereco esperado do VL6180X definido em contrato I2C. Ele deve inicializar o minimo necessario para leitura funcional futura, verificar o dispositivo esperado, ler o valor bruto do sensor e converter essa leitura para nivel logico `0..100` antes de retornar ao modulo funcional.

A conversao de leitura bruta para `0..100` nesta fase deve ser uma normalizacao logica provisoria, deterministica e validada por fake I2C. Ela nao deve tentar representar calibracao fisica real do sump, altura real do sensor, geometria da montagem ou curva empirica. Esses ajustes ficam `Pendente para Hardware Validation`.

Criar fake de I2C restrito aos testes automatizados para simular endereco encontrado, endereco ausente, erro de escrita, erro de leitura, timeout e valores de registradores. O adapter deve traduzir falhas do barramento para resultados proprios do boundary funcional.

O adapter pode reutilizar contratos de pinout e I2C preparados na Fase 1, mas nao deve acoplar o modulo funcional aos diagnosticos de bring-up. Resultados de diagnostico como `MATCH`, `NOT_FOUND`, `MISMATCH` ou resultados de scanner I2C nao devem ser usados como contrato publico do modulo funcional.

Se a leitura do VL6180X exigir espera ou polling de pronto, o adapter deve ser projetado para nao bloquear o loop principal de forma longa. A validacao fisica da leitura real do VL6180X fica `Pendente para Hardware Validation`.

### Arquivos afetados

- `firmware/src/drivers/i2c/`
- `firmware/src/drivers/sensors/vl6180x/`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/contracts/i2c_contract.h`
- `firmware/include/modules/water_level/water_level_sensor.h`
- `firmware/include/modules/water_level/water_level_config.h`
- `firmware/test/fakes/fake_i2c_bus.h`
- `firmware/test/unit/`
- `firmware/platformio.ini`, somente se necessario para dependencias de compilacao

### Criterios de aceitacao

- Existe boundary I2C substituivel em testes.
- Existe adapter real ou neutro para o build de firmware destinado ao hardware.
- Existe fake I2C restrito a `firmware/test/` ou configuracao de teste.
- Existe implementacao real do boundary de nivel para o VL6180X.
- O adapter real fica em `firmware/src/drivers/sensors/vl6180x/`.
- SDA GPIO21, SCL GPIO22 e endereco esperado do VL6180X vem de contratos oficiais.
- O adapter real fica separado do servico de nivel.
- O adapter traduz leitura bruta valida para nivel logico `0..100` por normalizacao logica provisoria.
- O adapter traduz sensor ausente para resultado proprio do boundary funcional.
- O adapter traduz erro de barramento para resultado proprio do boundary funcional.
- O adapter traduz leitura fora de faixa para resultado proprio do boundary funcional.
- O adapter nao expoe enums ou tipos de diagnostico da Fase 1 como contrato funcional.
- Headers publicos nao expoem tipos privados de drivers ou diagnosticos.
- O firmware compila sem exigir VL6180X fisicamente conectado.
- A tarefa nao atualiza System State, nao agenda leitura periodica e nao emite eventos.
- Nenhum teste fisico e necessario para concluir a tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar com fake I2C um endereco esperado encontrado.
- Testar com fake I2C um endereco esperado ausente.
- Testar erro de escrita em registrador.
- Testar erro de leitura em registrador.
- Testar timeout simulado.
- Executar teste unitario do adapter usando fake I2C.
- Testar caso de leitura valida simulada convertida para `0..100` por normalizacao logica provisoria.
- Testar caso de sensor ausente simulado.
- Testar caso de erro de escrita simulado.
- Testar caso de erro de leitura simulado.
- Testar caso de leitura fora de faixa simulada.
- Confirmar que o adapter usa SDA GPIO21 e SCL GPIO22 do contrato oficial.
- Confirmar que o adapter usa o endereco esperado do contrato I2C.
- Confirmar que o adapter nao exige diagnostico de bring-up para ser usado pelo modulo funcional.
- Confirmar que o teste nao depende de sensor real, barramento I2C real, Serial real ou ESP32 conectado.
- Confirmar que mocks usados no teste nao permanecem como dependencia do build real.

---

### ID

FW-P04-002

### Titulo

Implementar calibracao logica e maquina de estado de nivel

### Objetivo

Criar a regra pura que aplica calibracao logica sobre o nivel `0..100` e classifica o estado como NORMAL, LOW, HIGH ou SENSOR_OFFLINE usando limites configuraveis em memoria, tempo de monitoramento e ultima leitura valida.

### Dependencias

FW-P04-000.

FW-P02-003.

### Descricao

Implementar calibracao e avaliador de estado de nivel sem dependencias de Arduino ou hardware. O avaliador deve receber resultado do boundary de sensor, configuracao ATO em memoria, calibracao em memoria, contrato local de nivel, instante atual, instante de inicio do monitoramento e instante da ultima leitura valida, retornando o status correto para o bloco `waterLevel`.

`waterLevel.currentLevel` deve receber o valor logico calibrado usado pelo firmware. Nesta fase, a calibracao minima deve aplicar `waterLevelOffset` da configuracao em memoria e proteger contra overflow, underflow e leitura impossivel por meio de clamp para `0..100`. Qualquer calibracao fisica fina baseada em ensaio real do sump, altura real do sensor, geometria da montagem ou curva empirica deve ficar `Pendente para Hardware Validation`.

Os limites minimo e maximo devem vir de `ConfigManager::ato()` em memoria. O avaliador deve retornar LOW quando o nivel calibrado estiver abaixo de `minimumLevel`, HIGH quando estiver acima de `maximumLevel` e NORMAL quando estiver dentro da faixa. Quando `minimumLevel` e `maximumLevel` forem invalidos, a configuracao deve ser rejeitada pelo Config Manager ou tratada como entrada invalida sem corromper `waterLevel`.

O estado `SENSOR_OFFLINE` deve ocorrer apos timeout local do modulo sem leitura valida. Quando nunca houver leitura valida desde o inicio do monitoramento, o timeout offline deve contar a partir de `monitorStartedAt`. Quando ja houver leitura valida, o timeout offline deve contar a partir da ultima leitura valida. A fronteira de tempo deve ser deterministica: antes do timeout nao e offline, exatamente no timeout ainda nao e offline quando a regra exigir "mais do que o timeout", e apos o timeout e offline.

Esta tarefa tambem deve declarar os contratos locais de eventos de nivel em `modules/water_level`, sem implementar MQTT, Alert Manager, historico, notificacoes remotas, ATO automatico ou acionamento de bomba.

### Arquivos afetados

- `firmware/include/modules/water_level/water_level_state_evaluator.h`
- `firmware/include/modules/water_level/water_level_calibration.h`
- `firmware/include/modules/water_level/water_level_events.h`
- `firmware/include/modules/water_level/water_level_config.h`
- `firmware/src/modules/water_level/water_level_state_evaluator.cpp`
- `firmware/src/modules/water_level/water_level_calibration.cpp`
- `firmware/src/modules/water_level/water_level_events.cpp`
- `firmware/include/config/config_manager.h`, somente se ajuste de defaults ja previstos for necessario
- `firmware/src/config/config_manager.cpp`, somente se ajuste de defaults ja previstos for necessario
- `firmware/test/unit/`

### Criterios de aceitacao

- A calibracao aplica `waterLevelOffset` da configuracao em memoria.
- A calibracao protege contra overflow, underflow e leitura impossivel.
- A calibracao sempre retorna valor dentro de `0..100` quando houver leitura funcionalmente valida.
- O avaliador retorna NORMAL para nivel calibrado dentro dos limites.
- O avaliador retorna LOW para nivel calibrado abaixo do limite minimo.
- O avaliador retorna HIGH para nivel calibrado acima do limite maximo.
- O avaliador retorna SENSOR_OFFLINE apos mais do que o timeout local sem leitura valida.
- O avaliador trata ausencia de leitura valida desde o boot usando o instante de inicio do monitoramento.
- O avaliador usa `minimumLevel` e `maximumLevel` vindos da configuracao ATO em memoria.
- O avaliador usa intervalo e timeout do contrato local do modulo, sem NVS.
- O avaliador nao aciona bomba ATO e nao altera o bloco `ato`.
- Existem contratos locais para `WATER_LEVEL_UPDATED`, `WATER_LEVEL_LOW`, `WATER_LEVEL_HIGH`, `WATER_LEVEL_SENSOR_OFFLINE` e `WATER_LEVEL_SENSOR_RECOVERED`.
- Eventos de nivel nao publicam MQTT, nao criam alertas centralizados com cooldown e nao persistem historico.
- A regra e deterministica e testavel sem hardware.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar calibracao com offset zero.
- Testar calibracao com offset positivo.
- Testar calibracao com offset negativo.
- Testar protecao contra underflow.
- Testar protecao contra overflow.
- Testar clamp para `0..100`.
- Testar classificacao NORMAL com nivel simulado dentro dos limites.
- Testar classificacao LOW com nivel simulado abaixo do limite minimo.
- Testar classificacao HIGH com nivel simulado acima do limite maximo.
- Testar que falhas antes do timeout local nao geram SENSOR_OFFLINE.
- Testar que falhas exatamente no timeout local nao geram SENSOR_OFFLINE quando a regra exigir mais que o timeout.
- Testar que falhas apos o timeout local geram SENSOR_OFFLINE.
- Testar SENSOR_OFFLINE quando nunca houve leitura valida desde o inicio do monitoramento.
- Testar recuperacao conceitual apos leitura valida posterior ao offline.
- Testar limites customizados usando configuracao em memoria.
- Confirmar que os testes usam fake de tempo ou valores simulados, sem hardware fisico.

---

### ID

FW-P04-003

### Titulo

Implementar servico de nivel com System State e eventos locais

### Objetivo

Criar o servico funcional que executa uma leitura de nivel, aplica calibracao logica, avalia o estado, atualiza o bloco `waterLevel` como fonte unica de verdade e emite eventos locais oficiais.

### Dependencias

FW-P04-001.

FW-P04-002.

FW-P02-001.

FW-P02-002.

### Descricao

Implementar o servico do modulo de nivel. Em cada ciclo, o servico deve consultar o boundary de sensor, aplicar a configuracao de ATO e calibracao em memoria, avaliar o estado de nivel e atualizar `currentLevel`, `minimumLevel`, `maximumLevel`, `status` e `lastUpdate` em `waterLevel`.

`waterLevel.lastUpdate` deve representar a ultima leitura valida do sensor. Falhas de leitura, sensor ausente, leitura fora de faixa e transicoes para `SENSOR_OFFLINE` nao devem sobrescrever `lastUpdate` quando ja houver leitura valida anterior. Quando nunca houver leitura valida, o servico deve manter metadados internos suficientes para avaliar offline a partir do inicio do monitoramento sem inventar uma leitura valida.

Toda leitura valida que atualizar o estado deve gerar `WATER_LEVEL_UPDATED`. Transicoes para LOW, HIGH e SENSOR_OFFLINE devem gerar seus eventos especificos. Quando o sensor retornar de SENSOR_OFFLINE para um estado valido, deve ser gerado `WATER_LEVEL_SENSOR_RECOVERED`.

Os eventos `WATER_LEVEL_*` desta fase devem ser apenas eventos locais do modulo de nivel. Eles podem servir como materia-prima futura para ATO, alertas, historico ou MQTT, mas nao devem criar `ATO_*`, alertas centralizados, publicacoes MQTT, historico persistido ou qualquer acao de atuador nesta fase.

Eventos repetidos nao devem ser emitidos indefinidamente quando nao houver mudanca relevante de estado, exceto `WATER_LEVEL_UPDATED` quando houver nova leitura valida conforme o intervalo de leitura. O servico deve usar o System State como unica fonte de verdade para o estado exposto do modulo.

Esta tarefa nao deve acionar reles, controlar bomba ATO, executar timeout de reposicao, aplicar cooldown de ATO, publicar MQTT, persistir NVS, criar alertas centralizados ou depender de hardware fisico.

### Arquivos afetados

- `firmware/include/modules/water_level/water_level_service.h`
- `firmware/include/modules/water_level/water_level_events.h`
- `firmware/include/modules/water_level/water_level_config.h`
- `firmware/src/modules/water_level/water_level_service.cpp`
- `firmware/src/modules/water_level/water_level_events.cpp`
- `firmware/include/core/state/system_state.h`
- `firmware/src/core/state/system_state.cpp`
- `firmware/include/core/events/event_bus.h`
- `firmware/src/core/events/event_bus.cpp`
- `firmware/include/config/config_manager.h`
- `firmware/src/config/config_manager.cpp`
- `firmware/test/fakes/`
- `firmware/test/unit/`
- `firmware/test/integration/`

### Criterios de aceitacao

- O servico executa uma iteracao de leitura por chamada explicita.
- Leitura valida atualiza `waterLevel.currentLevel` com nivel logico `0..100`.
- Limite minimo configurado atualiza `waterLevel.minimumLevel`.
- Limite maximo configurado atualiza `waterLevel.maximumLevel`.
- Status calculado atualiza `waterLevel.status`.
- `waterLevel.lastUpdate` representa a ultima leitura valida do sensor.
- Falha isolada de leitura nao corrompe o ultimo valor valido.
- Falhas apos mais do que o timeout local sem leitura valida mudam o status para SENSOR_OFFLINE.
- Ausencia de leitura valida desde o boot pode levar a SENSOR_OFFLINE apos o timeout local.
- Leitura valida gera `WATER_LEVEL_UPDATED`.
- Transicao para LOW gera `WATER_LEVEL_LOW`.
- Transicao para HIGH gera `WATER_LEVEL_HIGH`.
- Transicao para SENSOR_OFFLINE gera `WATER_LEVEL_SENSOR_OFFLINE`.
- Recuperacao a partir de SENSOR_OFFLINE gera `WATER_LEVEL_SENSOR_RECOVERED`.
- Eventos `WATER_LEVEL_*` sao locais e nao criam eventos `ATO_*`.
- Eventos `WATER_LEVEL_*` nao criam alertas centralizados, publicacoes MQTT ou historico persistido.
- Eventos de transicao nao sao duplicados sem mudanca relevante.
- O System State permanece a fonte unica de verdade do modulo.
- O servico nao aciona reles, nao controla bomba ATO e nao executa fail-safe de atuadores.
- O servico nao altera `ato.status`, `ato.pumpRunning`, `ato.lastActivation`, `ato.lastCompletion` ou `ato.timeoutCounter`.
- Nenhum evento publica MQTT ou cria alerta centralizado nesta fase.
- O servico nao depende de hardware fisico para ser testado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar com fake sensor uma leitura valida e confirmar atualizacao de `waterLevel`.
- Testar com fake sensor nivel baixo e confirmar status LOW no System State.
- Testar com fake sensor nivel alto e confirmar status HIGH no System State.
- Testar com fake sensor falha isolada e confirmar preservacao do ultimo valor valido e de `lastUpdate`.
- Testar falhas antes, exatamente no limite e apos o timeout offline local.
- Testar falhas desde o boot sem leitura valida ate SENSOR_OFFLINE.
- Testar que limites configurados em memoria aparecem no bloco `waterLevel`.
- Testar que calibracao em memoria altera o valor gravado em `waterLevel.currentLevel`.
- Testar que `waterLevel.currentLevel` permanece em `0..100`.
- Testar que blocos nao relacionados do System State nao sao corrompidos.
- Testar que o bloco `ato` nao e alterado pelo servico de nivel.
- Testar que leitura valida publica `WATER_LEVEL_UPDATED`.
- Testar que NORMAL para LOW publica `WATER_LEVEL_LOW`.
- Testar que NORMAL para HIGH publica `WATER_LEVEL_HIGH`.
- Testar que falhas ate timeout publicam `WATER_LEVEL_SENSOR_OFFLINE`.
- Testar que recuperacao apos offline publica `WATER_LEVEL_SENSOR_RECOVERED`.
- Testar que eventos de nivel nao geram `ATO_START`, `ATO_STOP`, `ATO_TIMEOUT`, `ATO_SENSOR_OFFLINE` ou `ATO_RECOVERED`.
- Testar que eventos de nivel nao geram alerta centralizado, publicacao MQTT ou historico persistido.
- Testar que chamadas repetidas sem transicao nao duplicam eventos de estado.
- Confirmar que os testes usam fake sensor, fake tempo ou Event Bus local, sem hardware fisico.

---

### ID

FW-P04-004

### Titulo

Integrar leitura periodica ao scheduler sem bloqueio

### Objetivo

Integrar o servico de nivel ao Task Scheduler para executar leitura periodica pelo contrato local do modulo, preservando o loop principal, o watchdog e a validacao sem hardware.

### Dependencias

FW-P04-003.

FW-P02-004.

FW-P02-007.

### Descricao

Registrar uma tarefa periodica de nivel no scheduler do runtime do core. O intervalo padrao deve vir do contrato local do modulo de nivel em memoria, sem implementar persistencia NVS e sem alterar specs.

A tarefa deve chamar uma iteracao do servico de nivel e retornar sucesso ou falha de forma compativel com o scheduler. Falhas de leitura do sensor devem ser tratadas pelo servico e pelo avaliador de estado, sem travar o scheduler.

A integracao deve preservar a inicializacao deterministica do core: System State, Event Bus, Config Manager, Logger, Scheduler e Watchdog continuam sendo inicializados antes do modulo funcional de nivel quando necessario.

O loop principal nao deve bloquear aguardando resposta longa do VL6180X ou do barramento I2C. Caso a implementacao do sensor exija espera ou polling de pronto, o desenho deve usar estado interno, tentativa curta, retorno pendente ou outro mecanismo compativel com o scheduler, mantendo testes deterministas com fake de tempo.

Esta tarefa nao deve inicializar MQTT, NVS, Alert Manager, reles, ATO automatico, modos, iluminacao ou qualquer funcionalidade de fases posteriores.

### Arquivos afetados

- `firmware/src/app/`
- `firmware/include/modules/water_level/water_level_service.h`
- `firmware/include/modules/water_level/water_level_config.h`
- `firmware/src/modules/water_level/water_level_service.cpp`
- `firmware/include/modules/water_level/water_level_state_evaluator.h`
- `firmware/src/modules/water_level/water_level_state_evaluator.cpp`
- `firmware/include/core/scheduler/task_scheduler.h`
- `firmware/src/core/scheduler/task_scheduler.cpp`
- `firmware/test/fakes/`
- `firmware/test/unit/`
- `firmware/test/integration/`

### Criterios de aceitacao

- O runtime registra uma tarefa periodica para o modulo de nivel.
- O intervalo padrao da tarefa vem do contrato local do modulo de nivel.
- A tarefa executa uma iteracao do servico de nivel.
- A tarefa nao bloqueia o loop principal aguardando leitura fisica longa do VL6180X.
- Falha da tarefa e reportada de forma compativel com o scheduler quando aplicavel.
- Falhas de sensor sao refletidas em `waterLevel.status` sem travar o scheduler.
- O Watchdog continua sendo alimentado conforme a Fase 2.
- O build real nao depende de fake sensor, fake I2C ou fake tempo.
- Nenhuma funcionalidade de MQTT, NVS, Alert Manager, reles, ATO automatico, modos ou iluminacao e inicializada.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar com fake time source que a tarefa nao executa antes do intervalo configurado.
- Testar com fake time source que a tarefa executa quando o intervalo vence.
- Testar que uma execucao da tarefa chama o servico de nivel uma vez.
- Testar que multiplos ciclos respeitam o intervalo configurado.
- Testar que falha simulada no servico e refletida como falha de tarefa quando aplicavel.
- Testar que uma simulacao de leitura pendente ou timeout curto de I2C nao bloqueia o loop principal.
- Executar teste de integracao do runtime com fake sensor e fake tempo.
- Confirmar que nenhum teste exige VL6180X real, barramento I2C real, ESP32 conectado ou Serial real.

---

### ID

FW-P04-005

### Titulo

Consolidar validacao automatizada da Fase 4

### Objetivo

Criar o gate final da Fase 4, garantindo que o modulo de nivel esteja integrado, testado por mocks e fakes, limitado ao escopo definido e preparado para evolucao futura sem grandes refatoracoes.

### Dependencias

FW-P04-004.

### Descricao

Consolidar testes unitarios e de integracao da Fase 4 em comandos ou scripts de validacao. O gate deve provar que o VL6180X funcional esta preparado para o build real, que o modulo opera com fake sensor e fake I2C sem hardware, que o System State e atualizado, que eventos locais sao emitidos e que o scheduler executa leituras periodicas sem bloquear o loop principal.

Tambem deve haver uma revisao de escopo confirmando que a Fase 4 nao implementou ATO automatico, eventos `ATO_*`, acionamento da bomba ATO, acionamento do rele 3, controle de reles, timeout de reposicao, cooldown de reposicao, modos, iluminacao, persistencia NVS, Wi-Fi, MQTT, Alert Manager, cloud, app ou testes de resiliencia de fases posteriores.

### Arquivos afetados

- `firmware/test/unit/`
- `firmware/test/integration/`
- `firmware/test/fakes/`
- `firmware/include/modules/water_level/`
- `firmware/src/modules/water_level/`
- `firmware/src/drivers/i2c/`
- `firmware/src/drivers/sensors/vl6180x/`
- `firmware/src/app/`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste
- `tasks/firmware/phases/firmware-phase-04-nivel-sensor.md`

### Criterios de aceitacao

- Existe comando ou script para executar os testes unitarios da Fase 4.
- Existe comando ou script para executar os testes de integracao da Fase 4.
- Todos os testes da Fase 4 usam mocks, fakes ou valores simulados.
- O build real do firmware compila sem depender de mocks ou fakes.
- O modulo funcional nao depende de tipos ou rotinas de diagnostico da Fase 1.
- O bloco `waterLevel` do System State e atualizado conforme a spec.
- `waterLevel.currentLevel` usa nivel logico `0..100`.
- `waterLevel.lastUpdate` representa a ultima leitura valida do sensor.
- Eventos locais de nivel sao gerados conforme os contratos da Fase 4.
- Sensor offline apos mais do que o timeout local sem leitura valida e validado por teste automatizado.
- Ausencia de leitura valida desde o boot e validada por teste automatizado.
- Recuperacao de sensor offline e validada por teste automatizado.
- Leitura periodica e validada por fake de tempo.
- Nao bloqueio do loop principal durante leitura I2C e validado por teste automatizado ou revisao objetiva.
- Nenhum teste fisico e criterio de conclusao da Fase 4.
- Qualquer validacao real do VL6180X, barramento I2C, endereco fisico, estabilidade ou comportamento real do nivel fica marcada como `Pendente para Hardware Validation`.
- Revisao de escopo confirma que nenhum evento `ATO_*` foi implementado.
- Revisao de escopo confirma que o rele 3 e a bomba ATO nao sao acionados.
- Revisao de escopo confirma que MQTT, NVS e Alert Manager nao foram inicializados ou implementados.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar todos os testes unitarios da Fase 4 com sucesso.
- Executar todos os testes de integracao da Fase 4 com sucesso.
- Executar testes existentes da Fase 2 impactados por Event Bus, System State, Config Manager e Scheduler.
- Executar testes existentes da Fase 3 que possam ser impactados pelo runtime de sensores.
- Confirmar que o build real nao inclui fakes ou mocks.
- Confirmar que os testes nao exigem ESP32, VL6180X, barramento I2C real, Wi-Fi, MQTT, cloud, app ou qualquer validacao fisica.
- Confirmar que nao existem eventos `ATO_START`, `ATO_STOP`, `ATO_TIMEOUT`, `ATO_SENSOR_OFFLINE` ou `ATO_RECOVERED` criados pela Fase 4.
- Confirmar que nenhum teste ou implementacao aciona o rele 3 ou bomba ATO.
- Confirmar que nenhum teste ou implementacao inicializa MQTT, NVS ou Alert Manager.
- Confirmar por revisao que funcionalidades de fases posteriores nao foram iniciadas.
