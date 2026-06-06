CONTROLADOR DE AQUÁRIO MARINHO DIY
Documento 1 — Hardware e Arquitetura Elétrica
Objetivo
Definir toda a infraestrutura elétrica e eletrônica do controlador do aquário.
Este documento representa o estado físico do sistema e serve como referência única para montagem, manutenção e futuras expansões.

Arquitetura Geral
O sistema é dividido em:
Proteção AC
Distribuição 12V DC
Distribuição 5V DC
Controle lógico (ESP32)
Controle de potência (Relés e MOSFETs)

Fluxo Principal de Energia
127V AC
↓
DR 20A / 30mA
↓
Tomada dupla
Tomada 1:
Fonte 12V 50A
Tomada 2:
Filtro de linha

Distribuição 12V
Fonte 12V
V+
Luminária LED
LM2596
Futuras expansões 12V
V-
Barramento GND principal

Distribuição 5V
LM2596
OUT+
ESP32 VIN
Módulo Relé 4 canais
Bomba ATO (via Relé 3)
OUT-
Barramento GND comum

Alimentação 3.3V
ESP32 3V3
DS18B20
VL6180X

Pinout Oficial ESP32
GPIO4
DS18B20
GPIO21
SDA VL6180X
GPIO22
SCL VL6180X
GPIO16
Relé Recalque
GPIO17
Relé Aquecedor
GPIO18
Relé ATO
GPIO19
Relé Reserva
GPIO25
PWM Branco
GPIO26
PWM Azul
GPIO27
PWM Royal Blue
GPIO14
PWM Moonlight
GPIO13
PWM Reserva

Sensores
DS18B20
Alimentação:
3.3V
Ligação:
VCC → 3.3V
GND → GND
DATA → GPIO4
Resistor Pull-Up:
4.7kΩ entre DATA e VCC

VL6180X
Alimentação:
3.3V
Ligação:
VCC → 3.3V
GND → GND
SDA → GPIO21
SCL → GPIO22
Função:
Sensor óptico de nível para ATO.

Relés
Relé 1
Bomba recalque
Relé 2
Aquecedor
Relé 3
Bomba ATO
Relé 4
Reserva
Configuração:
COM + NO
NC não utilizado.

Luminária
Controle por PWM via MOSFET.
Canais:
Branco
Azul
Royal Blue
UV
Alimentação:
12V
Controle:
ESP32 via PWM

Segurança
Obrigatório:
DR
Aterramento
GND comum
Caixa de proteção
Drip Loop
Bornes isolados
Nunca utilizar:
Emendas expostas
Fios desencapados
Ligações sem isolamento

Estado Atual
Hardware definido e considerado congelado para a versão V1 do projeto.
Novas funcionalidades devem ser implementadas prioritariamente via firmware e software.

