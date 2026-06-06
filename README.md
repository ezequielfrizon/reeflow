REEFLOW

Controlador de aquário marinho baseado em ESP32.

Princípios:

- ESP32 é soberano
- Cloud é opcional
- App é interface
- Fail-safe local obrigatório
- Arquitetura modular
- Desenvolvimento orientado por especificações (SDD)

Estrutura:

architecture/
Documentos de arquitetura do sistema.

specs/
Especificações funcionais dos módulos.

tasks/
Roadmap e tarefas de implementação.

firmware/
Código PlatformIO do ESP32.

Fluxo de desenvolvimento:

Architecture
→ Specs
→ Tasks
→ Firmware
→ Cloud
→ Mobile App