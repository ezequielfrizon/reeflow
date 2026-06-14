# Fase 8 - Sistema de Iluminacao

## Escopo

Implementar o modulo funcional de iluminacao do REEFLOW para controlar a luminaria LED por PWM nos quatro canais oficiais da Fase 8: Branco no GPIO25, Azul no GPIO26, Royal Blue no GPIO27 e UV no GPIO14. A fase deve fornecer controle manual local, modo automatico, modo de aclimatacao, curvas por canal, sunrise, sunset, moonlight, limite maximo de intensidade, fade suave obrigatorio, atualizacao do bloco `lighting` do System State e eventos locais `LIGHTING_*`.

Esta fase segue a ordem oficial do roadmap apos a Fase 7, mas a dependencia tecnica minima definida no master plan para iluminacao e a Fase 1, pelos contratos e boundary PWM, e a Fase 2, pelo System State, Event Bus, Config Manager, Scheduler, Logger, Watchdog e Time Source. A Fase 8 nao deve alterar o comportamento de modos, ATO ou reles.

Esta fase e puramente de desenvolvimento e nao exige ESP32 conectado, luminaria real, MOSFET real, fonte 12V, carga LED, multimetro, osciloscopio, analisador logico, medicao de duty real, monitor Serial real, Wi-Fi, MQTT, cloud ou aplicativo.

Mocks e fakes devem ser usados para validacoes automatizadas das implementacoes, seguindo o padrao das fases anteriores. Mocks e fakes devem ficar restritos a `firmware/test/` ou a configuracoes de teste, sem se tornarem dependencia do build real do firmware destinado ao hardware.

O firmware real deve ficar preparado para controlar a luminaria fisica futuramente, mas a conclusao desta fase nao pode depender de teste fisico. Qualquer verificacao com LED real, MOSFET real, fonte 12V, corrente, temperatura de dissipador, resposta visual, medicao eletrica, ruido eletrico, Serial real ou seguranca de bancada deve ficar registrada como `Pendente para Hardware Validation` e fora dos criterios de conclusao desta fase.

Nao implementar NVS Preferences definitivo da Fase 9, Wi-Fi, MQTT, Alert Manager, cloud, aplicativo, historico, notificacoes, comandos remotos, resiliencia integrada ou funcionalidades de fases posteriores. As curvas, horarios, perfis e limites devem operar em memoria via contratos da Fase 8, Config Manager e System State nesta fase. A Fase 8 deve deixar esses dados preparados para persistencia futura, mas a gravacao, restauracao pos-reboot e politica de escrita em flash ficam reservadas para a Fase 9.

Os nomes canonicos do modulo funcional devem seguir `specs/lighting-spec.md` e `specs/system-state-spec.md`: canais `white`, `blue`, `royalBlue` e `uv`; modos `MANUAL`, `AUTOMATIC` e `ACCLIMATION`; eventos locais `LIGHTING_PROFILE_CHANGED`, `LIGHTING_STARTED` e `LIGHTING_STOPPED`.

O contrato de hardware V1 registra o GPIO14 como `PWM Moonlight/UV`, enquanto a spec funcional chama o canal de `UV`. Nesta fase, o canal funcional deve ser `UV` e usar GPIO14. `Moonlight` deve ser tratado como comportamento de curva/perfil de baixa intensidade, nao como quinto canal novo no System State. O PWM Reserva no GPIO13 nao deve ser usado pelo modulo funcional da Fase 8.

O modulo funcional em `modules/lighting` nao deve depender de tipos, resultados ou rotinas de diagnostico da Fase 1. Diagnosticos de bring-up e modulo funcional podem reutilizar contratos de pinout e boundary PWM quando adequado, mas nao devem compartilhar enums de diagnostico como contrato funcional.

Esta fase nao deve criar `lighting_store`, backend NVS, repositorio de perfis persistente ou qualquer boundary de storage definitivo. A evolucao para persistencia deve consumir os contratos de perfil e configuracao definidos nesta fase sem exigir movimentacao ampla dos arquivos de iluminacao.

Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` deve ser alterado.

## Estrutura Recomendada

```text
firmware/
|-- include/
|   |-- modules/
|   |   `-- lighting/
|   |       |-- lighting_types.h
|   |       |-- lighting_config.h
|   |       |-- lighting_events.h
|   |       |-- lighting_pwm_controller.h
|   |       |-- lighting_profile.h
|   |       |-- lighting_policy.h
|   |       `-- lighting_service.h
|   |-- core/
|   |-- config/
|   `-- contracts/
|-- src/
|   |-- modules/
|   |   `-- lighting/
|   |       |-- lighting_events.cpp
|   |       |-- lighting_policy.cpp
|   |       `-- lighting_service.cpp
|   |-- drivers/
|   |   `-- lighting/
|   |       |-- ledc_lighting_pwm_controller.h
|   |       `-- ledc_lighting_pwm_controller.cpp
|   |-- app/
|   `-- core/
`-- test/
    |-- fakes/
    |   `-- fake_lighting_pwm_controller.h
    |-- unit/
    |   |-- test_lighting_contracts.cpp
    |   |-- test_lighting_pwm_controller.cpp
    |   |-- test_lighting_policy.cpp
    |   `-- test_lighting_service.cpp
    |-- integration/
    |   |-- test_lighting_scheduler_integration.cpp
    |   |-- run_phase8_integration_tests.sh
    |   `-- run_phase8_scope_review.sh
    `-- hardware/
```

`firmware/test/hardware/` fica reservado para validacoes futuras e nao e criterio de conclusao da Fase 8.

## Tarefas

### ID

FW-P08-000

### Titulo

Definir contratos funcionais, eventos, configuracao local e fakes de iluminacao

### Objetivo

Criar os contratos minimos do modulo de iluminacao, incluindo canais, modos, comandos locais, perfis, pontos de curva, eventos locais, configuracao em memoria e boundary substituivel de PWM, permitindo validacao automatizada sem hardware e sem acoplamento aos diagnosticos da Fase 1.

### Dependencias

Fase 1 concluida.

Fase 2 concluida.

Fase 7 concluida por ordem oficial do roadmap.

### Descricao

