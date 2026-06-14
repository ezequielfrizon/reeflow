# Fase 10 - Rede

## Escopo

Implementar a camada de rede local do firmware REEFLOW no ESP32, cobrindo Wi-Fi, reconexao automatica, NTP, heartbeat local e status de rede no System State. A fase deve permitir que o controlador se conecte quando houver configuracao Wi-Fi restaurada localmente, mantenha automacoes criticas independentes da rede e exponha estado de conectividade para fases futuras.

Esta fase depende da Fase 2, pelo System State, Event Bus, Config Manager, Task Scheduler, Logger, Watchdog e Time Source, e da Fase 9, pelas configuracoes locais restauraveis de Wi-Fi. A rede e uma extensao operacional: nao pode bloquear sensores, relays, ATO, modos, iluminacao, persistencia local ou fail-safe ja existentes.

A conclusao desta fase nao pode depender de access point real, Internet real, servidor NTP real, broker MQTT, cloud, aplicativo, monitor Serial real, ESP32 conectado, medicao eletrica, reboot fisico ou qualquer teste fisico. Validacoes reais com roteador, RSSI real, DHCP real, DNS real, NTP publico, Internet real, ESP32, antena, caixa eletrica, aquario ou bancada devem ficar registradas como `Pendente para Hardware Validation` e fora dos criterios de conclusao.

Mocks e fakes devem ser usados para validacoes automatizadas das implementacoes, seguindo o padrao das fases anteriores. Mocks e fakes devem ficar restritos a `firmware/test/` ou a configuracoes de teste, sem se tornarem dependencia do build real do firmware destinado ao hardware.

Nao implementar MQTT runtime da Fase 11, fila de mensagens MQTT, publicacao de telemetria, recebimento de comandos remotos, QoS, broker Mosquitto, bridge cloud, Alert Manager da Fase 12, testes de resiliencia da Fase 13, aplicativo, UI, dashboard, provisionamento por app, captive portal, BLE, historico remoto, autenticacao de usuarios ou notificacoes.

Heartbeat nesta fase significa heartbeat local de rede do firmware, atualizado no System State e/ou emitido como evento local. Ele nao deve publicar MQTT, chamar cloud, persistir historico remoto, enviar notificacao ou criar dependencia externa.

O bloco `network` do System State deve seguir os campos existentes: `wifiConnected`, `mqttConnected`, `internetAvailable`, `ipAddress`, `rssi` e `lastHeartbeat`. A Fase 10 pode atualizar status de Wi-Fi, disponibilidade de Internet, IP, RSSI e heartbeat. `mqttConnected` deve permanecer falso ou inalterado por esta fase, pois conexao MQTT pertence a Fase 11.

Eventos de rede desta fase devem permanecer locais ao firmware e servir apenas para observabilidade interna, testes e integracao futura. Nenhum evento de rede deve publicar MQTT, acionar cloud, criar alerta centralizado ou persistir historico.

Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` deve ser alterado.

## Estrutura Recomendada

```text
firmware/
|-- include/
|   |-- network/
|   |   |-- network_types.h
|   |   |-- network_config.h
|   |   |-- wifi_adapter.h
|   |   |-- wifi_service.h
|   |   |-- network_events.h
|   |   |-- network_status_service.h
|   |   |-- ntp_client.h
|   |   |-- ntp_service.h
|   |   `-- network_heartbeat.h
|   |-- core/
|   |-- config/
|   `-- storage/
|-- src/
|   |-- network/
|   |   |-- arduino_wifi_adapter.h
|   |   |-- arduino_wifi_adapter.cpp
|   |   |-- wifi_service.cpp
|   |   |-- network_events.cpp
|   |   |-- network_status_service.cpp
|   |   |-- sntp_client.h
|   |   |-- sntp_client.cpp
|   |   |-- ntp_service.cpp
|   |   `-- network_heartbeat.cpp
|   |-- app/
|   `-- core/
`-- test/
    |-- fakes/
    |   |-- fake_wifi_adapter.h
    |   |-- fake_ntp_client.h
    |   `-- fake_internet_probe.h
    |-- unit/
    |   |-- test_network_contracts.cpp
    |   |-- test_wifi_service.cpp
    |   |-- test_network_reconnect_policy.cpp
    |   |-- test_ntp_service.cpp
    |   |-- test_network_status_service.cpp
    |   `-- test_network_heartbeat.cpp
    |-- integration/
    |   |-- test_network_boot_scheduler_integration.cpp
    |   |-- run_phase10_integration_tests.sh
    |   `-- run_phase10_scope_review.sh
    `-- hardware/
```

