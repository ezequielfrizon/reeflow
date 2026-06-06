# Visão Geral

O firmware do REEFLOW deve transformar o ESP32 na autoridade soberana do controlador de aquário marinho.

Toda automação crítica deve existir localmente no firmware e continuar operacional sem Internet, MQTT, cloud ou aplicativo. Cloud e aplicativo são extensões do sistema: refletem o estado oficial do ESP32 ou solicitam alterações, mas não substituem responsabilidades críticas do firmware.

O desenvolvimento deve seguir a ordem oficial definida no roadmap, com implementação incremental, hardware validado antes de cloud e aplicativo, arquitetura modular e uso obrigatório do estado global definido em `system-state-spec.md`.

A fonte única de verdade é o estado global mantido pelo firmware, composto por:

- `system`
- `temperature`
- `waterLevel`
- `lighting`
- `relays`
- `modes`
- `ato`
- `network`
- `alerts`
- `systemHealth`

Todo módulo deve atualizar ou consumir esse estado global. Toda telemetria MQTT, histórico, alerta e dado exibido pelo aplicativo deve ser derivado desse estado.

# Ordem Oficial de Implementação

## Fase 0 — Estrutura do Projeto

Preparar a base do desenvolvimento com estrutura de pastas, documentos de arquitetura, specs funcionais, roadmap, configuração PlatformIO e padrões de código.

## Fase 1 — Hardware Bring-Up

Validar inicialização do ESP32, mapeamento oficial de GPIO, relés, PWM e sensores definidos para a versão V1 do hardware.

## Fase 2 — Core do Firmware

Implementar a fundação modular do firmware: System State, Event Bus, Config Manager, Task Scheduler, Logger e Watchdog.

## Fase 3 — Sensor de Temperatura

Implementar o monitoramento do DS18B20 no GPIO4, com leitura periódica, estados de temperatura, detecção de sensor offline e eventos.

## Fase 4 — Sensor de Nível

Implementar leitura do VL6180X via I2C nos GPIO21 e GPIO22, com calibração, estados do sensor, detecção de falhas e atualização do `waterLevel`.

## Fase 5 — Sistema de Relés

Controlar os relés de recalque, aquecedor, ATO e reserva nos GPIO16, GPIO17, GPIO18 e GPIO19, incluindo controle manual e eventos.

## Fase 6 — Sistema ATO

Automatizar a reposição de água doce usando o nível de água e o Relé 3, com timeout, cooldown, proteções, fail-safe, estados e eventos.

## Fase 7 — Sistema de Modos

Implementar os modos NORMAL, FEEDING, TPA e MAINTENANCE, incluindo persistência do modo atual e retorno automático quando especificado.

## Fase 8 — Sistema de Iluminação

Controlar a luminária por PWM nos canais Branco, Azul, Royal Blue e UV, com sunrise, sunset, moonlight, aclimatação, curvas personalizadas, limite máximo de intensidade e fade suave obrigatório.

## Fase 9 — Persistência Local

Persistir configurações em NVS Preferences, incluindo Wi-Fi, MQTT, curvas da luminária, configurações do ATO, limites de temperatura, timers, modo atual e calibrações.

## Fase 10 — Rede

Implementar Wi-Fi, reconexão automática, NTP, heartbeat e status de rede no estado global.

## Fase 11 — MQTT

Implementar comunicação em tempo real com Mosquitto, publicação de telemetria derivada do estado global, recebimento de comandos, reconexão, QoS e heartbeat.

## Fase 12 — Sistema de Alertas

Centralizar alertas com categorias, cooldown, priorização, recuperação e publicação MQTT.

## Fase 13 — Testes de Resiliência

Validar recuperação automática em reboot inesperado, perda de Wi-Fi, perda de MQTT, sensor offline, falha de energia e ATO travado.

## Fase 14 — Infraestrutura Cloud

Integrar a camada remota após o firmware estar operacional, mantendo o ESP32 como autoridade máxima do sistema.

## Fase 15 — Aplicativo Mobile

Integrar a interface do usuário após a cloud, consumindo exclusivamente o estado oficial do ESP32 e solicitando alterações sempre através do firmware.

## Fase 16 — Beta Integrado

Validar o ecossistema completo com firmware, MQTT, cloud, aplicativo, persistência e alertas.

# Dependências entre Fases