Definir o modulo `modules/lighting` com os quatro canais canonicos do System State: `WHITE`, `BLUE`, `ROYAL_BLUE` e `UV`. Os tipos publicos devem mapear para `system.lighting.white`, `system.lighting.blue`, `system.lighting.royalBlue` e `system.lighting.uv`, sem criar canal separado para moonlight e sem usar o GPIO13 de PWM Reserva.

Definir tipos funcionais para os modos `MANUAL`, `AUTOMATIC` e `ACCLIMATION`, compativeis com `core::state::LightingMode`. A nomenclatura do firmware pode seguir o padrao de enums existente, mas o contrato publico deve permanecer semanticamente alinhado a spec.

Definir um contrato de comando local de iluminacao contendo modo solicitado, perfil alvo, canal alvo quando aplicavel, duty ou intensidade solicitada, origem local da solicitacao, instante da solicitacao e motivo funcional. Nesta fase, a entrada implementada deve ser local ao firmware. MQTT, app, cloud, Serial command shell e comandos remotos nao devem ser implementados.

Definir os contratos de perfil e curva. Um perfil deve conter nome, horario de inicio, horario de termino, flags de sunrise, sunset e aclimatacao, limite maximo global de intensidade, duracao de aclimatacao e curvas por canal. Cada curva deve permitir pontos de tempo em minuto do dia e alvo de PWM ou intensidade normalizada, com validacao de ordem crescente.

O contrato deve declarar limites fechados para permitir implementacao por agente sem ambiguidades: quantidade maxima de perfis em memoria, quantidade maxima de pontos por curva, tamanho maximo do nome de perfil, minuto do dia na faixa `0..1439`, duty dentro da faixa funcional da resolucao PWM, intensidade percentual dentro de `0..100` e regra para curva vazia. Esses limites devem ser compativeis com persistencia futura da Fase 9, mas nao devem implementar storage real nesta fase.

Declarar os eventos locais `LIGHTING_PROFILE_CHANGED`, `LIGHTING_STARTED` e `LIGHTING_STOPPED`, mapeados para o Event Bus local. Os eventos devem carregar, quando necessario, perfil, modo, canais afetados e instante, sem publicar MQTT, sem criar Alert Manager e sem persistir historico.

Definir `lighting_config.h` com configuracao local em memoria para intervalo de avaliacao, parametros PWM funcionais, passo maximo de fade por avaliacao e limites de duty. A frequencia, resolucao e faixa de duty funcionais da Fase 8 devem ser declaradas localmente no modulo ou em configuracao apropriada da Fase 8, sem depender dos parametros de bring-up como autoridade funcional.

Preparar fakes ou mocks restritos aos testes para validar comandos PWM por canal. O fake deve permitir simular configuracao de canal, escrita de duty, escrita de duty zero para todos os canais, falha em canal especifico, historico de comandos e estado atual por canal.

Esta tarefa nao deve implementar controlador LEDC real, politica de curvas, atualizacao do System State, emissao runtime de eventos, scheduler, `lighting_store`, NVS, MQTT, app, cloud ou diagnostico fisico.

### Arquivos afetados

- `firmware/include/modules/lighting/lighting_types.h`
- `firmware/include/modules/lighting/lighting_config.h`
- `firmware/include/modules/lighting/lighting_events.h`
- `firmware/include/modules/lighting/lighting_pwm_controller.h`
- `firmware/include/modules/lighting/lighting_profile.h`
- `firmware/include/modules/lighting/lighting_policy.h`
- `firmware/include/modules/lighting/lighting_service.h`
- `firmware/src/modules/lighting/lighting_events.cpp`
- `firmware/include/config/config_manager.h`, somente se ajuste de validacao de iluminacao em memoria for necessario
- `firmware/src/config/config_manager.cpp`, somente se ajuste de validacao de iluminacao em memoria for necessario
- `firmware/test/fakes/fake_lighting_pwm_controller.h`
- `firmware/test/unit/test_lighting_contracts.cpp`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe modulo `modules/lighting` com contratos publicos da Fase 8.
- Existem tipos funcionais para Branco, Azul, Royal Blue e UV.
- Cada canal funcional possui mapeamento claro para o campo correspondente em `system.lighting`.
- O canal UV usa a semantica funcional de `specs/lighting-spec.md`.
- Moonlight e representado como comportamento de perfil ou curva, nao como canal novo.
- O PWM Reserva no GPIO13 nao e exposto como canal funcional da Fase 8.
- Existem tipos funcionais para `MANUAL`, `AUTOMATIC` e `ACCLIMATION`.
- Os tipos funcionais sao compativeis com `core::state::LightingMode`.
- Nenhum modo de iluminacao novo e criado.
- Existe contrato de comando local de iluminacao.
- O contrato de comando nao implementa MQTT, app, cloud ou Serial command shell.
- Existe contrato de perfil de iluminacao com nome e horarios.
- Existe limite maximo de perfis em memoria.
- Existe tamanho maximo para nome de perfil.
- Existe contrato de curva por canal.
- Existe limite maximo de pontos por curva.
- Pontos de curva possuem minuto do dia e alvo de intensidade ou PWM.
- Pontos de curva aceitam somente minuto do dia dentro de `0..1439`.
- Existe validacao contratual para pontos de curva ordenados.
- Existe regra explicita para curva vazia.
- Existe limite maximo global de intensidade.
- Intensidade percentual aceita somente valores dentro de `0..100`.
- Existe limite maximo por canal.
- Duty aceito respeita a faixa funcional da resolucao PWM.
- Os contratos de perfil e curva ficam preparados para persistencia futura pela Fase 9.
- Nenhum `lighting_store`, backend NVS ou repositorio persistente e criado.
- Existe configuracao local para fade suave.
- Existe configuracao local para intervalo de avaliacao.
- Existem eventos locais `LIGHTING_PROFILE_CHANGED`, `LIGHTING_STARTED` e `LIGHTING_STOPPED`.
- Eventos de iluminacao sao mapeados para o Event Bus local.
- Eventos de iluminacao nao publicam MQTT.
- Eventos de iluminacao nao criam Alert Manager.
- Eventos de iluminacao nao persistem historico.
- Existe interface funcional de controlador PWM substituivel em testes.
- A interface permite configurar canais PWM.
- A interface permite escrever duty por canal.
- A interface permite zerar todos os canais funcionais.
- A interface representa sucesso e falha do controlador.
- A interface representa canal desconhecido ou invalido.
- A interface nao expoe tipos de diagnostico da Fase 1.
- A interface nao depende de Arduino, Serial real, Wi-Fi, MQTT, cloud, aplicativo, NVS, ATO, reles ou modos.
- Existe fake ou mock para simular PWM sem hardware.
- O fake permite inspecionar comandos recebidos em testes.
- O build real do firmware nao depende de fakes ou mocks.
- Nenhum comportamento funcional de iluminacao e iniciado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario confirmando os quatro canais canonicos.
- Executar teste unitario confirmando mapeamento dos canais para `system.lighting`.
- Executar teste unitario confirmando que GPIO13 nao e canal funcional da Fase 8.
- Executar teste unitario confirmando os tres modos canonicos de iluminacao.
- Executar teste unitario confirmando eventos `LIGHTING_*` no Event Bus local.
- Executar teste unitario confirmando que eventos de iluminacao nao publicam MQTT.
- Executar teste unitario confirmando validacao de pontos de curva ordenados.
- Executar teste unitario confirmando rejeicao de minuto do dia fora de `0..1439`.
- Executar teste unitario confirmando limite maximo de pontos por curva.
- Executar teste unitario confirmando limite maximo de perfis em memoria.
- Executar teste unitario confirmando rejeicao de curva com horario invalido.
- Executar teste unitario confirmando regra explicita de curva vazia.
- Executar teste unitario confirmando limite maximo global dentro da faixa valida.
- Executar teste unitario confirmando intensidade percentual fora de `0..100`.
- Executar teste unitario confirmando limite maximo por canal dentro da faixa valida.
- Executar teste unitario confirmando que o fake registra escrita PWM por canal.
- Executar teste unitario confirmando que o fake registra `allOff` ou duty zero em todos os canais.
- Executar teste unitario confirmando falha simulada em canal especifico.
- Executar teste unitario confirmando rejeicao de canal invalido.
- Confirmar que nenhum teste exige ESP32, luminaria real, MOSFET, fonte 12V, medicao eletrica, Wi-Fi, MQTT, cloud, app ou Serial real.
- Confirmar que o build real nao inclui o fake ou mock de teste.

