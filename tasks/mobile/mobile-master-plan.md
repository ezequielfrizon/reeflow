# REEFLOW Mobile Master Plan

## Objetivo

Definir o plano mestre exclusivo do aplicativo mobile REEFLOW.

O aplicativo deve oferecer interface de usuario para visualizacao, configuracao, historico, alertas e solicitacoes de controle remoto, sempre respeitando o ESP32 como autoridade do estado e das automacoes criticas.

## Escopo do Dominio Mobile

Pertence ao mobile:

- Login e sessao persistente.
- Dashboard do aquario.
- Visualizacao de temperatura, nivel, relays, iluminacao, modo, alertas e status do ESP32.
- Telas de configuracao de ATO, luminaria, timers, modos, Wi-Fi e MQTT quando expostas por interfaces oficiais.
- Solicitacoes de controle remoto encaminhadas ao firmware pelas interfaces cloud/MQTT previstas.
- Historico consultado a partir da cloud.
- Recebimento e exibicao de alertas e push notifications.
- Biometria, recuperacao de senha e multiusuario quando suportados pela camada de autenticacao.

Nao pertence ao mobile:

- Automacoes criticas do aquario.
- Regras de ATO, temperatura, relays, modos ou iluminacao.
- Fonte unica de verdade do estado.
- Persistencia local do firmware.
- MQTT Bridge ou banco de dados.
- Decisao de fail-safe.

## Regras Arquiteturais

- O mobile depende das interfaces expostas por firmware e cloud/backend.
- O mobile deve consumir o estado oficial do ESP32, refletido pela cloud quando aplicavel.
- O mobile nao deve calcular estado critico proprio.
- O mobile nao deve comandar atuadores diretamente fora do caminho oficial do ESP32.
- O mobile pode solicitar alteracoes, mas a aplicacao efetiva pertence ao firmware.
- Perda do aplicativo nao pode comprometer a operacao local do aquario.

## Fases de Alto Nivel

### Fase M0 - Base do Aplicativo

Preparar projeto React Native/Expo, estrutura de navegacao, padroes de tela, ambiente e integracao inicial com autenticacao.

### Fase M1 - Autenticacao e Sessao

Implementar login, sessao persistente, recuperacao de senha, biometria e suporte ao modelo multiusuario definido pela cloud.

### Fase M2 - Dashboard

Exibir o estado oficial do aquario: temperatura, nivel, status do ESP32, relays, iluminacao, modo atual, ATO, rede e alertas.

### Fase M3 - Configuracoes

Permitir edicao das configuracoes expostas pelas interfaces oficiais: ATO, luminaria, timers, modos, Wi-Fi, MQTT e backup.

### Fase M4 - Controle Remoto

Permitir solicitacoes de troca de modo, acionamento de equipamentos, ajustes de luminaria e configuracao de ATO, sempre atraves do caminho oficial ate o ESP32.

### Fase M5 - Historico e Alertas

Exibir historico diario, semanal e mensal, alertas ativos, recuperacoes e notificacoes recebidas da cloud.

### Fase M6 - Beta Mobile Integrado

Validar o aplicativo contra firmware e cloud ja expostos por interfaces oficiais, sem mover regras criticas para o app.

## Dependencias

- Depende do contrato de System State mantido pelo firmware.
- Depende das APIs, autenticacao, historico e notificacoes da cloud.
- Depende de comandos remotos implementados como solicitacoes ao ESP32.
- Nao bloqueia o desenvolvimento do firmware.

## Limite do Plano

Este plano cobre apenas o aplicativo mobile.

Firmware e cloud/backend possuem planos proprios em:

- `../firmware/firmware-master-plan.md`
- `../cloud/cloud-master-plan.md`