`firmware/test/hardware/` fica reservado para validacoes futuras e nao e criterio de conclusao da Fase 10.

## Tarefas

### ID

FW-P10-000

### Titulo

Definir contratos de rede, eventos locais, configuracao e fakes

### Objetivo

Criar os contratos publicos da camada de rede da Fase 10, incluindo Wi-Fi, status de conectividade, NTP, heartbeat local, eventos internos e boundaries substituiveis para testes automatizados sem rede real.

### Dependencias

Fase 2 concluida.

Fase 9 concluida.

### Descricao

Definir o modulo `network` com tipos funcionais para estado Wi-Fi, motivo de desconexao, resultado de conexao, politica de reconexao, status de Internet, status de sincronizacao NTP e heartbeat local.

Definir um boundary `WifiAdapter` ou equivalente para encapsular operacoes de Wi-Fi: iniciar interface, conectar com SSID e credencial configurados, desconectar, consultar status, IP, RSSI e motivo de falha quando disponivel. O boundary deve permitir uma implementacao real usando Wi-Fi do ESP32 e um fake em memoria para testes.

Definir um boundary `NtpClient` ou equivalente para iniciar sincronizacao, consultar se o horario esta sincronizado, obter timestamp quando disponivel e representar falhas sem bloquear o loop principal.

Definir um boundary de verificacao de Internet quando necessario, por exemplo `InternetProbe`, capaz de representar Internet disponivel, indisponivel, DNS falho, timeout e resultado desconhecido em testes. Essa verificacao nao deve depender de cloud REEFLOW, MQTT, Supabase ou aplicativo.

Declarar eventos locais de rede da Fase 10, como `WIFI_CONNECTED`, `WIFI_DISCONNECTED`, `WIFI_RECONNECTING`, `WIFI_RECONNECT_FAILED`, `NTP_SYNCED`, `NTP_SYNC_FAILED` e `NETWORK_HEARTBEAT`. Os nomes podem seguir o padrao de enums existente, mas devem permanecer locais ao firmware.

Definir configuracao local de rede com intervalos de tentativa, limites de backoff, timeout funcional, intervalo de heartbeat, intervalo de atualizacao de RSSI e servidores NTP default. Valores devem ser deterministas, testaveis e alteraveis por Config Manager quando ja houver contrato local.

Criar fakes ou mocks restritos a testes para Wi-Fi, NTP e probe de Internet. Os fakes devem permitir simular conexao bem sucedida, credencial ausente, falha de credencial, AP indisponivel, queda de conexao, IP atribuido, RSSI, Internet indisponivel, NTP sincronizado, NTP em timeout e contagem de chamadas.

Esta tarefa nao deve implementar Wi-Fi real, reconexao runtime, sincronizacao NTP real, heartbeat periodico, integracao de scheduler, MQTT, cloud, app, Alert Manager ou testes fisicos.

### Arquivos afetados

