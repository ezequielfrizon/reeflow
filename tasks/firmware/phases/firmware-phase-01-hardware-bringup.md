# Fase 1 - Hardware Bring-Up

## Escopo

Preparar a base do firmware, os contratos, os drivers minimos, os diagnosticos, as interfaces de teste, as rotinas de bring-up e os templates de relatorio para validar futuramente o hardware V1 do REEFLOW.

Esta fase e puramente de desenvolvimento. Nao exige ESP32 conectado, sensores conectados, modulo de reles conectado, luminaria conectada, multimetro, osciloscopio, analisador logico ou medicoes reais.

Toda validacao fisica deve ser registrada como `Pendente para Hardware Validation`.

Mocks e fakes devem ser usados apenas para testes automatizados ou simulados. Apos os testes, os mocks nao devem permanecer como dependencia do firmware de bring-up destinado ao hardware real. O resultado final da Fase 1 deve ficar preparado para conectar o modulo real e executar a validacao pratica em fase futura.

Nao implementar System State, Event Bus, Config Manager, Task Scheduler, Logger, Watchdog, persistencia, Wi-Fi, MQTT, alertas, modos operacionais, ATO automatico ou curvas de iluminacao.

## Tarefas

### ID

FW-P01-000

### Titulo

Preparar estrutura modular definitiva do firmware

### Objetivo

Criar a organizacao base do firmware para suportar Core, drivers, modulos, configuracao, diagnosticos, contratos e testes sem exigir movimentacao ampla de arquivos nas fases futuras.

### Dependencias

Fase 0 concluida.

### Descricao

Criar a estrutura definitiva de diretorios do firmware antes do firmware minimo de diagnostico. A estrutura deve reservar areas separadas para configuracao, contratos de hardware, diagnosticos, drivers, core, modulos, testes unitarios, testes de integracao e testes de hardware. Esta tarefa deve criar apenas organizacao estrutural e documentacao curta de responsabilidade dos diretorios, sem implementar funcionalidades futuras.

Estrutura esperada:

```text
firmware/
|-- include/
|   |-- config/
|   |-- contracts/
|   `-- diagnostics/
|-- src/
|   |-- app/
|   |-- config/
|   |-- core/
|   |-- diagnostics/
|   |-- drivers/
|   `-- modules/
`-- test/
    |-- unit/
    |-- integration/
    `-- hardware/
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
- Mocks e fakes ficam restritos a `firmware/test/` ou a configuracoes de teste, sem dependencia no firmware final de bring-up.
- Nenhum modulo funcional das fases futuras e implementado.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Verificar que todos os diretorios esperados existem.
- Executar build PlatformIO e confirmar que a estrutura nao quebra a compilacao.
- Confirmar que `main.cpp` nao contem drivers, contratos de hardware ou diagnosticos completos.
- Confirmar que a estrutura permite adicionar fases futuras sem mover arquivos criados na Fase 1.
- Confirmar que mocks/fakes de teste nao sao exigidos pelo build de bring-up para hardware real.

---

### ID

FW-P01-001

### Titulo

Criar firmware minimo de diagnostico para bring-up

### Objetivo

Preparar uma base minima compilavel para o ESP32, capaz de inicializar a interface de diagnostico e chamar rotinas de bring-up quando o hardware estiver disponivel.

### Dependencias

FW-P01-000.

### Descricao

Configurar o ponto de entrada do firmware para inicializar a interface de diagnostico, identificar o build de bring-up e chamar apenas rotinas da Fase 1. `main.cpp` deve permanecer como orquestrador minimo e delegar diagnosticos para a area propria. Upload, monitor Serial e reboot real devem ficar registrados como `Pendente para Hardware Validation`.

### Arquivos afetados

- `firmware/src/main.cpp`
- `firmware/src/app/`
- `firmware/src/diagnostics/`
- `firmware/include/diagnostics/`
- `firmware/platformio.ini`

### Criterios de aceitacao

- O projeto compila no ambiente `esp32dev` com framework Arduino.
- O firmware possui identificacao de modo Hardware Bring-Up.
- `main.cpp` contem somente boot minimo e chamada de diagnosticos.
- O firmware nao inicializa Wi-Fi, MQTT, NVS, estado global, eventos, modos ou automacoes.
- O firmware final de bring-up nao depende de mocks para compilar.
- Upload para ESP32, monitor Serial real e reboot real ficam marcados como `Pendente para Hardware Validation`.

### Criterios de teste

- Executar build PlatformIO com sucesso.
- Testar a identificacao do modo Hardware Bring-Up por teste automatizado, snapshot de saida ou mock de interface de diagnostico.
- Conferir que a logica de diagnostico nao foi concentrada em `main.cpp`.
- Confirmar que os mocks usados no teste sao removidos ou desabilitados do build de bring-up para hardware real.

---

### ID

FW-P01-002

### Titulo

Declarar contrato de pinout oficial do hardware V1

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
- Existe rotina ou funcao capaz de gerar a tabela de pinout para diagnostico futuro.
- Os nomes canonicos de reles, sensores, barramentos e canais PWM estao claros.
- O firmware nao altera nomes, regras, arquitetura ou specs existentes.
- Impressao da tabela em Serial real fica marcada como `Pendente para Hardware Validation`.

### Criterios de teste

- Compilar o firmware e verificar ausencia de erros de referencia ao contrato de pinout.
- Testar a geracao da tabela de pinout com mock ou snapshot sem hardware.
- Comparar manualmente o contrato com `architecture/hardware.md`, `specs/relay-spec.md`, `specs/lighting-spec.md`, `specs/ato-spec.md` e `specs/temperature-spec.md`.
- Confirmar que diagnosticos consomem o contrato de pinout, sem duplicar numeros de GPIO.
- Confirmar que mocks usados para snapshot nao permanecem como dependencia do build de hardware real.

---

### ID

FW-P01-002A

### Titulo

Declarar contrato eletrico ACTIVE_HIGH dos reles

### Objetivo

Registrar que o modulo de reles do hardware V1 opera como `ACTIVE_HIGH`, garantindo que os diagnosticos usem uma definicao unica para ON e OFF logicos.

### Dependencias

FW-P01-002.

### Descricao

Adicionar ao contrato de hardware a polaridade `ACTIVE_HIGH` dos reles. A tarefa deve diferenciar nivel logico aplicado ao GPIO de estado fisico esperado ON/OFF do rele. Como nao ha hardware conectado, a confirmacao fisica da polaridade deve ficar marcada como `Pendente para Hardware Validation`.

### Arquivos afetados

- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/contracts/relay_contract.h`
- `firmware/src/diagnostics/`
- `docs/firmware-phase-01-hardware-bringup-report.md`

