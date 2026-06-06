# Fase 1 - Hardware Bring-Up

## Escopo

Validar a inicializacao do ESP32, a estrutura definitiva do firmware, o pinout oficial do hardware V1, os reles, os sinais PWM e os sensores DS18B20 e VL6180X.

Esta fase deve produzir apenas estrutura de firmware e diagnosticos de validacao de hardware. Nao implementar System State, Event Bus, Config Manager, Task Scheduler, Logger, Watchdog, persistencia, Wi-Fi, MQTT, alertas, modos operacionais, ATO automatico ou curvas de iluminacao.

## Tarefas

### ID

FW-P01-000

### Titulo

Preparar estrutura modular definitiva do firmware

### Objetivo

Criar a organizacao base do firmware para suportar Core, drivers, modulos, configuracao, diagnosticos e testes sem exigir movimentacao ampla de arquivos nas fases futuras.

### Dependencias

Fase 0 concluida.

### Descricao

Criar a estrutura definitiva de diretorios do firmware antes do firmware minimo de diagnostico. A estrutura deve reservar areas separadas para configuracao, contratos de hardware, diagnosticos, drivers, core, modulos e testes. `main.cpp` deve permanecer como ponto de entrada e orquestrador minimo, sem concentrar drivers ou rotinas de diagnostico. Esta tarefa deve criar apenas organizacao estrutural e documentacao curta de responsabilidade dos diretorios, sem implementar funcionalidades futuras.

Estrutura esperada:

```text
firmware/
├── include/
│   ├── config/
│   ├── contracts/
│   └── diagnostics/
├── src/
│   ├── app/
│   ├── config/
│   ├── core/
│   ├── diagnostics/
│   ├── drivers/
│   └── modules/
└── test/
    ├── unit/
    ├── integration/
    └── hardware/
```

### Arquivos afetados

- `firmware/include/config/`
- `firmware/include/contracts/`
- `firmware/include/diagnostics/`
- `firmware/src/app/`
- `firmware/src/config/`
- `firmware/src/core/`
- `firmware/src/diagnostics/`
- `firmware/src/drivers/`
- `firmware/src/modules/`
- `firmware/test/unit/`
- `firmware/test/integration/`
- `firmware/test/hardware/`
- `firmware/platformio.ini`

### Criterios de aceitacao