- `firmware/include/network/network_types.h`
- `firmware/include/network/network_config.h`
- `firmware/include/network/wifi_adapter.h`
- `firmware/include/network/wifi_service.h`
- `firmware/include/network/network_events.h`
- `firmware/include/network/network_status_service.h`
- `firmware/include/network/ntp_client.h`
- `firmware/include/network/ntp_service.h`
- `firmware/include/network/network_heartbeat.h`
- `firmware/src/network/network_events.cpp`
- `firmware/include/core/events/event_bus.h`, somente se necessario para registrar eventos `WIFI_*`, `NTP_*` e `NETWORK_HEARTBEAT`
- `firmware/src/core/events/event_bus.cpp`, somente se necessario para registrar eventos `WIFI_*`, `NTP_*` e `NETWORK_HEARTBEAT`
- `firmware/test/fakes/fake_wifi_adapter.h`
- `firmware/test/fakes/fake_ntp_client.h`
- `firmware/test/fakes/fake_internet_probe.h`
- `firmware/test/unit/test_network_contracts.cpp`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe modulo `network` com contratos publicos da Fase 10.
- Existe boundary substituivel para Wi-Fi.
- Existe boundary substituivel para NTP.
- Existe boundary substituivel ou contrato claro para verificacao de Internet.
- Contratos representam credencial ausente.
- Contratos representam falha de credencial.
- Contratos representam AP indisponivel.
- Contratos representam queda de conexao.
- Contratos representam IP atribuido.
- Contratos representam RSSI.
- Contratos representam Internet disponivel e indisponivel.
- Contratos representam NTP sincronizado e falha de sincronizacao.
- Existe configuracao local para timeout de conexao.
- Existe configuracao local para backoff de reconexao.
- Existe configuracao local para intervalo de heartbeat.
- Existe configuracao local para intervalo de atualizacao de status.
- Existem eventos locais `WIFI_*`, `NTP_*` e `NETWORK_HEARTBEAT`.
- Eventos de rede sao mapeados para o Event Bus local.
- Eventos de rede nao publicam MQTT.
- Eventos de rede nao criam Alert Manager.
- Eventos de rede nao persistem historico.
- Eventos de rede nao chamam cloud ou app.
- O bloco `network` do System State e a referencia de status runtime.
- `mqttConnected` nao e tratado como responsabilidade ativa da Fase 10.
- Existem fakes ou mocks de Wi-Fi, NTP e Internet restritos a testes.
- Os fakes permitem simular sucesso, falha e queda de conexao.
- Os fakes permitem inspecionar chamadas feitas pelos services.
- O build real do firmware nao depende de fakes ou mocks.
- Nenhuma implementacao real de Wi-Fi, NTP, MQTT, cloud ou app e criada nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario confirmando os tipos de estado Wi-Fi.
- Executar teste unitario confirmando os resultados de conexao.
- Executar teste unitario confirmando configuracao local de timeout, backoff e heartbeat.
- Executar teste unitario confirmando eventos `WIFI_*`, `NTP_*` e `NETWORK_HEARTBEAT` no Event Bus local.
- Executar teste unitario confirmando que eventos de rede nao publicam MQTT.
- Executar teste unitario confirmando que eventos de rede nao criam Alert Manager.
- Executar teste unitario confirmando fake Wi-Fi com conexao bem sucedida.
- Executar teste unitario confirmando fake Wi-Fi com AP indisponivel.
- Executar teste unitario confirmando fake Wi-Fi com queda de conexao.
- Executar teste unitario confirmando fake NTP sincronizado e timeout.
- Executar teste unitario confirmando fake Internet disponivel e indisponivel.
- Confirmar que nenhum teste exige ESP32, access point real, Internet real, NTP real, MQTT, cloud, app ou Serial real.
- Confirmar que o build real nao inclui fakes ou mocks.

---

### ID

FW-P10-001

### Titulo

Implementar Wi-Fi runtime e reconexao automatica sem bloquear automacoes locais

### Objetivo

Implementar o servico de Wi-Fi e a politica de reconexao automatica usando configuracao local restaurada, mantendo o firmware operacional mesmo quando a rede estiver ausente ou instavel.

### Dependencias

FW-P10-000.

FW-P09-002, ou tarefa equivalente da Fase 9 que restaure configuracao Wi-Fi no Config Manager.

### Descricao

Implementar `WifiService` ou equivalente para consumir configuracao Wi-Fi restaurada localmente pelo Config Manager. O servico deve iniciar conexao somente quando houver SSID e credenciais validas em memoria, sem criar UI, captive portal, BLE, endpoint de provisionamento, comando remoto ou dependencia de aplicativo.

Implementar adaptador real de Wi-Fi para ESP32 em `src/network/`, isolado atras do boundary definido em `FW-P10-000`. O adaptador real deve compilar no firmware destinado ao hardware, mas testes automatizados devem usar fake.

Implementar reconexao automatica com backoff progressivo e limite superior configuravel. A reconexao nao deve usar `delay`, busy wait, loops bloqueantes ou impedir que Scheduler, Watchdog, sensores, relays, ATO, modos, iluminacao e storage continuem executando.

Quando Wi-Fi conectar, atualizar `system.network.wifiConnected`, `ipAddress`, `rssi` e `internetAvailable` quando a informacao estiver disponivel. Quando desconectar, atualizar `wifiConnected`, limpar ou marcar IP como indisponivel conforme contrato local, preservar automacoes criticas e emitir evento local de desconexao.

Credencial ausente deve ser tratada como rede desabilitada ou nao configurada, sem log de erro repetitivo e sem loop agressivo de reconexao. Falhas de credencial, timeout, AP indisponivel e queda durante runtime devem produzir estados testaveis e eventos locais adequados.

Esta tarefa nao deve implementar NTP, heartbeat periodico, MQTT, publicacao de telemetria, recebimento de comandos, Alert Manager, cloud, app, provisionamento, testes de resiliencia da Fase 13 ou testes fisicos.

### Arquivos afetados