### Criterios de aceitacao

- A polaridade dos reles esta registrada como `ACTIVE_HIGH`.
- O contrato define nivel logico LOW como OFF esperado.
- O contrato define nivel logico HIGH como ON esperado.
- Os diagnosticos de reles utilizam o contrato de polaridade, sem assumir nivel fixo diretamente.
- A confirmacao fisica da polaridade fica marcada como `Pendente para Hardware Validation`.
- Nenhuma automacao de reles e implementada.

### Criterios de teste

- Compilar o firmware usando o contrato `ACTIVE_HIGH`.
- Testar com mock GPIO que OFF esperado gera nivel LOW.
- Testar com mock GPIO que ON esperado gera nivel HIGH.
- Confirmar que o mock e removido ou desabilitado do build de bring-up para hardware real.
- Verificar que o template de relatorio possui campo para confirmar a polaridade fisica futuramente.

---

### ID

FW-P01-003

### Titulo

Preparar inicializacao logica segura dos GPIOs

### Objetivo

Garantir, em codigo e testes sem hardware, que os atuadores sejam configurados para estado logico seguro antes de qualquer rotina de diagnostico.

### Dependencias

FW-P01-002A.

### Descricao

Configurar os GPIOs de reles como saida e aplicar imediatamente LOW, que representa OFF esperado conforme contrato `ACTIVE_HIGH`. Configurar os canais PWM com duty inicial zero. Preparar os barramentos dos sensores sem acionar reles ou PWM automaticamente. A confirmacao fisica de estado seguro fica marcada como `Pendente para Hardware Validation`.

### Arquivos afetados

