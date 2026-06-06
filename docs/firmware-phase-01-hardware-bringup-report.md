# Firmware Phase 1 Hardware Bring-Up Report

## FW-P01-010 - Hardware Validation Template

Report date: Pendente para Hardware Validation

Board used: Pendente para Hardware Validation

ESP32 boot result: Pendente para Hardware Validation

Serial monitor result: Pendente para Hardware Validation

Firmware structure created: READY

Hardware Bring-Up mode identification: READY

Pinout table checked on Serial: Pendente para Hardware Validation

Relay activation polarity: `ACTIVE_HIGH`

Relay physical OFF GPIO level: `LOW`

Relay physical ON GPIO level: `HIGH`

Relay 1 Recalque result: Pendente para Hardware Validation

Relay 2 Aquecedor result: Pendente para Hardware Validation

Relay 3 ATO result: Pendente para Hardware Validation

Relay 4 Reserva result: Pendente para Hardware Validation

PWM bring-up frequency: `5000 Hz`

PWM bring-up resolution: `8 bits`

PWM duty zero: `0`

PWM duty test minimum: `64`

PWM duty test maximum: `128`

GPIO25 PWM Branco result: Pendente para Hardware Validation

GPIO26 PWM Azul result: Pendente para Hardware Validation

GPIO27 PWM Royal Blue result: Pendente para Hardware Validation

GPIO14 PWM Moonlight/UV result: Pendente para Hardware Validation

GPIO13 PWM Reserva result: Pendente para Hardware Validation

DS18B20 GPIO4 result: Pendente para Hardware Validation

VL6180X expected I2C address: `0x29`

VL6180X real I2C address found: Pendente para Hardware Validation

VL6180X basic read result: Pendente para Hardware Validation

VL6180X raw range: Pendente para Hardware Validation

VL6180X converted range mm: Pendente para Hardware Validation

VL6180X distance variation: Pendente para Hardware Validation

VL6180X idle stability: Pendente para Hardware Validation

5V bus resting voltage: Pendente para Hardware Validation

5V bus voltage during Relay 1: Pendente para Hardware Validation

5V bus voltage during Relay 2: Pendente para Hardware Validation

5V bus voltage during Relay 3: Pendente para Hardware Validation

5V bus voltage during Relay 4: Pendente para Hardware Validation

5V bus voltage during controlled relay sequence: Pendente para Hardware Validation

Sensor noise before relay activation: Pendente para Hardware Validation

Sensor noise during relay activation: Pendente para Hardware Validation

Sensor noise after relay activation: Pendente para Hardware Validation

Sensor communication loss observed: Pendente para Hardware Validation

Reset observed during relay activation: Pendente para Hardware Validation

Impossible sensor reading observed: Pendente para Hardware Validation

Overall Hardware Validation result: Pendente para Hardware Validation

Notes:

- This template is intentionally empty of real bench results.
- Physical PASS/FAIL must be filled only during future Hardware Validation.
- Firmware development readiness does not imply physical validation.

## Hardware Validation Status

Upload real para ESP32: Pendente para Hardware Validation

Monitor Serial real: Pendente para Hardware Validation

Reboot real: Pendente para Hardware Validation

Impressao real da tabela de pinout: Pendente para Hardware Validation

Polaridade fisica dos reles: Pendente para Hardware Validation

Estado fisico seguro dos reles e PWM: Pendente para Hardware Validation

Resposta fisica dos reles: Pendente para Hardware Validation

Medicao fisica dos sinais PWM: Pendente para Hardware Validation

Verificacao fisica SDA/SCL I2C: Pendente para Hardware Validation

Leitura fisica do VL6180X: Pendente para Hardware Validation

Variacao fisica por distancia do VL6180X: Pendente para Hardware Validation

Estabilidade fisica em repouso do VL6180X: Pendente para Hardware Validation

Estabilidade do barramento 5V em repouso: Pendente para Hardware Validation

Estabilidade do barramento 5V durante acionamento individual dos reles: Pendente para Hardware Validation

Estabilidade do barramento 5V durante sequencia controlada dos reles: Pendente para Hardware Validation

Ruido ou interferencia real nos sensores durante acionamento dos reles: Pendente para Hardware Validation