- `firmware/include/network/wifi_service.h`
- `firmware/include/network/wifi_adapter.h`
- `firmware/include/network/network_types.h`
- `firmware/include/network/network_config.h`
- `firmware/include/network/network_events.h`
- `firmware/src/network/arduino_wifi_adapter.h`
- `firmware/src/network/arduino_wifi_adapter.cpp`
- `firmware/src/network/wifi_service.cpp`
- `firmware/include/core/state/system_state.h`, somente se necessario para atualizar campos `network` ja existentes
- `firmware/src/core/state/system_state.cpp`, somente se necessario para atualizar campos `network` ja existentes
- `firmware/include/config/config_manager.h`, somente se necessario para expor configuracao Wi-Fi ja restaurada
- `firmware/src/config/config_manager.cpp`, somente se necessario para expor configuracao Wi-Fi ja restaurada
- `firmware/test/fakes/fake_wifi_adapter.h`
- `firmware/test/unit/test_wifi_service.cpp`
- `firmware/test/unit/test_network_reconnect_policy.cpp`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe `WifiService` ou equivalente.
- Existe adaptador real de Wi-Fi isolado atras do boundary.
- O service consome configuracao Wi-Fi local restaurada.
- Credencial ausente nao inicia tentativa agressiva de conexao.
- Credencial ausente nao bloqueia automacoes locais.
- Conexao bem sucedida marca `network.wifiConnected` como verdadeiro.
- Conexao bem sucedida registra IP quando disponivel.
- Conexao bem sucedida registra RSSI quando disponivel.
- Desconexao marca `network.wifiConnected` como falso.
- Desconexao nao altera estados criticos de sensores, relays, ATO, modos ou iluminacao.
- Falha de credencial e representada em estado ou resultado testavel.
- AP indisponivel e representado em estado ou resultado testavel.
- Timeout de conexao e representado em estado ou resultado testavel.
- Reconexao usa backoff progressivo.
- Backoff possui limite superior configuravel.
- Reconexao nao usa `delay`.
- Reconexao nao usa loop bloqueante.
- Scheduler continua podendo executar outras tarefas durante reconexao.
- Watchdog continua podendo ser alimentado durante reconexao.
- Eventos locais sao emitidos para conectado, desconectado, reconectando e falha de reconexao.
- Eventos de Wi-Fi nao publicam MQTT.
- Eventos de Wi-Fi nao criam Alert Manager.
- Eventos de Wi-Fi nao chamam cloud ou app.
- `network.mqttConnected` nao e marcado como verdadeiro por esta tarefa.
- Nenhum MQTT runtime, NTP, heartbeat, provisionamento, cloud, app ou teste fisico e implementado.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario de conexao Wi-Fi bem sucedida com fake.
- Executar teste unitario de credencial ausente.
- Executar teste unitario de falha de credencial.
- Executar teste unitario de AP indisponivel.
- Executar teste unitario de timeout de conexao.
- Executar teste unitario de queda de conexao durante runtime.
- Executar teste unitario confirmando atualizacao de `network.wifiConnected`.
- Executar teste unitario confirmando atualizacao de IP e RSSI.
- Executar teste unitario confirmando que desconexao nao altera estados de ATO, relays, modos ou iluminacao.
- Executar teste unitario confirmando backoff progressivo.
- Executar teste unitario confirmando limite superior de backoff.
- Executar teste unitario confirmando que reconexao nao bloqueia ticks do scheduler simulado.
- Executar teste unitario confirmando emissao de eventos locais `WIFI_*`.
- Executar teste unitario confirmando que `network.mqttConnected` nao fica verdadeiro.
- Confirmar que os testes usam fake Wi-Fi, sem ESP32, access point real, Internet real, MQTT, cloud, app ou Serial real.

---

### ID

FW-P10-002

### Titulo

Implementar NTP e fonte de tempo sincronizada opcional

### Objetivo

Adicionar sincronizacao NTP nao bloqueante para melhorar timestamps do firmware quando Wi-Fi estiver disponivel, sem tornar horario de Internet requisito para automacoes criticas.

### Dependencias

FW-P10-000.

FW-P10-001.

Fase 2 concluida.

### Descricao

Implementar `NtpService` ou equivalente usando o boundary `NtpClient` definido em `FW-P10-000`. O servico deve tentar sincronizacao somente quando Wi-Fi estiver conectado e a configuracao local permitir.

Implementar cliente real de NTP/SNTP para ESP32 atras do boundary, separado dos testes automatizados. O cliente real deve compilar para firmware de hardware, mas testes devem usar fake.

Integrar a sincronizacao ao Time Source existente quando houver contrato apropriado. Se o Time Source da Fase 2 for monotonic/local, a Fase 10 deve adicionar apenas um estado de tempo sincronizado ou adapter opcional, sem quebrar consumidores existentes.