- `firmware/src/diagnostics/`
- `firmware/src/drivers/`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/contracts/relay_contract.h`

### Criterios de aceitacao

- A rotina de inicializacao aplica LOW nos GPIOs dos quatro reles antes de qualquer teste.
- O OFF logico usa a polaridade `ACTIVE_HIGH` definida em `FW-P01-002A`.
- Todos os canais PWM iniciam com duty zero.
- Nenhum teste de atuador e executado automaticamente sem acao explicita do fluxo de diagnostico.
- O diagnostico registra que a confirmacao fisica de estado seguro esta `Pendente para Hardware Validation`.

### Criterios de teste

- Testar com mock GPIO que os quatro reles recebem LOW na inicializacao.
- Testar com mock PWM que todos os canais iniciam em duty zero.
- Testar que nenhuma rotina de acionamento e chamada automaticamente no boot.
- Confirmar que mocks usados nos testes nao permanecem no build de bring-up para hardware real.
- Registrar no template de relatorio que observacao fisica dos reles esta pendente.

---

### ID

FW-P01-004

### Titulo

Preparar teste individual dos reles

### Objetivo

Criar a rotina de diagnostico para acionar individualmente os quatro reles quando o hardware estiver conectado, validando em desenvolvimento a sequencia logica com mocks.

### Dependencias

FW-P01-003.

### Descricao

Adicionar rotina de diagnostico para acionar individualmente Rele 1/Recalque no GPIO16, Rele 2/Aquecedor no GPIO17, Rele 3/ATO no GPIO18 e Rele 4/Reserva no GPIO19. Cada teste deve comandar apenas um rele por vez usando a polaridade `ACTIVE_HIGH`, aguardar intervalo curto e retornar todos ao estado OFF esperado. A resposta fisica dos reles fica marcada como `Pendente para Hardware Validation`.

### Arquivos afetados

- `firmware/src/diagnostics/`
- `firmware/src/drivers/relays/`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/contracts/relay_contract.h`

### Criterios de aceitacao

- Existe rotina de diagnostico para testar cada rele individualmente.
- A rotina comanda apenas um rele por vez.
- O acionamento usa explicitamente o contrato `ACTIVE_HIGH`.
- Todos os reles retornam logicamente para OFF esperado ao final da rotina.
- O diagnostico registra que a resposta fisica dos reles esta `Pendente para Hardware Validation`.
- Nao ha controle remoto, MQTT, eventos ou automacao de reles nesta fase.

### Criterios de teste

- Testar com mock GPIO a sequencia do Rele 1/Recalque.
- Testar com mock GPIO a sequencia do Rele 2/Aquecedor.
- Testar com mock GPIO a sequencia do Rele 3/ATO.
- Testar com mock GPIO a sequencia do Rele 4/Reserva.
- Confirmar com mock que apenas um rele recebe HIGH por vez.
- Confirmar que todos os mocks sao removidos ou desabilitados do build de bring-up para hardware real.

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
- A medicao fisica do PWM fica marcada como `Pendente para Hardware Validation`.
- A tarefa nao implementa sunrise, sunset, moonlight, aclimatacao, curvas, horarios ou persistencia.

### Criterios de teste

- Compilar o firmware usando a configuracao PWM de bring-up.
- Testar com mock ou abstracao LEDC que os parametros PWM de bring-up sao carregados pelo diagnostico.
- Verificar que todos os canais PWM usam a frequencia e a resolucao de teste registradas.
- Registrar os parametros PWM usados no bring-up no template de relatorio.
- Confirmar que mocks usados no teste nao permanecem no build de bring-up para hardware real.

---

### ID

FW-P01-005

### Titulo

Preparar teste de sinais PWM da luminaria

### Objetivo

Criar a rotina de diagnostico para gerar sinais PWM nos GPIOs oficiais da luminaria quando o hardware estiver conectado, validando em desenvolvimento a sequencia logica com mocks.

### Dependencias

FW-P01-005A.

### Descricao

Adicionar rotina de diagnostico para testar PWM nos GPIO25, GPIO26, GPIO27, GPIO14 e GPIO13. O teste deve aplicar niveis discretos de duty por canal conforme configuracao PWM de bring-up, um canal por vez, retornando cada canal para zero antes de avancar. Medicoes reais com multimetro, osciloscopio, analisador logico ou carga ficam marcadas como `Pendente para Hardware Validation`.

### Arquivos afetados