---

### ID

FW-P08-001

### Titulo

Implementar controlador PWM real da luminaria com LEDC e pinout oficial

### Objetivo

Criar a implementacao real do controlador funcional de iluminacao usando o boundary PWM/LEDC existente, o pinout oficial e os parametros funcionais da Fase 8, preparada para hardware futuro e validavel por fake PWM sem luminaria conectada.

### Dependencias

FW-P08-000.

FW-P01-005B.

### Descricao

Implementar o controlador real em `drivers/lighting/`, separado do servico funcional. O controlador deve implementar a interface definida em `FW-P08-000` e usar `PwmLedcPort` ou boundary equivalente para configurar canais LEDC, anexar pinos e escrever duty.

O controlador deve buscar os GPIOs oficiais em `hardware_pins.h`: Branco no GPIO25, Azul no GPIO26, Royal Blue no GPIO27 e UV no GPIO14. Numeros de GPIO nao devem ser duplicados em modulo funcional. A observacao `PWM Moonlight/UV` do contrato de hardware deve ser preservada como mapeamento do canal funcional UV, sem alterar arquitetura ou specs.

O controlador deve configurar frequencia, resolucao e canais LEDC conforme a configuracao funcional da Fase 8. Esses parametros nao devem depender da rotina de diagnostico de bring-up como contrato de negocio, ainda que possam reutilizar o mesmo boundary PWM.

Toda inicializacao do controlador deve deixar os quatro canais funcionais em duty zero antes de qualquer valor acima de zero. A operacao para zerar todos os canais deve ser deterministica e testavel. O PWM Reserva no GPIO13 nao deve ser configurado ou comandado por este controlador funcional.

Como a validacao desta fase nao depende de hardware, falhas de controlador devem ser simuladas por fake do boundary funcional ou por fake PWM nos testes. Se o boundary PWM/LEDC existente nao retornar status de escrita ou configuracao, a implementacao real nao deve inventar falha fisica de LEDC. Nesse caso, falhas funcionais de controlador ficam restritas ao boundary funcional de iluminacao e aos fakes usados nos testes do servico. A implementacao real deve compilar para o firmware destinado ao hardware sem incluir mocks.

Esta tarefa nao deve atualizar System State, nao deve emitir eventos, nao deve calcular curvas, nao deve registrar scheduler e nao deve implementar NVS, MQTT, app, cloud ou diagnostico fisico.

### Arquivos afetados

