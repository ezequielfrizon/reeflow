# REEFLOW Cloud/Backend Master Plan

## Objetivo

Definir o plano mestre exclusivo da infraestrutura cloud/backend do REEFLOW.

A cloud deve fornecer acesso remoto, historico, autenticacao, sincronizacao e notificacoes sem assumir controle critico do aquario. O ESP32 permanece a autoridade maxima do sistema.

## Escopo do Dominio Cloud/Backend

Pertence a cloud/backend:

- Broker Mosquitto e integracao MQTT conforme contrato do firmware.
- Bridge MQTT <-> Supabase.
- Supabase PostgreSQL, Auth, Realtime, Edge Functions e Storage.
- Persistencia remota de historico.
- Sincronizacao de estado oficial publicado pelo ESP32.
- Autenticacao e modelo de usuarios.
- Push notifications e politica anti-spam.
- APIs ou Edge Functions para o aplicativo mobile.

Nao pertence a cloud/backend:

- Automacoes criticas do aquario.
- Decisao de ligar/desligar relays por seguranca.
- Regras de ATO, temperatura, modos ou iluminacao.
- Fonte unica de verdade operacional.
- Persistencia local NVS do ESP32.
- Funcionamento minimo do aquario sem Internet.

## Regras Arquiteturais

- A cloud depende do firmware expor telemetria, eventos e comandos por interfaces oficiais.
- A cloud nao pode substituir o ESP32 em nenhuma responsabilidade critica.
- A cloud deve armazenar historico derivado do estado global e dos eventos oficiais.
- Comandos remotos devem ser encaminhados ao ESP32 como solicitacoes.
- Ausencia de cloud nao pode impedir o firmware de operar o aquario.
- Alertas e notificacoes remotas complementam, mas nao substituem, alertas e fail-safes locais.

## Fases de Alto Nivel

### Fase C0 - Fundacao Cloud

Preparar Supabase, ambientes, estrutura de banco, Auth, Storage quando necessario e padroes de deploy.

### Fase C1 - Broker e Bridge MQTT

Configurar Mosquitto e a bridge MQTT <-> Supabase para receber telemetria, eventos, heartbeat e publicar solicitacoes autorizadas ao ESP32.

### Fase C2 - Modelo de Dados e Historico

Persistir temperatura, nivel, eventos, modos, reboots, alertas, estado dos relays, iluminacao e status do ESP32 a partir do estado oficial.

### Fase C3 - APIs e Realtime

Expor dados e alteracoes ao aplicativo por APIs, Realtime ou Edge Functions, sem criar logica critica fora do firmware.

### Fase C4 - Alertas Remotos e Notificacoes

Sincronizar alertas, aplicar politica anti-spam remota e enviar push notifications para eventos relevantes.

### Fase C5 - Controle Remoto Mediado

Receber solicitacoes autenticadas do mobile e encaminhar comandos ao ESP32 pelo caminho MQTT oficial, com auditoria e sem assumir execucao critica.

### Fase C6 - Beta Cloud Integrado

Validar sincronizacao entre firmware, MQTT, Supabase e mobile, mantendo a independencia operacional do ESP32.

## Dependencias

- Depende do firmware para System State, eventos oficiais, telemetria MQTT e comandos aceitos pelo ESP32.
- Depende do contrato MQTT para topicos, QoS, heartbeat e reconexao.
- Serve o mobile com dados, autenticacao, historico e notificacoes.
- Nao bloqueia o desenvolvimento local do firmware.

## Limite do Plano

Este plano cobre apenas cloud/backend.

Firmware e mobile possuem planos proprios em:

- `../firmware/firmware-master-plan.md`
- `../mobile/mobile-master-plan.md`