- `firmware/src/diagnostics/`
- `firmware/src/drivers/pwm/`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/config/pwm_bringup_config.h`

### Criterios de aceitacao

- Existe rotina de diagnostico para os cinco GPIOs PWM do pinout oficial.
- Cada canal PWM usa a frequencia e resolucao definidas para o bring-up.
- Cada canal PWM pode ser comandado para duty zero e para pelo menos dois niveis acima de zero.
- Apenas um canal PWM e comandado como ativo por vez durante o teste.
- Todos os canais retornam logicamente a duty zero ao final.
- Medicao fisica dos sinais PWM fica marcada como `Pendente para Hardware Validation`.
- Nao ha sunrise, sunset, moonlight, aclimatacao, curvas, horarios ou persistencia nesta fase.

### Criterios de teste

- Testar com mock PWM/LEDC que GPIO25, GPIO26, GPIO27, GPIO14 e GPIO13 recebem comandos individualmente.
- Confirmar com mock que os demais canais permanecem em duty zero enquanto um canal e testado.
- Confirmar com mock que todos os canais ficam em duty zero apos concluir a rotina.
- Confirmar que frequencia e resolucao PWM de bring-up sao aplicadas na configuracao.
- Confirmar que mocks usados no teste nao permanecem no build de bring-up para hardware real.

---

### ID

FW-P01-005B

### Titulo

Alinhar infraestrutura de bring-up com a estrategia de validacao sem hardware

### Objetivo

Resolver os ajustes arquiteturais e de testabilidade identificados na auditoria das tasks `FW-P01-001`, `FW-P01-002`, `FW-P01-002A`, `FW-P01-003`, `FW-P01-004`, `FW-P01-005A` e `FW-P01-005`, garantindo que o firmware de bring-up permaneca preparado para hardware real, mas validavel sem ESP32, sem sensores, sem modulo de reles e sem luminaria conectados.

### Dependencias

FW-P01-005.

### Descricao

Refatorar a infraestrutura de bring-up ja implementada para alinhar a Fase 1 a estrategia atual de validacao sem hardware fisico.

A tarefa deve introduzir abstracoes minimas e mockaveis para GPIO e PWM/LEDC, mantendo a implementacao real Arduino isolada do codigo de diagnostico. Mocks e fakes devem existir somente em `firmware/test/` ou em configuracao de teste, sem se tornarem dependencia do firmware final destinado ao hardware real.

Separar os diagnosticos por dominio para reduzir acoplamento e evitar crescimento excessivo de um unico arquivo de diagnostico. O diagnostico principal deve atuar apenas como dispatcher/orquestrador da Fase 1, delegando para diagnosticos especificos de pinout, reles, PWM e estado inicial seguro.

Remover dependencias indevidas entre headers publicos em `firmware/include/` e headers privados de `firmware/src/`, especialmente quando uma interface publica de diagnostico expoe tipos internos de drivers.

Adequar o template de relatorio da Fase 1 para usar exatamente a marca `Pendente para Hardware Validation` em todos os campos que dependem de validacao fisica futura, incluindo upload real, monitor Serial real, reboot real, impressao real da tabela de pinout, polaridade fisica dos reles, estado fisico seguro, resposta fisica dos reles e medicao fisica dos sinais PWM.

Adicionar testes automatizados ou estruturas de teste sem hardware para validar a logica ja preparada nas tasks anteriores: identificacao do modo Hardware Bring-Up, geracao da tabela de pinout, contrato `ACTIVE_HIGH`, estado inicial seguro, sequencia individual dos reles, parametros PWM de bring-up e sequencia PWM um canal por vez.

Esta tarefa nao deve alterar arquitetura, specs ou o plano mestre. Tambem nao deve iniciar diagnosticos de DS18B20, I2C, VL6180X, estabilidade 5V, ruido em sensores, consolidacao final ou template completo da `FW-P01-010`.

### Arquivos afetados

- `firmware/include/diagnostics/`
- `firmware/include/contracts/`
- `firmware/include/config/`
- `firmware/src/app/`
- `firmware/src/diagnostics/`
- `firmware/src/drivers/`
- `firmware/src/drivers/relays/`
- `firmware/src/drivers/pwm/`
- `firmware/test/unit/`
- `firmware/test/integration/`
- `firmware/test/hardware/`
- `docs/firmware-phase-01-hardware-bringup-report.md`
- `firmware/platformio.ini`, somente se necessario para ambiente de teste

### Criterios de aceitacao

- Existe uma abstracao minima para operacoes GPIO usadas no bring-up.
- Existe uma implementacao Arduino real para GPIO usada pelo firmware destinado ao hardware real.
- Existe mock ou fake GPIO restrito a `firmware/test/` ou configuracao de teste.
- Existe uma abstracao minima para PWM/LEDC usada no bring-up.
- Existe uma implementacao Arduino real para PWM/LEDC usada pelo firmware destinado ao hardware real.
- Existe mock ou fake PWM/LEDC restrito a `firmware/test/` ou configuracao de teste.
- O firmware final de bring-up nao depende de mocks ou fakes para compilar.
- Headers publicos em `firmware/include/` nao expoem tipos privados de `firmware/src/`.
- Diagnosticos ficam separados por dominio, no minimo: pinout, estado inicial seguro, reles e PWM.
- O diagnostico principal da Fase 1 atua como dispatcher/orquestrador e nao concentra toda a logica de dominio.
- A identificacao do modo Hardware Bring-Up pode ser validada sem hardware.
- A tabela de pinout pode ser gerada e testada sem Serial real.
- O contrato de reles registra `ACTIVE_HIGH`, OFF esperado como `LOW` e ON esperado como `HIGH`.
- A rotina de estado inicial seguro e testavel sem hardware.
- A rotina de teste individual dos reles e testavel sem hardware e garante que apenas um rele recebe ON logico por vez.
- A configuracao PWM de bring-up e testavel sem hardware.
- A rotina de teste PWM e testavel sem hardware e garante que apenas um canal recebe duty acima de zero por vez.
- Todos os canais PWM retornam logicamente para duty zero ao final da rotina.
- O relatorio usa exatamente `Pendente para Hardware Validation` para todos os campos que exigem validacao fisica futura.
- O relatorio nao afirma PASS fisico, medicao real ou validacao real que nao tenha ocorrido.
- Nenhum System State, Event Bus, Config Manager, Task Scheduler, Logger, Watchdog, persistencia, Wi-Fi, MQTT, alertas, modos operacionais, ATO automatico, leitura DS18B20, I2C/VL6180X ou curvas de iluminacao e implementado nesta tarefa.
- Nenhum arquivo em `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md` e alterado.

### Criterios de teste

- Executar build PlatformIO do firmware de bring-up para hardware real com sucesso.
- Confirmar que o build de hardware real nao inclui mocks ou fakes.
- Executar testes sem hardware para a identificacao do modo Hardware Bring-Up.
- Executar teste ou snapshot sem hardware da tabela de pinout.
- Executar teste sem hardware do contrato `ACTIVE_HIGH`, confirmando OFF `LOW` e ON `HIGH`.
- Executar teste com mock GPIO confirmando que a inicializacao segura aplica LOW nos quatro reles.
- Executar teste com mock PWM confirmando que todos os canais iniciam em duty zero.
- Executar teste com mock GPIO para cada rele: Recalque, Aquecedor, ATO e Reserva.
- Confirmar com mock GPIO que apenas o rele em teste recebe ON logico durante a rotina.
- Confirmar com mock GPIO que todos os reles retornam para OFF logico ao final da rotina.
- Executar teste sem hardware dos parametros PWM de bring-up: frequencia, resolucao, duty zero, duty minimo, duty maximo e canais LEDC.
- Executar teste com mock PWM/LEDC confirmando que GPIO25, GPIO26, GPIO27, GPIO14 e GPIO13 sao configurados com a frequencia e resolucao de bring-up.
- Confirmar com mock PWM/LEDC que apenas um canal PWM recebe duty acima de zero por vez.
- Confirmar com mock PWM/LEDC que todos os canais ficam em duty zero ao final da rotina PWM.
- Conferir que o relatorio contem `Pendente para Hardware Validation` nos campos de upload real, monitor Serial real, reboot real, tabela de pinout real, polaridade fisica, estado fisico seguro, resposta fisica dos reles e medicao fisica PWM.
- Conferir que o relatorio nao exige hardware fisico para concluir esta task.

---

### ID

FW-P01-006

### Titulo

Preparar diagnostico do DS18B20

### Objetivo

Criar driver minimo e rotina de diagnostico para o DS18B20 no GPIO4, com testes sem hardware usando fake ou mock de sensor.

### Dependencias

FW-P01-002.

### Descricao

Adicionar driver minimo de bring-up para o barramento OneWire e rotina de diagnostico separada para o DS18B20 no GPIO4. A rotina deve suportar resultados como `FOUND`, `NOT_FOUND` e `READ_ERROR`, imprimindo ou retornando status de diagnostico. A deteccao real e leitura real do sensor ficam marcadas como `Pendente para Hardware Validation`.

### Arquivos afetados

- `firmware/src/drivers/onewire/`
- `firmware/src/drivers/sensors/`
- `firmware/src/diagnostics/`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/platformio.ini`