- A estrutura modular definitiva existe antes das demais tarefas da Fase 1.
- As areas de `config`, `contracts`, `diagnostics`, `drivers`, `core`, `modules` e `test` estao separadas.
- Existe uma descricao curta da responsabilidade de cada area criada.
- O projeto PlatformIO continua compilavel apos a criacao da estrutura.
- Nenhum modulo funcional das fases futuras e implementado.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware-master-plan.md` e alterado.

### Criterios de teste

- Verificar que todos os diretorios esperados existem.
- Executar build PlatformIO e confirmar que a estrutura nao quebra a compilacao.
- Confirmar que `main.cpp` nao contem drivers, contratos de hardware ou diagnosticos completos.
- Confirmar que a estrutura permite adicionar fases futuras sem mover arquivos criados na Fase 1.

---

### ID

FW-P01-001

### Titulo

Criar firmware minimo de diagnostico para bring-up

### Objetivo

Preparar uma base minima carregavel no ESP32 para executar validacoes de hardware via Serial, sem automacoes e sem dependencias funcionais de fases futuras.

### Dependencias

FW-P01-000.

### Descricao

Configurar o ponto de entrada do firmware para inicializar a porta Serial, identificar o build de bring-up e chamar apenas rotinas de diagnostico da Fase 1. O firmware deve ser explicitamente limitado a testes de hardware. `main.cpp` deve permanecer como orquestrador minimo e delegar diagnosticos para a area propria.

### Arquivos afetados

- `firmware/src/main.cpp`
- `firmware/src/app/`
- `firmware/src/diagnostics/`
- `firmware/include/diagnostics/`
- `firmware/platformio.ini`

### Criterios de aceitacao

- O projeto compila no ambiente `esp32dev` com framework Arduino.
- O firmware inicializa no ESP32 e exibe no Serial que esta em modo Hardware Bring-Up.
- `main.cpp` contem somente boot minimo e chamada de diagnosticos.
- O firmware nao inicializa Wi-Fi, MQTT, NVS, estado global, eventos, modos ou automacoes.
- O loop principal permanece responsivo para execucao dos testes das tarefas seguintes.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Fazer upload para o ESP32 com sucesso.
- Abrir o monitor Serial e confirmar mensagem de inicializacao do modo Hardware Bring-Up.
- Reiniciar o ESP32 e confirmar que a mensagem aparece novamente sem erro ou travamento.
- Conferir que a logica de diagnostico nao foi concentrada em `main.cpp`.

---

### ID

FW-P01-002

### Titulo

Declarar e validar o contrato de pinout oficial do hardware V1

### Objetivo

Centralizar o mapeamento dos GPIOs oficiais como contrato de hardware V1 para que todos os diagnosticos e modulos futuros usem os mesmos nomes e pinos.

### Dependencias

FW-P01-001.

### Descricao

Declarar o contrato de pinout em area propria de contratos, contemplando DS18B20 no GPIO4, VL6180X com SDA no GPIO21 e SCL no GPIO22, reles nos GPIO16, GPIO17, GPIO18 e GPIO19, PWM nos GPIO25, GPIO26, GPIO27, GPIO14 e GPIO13. O contrato deve registrar nomes canonicos de perifericos, barramentos e canais fisicos. Divergencias de nomenclatura existentes entre documentos, como Moonlight/UV, devem ser apenas registradas como observacao de contrato vigente, sem alterar specs ou arquitetura.

### Arquivos afetados

- `firmware/include/contracts/hardware_pins.h`
- `firmware/src/config/`
- `firmware/src/diagnostics/`
- `firmware/src/main.cpp`

### Criterios de aceitacao

- Todos os GPIOs oficiais do hardware V1 estao representados no contrato de pinout.
- Nenhum GPIO diferente do pinout oficial e usado nos diagnosticos.
- O Serial imprime uma tabela de pinout no boot ou por comando de diagnostico.
- Os nomes canonicos de reles, sensores, barramentos e canais PWM estao claros.
- O firmware nao altera nomes, regras, arquitetura ou specs existentes.

### Criterios de teste

- Compilar o firmware e verificar ausencia de erros de referencia ao contrato de pinout.
- Conferir no monitor Serial a tabela de GPIOs esperada.
- Comparar manualmente a tabela impressa com `architecture/hardware.md`, `specs/relay-spec.md`, `specs/lighting-spec.md`, `specs/ato-spec.md` e `specs/temperature-spec.md`.
- Confirmar que diagnosticos consomem o contrato de pinout, sem duplicar numeros de GPIO.

---

### ID

FW-P01-002A

### Titulo

Definir contrato eletrico de ativacao dos reles

### Objetivo

Identificar e registrar se o modulo de reles do hardware V1 opera como `ACTIVE_HIGH` ou `ACTIVE_LOW`, garantindo que OFF fisico seja inequivo no boot e nos diagnosticos.

### Dependencias

FW-P01-002.

### Descricao

Adicionar ao contrato de hardware a polaridade oficial dos reles apos validacao fisica. A tarefa deve diferenciar nivel logico aplicado ao GPIO de estado fisico ON/OFF do rele. O resultado deve ser usado pelos diagnosticos de estado seguro e teste individual dos reles.

### Arquivos afetados

- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/contracts/relay_contract.h`
- `firmware/src/diagnostics/`
- `docs/firmware-phase-01-hardware-bringup-report.md`

### Criterios de aceitacao

- A polaridade dos reles esta explicitamente registrada como `ACTIVE_HIGH` ou `ACTIVE_LOW`.
- O contrato define qual nivel logico representa OFF fisico.
- O contrato define qual nivel logico representa ON fisico.
- Os diagnosticos de reles utilizam o contrato de polaridade, sem assumir nivel fixo diretamente.
- Nenhuma automacao de reles e implementada.

### Criterios de teste

- Medir ou observar fisicamente o estado dos quatro reles com GPIO no nivel de OFF definido.
- Medir ou observar fisicamente o estado dos quatro reles com GPIO no nivel de ON definido.
- Confirmar que o Serial informa polaridade, nivel logico de OFF e nivel logico de ON.
- Registrar a polaridade validada no relatorio da Fase 1.

---

### ID