- Fase 0 é pré-requisito para todas as demais fases.
- Fase 1 deve concluir a validação elétrica e de periféricos antes de qualquer automação.
- Fase 2 é pré-requisito para todos os módulos funcionais, pois define estado global, eventos, configuração, agendamento, logs e watchdog.
- Fase 3 depende das Fases 1 e 2 para leitura do DS18B20 e atualização do `temperature`.
- Fase 4 depende das Fases 1 e 2 para leitura do VL6180X e atualização do `waterLevel`.
- Fase 5 depende das Fases 1 e 2 para controle seguro dos relés e atualização do `relays`.
- Fase 6 depende das Fases 4 e 5, pois o ATO usa o sensor de nível e a bomba no Relé 3.
- Fase 7 depende da Fase 5 e deve coordenar o comportamento dos relés e automações conforme o modo atual.
- Fase 8 depende das Fases 1 e 2 para PWM e atualização do `lighting`.
- Fase 9 depende dos módulos que possuem configurações persistíveis e deve restaurar o estado após reboot.
- Fase 10 depende da Fase 2 e deve atualizar `network` sem afetar automações críticas locais.
- Fase 11 depende das Fases 2 e 10, e toda publicação deve ser derivada do estado global.
- Fase 12 depende dos eventos e estados produzidos pelos módulos anteriores.
- Fase 13 depende da conclusão dos sistemas críticos locais.
- Fases 14, 15 e 16 dependem do firmware funcional e não podem assumir responsabilidades críticas do ESP32.

# Critérios de Conclusão de Cada Fase

## Fase 0

Projeto organizado e pronto para desenvolvimento.

## Fase 1

Todos os periféricos definidos no hardware V1 respondem corretamente usando o pinout oficial.

## Fase 2

Arquitetura base funcionando, com todos os módulos usando o estado global como referência.

## Fase 3

Temperatura atualizada corretamente, estados refletidos em `temperature` e eventos gerados conforme a spec.

## Fase 4

Nível de água lido de forma confiável, com estados refletidos em `waterLevel` e falhas detectadas.

## Fase 5

Todos os relés são controlados pelo firmware, iniciam desligados após boot salvo exigência de restauração de modo, e publicam mudanças no estado global.

## Fase 6

ATO funcional e seguro, com acionamento, parada, timeout, cooldown, proteções e estados refletidos em `ato`.

## Fase 7

Mudança de modo funcionando, com persistência do modo atual e comportamento conforme NORMAL, FEEDING, TPA e MAINTENANCE.

## Fase 8

Luminária automatizada, com PWM por canal, curvas, horários, aclimatação, limite máximo e fade suave.

## Fase 9

Sistema restaura configurações e estado persistível após reinicialização, evitando gravações excessivas.

## Fase 10

Conectividade Wi-Fi estável, reconexão automática, NTP, heartbeat e status de rede atualizados.

## Fase 11

Controle remoto via MQTT funcionando, com telemetria derivada do estado global, comandos recebidos pelo ESP32, QoS conforme especificado e reconexão automática.

## Fase 12

Alertas consistentes em todo o sistema, com cooldown configurável, recuperação e publicação.

## Fase 13

Sistema recupera operação automaticamente nos cenários de resiliência definidos.

## Fase 14

Cloud sincronizada com o firmware sem substituir responsabilidades críticas locais.

## Fase 15

Aplicativo operacional, consumindo o estado global oficial e enviando solicitações sempre através do ESP32.

## Fase 16

Sistema completo pronto para uso contínuo.

# Riscos Técnicos

- Divergência entre módulos e estado global, quebrando a fonte única de verdade.
- Dependência indevida de MQTT, cloud ou aplicativo para automações críticas.
- Alteração acidental do pinout oficial ou do hardware V1 congelado.
- Falha de sensor DS18B20 causando temperatura em estado inválido ou sem alerta.
- Falha de sensor VL6180X causando leitura impossível, sensor offline ou acionamento indevido do ATO.
- ATO travado por timeout não tratado, ciclo excessivo, anti-rearme ausente ou sensor inválido.
- Relés ligando em estado inseguro após boot ou restauração incorreta de modo.
- Persistência local com gravações excessivas em NVS Preferences.
- Reboot inesperado sem restauração correta de configurações, modo atual e calibrações.
- Perda de Wi-Fi ou MQTT afetando indevidamente a operação local.
- Alertas repetidos sem cooldown ou sem evento de recuperação.
- Telemetria MQTT publicada diretamente por módulos sem derivar do estado global.

# Estratégia de Testes

Os testes devem seguir a ordem incremental do roadmap. Cada fase deve resultar em um sistema funcional e testável antes do avanço para a próxima.

Na validação de hardware, todos os GPIO oficiais, relés, PWM e sensores devem ser verificados conforme a arquitetura elétrica.