### Criterios de aceitacao

- Existe driver minimo ou wrapper de bring-up para OneWire/DS18B20.
- Existe rotina de diagnostico para DS18B20 no GPIO4.
- A rotina informa status `FOUND`, `NOT_FOUND` ou `READ_ERROR`.
- Driver minimo e diagnostico ficam separados.
- Deteccao e leitura fisica do DS18B20 ficam marcadas como `Pendente para Hardware Validation`.
- A rotina nao implementa limites configuraveis, leitura a cada 5 segundos, SENSOR_OFFLINE, eventos ou MQTT.

### Criterios de teste

- Compilar com as dependencias necessarias para leitura futura do DS18B20.
- Testar com fake sensor o caso `FOUND` com temperatura simulada.
- Testar com fake sensor o caso `NOT_FOUND`.
- Testar com fake sensor o caso `READ_ERROR`.
- Confirmar que nenhum estado de temperatura ou evento foi criado.
- Confirmar que fake/mock de sensor nao permanece no build de bring-up para hardware real.

---

### ID

FW-P01-007

### Titulo

Preparar diagnostico do barramento I2C do VL6180X

### Objetivo

Criar driver minimo e rotina de diagnostico para o barramento I2C nos GPIO21 e GPIO22, com scanner preparado para identificar o VL6180X quando o hardware estiver conectado.

