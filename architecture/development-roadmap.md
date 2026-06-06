# REEFLOW Development Roadmap

## Objetivo

Definir a ordem oficial de desenvolvimento do projeto REEFLOW.

Toda implementação deve seguir este roadmap.

O objetivo é reduzir complexidade, evitar retrabalho e permitir evolução incremental do sistema.

---

# Princípios

## ESP32 é soberano

O firmware deve ser totalmente funcional sem:

* Internet
* MQTT
* Cloud
* Aplicativo

Cloud e App são extensões do sistema.

---

## Implementação incremental

Cada fase deve resultar em um sistema funcional e testável.

Nenhuma fase deve depender de funcionalidades futuras.

---

## Hardware primeiro

Todo hardware deve estar operacional antes da implementação da cloud e do aplicativo.

---

# Fase 0 — Estrutura do Projeto

## Objetivo

Preparar a base do desenvolvimento.

## Entregáveis

Estrutura de pastas.

Documentos de arquitetura.

Specs funcionais.

Roadmap.

Configuração PlatformIO.

Padrões de código.

## Critério de conclusão

Projeto organizado e pronto para desenvolvimento.

---

# Fase 1 — Hardware Bring-Up

## Objetivo

Validar toda a eletrônica.

## Implementar

Inicialização do ESP32.

Mapeamento de GPIO.

Teste dos relés.

Teste dos PWM.

Teste dos sensores.

## Critério de conclusão

Todos os periféricos respondem corretamente.

---

# Fase 2 — Core do Firmware

## Objetivo

Criar a fundação do sistema.

## Implementar

System State.

Event Bus.

Config Manager.

Task Scheduler.

Logger.

Watchdog.

## Critério de conclusão

Arquitetura base funcionando.

Todos os módulos utilizam o estado global.

---

# Fase 3 — Sensor de Temperatura

## Objetivo

Implementar monitoramento térmico.

## Implementar

Driver DS18B20.

Leitura periódica.

Estados de temperatura.

Detecção de sensor offline.

Eventos.

## Critério de conclusão

Temperatura atualizada corretamente.

Eventos gerados corretamente.

---

# Fase 4 — Sensor de Nível

## Objetivo

Implementar leitura do VL6180X.

## Implementar

Driver I2C.

Leitura periódica.

Calibração.

Estados do sensor.

Detecção de falhas.

## Critério de conclusão

Leitura confiável do nível de água.

---

# Fase 5 — Sistema de Relés

## Objetivo

Controlar equipamentos.

## Implementar

Recalque.

Aquecedor.

ATO.

Reserva.

Controle manual.

Eventos.

## Critério de conclusão

Todos os relés controlados pelo firmware.

---

# Fase 6 — Sistema ATO

## Objetivo

Automatizar reposição de água doce.

## Implementar

Leitura do nível.

Acionamento da bomba.

Timeout.

Cooldown.

Fail-safe.

Estados.

Eventos.

## Critério de conclusão

ATO totalmente funcional e seguro.

---

# Fase 7 — Sistema de Modos

## Objetivo

Criar comportamento operacional.

## Implementar

Modo Normal.

Modo Alimentação.

Modo TPA.

Modo Manutenção.

Persistência dos modos.

## Critério de conclusão

Mudança de modo funcionando.

---

# Fase 8 — Sistema de Iluminação

## Objetivo

Controlar a luminária.

## Implementar

PWM.

Curvas.

Sunrise.

Sunset.

Moonlight.

Aclimatação.

Fade suave.

## Critério de conclusão

Luminária totalmente automatizada.

---

# Fase 9 — Persistência Local

## Objetivo

Garantir sobrevivência ao reboot.

## Implementar

NVS.

Configurações.

Curvas.

Timers.

Wi-Fi.

Calibrações.

Modo atual.

## Critério de conclusão

Sistema restaura estado após reinicialização.

---

# Fase 10 — Rede

## Objetivo

Conectar o controlador à rede.

## Implementar

Wi-Fi.

Reconexão automática.

NTP.

Heartbeat.

Status de rede.

## Critério de conclusão

Conectividade estável.

---

# Fase 11 — MQTT

## Objetivo

Comunicação em tempo real.

## Implementar

Broker.

Publicação de telemetria.

Recebimento de comandos.

Reconexão.

QoS.

Heartbeat.

## Critério de conclusão

Controle remoto via MQTT.

---

# Fase 12 — Sistema de Alertas

## Objetivo

Centralizar eventos críticos.

## Implementar

Alert Manager.

Cooldown.

Priorização.

Recuperação.

## Critério de conclusão

Alertas consistentes em todo o sistema.

---

# Fase 13 — Testes de Resiliência

## Objetivo

Validar estabilidade.

## Testes

Reboot inesperado.

Perda de Wi-Fi.

Perda de MQTT.

Sensor offline.

Falha de energia.

ATO travado.

## Critério de conclusão

Sistema recupera operação automaticamente.

---

# Fase 14 — Infraestrutura Cloud

## Objetivo

Criar camada remota.

## Implementar

Supabase.

Banco PostgreSQL.

Edge Functions.

MQTT Bridge.

Histórico.

## Critério de conclusão

Cloud sincronizada com o firmware.

---

# Fase 15 — Aplicativo Mobile

## Objetivo

Disponibilizar interface para o usuário.

## Implementar

Login.

Dashboard.

Controle remoto.

Histórico.

Configurações.

Alertas.

## Critério de conclusão

Aplicativo operacional.

---

# Fase 16 — Beta Integrado

## Objetivo

Validar todo o ecossistema.

## Validar

Firmware.

MQTT.

Cloud.

App.

Persistência.

Alertas.

## Critério de conclusão

Sistema pronto para uso contínuo.

---

# Funcionalidades Futuras

## Sensores

pH

Salinidade

ORP

Condutividade

TDS

---

## Equipamentos

Dosadoras.

Skimmer.

Fans.

UV.

Bombas adicionais.

---

## Plataforma

Dashboard Web.

Múltiplos aquários.

Múltiplos ESP32.

Controle por voz.

Alexa.

Google Home.

---

# Regra Fundamental

Nenhuma funcionalidade de cloud ou aplicativo pode substituir uma responsabilidade crítica do firmware.

Toda automação crítica deve existir localmente no ESP32.