FW-P01-003

### Titulo

Inicializar GPIOs em estado eletricamente seguro

### Objetivo

Garantir que, durante o boot de diagnostico, os atuadores iniciem desligados e os pinos sejam configurados antes de qualquer teste manual.

### Dependencias

FW-P01-002A.

### Descricao

Configurar os GPIOs de reles como saida e aplica-los imediatamente no nivel logico correspondente a OFF fisico conforme contrato de polaridade. Configurar os canais PWM com duty inicial zero. Preparar os barramentos dos sensores sem acionar reles ou PWM automaticamente.

### Arquivos afetados

- `firmware/src/diagnostics/`
- `firmware/src/drivers/`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/contracts/relay_contract.h`

### Criterios de aceitacao

- Os quatro reles iniciam fisicamente desligados apos boot.
- O OFF fisico usa a polaridade definida em `FW-P01-002A`.
- Todos os canais PWM iniciam com duty zero.
- Nenhum teste de atuador e executado automaticamente sem acao explicita do fluxo de diagnostico.
- O monitor Serial informa que o estado inicial seguro foi aplicado.

### Criterios de teste

- Energizar o ESP32 e observar que nenhum rele aciona durante o boot.
- Medir o nivel logico aplicado aos GPIOs de reles e confirmar correspondencia com OFF fisico.
- Medir ou observar que os canais PWM permanecem em zero antes dos testes.
- Reiniciar o ESP32 tres vezes e confirmar comportamento consistente.
- Confirmar no Serial a mensagem de estado inicial seguro.

---

### ID

FW-P01-004

### Titulo

Implementar teste individual dos reles

### Objetivo

Validar que os quatro reles respondem nos GPIOs oficiais, respeitam a polaridade validada e retornam ao estado OFF ao final do teste.

### Dependencias

FW-P01-003.

### Descricao

Adicionar rotina de diagnostico para acionar individualmente Rele 1/Recalque no GPIO16, Rele 2/Aquecedor no GPIO17, Rele 3/ATO no GPIO18 e Rele 4/Reserva no GPIO19. Cada teste deve ligar apenas um rele por vez usando a polaridade oficial, aguardar intervalo curto e desligar antes de seguir.

### Arquivos afetados

- `firmware/src/diagnostics/`
- `firmware/src/drivers/relays/`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/contracts/relay_contract.h`

### Criterios de aceitacao

- Cada rele pode ser testado individualmente.
- Apenas o rele em teste muda de estado.
- O acionamento usa explicitamente o contrato `ACTIVE_HIGH` ou `ACTIVE_LOW`.
- Todos os reles ficam fisicamente OFF ao final da rotina.
- O Serial registra inicio, GPIO, polaridade, resultado esperado e fim de cada teste.
- Nao ha controle remoto, MQTT, eventos ou automacao de reles nesta fase.

### Criterios de teste

- Executar o teste do Rele 1 e confirmar acionamento apenas do canal Recalque.
- Executar o teste do Rele 2 e confirmar acionamento apenas do canal Aquecedor.
- Executar o teste do Rele 3 e confirmar acionamento apenas do canal ATO.
- Executar o teste do Rele 4 e confirmar acionamento apenas do canal Reserva.
- Confirmar que todos os reles permanecem desligados apos a rotina completa.
- Confirmar que o relatorio registra a polaridade usada no teste.

---

### ID

FW-P01-005A

### Titulo

Definir parametros PWM de bring-up da luminaria

### Objetivo

Definir os parametros PWM usados apenas nos diagnosticos da Fase 1, registrando frequencia, resolucao, canais LEDC e limites de duty aplicados durante o bring-up.

### Dependencias

FW-P01-002.

### Descricao

Registrar os parametros PWM utilizados no bring-up do hardware V1. A tarefa deve definir, somente para teste, frequencia, resolucao, canais LEDC, duty minimo, duty maximo e duty zero para os GPIO25, GPIO26, GPIO27, GPIO14 e GPIO13. Estes valores nao sao parametros oficiais definitivos da luminaria; a definicao oficial deve ocorrer na Fase 8 - Sistema de Iluminacao. Esta tarefa nao deve implementar curvas, fade, horarios ou modos de iluminacao.

### Arquivos afetados

- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/config/pwm_bringup_config.h`
- `firmware/src/config/`
- `docs/firmware-phase-01-hardware-bringup-report.md`

### Criterios de aceitacao

- A frequencia PWM usada no bring-up esta definida.
- A resolucao PWM usada no bring-up esta definida.
- Os canais LEDC usados no bring-up dos cinco GPIOs PWM estao registrados.
- Duty zero, duty minimo de teste e duty maximo de teste estao definidos para diagnostico.
- O texto deixa explicito que os parametros oficiais da luminaria serao definidos na Fase 8.
- A tarefa nao implementa sunrise, sunset, moonlight, aclimatacao, curvas, horarios ou persistencia.

### Criterios de teste

- Compilar o firmware usando a configuracao PWM de bring-up.
- Confirmar no Serial os parametros PWM de bring-up carregados pelo diagnostico.
- Verificar que todos os canais PWM usam a frequencia e a resolucao de teste registradas.
- Registrar os parametros PWM usados no bring-up no relatorio da Fase 1.

---

### ID

FW-P01-005

### Titulo

Implementar teste de sinais PWM da luminaria

### Objetivo

Validar que os GPIOs PWM oficiais geram sinal controlavel para os canais fisicos da luminaria usando os parametros PWM de bring-up.

### Dependencias

FW-P01-005A.

### Descricao

Adicionar rotina de diagnostico para testar PWM nos GPIO25, GPIO26, GPIO27, GPIO14 e GPIO13. O teste deve aplicar niveis discretos de duty por canal conforme configuracao PWM de bring-up, um canal por vez, retornando cada canal para zero antes de avancar.

### Arquivos afetados

- `firmware/src/diagnostics/`
- `firmware/src/drivers/pwm/`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/config/pwm_bringup_config.h`

### Criterios de aceitacao

- Os cinco GPIOs PWM do pinout oficial sao testaveis.
- Cada canal PWM usa a frequencia e resolucao definidas para o bring-up.
- Cada canal PWM pode ser colocado em duty zero e em pelo menos dois niveis acima de zero.
- Apenas um canal PWM fica ativo por vez durante o teste.
- Todos os canais retornam a duty zero ao final.
- Nao ha sunrise, sunset, moonlight, aclimatacao, curvas, horarios ou persistencia nesta fase.

### Criterios de teste

- Medir com multimetro, osciloscopio, analisador logico ou carga de teste que cada GPIO PWM altera o duty quando solicitado.
- Confirmar que GPIO25, GPIO26, GPIO27, GPIO14 e GPIO13 respondem individualmente.
- Confirmar frequencia e resolucao PWM de bring-up durante os testes.
- Confirmar que os demais canais permanecem em duty zero enquanto um canal e testado.
- Confirmar que todos os canais ficam em duty zero apos concluir a rotina.

---

### ID

FW-P01-006

### Titulo

Validar presenca e leitura basica do DS18B20

### Objetivo

Confirmar que o sensor de temperatura DS18B20 esta conectado ao GPIO4, responde ao barramento OneWire e fornece leitura basica.

### Dependencias

FW-P01-002.

### Descricao

Adicionar driver minimo de bring-up para o barramento OneWire e rotina de diagnostico separada para o DS18B20 no GPIO4. A rotina deve detectar dispositivo DS18B20 e imprimir uma leitura de temperatura no Serial. A leitura deve ser usada apenas como validacao de hardware, sem implementar monitoramento periodico, estados de temperatura, alertas ou eventos.

### Arquivos afetados