### Dependencias

FW-P01-002.

### Descricao

Adicionar driver minimo de bring-up para I2C e rotina de diagnostico separada para inicializar SDA no GPIO21 e SCL no GPIO22. A rotina deve executar varredura de enderecos e reportar dispositivos encontrados quando houver hardware. A deteccao real do VL6180X e a verificacao fisica de SDA/SCL ficam marcadas como `Pendente para Hardware Validation`.

### Arquivos afetados

- `firmware/src/drivers/i2c/`
- `firmware/src/drivers/sensors/`
- `firmware/src/diagnostics/`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/platformio.ini`

### Criterios de aceitacao

- Existe driver minimo ou wrapper de bring-up para I2C.
- O diagnostico inicializa I2C com SDA no GPIO21 e SCL no GPIO22.
- Existe rotina de scanner I2C para uso futuro com hardware.
- Driver minimo e diagnostico ficam separados.
- Deteccao real do VL6180X fica marcada como `Pendente para Hardware Validation`.
- A rotina nao implementa waterLevel, calibracao, LOW_LEVEL, SENSOR_OFFLINE, ATO ou MQTT.

### Criterios de teste

- Testar com fake bus I2C um resultado com endereco encontrado.
- Testar com fake bus I2C um resultado sem dispositivos.
- Testar que SDA e SCL usados pelo diagnostico correspondem ao contrato de pinout.
- Confirmar que fake/mock I2C nao permanece no build de bring-up para hardware real.
- Registrar no template de relatorio que a verificacao fisica de SDA/SCL esta pendente.

---

### ID

FW-P01-007A

### Titulo

Preparar comparacao de endereco I2C do VL6180X

### Objetivo

Registrar o endereco I2C esperado do VL6180X e preparar a rotina de comparacao entre endereco esperado e endereco encontrado para uso futuro em bancada.

### Dependencias

FW-P01-007.

### Descricao

Registrar no contrato de hardware o endereco I2C esperado para o VL6180X e preparar diagnostico que compare esse valor com o endereco encontrado pela varredura. Como nao ha hardware conectado, a confirmacao do endereco real no barramento fica marcada como `Pendente para Hardware Validation`.

### Arquivos afetados

- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/contracts/i2c_contract.h`
- `firmware/src/diagnostics/`
- `docs/firmware-phase-01-hardware-bringup-report.md`

### Criterios de aceitacao

- O endereco I2C esperado do VL6180X esta definido no contrato.
- Existe rotina de diagnostico para comparar endereco esperado e endereco encontrado.
- A rotina consegue reportar `MATCH`, `NOT_FOUND` ou `MISMATCH`.
- O template de relatorio possui campo para registrar endereco real encontrado futuramente.
- Confirmacao do endereco fisico fica marcada como `Pendente para Hardware Validation`.
- Nenhum comportamento de Level Module ou ATO e implementado.

### Criterios de teste

- Testar com fake bus I2C o caso `MATCH`.
- Testar com fake bus I2C o caso `NOT_FOUND`.
- Testar com fake bus I2C o caso `MISMATCH`.
- Confirmar que o endereco esperado aparece no resumo de diagnostico simulado.
- Confirmar que fake/mock I2C nao permanece no build de bring-up para hardware real.

---

### ID

FW-P01-008

### Titulo

Preparar leitura basica do VL6180X

### Objetivo

Criar rotina de leitura basica do VL6180X para uso futuro em bancada, com testes sem hardware usando fake ou mock de sensor.

### Dependencias

FW-P01-007A.

### Descricao

Adicionar rotina de diagnostico para realizar uma leitura simples do VL6180X e reportar valor bruto, valor convertido ou erro. A leitura deve ser usada somente para verificar resposta futura do sensor e variacao fisica em bancada. Leitura real, variacao por distancia e estabilidade fisica ficam marcadas como `Pendente para Hardware Validation`.

### Arquivos afetados