Falha de NTP, timeout, servidor indisponivel, horario ainda nao sincronizado ou perda de Wi-Fi nao devem bloquear Scheduler, Watchdog, sensores, relays, ATO, modos, iluminacao, storage ou Wi-Fi service.

Emitir eventos locais `NTP_SYNCED` e `NTP_SYNC_FAILED` quando aplicavel. Esses eventos devem permanecer internos ao firmware e nao devem publicar MQTT, gerar alerta centralizado, persistir historico ou chamar cloud.

Esta tarefa nao deve implementar heartbeat, MQTT, telemetria, comandos remotos, Alert Manager, cloud, app, UI de configuracao, troca remota de timezone ou testes fisicos com servidor NTP real.

### Arquivos afetados

- `firmware/include/network/ntp_client.h`
- `firmware/include/network/ntp_service.h`
- `firmware/include/network/network_config.h`
- `firmware/include/network/network_events.h`
- `firmware/src/network/sntp_client.h`
- `firmware/src/network/sntp_client.cpp`
- `firmware/src/network/ntp_service.cpp`
- `firmware/include/core/platform/time_source.h`, somente se necessario para integracao opcional com contrato existente
- `firmware/src/core/platform/`, somente se necessario para adapter de tempo sincronizado
- `firmware/test/fakes/fake_ntp_client.h`
- `firmware/test/unit/test_ntp_service.cpp`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe `NtpService` ou equivalente.
- Existe cliente real de NTP/SNTP isolado atras do boundary.
- NTP so tenta sincronizar quando Wi-Fi esta conectado.
- NTP nao bloqueia o loop principal.
- NTP nao usa `delay` para aguardar sincronizacao.
- Timeout de NTP e representado em resultado testavel.
- Servidor indisponivel e representado em resultado testavel.
- Estado sincronizado e representado em resultado testavel.
- Perda de Wi-Fi cancela, adia ou ignora sincronizacao sem erro fatal.
- Time Source existente nao e quebrado.
- Consumidores locais continuam operando com tempo monotonic/local quando NTP falha.
- `NTP_SYNCED` e emitido quando sincronizacao ocorre.
- `NTP_SYNC_FAILED` e emitido quando sincronizacao falha apos politica definida.
- Eventos NTP nao publicam MQTT.
- Eventos NTP nao criam Alert Manager.
- Eventos NTP nao persistem historico.
- Eventos NTP nao chamam cloud ou app.
- Nenhum heartbeat, MQTT runtime, telemetria, comando remoto, cloud, app ou teste fisico e implementado.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario de NTP sincronizado com fake.
- Executar teste unitario de NTP em timeout.
- Executar teste unitario de servidor NTP indisponivel.
- Executar teste unitario confirmando que NTP nao inicia sem Wi-Fi conectado.
- Executar teste unitario confirmando que perda de Wi-Fi nao gera erro fatal.
- Executar teste unitario confirmando que Scheduler simulado continua executando durante tentativa NTP.
- Executar teste unitario confirmando emissao de `NTP_SYNCED`.
- Executar teste unitario confirmando emissao de `NTP_SYNC_FAILED`.
- Executar teste unitario confirmando que eventos NTP nao publicam MQTT.
- Executar teste unitario confirmando fallback para tempo local ou monotonic quando NTP falha.
- Confirmar que nenhum teste exige ESP32, access point real, servidor NTP real, Internet real, MQTT, cloud, app ou Serial real.

---

### ID

FW-P10-003

### Titulo

Implementar status de rede e heartbeat local no System State

### Objetivo

Atualizar o bloco `network` do System State de forma periodica e emitir heartbeat local de rede, sem publicar telemetria externa ou depender de MQTT.

### Dependencias

FW-P10-001.

FW-P10-002.

Fase 2 concluida.

### Descricao

Implementar `NetworkStatusService` ou equivalente para consolidar informacoes de Wi-Fi, Internet, IP, RSSI, NTP e heartbeat local no bloco `network` do System State.

Implementar `NetworkHeartbeat` ou equivalente para registrar `lastHeartbeat` em intervalo configuravel quando o firmware estiver vivo. O heartbeat deve ser local e derivado do scheduler/time source, sem publicar MQTT, sem enviar HTTP, sem chamar cloud e sem abrir socket remoto obrigatorio.

Atualizar `internetAvailable` usando o boundary de Internet definido em `FW-P10-000`, quando habilitado. A ausencia de Internet deve ser refletida em `network.internetAvailable`, mas nao deve desligar automacoes criticas ou impedir que o controlador opere localmente.