- `firmware/src/drivers/onewire/`
- `firmware/src/drivers/sensors/`
- `firmware/src/diagnostics/`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/platformio.ini`

### Criterios de aceitacao

- O firmware detecta o DS18B20 no GPIO4.
- O Serial informa se o sensor foi encontrado ou nao.
- Quando encontrado, o Serial imprime uma temperatura numerica plausivel.
- Driver minimo e diagnostico ficam separados.
- A rotina nao implementa limites configuraveis, leitura a cada 5 segundos, SENSOR_OFFLINE, eventos ou MQTT.

### Criterios de teste

- Compilar com as dependencias necessarias para leitura do DS18B20.
- Executar a rotina com o sensor conectado e confirmar deteccao.
- Desconectar o sensor com o ESP32 desenergizado, religar e confirmar falha de deteccao informada no Serial.
- Reconectar o sensor e confirmar recuperacao da leitura apos novo boot ou nova execucao do diagnostico.
- Confirmar que nenhum estado de temperatura ou evento foi criado.

---

### ID

FW-P01-007

### Titulo

Validar barramento I2C do VL6180X

### Objetivo

Confirmar que o barramento I2C nos GPIO21 e GPIO22 esta operacional e que o sensor VL6180X responde no hardware V1.

### Dependencias

FW-P01-002.

### Descricao

Adicionar driver minimo de bring-up para I2C e rotina de diagnostico separada para inicializar SDA no GPIO21 e SCL no GPIO22. A rotina deve executar varredura de enderecos e identificar a presenca do VL6180X. A validacao deve ser fisica e de comunicacao, sem implementar calibracao, estados de nivel, ATO automatico ou deteccao funcional de falhas.

### Arquivos afetados

- `firmware/src/drivers/i2c/`
- `firmware/src/drivers/sensors/`
- `firmware/src/diagnostics/`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/platformio.ini`

### Criterios de aceitacao

- O barramento I2C inicializa nos GPIO21 e GPIO22.
- A varredura I2C lista os dispositivos encontrados.
- O VL6180X e identificado quando conectado corretamente.
- Driver minimo e diagnostico ficam separados.
- A rotina nao implementa waterLevel, calibracao, LOW_LEVEL, SENSOR_OFFLINE, ATO ou MQTT.

### Criterios de teste

- Executar o diagnostico com o VL6180X conectado e confirmar endereco I2C detectado.
- Executar o diagnostico com o sensor desconectado e confirmar ausencia informada no Serial.
- Verificar fisicamente SDA no GPIO21 e SCL no GPIO22.
- Repetir o teste apos reinicializacao do ESP32 e confirmar resultado consistente.

---

### ID

FW-P01-007A

### Titulo

Validar e registrar endereco I2C do VL6180X

### Objetivo

Confirmar o endereco I2C esperado e encontrado do VL6180X para deixar o contrato de hardware pronto para o Level Module futuro.

### Dependencias

FW-P01-007.

### Descricao

Registrar no contrato de hardware o endereco I2C esperado para o VL6180X e comparar esse valor com o endereco encontrado na varredura. O diagnostico deve imprimir endereco esperado, endereco encontrado e resultado PASS/FAIL. Esta tarefa nao deve implementar leitura periodica, calibracao ou estado de nivel.

### Arquivos afetados

- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/contracts/i2c_contract.h`
- `firmware/src/diagnostics/`
- `docs/firmware-phase-01-hardware-bringup-report.md`

### Criterios de aceitacao

- O endereco I2C esperado do VL6180X esta definido no contrato.
- O endereco encontrado no barramento e impresso no Serial.
- O diagnostico compara endereco esperado e encontrado.
- O relatorio da Fase 1 registra o endereco I2C validado.
- Nenhum comportamento de Level Module ou ATO e implementado.

### Criterios de teste

- Executar varredura I2C com o VL6180X conectado.
- Confirmar que o endereco esperado aparece no Serial.
- Confirmar que o endereco encontrado coincide com o esperado.
- Desconectar o sensor com o ESP32 desenergizado, religar e confirmar FAIL claro para ausencia do endereco esperado.

---

### ID

FW-P01-008

### Titulo

Validar leitura basica do VL6180X

### Objetivo

Confirmar que o VL6180X, alem de responder no I2C, fornece leitura basica de distancia/nivel para validacao do sensor fisico.

### Dependencias

FW-P01-007A.

### Descricao

Adicionar rotina de diagnostico para realizar uma leitura simples do VL6180X e imprimir o valor bruto ou convertido no Serial. A leitura deve ser usada somente para verificar resposta do sensor e variacao fisica diante de mudanca de distancia. A rotina tambem deve registrar estabilidade basica em repouso eletrico, sem calibrar limites ou criar estados.

### Arquivos afetados

- `firmware/src/drivers/sensors/`
- `firmware/src/diagnostics/`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/contracts/i2c_contract.h`
- `firmware/platformio.ini`

### Criterios de aceitacao