- `firmware/src/drivers/sensors/`
- `firmware/src/diagnostics/`
- `firmware/include/contracts/hardware_pins.h`
- `firmware/include/contracts/i2c_contract.h`
- `firmware/platformio.ini`

### Criterios de aceitacao

- Existe rotina de diagnostico para leitura basica do VL6180X.
- A rotina consegue reportar leitura simulada valida.
- A rotina consegue reportar falha simulada de leitura.
- O template de relatorio possui campos para leitura real, variacao por distancia e estabilidade em repouso.
- Leitura fisica do VL6180X fica marcada como `Pendente para Hardware Validation`.
- Nao ha calibracao, limites minimo/maximo, status de nivel, ATO ou alertas nesta fase.

### Criterios de teste

- Testar com fake sensor leitura valida.
- Testar com fake sensor falha de leitura.
- Testar que o diagnostico nao cria estados de waterLevel.
- Confirmar que fake/mock de sensor nao permanece no build de bring-up para hardware real.
- Registrar no template de relatorio que leitura real e variacao por distancia estao pendentes.

---

### ID

FW-P01-009A

### Titulo

Preparar validacao de estabilidade do barramento 5V

### Objetivo

Criar procedimento, checklist e campos de relatorio para medir futuramente a estabilidade do barramento 5V durante acionamento dos reles.

### Dependencias

FW-P01-004.

### Descricao

Adicionar procedimento de bancada ao diagnostico da Fase 1 para orientar medicao do barramento 5V em repouso, durante acionamento individual dos reles e durante acionamento sequencial controlado. Nesta fase, o entregavel e a rotina/procedimento preparado e o template de relatorio. Medicoes reais ficam marcadas como `Pendente para Hardware Validation`.

### Arquivos afetados

- `firmware/src/diagnostics/`
- `docs/firmware-phase-01-hardware-bringup-report.md`

### Criterios de aceitacao

- Existe procedimento ou checklist para medicao futura do 5V em repouso.
- Existe procedimento ou checklist para medicao futura do 5V durante acionamento individual dos quatro reles.
- Existe procedimento ou checklist para medicao futura do 5V durante acionamento sequencial controlado.
- O template de relatorio possui campos para PASS/FAIL ou pendencia eletrica de estabilidade 5V.
- Medicao real do 5V fica marcada como `Pendente para Hardware Validation`.
- Nenhuma correcao eletrica ou funcionalidade de fail-safe e implementada nesta tarefa.

### Criterios de teste

- Verificar que o procedimento de bancada foi criado.
- Verificar que o template de relatorio contem campos para tensao em repouso, tensao por rele e tensao em sequencia.
- Testar com mock GPIO que a sequencia futura de acionamento retorna todos os reles para OFF esperado.
- Confirmar que mock GPIO nao permanece no build de bring-up para hardware real.
- Confirmar que nao ha exigencia de multimetro ou medicao real para concluir a Fase 1.

---

### ID

FW-P01-009B

### Titulo

Preparar validacao de ruido nos sensores durante acionamento dos reles

### Objetivo

Criar rotina, formato de log e campos de relatorio para avaliar futuramente ruido ou interferencia nos sensores durante acionamento dos reles.

### Dependencias

FW-P01-006, FW-P01-008, FW-P01-009A.

### Descricao

Adicionar procedimento de diagnostico para registrar leituras basicas dos sensores antes, durante e apos acionamentos controlados dos reles quando o hardware estiver conectado. Nesta fase, o entregavel e a rotina/logica preparada e testada com mocks. Ruido real, perda real de comunicacao, resets reais e leituras impossiveis reais ficam marcados como `Pendente para Hardware Validation`.

### Arquivos afetados

- `firmware/src/diagnostics/`
- `docs/firmware-phase-01-hardware-bringup-report.md`

### Criterios de aceitacao

- Existe rotina ou procedimento para coletar leitura basica futura do DS18B20 antes e apos acionamento dos reles.
- Existe rotina ou procedimento para coletar leitura basica futura do VL6180X antes e apos acionamento dos reles.
- Existe formato de log para registrar perda de leitura, reset ou oscilacao grosseira.
- Todos os reles e PWM retornam logicamente ao estado seguro ao final da rotina.
- Validacao real de ruido/interferencia fica marcada como `Pendente para Hardware Validation`.
- Nenhum filtro, alerta, evento ou automacao e implementado.

### Criterios de teste