Atualizar RSSI e IP periodicamente quando Wi-Fi estiver conectado. Quando Wi-Fi estiver desconectado, o service deve refletir estado desconectado de forma deterministica e nao gerar spam de eventos.

Garantir que `network.mqttConnected` nao seja atualizado para verdadeiro nesta fase. A Fase 10 pode manter o valor default falso ou preservar o campo como responsabilidade futura da Fase 11.

Emitir `NETWORK_HEARTBEAT` local quando o heartbeat for atualizado. Esse evento nao deve publicar MQTT, persistir historico, criar Alert Manager, chamar cloud ou app.

Esta tarefa nao deve implementar MQTT, fila de mensagens, publicacao de telemetria, comandos remotos, Alert Manager, cloud, app, resiliencia da Fase 13 ou testes fisicos.

### Arquivos afetados

- `firmware/include/network/network_status_service.h`
- `firmware/include/network/network_heartbeat.h`
- `firmware/include/network/network_types.h`
- `firmware/include/network/network_config.h`
- `firmware/include/network/network_events.h`
- `firmware/src/network/network_status_service.cpp`
- `firmware/src/network/network_heartbeat.cpp`
- `firmware/include/core/state/system_state.h`, somente se necessario para helpers de atualizacao do bloco `network`
- `firmware/src/core/state/system_state.cpp`, somente se necessario para helpers de atualizacao do bloco `network`
- `firmware/test/fakes/fake_wifi_adapter.h`
- `firmware/test/fakes/fake_internet_probe.h`
- `firmware/test/unit/test_network_status_service.cpp`
- `firmware/test/unit/test_network_heartbeat.cpp`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe service de status de rede.
- Existe heartbeat local de rede.
- `network.wifiConnected` reflete o estado Wi-Fi consolidado.
- `network.internetAvailable` reflete o resultado do probe quando habilitado.
- `network.ipAddress` e atualizado quando Wi-Fi conectado possui IP.
- `network.rssi` e atualizado quando Wi-Fi conectado possui RSSI.
- `network.lastHeartbeat` e atualizado em intervalo configuravel.
- Heartbeat local nao publica MQTT.
- Heartbeat local nao envia HTTP.
- Heartbeat local nao chama cloud ou app.
- Heartbeat local nao persiste historico.
- `NETWORK_HEARTBEAT` e emitido localmente quando aplicavel.
- `NETWORK_HEARTBEAT` nao cria Alert Manager.
- Ausencia de Internet nao desliga automacoes criticas.
- Ausencia de Internet nao bloqueia Scheduler.
- Ausencia de Internet nao impede sensores, relays, ATO, modos ou iluminacao de continuarem.
- Wi-Fi desconectado produz status deterministico.
- Wi-Fi desconectado nao gera spam de eventos em cada tick.
- `network.mqttConnected` permanece falso ou inalterado pela Fase 10.
- Nenhum MQTT runtime, telemetria, comando remoto, cloud, app ou teste fisico e implementado.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario confirmando atualizacao de `wifiConnected`.
- Executar teste unitario confirmando atualizacao de `internetAvailable`.
- Executar teste unitario confirmando atualizacao de IP.
- Executar teste unitario confirmando atualizacao de RSSI.
- Executar teste unitario confirmando atualizacao de `lastHeartbeat`.
- Executar teste unitario confirmando intervalo configuravel de heartbeat.
- Executar teste unitario confirmando emissao de `NETWORK_HEARTBEAT`.
- Executar teste unitario confirmando que heartbeat nao publica MQTT.
- Executar teste unitario confirmando que Internet indisponivel nao altera estados de automacoes criticas.
- Executar teste unitario confirmando que Wi-Fi desconectado nao gera eventos repetidos a cada tick.
- Executar teste unitario confirmando que `network.mqttConnected` nao fica verdadeiro.
- Confirmar que nenhum teste exige ESP32, access point real, Internet real, NTP real, MQTT, cloud, app ou Serial real.

---

### ID

FW-P10-004

### Titulo

Integrar rede ao boot, scheduler, logger e watchdog

### Objetivo

Conectar os services de rede ao ciclo de vida do firmware, garantindo inicializacao ordenada, execucao periodica e degradacao segura quando rede estiver ausente.

### Dependencias

FW-P10-001.

FW-P10-002.

FW-P10-003.

Fase 9 concluida.

### Descricao