- O firmware imprime uma leitura numerica do VL6180X quando o sensor esta conectado.
- A leitura muda de forma observavel quando a distancia ao alvo e alterada.
- Falha de leitura e informada no Serial de forma clara.
- A leitura em repouso eletrico e registrada como observacao de estabilidade basica.
- Nao ha calibracao, limites minimo/maximo, status de nivel, ATO ou alertas nesta fase.

### Criterios de teste

- Executar leitura com alvo proximo ao sensor e registrar valor apresentado.
- Alterar a distancia do alvo e confirmar variacao no valor.
- Manter alvo parado por periodo curto e confirmar que a leitura nao oscila de forma grosseira.
- Remover o sensor com o ESP32 desenergizado, religar e confirmar falha clara de leitura.
- Reconectar o sensor e confirmar leitura valida apos novo boot ou nova execucao do diagnostico.

---

### ID

FW-P01-009A

### Titulo

Validar estabilidade do barramento 5V durante acionamento dos reles

### Objetivo

Confirmar que o barramento 5V permanece estavel durante acionamento individual e sequencial dos reles no hardware V1.

### Dependencias

FW-P01-004.

### Descricao

Adicionar procedimento de validacao manual ao diagnostico da Fase 1 para medir o barramento 5V em repouso, durante acionamento individual dos reles e durante acionamento sequencial controlado. A tarefa deve registrar observacoes eletricas e queda de tensao percebida, sem alterar arquitetura eletrica ou criar protecoes de firmware.

### Arquivos afetados

- `firmware/src/diagnostics/`
- `docs/firmware-phase-01-hardware-bringup-report.md`

### Criterios de aceitacao

- O diagnostico orienta medicao do 5V em repouso.
- O diagnostico orienta medicao do 5V durante acionamento individual dos quatro reles.
- O diagnostico orienta medicao do 5V durante acionamento sequencial controlado.
- O relatorio registra PASS/FAIL ou pendencia eletrica para estabilidade do 5V.
- Nenhuma correcao eletrica ou funcionalidade de fail-safe e implementada nesta tarefa.

### Criterios de teste

- Medir o barramento 5V antes de acionar reles.
- Medir o barramento 5V durante acionamento de cada rele.
- Medir o barramento 5V durante sequencia controlada dos reles.
- Registrar qualquer queda, reset, instabilidade ou comportamento anormal observado.
- Confirmar que todos os reles retornam a OFF apos a validacao.

---

### ID

FW-P01-009B

### Titulo

Validar ruido nos sensores durante acionamento dos reles

### Objetivo

Confirmar que o acionamento dos reles nao causa interferencia grosseira nas leituras basicas do DS18B20 e do VL6180X durante o bring-up.

### Dependencias

FW-P01-006, FW-P01-008, FW-P01-009A.

### Descricao

Adicionar procedimento de diagnostico para registrar leituras basicas dos sensores antes, durante e apos acionamentos controlados dos reles. A tarefa deve identificar ruido, perda de comunicacao, resets ou leituras impossiveis provocadas por comutacao. O objetivo e validar a eletrica da Fase 1, sem implementar filtros, estados de falha, alertas ou recuperacao automatica.

### Arquivos afetados

- `firmware/src/diagnostics/`
- `docs/firmware-phase-01-hardware-bringup-report.md`

### Criterios de aceitacao

- O diagnostico coleta leitura basica do DS18B20 antes e apos acionamento dos reles.
- O diagnostico coleta leitura basica do VL6180X antes e apos acionamento dos reles.
- Perda de leitura, reset ou oscilacao grosseira e registrada como FAIL ou pendencia eletrica.
- Todos os reles e PWM ficam em estado seguro ao final.
- Nenhum filtro, alerta, evento ou automacao e implementado.

### Criterios de teste

- Registrar leitura DS18B20 e VL6180X com todos os reles OFF.
- Acionar cada rele individualmente e registrar se houve perda ou variacao grosseira de leitura.
- Executar sequencia controlada de reles e registrar comportamento dos sensores.
- Confirmar que o ESP32 nao reinicia durante a validacao.
- Registrar resultado no relatorio da Fase 1.

---

### ID

FW-P01-009

### Titulo

Consolidar diagnosticos eletricos e de perifericos da Fase 1

### Objetivo

Consolidar os testes da Fase 1 em uma sequencia controlada e gerar um resumo PASS/FAIL final no Serial.

