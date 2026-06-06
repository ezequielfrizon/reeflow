# CONTROLADOR DE AQUÁRIO MARINHO DIY

## Documento 3 — Plataforma Cloud e Aplicativo

### Objetivo

Fornecer acesso remoto, histórico, configurações e notificações sem assumir controle crítico do aquário.

O ESP32 continua sendo a autoridade máxima do sistema.

---

# Stack Tecnológica

## Firmware

ESP32

Arduino Framework

---

## Comunicação

MQTT

Broker Mosquitto

---

## Backend

Supabase

Recursos:

* PostgreSQL
* Auth
* Realtime
* Edge Functions
* Storage

---

## Aplicativo

React Native

Expo

Android e iOS

---

# Arquitetura Geral

ESP32
↓
MQTT Broker
↓
Bridge MQTT ↔ Supabase
↓
Supabase
↓
Aplicativo

---

# Dashboard

Exibir:

* Temperatura
* Nível da água
* Status do ESP32
* Estado dos relés
* Estado da luminária
* Modo atual

---

# Controle Remoto

Permitir:

* Alterar modo
* Acionar equipamentos
* Ajustar luminária
* Configurar ATO

Sempre através do ESP32.

---

# Histórico

Armazenar:

* Temperatura
* Nível
* Eventos
* Modos
* Reboots
* Alertas

Visualização:

* Diário
* Semanal
* Mensal

---

# Alertas

Push Notifications:

* Temperatura alta
* Temperatura baixa
* Sensor offline
* ATO travado
* ESP32 offline
* Wi-Fi offline
* MQTT offline

Sistema anti-spam obrigatório.

---

# Configurações

Wi-Fi

MQTT

ATO

Luminária

Timers

Modos

Backup

---

# Autenticação

Login

Sessão persistente

Biometria

Recuperação de senha

Multiusuário

---

# Funcionalidades Futuras

* pH
* Salinidade
* ORP
* Dosadoras
* Múltiplos ESP32
* Controle por voz
* Alexa
* Google Home
* Câmeras
* Dashboard Web

---

# Diretriz de Desenvolvimento com IA

Toda implementação deve ser pensada para geração assistida por IA.

Prioridade:

1. Firmware ESP32
2. Banco Supabase
3. MQTT
4. Edge Functions
5. Aplicativo React Native
6. Testes
7. Documentação

Objetivo final:

Sistema modular, escalável, resiliente e independente da cloud para operação crítica.