Integrar os services de Wi-Fi, NTP, status de rede e heartbeat ao app shell ou ponto equivalente de inicializacao do firmware. A ordem deve respeitar a restauracao local de configuracao da Fase 9 antes de iniciar tentativa de Wi-Fi.

Registrar tarefas periodicas no Task Scheduler para conexao/reconexao Wi-Fi, atualizacao de status, tentativa NTP e heartbeat local. As tarefas devem ser curtas, nao bloqueantes e compativeis com alimentacao do Watchdog.

Adicionar logs funcionais de rede em nivel adequado para boot, conectado, desconectado, reconectando, NTP sincronizado, NTP falho e heartbeat quando util. Logs nao devem conter credenciais, senha Wi-Fi, tokens, payloads sensiveis ou spam por tick.

Garantir que falhas de rede durante boot nao impeçam o firmware de inicializar automacoes locais. Sem credencial Wi-Fi, o firmware deve inicializar normalmente com rede desconectada. Com credencial invalida ou AP ausente, o firmware deve registrar estado e continuar operando localmente.

Garantir que a integracao nao inicie MQTT, nao publique telemetria, nao abra broker, nao receba comando remoto, nao crie Alert Manager, nao chame cloud, nao dependa de app e nao altere fases ja concluidas fora do necessario para integracao.

Esta tarefa deve incluir testes de integracao host-side com fakes e script de validacao da Fase 10. Testes fisicos reais ficam fora do criterio de conclusao.

### Arquivos afetados

- `firmware/src/app/`, arquivos de app shell ou inicializacao existentes
- `firmware/include/network/wifi_service.h`
- `firmware/src/network/wifi_service.cpp`
- `firmware/include/network/ntp_service.h`
- `firmware/src/network/ntp_service.cpp`
- `firmware/include/network/network_status_service.h`
- `firmware/src/network/network_status_service.cpp`
- `firmware/include/network/network_heartbeat.h`
- `firmware/src/network/network_heartbeat.cpp`
- `firmware/include/core/scheduler/`, somente se necessario para registrar tarefas de rede
- `firmware/src/core/scheduler/`, somente se necessario para registrar tarefas de rede
- `firmware/test/integration/test_network_boot_scheduler_integration.cpp`
- `firmware/test/integration/run_phase10_integration_tests.sh`
- `firmware/test/integration/run_phase10_scope_review.sh`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Services de rede sao inicializados pelo app shell ou ponto equivalente.
- Configuracao Wi-Fi restaurada pela Fase 9 e consultada antes de iniciar Wi-Fi.
- Ausencia de configuracao Wi-Fi nao falha o boot.
- Credencial invalida nao falha o boot.
- AP indisponivel nao falha o boot.
- Scheduler possui tarefa ou tarefas para Wi-Fi/reconexao.
- Scheduler possui tarefa ou tarefas para status de rede.
- Scheduler possui tarefa ou tarefas para NTP quando aplicavel.
- Scheduler possui tarefa ou tarefas para heartbeat local.
- Tarefas de rede sao nao bloqueantes.
- Watchdog continua sendo alimentado conforme contrato existente.
- Logs de rede nao incluem senha Wi-Fi.
- Logs de rede nao incluem tokens ou credenciais sensiveis.
- Logs de falha nao fazem spam por tick.
- Automacoes criticas continuam inicializando sem rede.
- Sensores, relays, ATO, modos, iluminacao e storage nao passam a depender de Wi-Fi.
- MQTT nao e inicializado.
- Broker MQTT nao e aberto.
- Telemetria MQTT nao e publicada.
- Comandos remotos nao sao recebidos.
- Alert Manager nao e criado.
- Cloud e app nao sao chamados.
- Existe script de validacao host-side da Fase 10.
- Existe revisao automatizada de escopo para bloquear MQTT, cloud, app, Alert Manager e testes fisicos obrigatorios.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar testes unitarios da Fase 10 com sucesso.
- Executar teste de integracao de boot com configuracao Wi-Fi presente.
- Executar teste de integracao de boot sem configuracao Wi-Fi.
- Executar teste de integracao de boot com credencial invalida simulada.
- Executar teste de integracao de AP indisponivel durante boot.
- Executar teste de integracao confirmando tarefas de rede registradas no Scheduler.
- Executar teste de integracao confirmando que tarefas de rede nao bloqueiam tarefas locais simuladas.
- Executar teste de integracao confirmando que Watchdog continua operacional.
- Executar teste de integracao confirmando que logs nao incluem senha ou token.
- Executar teste de integracao confirmando que MQTT nao e inicializado.
- Executar teste de integracao confirmando que cloud/app nao sao chamados.
- Executar `firmware/test/integration/run_phase10_integration_tests.sh`.
- Executar `firmware/test/integration/run_phase10_scope_review.sh`.
- Confirmar que nenhum teste exige ESP32, access point real, Internet real, NTP real, MQTT, cloud, app, Serial real ou qualquer validacao fisica.