- Testar com mocks de sensores e GPIO a coleta antes/durante/depois.
- Testar com fake sensor um caso de leitura normal.
- Testar com fake sensor um caso de falha simulada.
- Testar que a rotina retorna reles e PWM ao estado seguro ao final.
- Confirmar que mocks/fakes nao permanecem no build de bring-up para hardware real.
- Confirmar que nao ha exigencia de sensores, reles ou ESP32 reais para concluir a Fase 1.

---

### ID

FW-P01-009

### Titulo

Consolidar prontidao dos diagnosticos da Fase 1

### Objetivo

Consolidar a prontidao dos diagnosticos, contratos, drivers minimos, templates e pendencias de Hardware Validation em um resumo final da Fase 1.

### Dependencias

FW-P01-005, FW-P01-009B.

### Descricao

Adicionar uma rotina integrada de consolidacao que apresente, sem exigir hardware conectado, o estado dos diagnosticos preparados: pinout, estado inicial seguro, polaridade `ACTIVE_HIGH` dos reles, teste individual dos reles, teste dos PWM, diagnostico DS18B20, diagnostico I2C/VL6180X, checklist de estabilidade 5V e checklist de ruido/interferencia. O resumo deve usar estados como `READY`, `NOT_IMPLEMENTED` e `PENDING_HARDWARE_VALIDATION`, sem exigir PASS fisico de perifericos.

### Arquivos afetados

- `firmware/src/diagnostics/`
- `firmware/include/diagnostics/`
- `firmware/src/app/`
- `firmware/src/main.cpp`

### Criterios de aceitacao

- A consolidacao cobre estrutura, contratos, GPIOs, reles, PWM, DS18B20 e VL6180X.
- O resumo final mostra prontidao dos diagnosticos por grupo.
- O resumo inclui polaridade `ACTIVE_HIGH` dos reles, parametros PWM de bring-up, endereco I2C esperado do VL6180X, checklist de estabilidade 5V e checklist de ruido/interferencia.
- Validacoes fisicas aparecem como `PENDING_HARDWARE_VALIDATION`.
- Todos os reles e PWM sao comandados logicamente para estado seguro ao final da consolidacao.
- A rotina nao cria funcionalidades das fases 2 a 16.

### Criterios de teste

- Testar a consolidacao sem hardware conectado usando mocks/fakes.
- Confirmar que todos os grupos preparados aparecem como `READY`.
- Confirmar que validacoes fisicas aparecem como `PENDING_HARDWARE_VALIDATION`.
- Confirmar que falha simulada em um diagnostico nao impede os demais grupos de serem reportados.
- Confirmar que mocks/fakes nao permanecem no build de bring-up para hardware real.

---

### ID

FW-P01-010

### Titulo

Criar template de relatorio de Hardware Validation

### Objetivo

Criar um template vazio de relatorio para registrar futuramente os resultados reais de bancada, sem exigir resultados fisicos durante a Fase 1.

### Dependencias

FW-P01-009.

### Descricao

Criar um template de validacao contendo data, placa usada, resultado de boot, estrutura de firmware criada, tabela de pinout conferida, polaridade `ACTIVE_HIGH` dos reles, resultado futuro dos quatro reles, parametros PWM de bring-up, resultado futuro dos cinco GPIOs PWM, resultado futuro do DS18B20, endereco I2C esperado e encontrado do VL6180X, leitura futura do VL6180X, estabilidade futura do barramento 5V e ruido futuro observado durante acionamento dos reles. Campos que dependem de hardware devem iniciar como `Pendente para Hardware Validation`.

### Arquivos afetados

- `docs/firmware-phase-01-hardware-bringup-report.md`

### Criterios de aceitacao

- O template de relatorio existe.
- O template inclui campos para todos os perifericos da Fase 1.
- O template inclui polaridade `ACTIVE_HIGH` dos reles.
- O template inclui os parametros PWM utilizados no bring-up.
- O template inclui endereco I2C esperado do VL6180X.
- Campos que exigem hardware fisico iniciam como `Pendente para Hardware Validation`.
- O template nao exige PASS/FAIL real para concluir a Fase 1.
- O template nao altera `architecture/`, `specs/` ou `tasks/firmware/firmware-master-plan.md`.

### Criterios de teste

- Conferir que o template existe e contem todos os grupos de perifericos da Fase 1.
- Conferir que campos de validacao fisica estao marcados como `Pendente para Hardware Validation`.
- Conferir que polaridade dos reles, parametros PWM de bring-up e endereco I2C esperado foram registrados.
- Verificar que nenhuma alteracao foi feita em arquivos de arquitetura ou specs.
- Confirmar que o template esta pronto para ser preenchido quando o modulo real for conectado e testado na pratica.
