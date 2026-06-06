# Firmware Phase 1 Hardware Bring-Up Report

## Hardware Validation Status

Upload real para ESP32: Pendente para Hardware Validation

Monitor Serial real: Pendente para Hardware Validation

Reboot real: Pendente para Hardware Validation

Impressao real da tabela de pinout: Pendente para Hardware Validation

Polaridade fisica dos reles: Pendente para Hardware Validation

Estado fisico seguro dos reles e PWM: Pendente para Hardware Validation

Resposta fisica dos reles: Pendente para Hardware Validation

Medicao fisica dos sinais PWM: Pendente para Hardware Validation

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