---

### ID

FW-P10-005

### Titulo

Fechar validacao da Fase 10 e documentar pendencias de hardware

### Objetivo

Consolidar a validacao automatizada da Fase 10, revisar limites de escopo e registrar explicitamente o que fica pendente para validacao real em hardware.

### Dependencias

FW-P10-004.

### Descricao

Executar revisao final da Fase 10 verificando que Wi-Fi, reconexao, NTP, heartbeat e status de rede foram implementados somente dentro do firmware, sem iniciar MQTT, cloud, app, Alert Manager, resiliencia da Fase 13 ou provisionamento futuro.

Garantir que os testes host-side cubram contratos, Wi-Fi service, politica de reconexao, NTP, status de rede, heartbeat, integracao com boot/scheduler e revisao de escopo.

Garantir que toda validacao que dependa de access point real, DHCP real, RSSI real, Internet real, NTP publico, ESP32, antena, fonte, caixa, aquario, Serial real ou bancada esteja listada como `Pendente para Hardware Validation` e nao seja criterio de conclusao da fase.

Conferir que `network.mqttConnected` nao e controlado como conexao real nesta fase e que nenhuma telemetria MQTT foi adicionada.

Esta tarefa nao deve criar novas funcionalidades. Deve apenas fechar validacao, escopo e evidencia de conclusao da Fase 10.

### Arquivos afetados

- `firmware/test/unit/test_network_contracts.cpp`
- `firmware/test/unit/test_wifi_service.cpp`
- `firmware/test/unit/test_network_reconnect_policy.cpp`
- `firmware/test/unit/test_ntp_service.cpp`
- `firmware/test/unit/test_network_status_service.cpp`
- `firmware/test/unit/test_network_heartbeat.cpp`
- `firmware/test/integration/test_network_boot_scheduler_integration.cpp`
- `firmware/test/integration/run_phase10_integration_tests.sh`
- `firmware/test/integration/run_phase10_scope_review.sh`
- `firmware/README.md`, somente se ja existir secao de validacao por fase e for necessario registrar comandos de teste

### Criterios de aceitacao

- Build PlatformIO da firmware passa.
- Testes unitarios da Fase 10 passam.
- Testes de integracao host-side da Fase 10 passam.
- Scope review da Fase 10 passa.
- Validacao cobre Wi-Fi configurado.
- Validacao cobre Wi-Fi sem credencial.
- Validacao cobre falha de conexao.
- Validacao cobre queda e reconexao.
- Validacao cobre backoff progressivo.
- Validacao cobre NTP sincronizado.
- Validacao cobre NTP indisponivel.
- Validacao cobre heartbeat local.
- Validacao cobre status `network` no System State.
- Validacao cobre ausencia de bloqueio em automacoes locais.
- Revisao confirma que MQTT runtime nao foi implementado.
- Revisao confirma que telemetria MQTT nao foi publicada.
- Revisao confirma que comandos remotos nao foram implementados.
- Revisao confirma que Alert Manager nao foi implementado.
- Revisao confirma que cloud e app nao foram chamados.
- Revisao confirma que provisionamento futuro nao foi implementado.
- Revisao confirma que testes fisicos nao sao criterio de conclusao.
- Pendencias reais estao marcadas como `Pendente para Hardware Validation`.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar todos os testes unitarios da Fase 10.
- Executar `firmware/test/integration/run_phase10_integration_tests.sh`.
- Executar `firmware/test/integration/run_phase10_scope_review.sh`.
- Confirmar por busca automatizada que nao ha inicializacao MQTT adicionada pela Fase 10.
- Confirmar por busca automatizada que nao ha publicacao MQTT adicionada pela Fase 10.
- Confirmar por busca automatizada que nao ha chamadas para cloud, app, Supabase, bridge, push notification ou dashboard.
- Confirmar por busca automatizada que nao ha dependencia obrigatoria de access point real, Internet real, servidor NTP real, ESP32 conectado, Serial real ou teste fisico.
- Registrar como `Pendente para Hardware Validation`: conexao com roteador real, DHCP real, RSSI real, reconexao real por perda de sinal, sincronizacao com NTP publico, estabilidade de longo prazo em ESP32, efeito de caixa/antena e comportamento em bancada real.