- `firmware/src/drivers/lighting/ledc_lighting_pwm_controller.h`
- `firmware/src/drivers/lighting/ledc_lighting_pwm_controller.cpp`
- `firmware/include/modules/lighting/lighting_pwm_controller.h`
- `firmware/include/modules/lighting/lighting_types.h`
- `firmware/include/modules/lighting/lighting_config.h`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/src/drivers/io/pwm_ledc_interface.h`
- `firmware/src/drivers/io/arduino_hardware_io.h`
- `firmware/src/drivers/io/arduino_hardware_io.cpp`
- `firmware/test/fakes/fake_lighting_pwm_controller.h`
- `firmware/test/unit/test_lighting_pwm_controller.cpp`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe implementacao real do controlador PWM funcional da luminaria.
- O controlador real implementa a interface definida em `FW-P08-000`.
- O controlador real usa boundary PWM/LEDC substituivel em testes.
- O controlador real configura os canais PWM antes de escrever duty.
- Branco usa GPIO25 vindo do contrato oficial.
- Azul usa GPIO26 vindo do contrato oficial.
- Royal Blue usa GPIO27 vindo do contrato oficial.
- UV usa GPIO14 vindo do contrato oficial.
- GPIO14 e tratado como canal funcional UV mesmo quando o contrato de hardware observa `Moonlight/UV`.
- GPIO13 PWM Reserva nao e configurado pelo controlador funcional da Fase 8.
- Numeros de GPIO nao sao duplicados fora do contrato de hardware.
- O controlador usa frequencia e resolucao funcionais da Fase 8.
- O controlador nao usa parametros de bring-up como contrato funcional obrigatorio.
- A inicializacao aplica duty zero nos quatro canais funcionais.
- A operacao de zerar todos os canais aplica duty zero em ordem deterministica.
- Duty solicitado acima do limite funcional e rejeitado ou limitado de forma deterministica pelo contrato do controlador.
- Falha fisica de LEDC real nao e inventada quando o boundary PWM existente nao retorna erro.
- Falhas de controlador sao representadas no boundary funcional de iluminacao e validadas com fakes.
- O controlador nao expoe enums ou tipos de diagnostico da Fase 1 como contrato funcional.
- O firmware compila sem exigir luminaria fisicamente conectada.
- A tarefa nao atualiza System State, nao publica eventos e nao inicializa funcionalidades futuras.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste unitario do controlador usando fake PWM/LEDC.
- Testar que Branco configura GPIO25.
- Testar que Azul configura GPIO26.
- Testar que Royal Blue configura GPIO27.
- Testar que UV configura GPIO14.
- Testar que GPIO13 nao recebe setup, attach ou write pelo controlador funcional.
- Testar que a frequencia funcional da Fase 8 e aplicada.
- Testar que a resolucao funcional da Fase 8 e aplicada.
- Testar que inicializacao escreve duty zero nos quatro canais funcionais.
- Testar escrita de duty por canal.
- Testar rejeicao ou limitacao deterministica de duty fora da faixa.
- Testar que falha simulada do controlador funcional nao depende de erro fisico real do LEDC.
- Testar que `allOff` escreve duty zero nos quatro canais funcionais.
- Testar rejeicao de canal invalido.
- Confirmar que o teste nao depende de luminaria real, MOSFET, fonte 12V, medicao eletrica, Serial real ou ESP32 conectado.
- Confirmar que mocks usados no teste nao permanecem como dependencia do build real.

---

### ID

FW-P08-002

### Titulo

Implementar politica pura de curvas, aclimatacao, moonlight e fade suave

### Objetivo

Criar a regra deterministica que calcula os alvos PWM da luminaria a partir de modo, perfil, curvas, horarios, aclimatacao, limite maximo e estado atual, aplicando fade suave sem depender de hardware, scheduler, Arduino, NVS ou MQTT.

### Dependencias

FW-P08-000.

FW-P02-001.

FW-P02-003.

### Descricao

Implementar `LightingPolicy` como regra pura. A politica deve receber uma visao imutavel de `system.lighting`, configuracao de iluminacao em memoria, perfil ativo, instante atual, comando local opcional e parametros locais do modulo, retornando uma decisao funcional com alvos PWM por canal e motivos de decisao. A politica nao deve emitir eventos, nao deve decidir deduplicacao de eventos e nao deve retornar metadados de Event Bus; essa responsabilidade pertence ao `LightingService`.

No modo `MANUAL`, a politica deve aceitar comandos locais explicitos para definir alvos por canal ou para aplicar um perfil manual, sempre respeitando canal habilitado, limite maximo por canal, limite maximo global de intensidade e faixa de duty valida. Comandos idempotentes nao devem solicitar escrita PWM nem transicao duplicada.

No modo `AUTOMATIC`, a politica deve calcular alvos por canal a partir das curvas configuradas e do minuto atual do dia. Curvas personalizadas devem permitir interpolacao deterministica entre pontos. Quando o horario estiver dentro da janela de sunrise, os alvos devem subir gradualmente. Quando estiver dentro da janela de sunset, os alvos devem cair gradualmente. Durante o periodo principal, a curva do perfil deve prevalecer.

`Moonlight` deve ser calculado como comportamento de baixa intensidade fora do periodo principal, usando curvas ou alvo configurado do perfil nos canais permitidos, sem criar canal novo. O canal funcional UV pode participar do moonlight se o perfil assim definir, mas a politica nao deve assumir um quinto canal.

No modo `ACCLIMATION`, a politica deve aplicar um fator gradual sobre os alvos calculados, limitado pela duracao de aclimatacao em dias e pelo progresso injetado pela configuracao em memoria. Aclimatacao nao deve ultrapassar o limite maximo global nem o limite maximo por canal.

O fade suave obrigatorio deve ser aplicado entre o PWM atual do System State e o alvo calculado. A politica deve limitar a variacao por avaliacao usando o passo maximo configurado, evitando salto abrupto de duty. Quando o alvo for menor que o atual, o fade deve reduzir duty gradualmente. Quando o alvo for maior, deve aumentar duty gradualmente. Quando o alvo for igual ao atual, nao deve solicitar escrita.

A politica deve tratar configuracao invalida de forma segura e deterministica: comandos manuais invalidos devem ser rejeitados preservando os alvos atuais; perfil automatico ou de aclimatacao invalido nao deve iniciar novos duty acima de zero; se algum canal ja estiver ativo sob perfil automatico ou de aclimatacao invalido, a decisao deve convergir esse canal para duty zero usando o mesmo fade suave. Canais desabilitados tambem devem convergir para duty zero por fade. Configuracao invalida nao deve criar estados novos no System State.

Esta tarefa nao deve escrever PWM, atualizar System State, emitir eventos no Event Bus, registrar scheduler, persistir configuracao, publicar MQTT ou acessar hardware.

### Arquivos afetados

- `firmware/include/modules/lighting/lighting_policy.h`
- `firmware/include/modules/lighting/lighting_profile.h`
- `firmware/include/modules/lighting/lighting_types.h`
- `firmware/include/modules/lighting/lighting_config.h`
- `firmware/src/modules/lighting/lighting_policy.cpp`
- `firmware/test/unit/test_lighting_policy.cpp`

### Criterios de aceitacao

- A politica e uma regra pura testavel sem hardware.
- A politica usa `system.lighting` como entrada de estado atual.
- A politica usa configuracao de iluminacao em memoria.
- A politica usa perfil ativo e curvas por canal.
- A politica usa tempo injetado.
- A politica retorna alvos PWM e motivos de decisao, nao metadados de Event Bus.
- A politica nao le GPIO.
- A politica nao escreve PWM diretamente.
- A politica nao emite eventos e nao decide deduplicacao de eventos.
- A politica nao acessa NVS.
- A politica nao acessa MQTT, Wi-Fi, cloud ou app.
- Modo `MANUAL` aceita alvo local por canal.
- Modo `MANUAL` respeita canal desabilitado.
- Modo `MANUAL` respeita limite maximo por canal.
- Modo `MANUAL` respeita limite maximo global de intensidade.
- Comando manual idempotente nao solicita escrita PWM duplicada.
- Comando manual invalido preserva os alvos atuais.
- Modo `AUTOMATIC` calcula alvos por curva.
- Curvas por canal usam interpolacao deterministica entre pontos.
- Sunrise aumenta intensidade gradualmente.
- Sunset reduz intensidade gradualmente.
- Periodo principal usa a curva configurada.
- Moonlight e comportamento de baixa intensidade fora do periodo principal.
- Moonlight nao cria canal novo.
- Modo `ACCLIMATION` aplica fator gradual sobre alvos calculados.
- Aclimatacao respeita duracao configurada.
- Aclimatacao respeita limite maximo global.
- Aclimatacao respeita limite maximo por canal.
- Fade suave limita aumento de duty por avaliacao.
- Fade suave limita reducao de duty por avaliacao.
- Alvo igual ao duty atual nao solicita escrita.
- Duty calculado nunca ultrapassa a faixa funcional.
- Canal desabilitado converge para duty zero usando fade suave.
- Configuracao invalida nao produz duty acima de zero indevido.
- Perfil automatico invalido nao inicia novos duty acima de zero.
- Perfil automatico invalido converge canais ativos para duty zero por fade suave.
- Perfil de aclimatacao invalido nao inicia novos duty acima de zero.
- Perfil de aclimatacao invalido converge canais ativos para duty zero por fade suave.
- Configuracao invalida nao cria estado novo no System State.
- A politica nao altera `temperature`, `waterLevel`, `relays`, `modes`, `ato`, `network`, `alerts` ou `systemHealth`.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar modo `MANUAL` ligando canal Branco com alvo valido.
- Testar modo `MANUAL` desligando canal Azul.
- Testar modo `MANUAL` rejeitando ou limitando alvo acima do maximo por canal.
- Testar modo `MANUAL` respeitando limite maximo global.
- Testar comando manual idempotente.
- Testar comando manual invalido preservando alvos atuais.
- Testar curva automatica com ponto exato.
- Testar curva automatica com interpolacao entre dois pontos.
- Testar curva com pontos fora de ordem e rejeicao deterministica.
- Testar sunrise aumentando alvo ao longo do tempo.
- Testar sunset reduzindo alvo ao longo do tempo.
- Testar periodo principal usando curva configurada.
- Testar moonlight fora do periodo principal sem criar canal novo.
- Testar aclimatacao no primeiro dia com fator reduzido.
- Testar aclimatacao no ultimo dia atingindo limite configurado.
- Testar aclimatacao sem ultrapassar limite global.
- Testar fade aumentando duty em passos.
- Testar fade reduzindo duty em passos.
- Testar que duty igual ao alvo nao gera comando.
- Testar canal desabilitado.
- Testar configuracao invalida de horario.
- Testar configuracao invalida de limite de intensidade.
- Testar perfil automatico invalido convergindo canal ativo para duty zero por fade.
- Testar perfil de aclimatacao invalido convergindo canal ativo para duty zero por fade.
- Confirmar que os testes usam fake de tempo e estados simulados, sem hardware fisico.

---

### ID

FW-P08-003

### Titulo

Implementar servico de iluminacao com System State, PWM e eventos locais

### Objetivo

Criar o servico funcional que executa uma avaliacao de iluminacao, aplica a decisao da politica, comanda PWM pelo controlador funcional, atualiza o bloco `lighting` como fonte unica de verdade e emite eventos locais oficiais.

### Dependencias

FW-P08-001.

FW-P08-002.

FW-P02-002.

### Descricao

Implementar o `LightingService`. Em cada ciclo, o servico deve ler `currentSystemState().lighting` e `ConfigManager::lighting()`, executar a politica pura da Fase 8 e aplicar a decisao resultante. O servico deve ser o unico responsavel por derivar, emitir e deduplicar eventos locais de iluminacao a partir das transicoes aplicadas com sucesso.

O servico deve comandar os canais somente pelo controlador funcional de PWM definido nesta fase. Ele nao deve chamar `ledcWrite`, `ledcSetup`, `analogWrite`, GPIO direto ou diagnosticos de bring-up diretamente.

O System State deve ser atualizado somente depois que os comandos PWM necessarios tiverem sucesso. Falha simulada do controlador nao deve marcar `currentPWM`, `maxPWM`, `enabled`, `mode`, `sunriseEnabled`, `sunsetEnabled`, `acclimationEnabled` ou `currentProfile` como aplicados se o comando correspondente nao foi aceito pelo boundary funcional.

Ao aplicar uma mudanca de perfil, modo ou configuracao de curva ao runtime, o servico deve emitir `LIGHTING_PROFILE_CHANGED` uma vez por mudanca efetiva aplicada com sucesso. A avaliacao periodica de uma curva ja ativa nao deve gerar `LIGHTING_PROFILE_CHANGED` a cada ciclo. Quando a luminaria sair do estado totalmente desligado para qualquer canal funcional com duty acima de zero, deve emitir `LIGHTING_STARTED`. Quando todos os canais funcionais chegarem a duty zero apos estarem ativos, deve emitir `LIGHTING_STOPPED`.

Eventos de transicao nao devem ser duplicados sem mudanca relevante. Avaliacoes repetidas durante fade, automatico estavel, moonlight estavel, manual idempotente, configuracao invalida persistente ou canais ja zerados nao devem gerar eventos repetidos indevidamente.

O servico deve manter `system.lighting` como fonte unica de verdade do modulo. O servico nao deve alterar `temperature`, `waterLevel`, `relays`, `modes`, `ato`, `network`, `alerts` ou `systemHealth`.

Esta tarefa nao deve implementar NVS Preferences real, MQTT, app, comandos remotos, Alert Manager, cloud, historico, notificacoes, comandos por Serial real ou testes fisicos.

### Arquivos afetados

- `firmware/include/modules/lighting/lighting_service.h`
- `firmware/include/modules/lighting/lighting_events.h`
- `firmware/include/modules/lighting/lighting_policy.h`
- `firmware/include/modules/lighting/lighting_types.h`
- `firmware/include/modules/lighting/lighting_profile.h`
- `firmware/src/modules/lighting/lighting_service.cpp`
- `firmware/src/modules/lighting/lighting_events.cpp`
- `firmware/src/modules/lighting/lighting_policy.cpp`
- `firmware/include/core/state/system_state.h`
- `firmware/src/core/state/system_state.cpp`
- `firmware/include/core/events/event_bus.h`
- `firmware/src/core/events/event_bus.cpp`
- `firmware/include/config/config_manager.h`
- `firmware/src/config/config_manager.cpp`
- `firmware/test/fakes/fake_lighting_pwm_controller.h`
- `firmware/test/unit/test_lighting_service.cpp`
- `firmware/test/integration/`

### Criterios de aceitacao

- O servico executa uma avaliacao de iluminacao por chamada explicita.
- O servico le `lighting` do System State.
- O servico le configuracao de iluminacao do Config Manager em memoria.
- O servico usa a politica pura da Fase 8.
- O servico e o unico responsavel por emitir eventos locais de iluminacao.
- O servico e o unico responsavel por deduplicar eventos locais de iluminacao.
- O servico comanda PWM somente pelo controlador funcional.
- O servico nao escreve GPIO diretamente.
- O servico nao chama APIs Arduino diretamente.
- O servico nao usa diagnosticos de bring-up como caminho funcional.
- Sucesso ao aplicar duty atualiza `currentPWM` do canal correspondente.
- Sucesso ao aplicar limite atualiza `maxPWM` do canal correspondente quando necessario.
- Sucesso ao habilitar canal atualiza `enabled` do canal correspondente.
- Sucesso ao trocar modo atualiza `system.lighting.mode`.
- Sucesso ao trocar perfil atualiza `currentProfile`.
- Sucesso ao aplicar configuracao atualiza flags `sunriseEnabled`, `sunsetEnabled` e `acclimationEnabled` quando necessario.
- Falha simulada do controlador preserva o estado anterior do canal afetado.
- Falha simulada do controlador e reportada sem fingir convergencia do estado.
- `LIGHTING_PROFILE_CHANGED` e emitido em mudanca efetiva de perfil, modo ou configuracao de curva aplicada ao runtime.
- Avaliacao periodica de curva ja ativa nao emite `LIGHTING_PROFILE_CHANGED` repetido.
- `LIGHTING_STARTED` e emitido quando algum canal sai de zero para duty acima de zero.
- `LIGHTING_STOPPED` e emitido quando todos os canais funcionais chegam a zero apos atividade.
- Eventos de iluminacao nao sao duplicados sem transicao relevante.
- Comandos idempotentes nao emitem eventos duplicados.
- O servico mantem `system.lighting` como fonte unica de verdade.
- O servico nao altera blocos nao relacionados do System State.
- O servico nao implementa NVS, MQTT, Alert Manager, modos, ATO, cloud, app ou historico.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar avaliacao manual com fake PWM controller.
- Testar avaliacao automatica com perfil e fake de tempo.
- Testar avaliacao de aclimatacao com fake de tempo.
- Testar que escrita PWM bem sucedida atualiza `system.lighting`.
- Testar que apenas o canal alvo muda quando o comando e por canal.
- Testar que falha simulada do controller preserva estado anterior.
- Testar que falha simulada e reportada.
- Testar `LIGHTING_PROFILE_CHANGED` em troca efetiva de perfil.
- Testar `LIGHTING_PROFILE_CHANGED` em troca efetiva de modo.
- Testar `LIGHTING_PROFILE_CHANGED` em troca efetiva de configuracao de curva aplicada ao runtime.
- Testar ausencia de `LIGHTING_PROFILE_CHANGED` em avaliacao periodica de curva ja ativa.
- Testar ausencia de `LIGHTING_PROFILE_CHANGED` em comando idempotente.
- Testar `LIGHTING_STARTED` quando duty sai de zero.
- Testar ausencia de `LIGHTING_STARTED` duplicado durante fade com luminaria ja ativa.
- Testar `LIGHTING_STOPPED` quando todos os canais chegam a zero.
- Testar ausencia de `LIGHTING_STOPPED` quando apenas um canal zera e outros continuam ativos.
- Testar que blocos nao relacionados do System State nao sao corrompidos.
- Testar que eventos de iluminacao nao geram MQTT, alertas centralizados ou historico.
- Confirmar que os testes usam fake PWM, fake tempo e Event Bus local, sem hardware fisico.

---

### ID

FW-P08-004

### Titulo

Integrar iluminacao ao runtime com scheduler sem bloqueio e API local

### Objetivo

Registrar o servico de iluminacao no runtime do core para avaliacao periodica local, garantindo inicializacao segura com duty zero, fade suave sem bloqueio, watchdog preservado e comandos locais por chamada direta.

### Dependencias

FW-P08-003.

FW-P02-004.

FW-P02-007.

Fase 7 concluida por ordem oficial do roadmap.

### Descricao

Integrar o `LightingService` ao `CoreApp` depois da inicializacao de System State, Event Bus, Config Manager, Logger, Scheduler, Watchdog e dos servicos anteriores ja presentes no runtime. A Fase 7 deve estar concluida por ordem oficial do roadmap, mas a iluminacao nao deve depender funcionalmente de `ModeService`, `ModeAutomationGate` ou efeitos de modo nesta fase. A integracao nao deve alterar comportamento de modos, ATO ou reles, e nao deve fazer o modo operacional comandar iluminacao nesta fase.

Durante `setup`, o runtime deve configurar os canais PWM funcionais da luminaria e aplicar duty zero nos quatro canais antes de aceitar qualquer comando local ou avaliacao automatica. O estado inicial seguro deve manter `currentPWM = 0` nos quatro canais. Se a configuracao em memoria habilitar automatico, a primeira avaliacao deve respeitar fade suave e nao aplicar salto abrupto indevido.

Registrar uma tarefa periodica de iluminacao no scheduler com intervalo vindo do contrato local do modulo de iluminacao. A tarefa deve chamar uma iteracao curta do `LightingService` e retornar sucesso ou falha de forma compativel com o scheduler.

Expor API local por chamada direta para comandos de iluminacao em testes de integracao e futuros modulos locais. A API deve permitir, no minimo, solicitar modo manual, modo automatico, modo de aclimatacao, troca de perfil em memoria e alvo manual por canal. Essa API nao deve abrir shell Serial, nao deve implementar MQTT, nao deve aceitar comando remoto e nao deve depender de app.

O loop principal nao deve bloquear por causa da iluminacao. Fade deve ser realizado por avaliacoes sucessivas do scheduler usando tempo injetado ou Time Source, sem delay real, polling fisico longo ou espera por resposta visual.

O Watchdog deve continuar sendo alimentado conforme a Fase 2. Falhas de avaliacao ou escrita PWM devem ser reportadas sem travar o loop e sem deixar o System State indicando valores que nao foram aplicados.

Esta tarefa nao deve inicializar NVS real, Wi-Fi, MQTT, Alert Manager, cloud, aplicativo, comandos remotos ou testes de resiliencia de fases posteriores.

### Arquivos afetados

- `firmware/src/app/core_app.h`
- `firmware/src/app/core_app.cpp`
- `firmware/src/app/core_app_arduino.cpp`
- `firmware/include/modules/lighting/lighting_service.h`
- `firmware/include/modules/lighting/lighting_config.h`
- `firmware/src/modules/lighting/lighting_service.cpp`
- `firmware/src/drivers/lighting/ledc_lighting_pwm_controller.h`
- `firmware/src/drivers/lighting/ledc_lighting_pwm_controller.cpp`
- `firmware/src/drivers/io/arduino_hardware_io.h`
- `firmware/src/drivers/io/arduino_hardware_io.cpp`
- `firmware/include/core/scheduler/task_scheduler.h`
- `firmware/src/core/scheduler/task_scheduler.cpp`
- `firmware/test/fakes/fake_lighting_pwm_controller.h`
- `firmware/test/unit/`
- `firmware/test/integration/test_lighting_scheduler_integration.cpp`

### Criterios de aceitacao

- O runtime cria ou recebe o servico funcional de iluminacao.
- O runtime inicializa iluminacao apos System State.
- O runtime inicializa iluminacao apos Event Bus.
- O runtime inicializa iluminacao apos Config Manager.
- O runtime inicializa iluminacao apos Scheduler.
- O runtime inicializa iluminacao apos Watchdog.
- A integracao nao consulta `ModeService` para decidir iluminacao nesta fase.
- A integracao nao consulta `ModeAutomationGate` para decidir iluminacao nesta fase.
- A integracao nao cria efeitos de modo sobre iluminacao nesta fase.
- O runtime configura os canais PWM funcionais durante `setup`.
- O runtime aplica duty zero nos quatro canais funcionais durante `setup`.
- O estado global apos `setup` mantem Branco com `currentPWM = 0`.
- O estado global apos `setup` mantem Azul com `currentPWM = 0`.
- O estado global apos `setup` mantem Royal Blue com `currentPWM = 0`.
- O estado global apos `setup` mantem UV com `currentPWM = 0`.
- Nenhum canal e elevado acima de zero automaticamente antes da avaliacao do servico.
- A primeira avaliacao respeita fade suave quando houver alvo acima de zero.
- O runtime registra tarefa periodica de iluminacao.
- O intervalo da tarefa vem do contrato local da Fase 8.
- A tarefa de iluminacao executa uma iteracao do servico.
- A tarefa de iluminacao nao executa antes do intervalo configurado.
- A tarefa de iluminacao nao bloqueia o loop principal.
- Existe API local por chamada direta para solicitar modo manual.
- Existe API local por chamada direta para solicitar modo automatico.
- Existe API local por chamada direta para solicitar modo de aclimatacao.
- Existe API local por chamada direta para trocar perfil em memoria.
- Existe API local por chamada direta para definir alvo manual por canal.
- API local nao implementa MQTT, app, cloud ou Serial command shell.
- O Watchdog continua sendo alimentado conforme a Fase 2.
- O build real nao depende de fake PWM controller ou fake tempo.
- Nenhuma funcionalidade de NVS real, Wi-Fi, MQTT, Alert Manager, cloud ou aplicativo e inicializada.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar teste de integracao do runtime com fake PWM controller e fake tempo.
- Testar que `setup` configura os quatro canais funcionais.
- Testar que integracao de iluminacao nao depende de fake `ModeService`.
- Testar que integracao de iluminacao nao depende de fake `ModeAutomationGate`.
- Testar que `setup` escreve duty zero nos quatro canais funcionais.
- Testar que apos `setup` os quatro canais estao com `currentPWM = 0` no System State.
- Testar que nenhum evento `LIGHTING_STARTED` e emitido durante boot seguro com estado default zero.
- Testar que a tarefa de iluminacao e registrada.
- Testar que a tarefa de iluminacao nao executa antes do intervalo.
- Testar que a tarefa de iluminacao executa quando o intervalo vence.
- Testar que uma execucao da tarefa chama o servico uma vez.
- Testar que multiplos ciclos respeitam o intervalo configurado.
- Testar fade por ciclos sucessivos sem delay real.
- Testar API local para modo manual.
- Testar API local para modo automatico.
- Testar API local para modo de aclimatacao.
- Testar API local para troca de perfil em memoria.
- Testar API local para alvo manual por canal.
- Testar que `loopOnce` permanece nao bloqueante.
- Testar que falha simulada no servico e refletida como falha de tarefa quando aplicavel.
- Confirmar que nenhum teste exige ESP32, luminaria real, MOSFET, fonte 12V, medicao eletrica, NVS real, Wi-Fi, MQTT, cloud, app ou Serial real.

---

### ID

FW-P08-005

### Titulo

Consolidar validacao automatizada da Fase 8

### Objetivo

Criar o gate final da Fase 8, garantindo que o sistema de iluminacao esteja integrado, testado por mocks e fakes, limitado ao escopo definido e preparado para validacao fisica futura sem depender dela.

### Dependencias

FW-P08-004.

### Descricao

Consolidar testes unitarios e de integracao da Fase 8 em comandos ou scripts de validacao. O gate deve provar que contratos de iluminacao operam sem hardware, que o controlador PWM real esta preparado para build destinado ao hardware, que a politica pura cobre manual, automatico, sunrise, sunset, moonlight, aclimatacao, curvas e fade suave, que o servico atualiza `lighting`, que eventos `LIGHTING_*` sao emitidos corretamente e que o scheduler executa a avaliacao periodica sem bloquear o loop principal.

Tambem deve haver uma revisao de escopo confirmando que a Fase 8 nao implementou `lighting_store`, backend NVS, repositorio persistente de perfis, NVS Preferences definitivo da Fase 9, Wi-Fi, MQTT, Alert Manager, cloud, app, historico, notificacoes, comandos remotos, comandos por Serial real, alteracoes em modos, alteracoes em ATO, alteracoes em reles, uso funcional do GPIO13 PWM Reserva ou testes de resiliencia de fases posteriores.

Qualquer validacao real com luminaria, MOSFET, fonte 12V, carga LED, corrente, dissipacao, resposta visual, medicao de PWM, ruido eletrico, Serial real ou seguranca de bancada deve ficar marcada como `Pendente para Hardware Validation` e nao pode bloquear a conclusao de desenvolvimento da Fase 8.

### Arquivos afetados

- `firmware/test/unit/`
- `firmware/test/integration/`
- `firmware/test/fakes/`
- `firmware/test/run_phase8_validation.sh`
- `firmware/test/integration/run_phase8_integration_tests.sh`
- `firmware/test/integration/run_phase8_scope_review.sh`
- `firmware/include/modules/lighting/`
- `firmware/src/modules/lighting/`
- `firmware/src/drivers/lighting/`
- `firmware/src/app/`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste
- `tasks/firmware/phases/firmware-phase-08-light-system.md`

### Criterios de aceitacao

- Existe comando ou script para executar os testes unitarios da Fase 8.
- Existe comando ou script para executar os testes de integracao da Fase 8.
- Existe revisao automatizada ou objetiva de escopo da Fase 8.
- Todos os testes da Fase 8 usam mocks, fakes ou valores simulados.
- O build real do firmware compila sem depender de mocks ou fakes.
- Contratos de iluminacao sao validados sem hardware.
- Controlador PWM funcional e validado sem hardware.
- Politica de iluminacao e validada sem hardware.
- Servico de iluminacao e validado sem hardware.
- Integracao do scheduler de iluminacao e validada sem hardware.
- Branco, Azul, Royal Blue e UV sao suportados.
- GPIO25, GPIO26, GPIO27 e GPIO14 sao usados conforme contrato oficial.
- GPIO13 PWM Reserva nao e usado funcionalmente pela Fase 8.
- `system.lighting.mode` e atualizado conforme a spec.
- `system.lighting.currentProfile` e validado por teste automatizado.
- `system.lighting.sunriseEnabled` e validado por teste automatizado.
- `system.lighting.sunsetEnabled` e validado por teste automatizado.
- `system.lighting.acclimationEnabled` e validado por teste automatizado.
- `currentPWM`, `maxPWM` e `enabled` de cada canal sao validados por teste automatizado.
- Modo manual e validado por teste automatizado.
- Modo automatico e validado por teste automatizado.
- Modo de aclimatacao e validado por teste automatizado.
- Sunrise e validado por teste automatizado.
- Sunset e validado por teste automatizado.
- Moonlight e validado como comportamento de perfil ou curva sem canal novo.
- Curvas personalizadas sao validadas por teste automatizado.
- Limite maximo global de intensidade e validado por teste automatizado.
- Limite maximo por canal e validado por teste automatizado.
- Fade suave e validado por teste automatizado.
- Boot seguro com duty zero e validado por teste automatizado.
- Eventos locais `LIGHTING_PROFILE_CHANGED`, `LIGHTING_STARTED` e `LIGHTING_STOPPED` sao validados por teste automatizado.
- Eventos de iluminacao nao publicam MQTT, nao criam alertas centralizados e nao persistem historico.
- Configuracoes de iluminacao operam em memoria nesta fase.
- Contratos de perfis e curvas ficam preparados para persistencia futura pela Fase 9.
- Nenhum `lighting_store` e implementado.
- Nenhum backend NVS de iluminacao e implementado.
- Nenhum repositorio persistente de perfis e implementado.
- Nenhuma NVS Preferences real e implementada.
- Nenhum teste fisico e criterio de conclusao da Fase 8.
- Qualquer validacao real da luminaria fica marcada como `Pendente para Hardware Validation`.
- Revisao de escopo confirma que NVS Preferences definitivo nao foi implementado.
- Revisao de escopo confirma que `lighting_store` nao foi implementado.
- Revisao de escopo confirma que backend NVS de iluminacao nao foi implementado.
- Revisao de escopo confirma que repositorio persistente de perfis nao foi implementado.
- Revisao de escopo confirma que Wi-Fi e MQTT nao foram implementados.
- Revisao de escopo confirma que Alert Manager nao foi implementado.
- Revisao de escopo confirma que cloud e app nao foram implementados.
- Revisao de escopo confirma que comandos remotos nao foram implementados.
- Revisao de escopo confirma que modos, ATO e reles nao foram alterados funcionalmente.
- Revisao de escopo confirma que GPIO13 nao foi usado como canal funcional.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Executar todos os testes unitarios da Fase 8 com sucesso.
- Executar todos os testes de integracao da Fase 8 com sucesso.
- Executar script de validacao final da Fase 8.
- Executar testes existentes da Fase 2 impactados por Event Bus, System State, Config Manager, Scheduler e Watchdog.
- Executar testes existentes da Fase 5 impactados por boundaries de atuadores, quando aplicavel.
- Executar testes existentes da Fase 6 impactados por runtime e Scheduler, quando aplicavel.
- Executar testes existentes da Fase 7 impactados por runtime e Scheduler, quando aplicavel.
- Confirmar que o build real nao inclui fakes ou mocks.
- Confirmar que os testes nao exigem ESP32, luminaria real, MOSFET, fonte 12V, carga LED, multimetro, osciloscopio, Wi-Fi, MQTT, cloud, app, Serial real ou qualquer validacao fisica.
- Confirmar que eventos `LIGHTING_*` existem somente como eventos locais nesta fase.
- Confirmar que nenhum teste ou implementacao inicializa NVS real, `lighting_store`, Wi-Fi, MQTT, Alert Manager, cloud ou app.
- Confirmar que nenhum teste ou implementacao cria backend persistente de perfis.
- Confirmar que nenhum teste ou implementacao implementa comandos remotos.
- Confirmar que nenhum teste ou implementacao altera funcionalmente modos, ATO ou reles.
- Confirmar que nenhum teste ou implementacao usa GPIO13 como canal funcional de iluminacao.
- Confirmar que nenhum teste ou implementacao altera `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md`.
- Confirmar por revisao que funcionalidades de fases posteriores nao foram iniciadas.