Na validação do core, System State, Event Bus, Config Manager, Task Scheduler, Logger e Watchdog devem demonstrar operação integrada.

Nos módulos funcionais, cada leitura, comando, transição de estado e evento deve atualizar o estado global correspondente antes de qualquer publicação MQTT ou consumo externo.

Na persistência, as configurações especificadas devem sobreviver ao reboot e ser salvas apenas quando houver mudança.

Na comunicação, a telemetria deve ser derivada do estado global, com heartbeat, reconexão automática, QoS 1 para eventos importantes e QoS 0 para telemetria.

Na resiliência, devem ser validados os cenários oficiais: reboot inesperado, perda de Wi-Fi, perda de MQTT, sensor offline, falha de energia e ATO travado.

# Marcos de Integração

- Integração de hardware: ESP32 inicializa e todos os periféricos respondem no pinout oficial.
- Integração de core: módulos passam a usar System State e Event Bus como base comum.
- Integração de sensores: temperatura e nível atualizam `temperature` e `waterLevel`.
- Integração de atuadores: relés e iluminação atualizam `relays` e `lighting`.
- Integração de automação local: ATO e modos operam sem dependência externa.
- Integração de persistência: configurações, curvas, timers, Wi-Fi, MQTT, calibrações e modo atual sobrevivem ao reboot.
- Integração de rede: Wi-Fi, NTP e heartbeat atualizam `network`.
- Integração MQTT: telemetria, eventos e comandos passam pelo ESP32 e refletem o estado global.
- Integração de alertas: eventos críticos geram alertas com cooldown, recuperação e histórico.
- Integração cloud: cloud sincroniza com o firmware sem assumir controle crítico.
- Integração aplicativo: aplicativo exibe dados derivados do estado oficial e solicita comandos ao ESP32.
- Beta integrado: firmware, MQTT, cloud, aplicativo, persistência e alertas operam em conjunto.

# Definition of Done para cada fase

## Fase 0

Concluída quando a estrutura do projeto, documentação, specs, roadmap, PlatformIO e padrões de código estiverem prontos para desenvolvimento.

## Fase 1

Concluída quando ESP32, GPIO, relés, PWM, DS18B20 e VL6180X forem validados conforme hardware V1.

## Fase 2

Concluída quando o firmware possuir base modular operacional e todos os módulos passarem a utilizar o estado global.

## Fase 3

Concluída quando o DS18B20 atualizar a temperatura a cada 5 segundos, detectar sensor offline após mais de 30 segundos sem leitura válida, gerar eventos e refletir estados no `temperature`.

## Fase 4

Concluída quando o VL6180X fornecer leitura periódica confiável, calibração, detecção de falhas e estados refletidos no `waterLevel`.

## Fase 5

Concluída quando recalque, aquecedor, ATO e reserva forem controlados pelo firmware, com eventos e origem das mudanças registrada no estado dos relés.

## Fase 6

Concluída quando o ATO iniciar por nível mínimo, parar por nível máximo, desligar em timeout, respeitar cooldown e registrar eventos e estados.

## Fase 7

Concluída quando NORMAL, FEEDING, TPA e MAINTENANCE alterarem o comportamento previsto, persistirem o modo atual e emitirem eventos.

## Fase 8

Concluída quando os canais Branco, Azul, Royal Blue e UV operarem por PWM com fade suave, curvas, horários, aclimatação e limite máximo persistíveis.

## Fase 9

Concluída quando NVS Preferences salvar e restaurar Wi-Fi, MQTT, curvas, ATO, temperatura, timers, modo atual e calibrações sem gravações excessivas.

## Fase 10

Concluída quando Wi-Fi, reconexão automática, NTP, heartbeat e status de rede estiverem operacionais no `network`.

## Fase 11

Concluída quando MQTT publicar telemetria derivada do estado global, receber comandos, reconectar automaticamente, publicar heartbeat e aplicar QoS conforme especificado.

## Fase 12

Concluída quando os alertas oficiais forem centralizados, categorizados, publicados, sincronizáveis, protegidos por cooldown e acompanhados de recuperação.

## Fase 13

Concluída quando o sistema demonstrar recuperação automática nos testes oficiais de resiliência sem depender de cloud ou aplicativo.

## Fase 14

Concluída quando Supabase, PostgreSQL, Edge Functions, MQTT Bridge e histórico estiverem sincronizados com o firmware.

## Fase 15

Concluída quando login, dashboard, controle remoto, histórico, configurações e alertas estiverem operacionais através do estado oficial do ESP32.

## Fase 16

Concluída quando firmware, MQTT, cloud, aplicativo, persistência e alertas forem validados em operação integrada e contínua.