Perda real de comunicacao dos sensores durante acionamento dos reles: Pendente para Hardware Validation

Reset real observado durante acionamento dos reles: Pendente para Hardware Validation

Leitura impossivel real durante acionamento dos reles: Pendente para Hardware Validation

## FW-P01-002A - Relay Electrical Contract

Relay activation polarity: `ACTIVE_HIGH`

Physical OFF GPIO level: `LOW`

Physical ON GPIO level: `HIGH`

Validation status: Pendente para Hardware Validation

Notes:

- This report records the relay electrical contract required before safe relay diagnostics.
- No relay GPIO initialization, relay actuation, safe-state routine or automation was implemented in this task.

## FW-P01-004 - Individual Relay Diagnostic

Relay diagnostic polarity used: `ACTIVE_HIGH`

Physical OFF GPIO level: `LOW`

Physical ON GPIO level: `HIGH`

Serial commands:

- `1`: Relay 1 Recalque, GPIO16
- `2`: Relay 2 Aquecedor, GPIO17
- `3`: Relay 3 ATO, GPIO18
- `4`: Relay 4 Reserva, GPIO19

Validation status: Pendente para Hardware Validation

Notes:

- The diagnostic turns on only the selected relay for a short interval.
- All relays are commanded back to physical OFF at the end of each diagnostic run.
- No remote control, MQTT, events or relay automation was implemented in this task.

## FW-P01-005A - PWM Bring-Up Parameters

Frequency: `5000 Hz`

Resolution: `8 bits`

Duty zero: `0`

Duty test minimum: `64`

Duty test maximum: `128`

LEDC channels:

- GPIO25, PWM Branco: channel `0`
- GPIO26, PWM Azul: channel `1`
- GPIO27, PWM Royal Blue: channel `2`
- GPIO14, PWM Moonlight/UV: channel `3`
- GPIO13, PWM Reserva: channel `4`

Scope note: these are diagnostic bring-up parameters only. Official lighting parameters belong to Phase 8.

Validation status: Pendente para Hardware Validation

Notes:

- No PWM signal generation, LEDC setup, sunrise, sunset, moonlight, acclimation, curves, schedules or persistence was implemented in this task.

## FW-P01-005 - PWM Signal Diagnostic

Serial command: `p`

Test sequence:

- Configure the five PWM GPIOs with the bring-up frequency and resolution.
- Force all PWM channels to duty zero.
- Test one channel at a time.
- Apply duty `64`, then duty `128`, then return the channel to duty `0`.
- Force all PWM channels back to duty zero at the end.

Validation status: Pendente para Hardware Validation

Notes:

- No lighting automation, sunrise, sunset, moonlight, acclimation, curves, schedules or persistence was implemented in this task.

## FW-P01-005B - Validation Without Hardware

GPIO abstraction: prepared with Arduino implementation for hardware build and fakes restricted to unit tests.

PWM/LEDC abstraction: prepared with Arduino implementation for hardware build and fakes restricted to unit tests.

Diagnostics split by domain: pinout, initial safe state, relays and PWM.

Hardware build mock dependency: none.

Physical validation fields: Pendente para Hardware Validation

Notes:

- Tests validate bring-up logic without ESP32, sensors, relay module or lighting connected.
- This report does not record physical validation results.
- No DS18B20, I2C, VL6180X, 5V stability, sensor noise, System State, Event Bus, Wi-Fi, MQTT, alerts, modes, ATO automation or lighting curves were implemented in this task.

## FW-P01-006 - DS18B20 Bring-Up Diagnostic

Sensor target: `DS18B20`

GPIO: `4`

Bus: `OneWire`

Serial command: `t`

Supported diagnostic statuses:

- `FOUND`
- `NOT_FOUND`
- `READ_ERROR`

Physical detection: Pendente para Hardware Validation

Physical temperature read: Pendente para Hardware Validation

Notes:

- The diagnostic performs a single explicit bring-up read only when requested by Serial command.
- Tests cover simulated `FOUND`, `NOT_FOUND` and `READ_ERROR` results without hardware.
- No temperature state, periodic reading, limits, SENSOR_OFFLINE handling, events, alerts, MQTT or persistence were implemented in this task.

## FW-P01-007 - I2C Bus Bring-Up Diagnostic

Bus target: `I2C`