### Dependencias

FW-P01-005, FW-P01-009B.

### Descricao

Adicionar uma rotina integrada de consolidacao que apresente, sob comando ou confirmacao explicita, os resultados de validacao de pinout, estado inicial seguro, polaridade dos reles, teste individual dos reles, teste dos PWM, deteccao/leitura do DS18B20, deteccao/endereco/leitura do VL6180X, estabilidade do barramento 5V e ruido/interferencia durante acionamento dos reles. O resumo deve separar falhas por periferico e por validacao eletrica para facilitar correcao.

### Arquivos afetados

- `firmware/src/diagnostics/`
- `firmware/include/diagnostics/`
- `firmware/src/app/`
- `firmware/src/main.cpp`

### Criterios de aceitacao

- A consolidacao cobre ESP32, GPIOs, reles, PWM, DS18B20 e VL6180X.
- O resumo final mostra PASS/FAIL por grupo de periferico e por validacao eletrica complementar.
- O resumo inclui polaridade de reles, parametros PWM de bring-up, endereco I2C do VL6180X, estabilidade 5V e ruido/interferencia.
- Falha em um sensor nao impede que os demais grupos sejam reportados.
- Todos os reles e PWM ficam desligados ao final da consolidacao, mesmo se algum teste falhar.
- A rotina nao cria funcionalidades das fases 2 a 16.

### Criterios de teste

- Executar a consolidacao com todos os perifericos conectados e confirmar PASS nos grupos esperados.
- Executar a consolidacao com um sensor desconectado e confirmar FAIL apenas no grupo correspondente.
- Confirmar que reles e PWM ficam OFF/zero apos a consolidacao completa.
- Reiniciar o ESP32 e repetir a consolidacao para verificar consistencia.
- Confirmar que o resumo final inclui resultados de polaridade, PWM de bring-up, I2C, estabilidade 5V e ruido/interferencia.

---

### ID

FW-P01-010

### Titulo

Registrar evidencia manual de validacao do hardware V1

### Objetivo

Documentar os resultados da Fase 1 para liberar a implementacao da Fase 2 sem alterar arquitetura ou specs.

### Dependencias

FW-P01-009.

### Descricao

Criar um registro simples de validacao contendo data, placa usada, resultado de boot, estrutura de firmware criada, tabela de pinout conferida, polaridade dos reles, resultado dos quatro reles, parametros PWM de bring-up, resultado dos cinco GPIOs PWM, resultado do DS18B20, endereco I2C e leitura do VL6180X, estabilidade do barramento 5V e ruido observado durante acionamento dos reles. O registro deve apontar pendencias eletricas, se houver, sem propor mudancas de arquitetura.

### Arquivos afetados

- `docs/firmware-phase-01-hardware-bringup-report.md`

### Criterios de aceitacao

- O registro informa PASS/FAIL para cada periferico da Fase 1.
- O registro inclui estrutura de firmware validada.
- O registro inclui polaridade `ACTIVE_HIGH` ou `ACTIVE_LOW` dos reles.
- O registro inclui os parametros PWM utilizados no bring-up.
- O registro inclui endereco I2C validado do VL6180X.
- O registro inclui estabilidade do barramento 5V.
- O registro inclui ruido ou interferencia observado durante acionamento dos reles.
- O registro inclui observacoes de falha quando algum periferico nao responde.
- O registro nao altera `architecture/`, `specs/` ou `tasks/firmware-master-plan.md`.
- A Fase 1 so e considerada concluida quando todos os perifericos definidos no hardware V1 estiverem PASS ou quando a pendencia eletrica estiver explicitamente registrada.

### Criterios de teste

- Conferir que o relatorio existe e contem todos os grupos de perifericos da Fase 1.
- Comparar o relatorio com o resumo PASS/FAIL emitido no Serial.
- Verificar que polaridade dos reles, parametros PWM e endereco I2C foram registrados.
- Verificar que estabilidade 5V e ruido durante acionamento dos reles foram registrados.
- Verificar que nenhuma alteracao foi feita em arquivos de arquitetura ou specs.
- Confirmar que pendencias registradas sao acionaveis por montagem/correcao eletrica ou por ajuste posterior dentro do escopo da Fase 1.
