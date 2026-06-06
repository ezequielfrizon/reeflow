# CONTROLADOR DE AQUÁRIO MARINHO DIY

## Documento 2 — Arquitetura do Firmware ESP32

### Objetivo

Transformar o ESP32 no controlador soberano do aquário.

Todo comportamento crítico deve existir localmente.

O sistema deve continuar funcionando sem:

* Internet
* MQTT
* Cloud
* Aplicativo

---

# Responsabilidades do Firmware

## Sensores

Leitura contínua:

* DS18B20
* VL6180X

Futuros:

* pH
* Salinidade
* ORP
* Sensores adicionais

---

# Controle de Relés

Relé 1

* Bomba recalque

Relé 2

* Aquecedor

Relé 3

* ATO

Relé 4

* Reserva

---

# Controle da Luminária

PWM independente por canal.

Canais:

* Branco
* Azul
* Royal Blue
* Moonlight

Funções:

* Sunrise
* Sunset
* Moonlight
* Aclimatação
* Curvas personalizadas
* Fade suave
* Limite máximo de intensidade

---

# Modos Operacionais

## Normal

Operação padrão.

## Alimentação

Desliga recalque temporariamente.

Retorno automático.

## TPA

Desliga equipamentos configurados.

Retorno automático.

## Manutenção

Desliga equipamentos selecionados.

Sem automações críticas.

---

# Sistema ATO

Responsável por:

* Detectar evaporação
* Acionar bomba
* Confirmar recuperação de nível

Proteções:

* Timeout máximo
* Anti-loop
* Anti-rearme imediato
* Sensor inválido

---

# Controle de Temperatura

Leitura via DS18B20.

Funções:

* Monitoramento
* Alertas
* Fail-safe

O termostato físico continua sendo a proteção principal.

---

# Persistência Local

Salvar em memória:

* Curvas da luminária
* Configurações
* Timers
* Modos
* Calibrações
* Wi-Fi

Após reboot:

Restaurar estado automaticamente.

---

# Comunicação

Wi-Fi

MQTT

NTP

Heartbeat

Reconexão automática

Fila de mensagens

---

# Fail-Safe

Implementar:

* Watchdog
* Recuperação após reboot
* Recuperação Wi-Fi
* Recuperação MQTT
* Sensor offline
* Temperatura fora da faixa
* ATO travado

---

# Estrutura Recomendada

Core
Sensors
ATO
Relays
Lighting
Modes
Storage
MQTT
WiFi
NTP
Alerts
Watchdog

Arquitetura modular.

---

# Objetivo Final

O firmware deve ser capaz de operar o aquário integralmente sem qualquer dependência externa.