SDA GPIO: `21`

SCL GPIO: `22`

Serial command: `i`

Scanner range: `0x03` to `0x77`

VL6180X physical detection: Pendente para Hardware Validation

SDA/SCL physical verification: Pendente para Hardware Validation

Notes:

- The diagnostic initializes I2C using the hardware contract pins and performs a generic bus scan only when requested by Serial command.
- Tests cover simulated scan results with one device found and with no devices found.
- No expected VL6180X address comparison, VL6180X read, waterLevel state, calibration, LOW_LEVEL, SENSOR_OFFLINE, ATO or MQTT behavior was implemented in this task.

## FW-P01-007A - VL6180X I2C Address Comparison

Expected VL6180X I2C address: `0x29`

Supported comparison statuses:

- `MATCH`
- `NOT_FOUND`
- `MISMATCH`

Real address found on bus: Pendente para Hardware Validation

Physical address confirmation: Pendente para Hardware Validation

Notes:

- The diagnostic compares the expected VL6180X address with addresses reported by the existing I2C scanner.
- Tests cover simulated `MATCH`, `NOT_FOUND` and `MISMATCH` results without hardware.
- No VL6180X reading, waterLevel state, calibration, LOW_LEVEL, SENSOR_OFFLINE, ATO or MQTT behavior was implemented in this task.

## FW-P01-008 - VL6180X Basic Read Diagnostic

Expected VL6180X I2C address: `0x29`

Supported read statuses:

- `VALID`
- `READ_ERROR`

Real raw range: Pendente para Hardware Validation

Real converted range mm: Pendente para Hardware Validation

Distance variation check: Pendente para Hardware Validation

Idle stability check: Pendente para Hardware Validation

Notes:

- The diagnostic performs a single basic VL6180X range register read for future bench verification.
- Tests cover simulated valid read and simulated read failure without hardware.
- No waterLevel state, calibration, minimum/maximum limits, LOW_LEVEL, SENSOR_OFFLINE, ATO, alerts or MQTT behavior was implemented in this task.

## FW-P01-009A - 5V Bus Stability Validation Procedure

Serial command: `v`

Resting 5V bus voltage: Pendente para Hardware Validation

Relay 1 Recalque 5V bus voltage: Pendente para Hardware Validation

Relay 2 Aquecedor 5V bus voltage: Pendente para Hardware Validation

Relay 3 ATO 5V bus voltage: Pendente para Hardware Validation

Relay 4 Reserva 5V bus voltage: Pendente para Hardware Validation

Controlled relay sequence 5V bus voltage: Pendente para Hardware Validation

Electrical stability result: Pendente para Hardware Validation

Bench checklist:

- Measure the 5V bus with all relays commanded OFF.
- Measure the 5V bus while each relay is activated individually.
- Measure the 5V bus during a controlled one-relay-at-a-time sequence.
- Record PASS/FAIL only during future Hardware Validation.

Notes:

- The diagnostic prepares the future bench measurement procedure and relay sequence only.
- Tests cover the controlled relay sequence with fake GPIO and confirm all relays return to expected OFF.
- No electrical correction, fail-safe functionality, System State, alerts, MQTT, automation or future phase behavior was implemented in this task.

## FW-P01-009B - Sensor Noise Validation Procedure

Serial command: `n`

Log format:

- Relay index
- Phase: `BEFORE_RELAY`, `DURING_RELAY`, `AFTER_RELAY`
- DS18B20 status and temperature
- VL6180X status and range
- Communication loss flag
- Reset observed flag
- Gross oscillation flag

DS18B20 before/during/after relay activation: Pendente para Hardware Validation

VL6180X before/during/after relay activation: Pendente para Hardware Validation

Real sensor noise/interference result: Pendente para Hardware Validation

Real communication loss result: Pendente para Hardware Validation

Real reset observation result: Pendente para Hardware Validation

Real impossible-reading result: Pendente para Hardware Validation

Notes:

- The diagnostic prepares the future before/during/after sensor log around controlled relay activation.
- Tests cover normal simulated sensor readings and simulated read failures with fakes.
- The routine commands all relays to physical OFF and all PWM channels to duty zero at the end.
- No filtering, alert, event, automation, System State, MQTT or future phase behavior was implemented in this task.
