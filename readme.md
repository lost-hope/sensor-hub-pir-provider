# PIR Motion Sensor Provider

A [Sensor Hub](../sensor-hub/readme.md) provider usermod for a simple
digital PIR module (HC-SR501 and similar, active HIGH) - registers a
single binary `pir_motion` sensor with the hub by default, which then
handles MQTT, Home Assistant discovery, the JSON API and the Info tab.

## Hardware

Set the **Pin** in this usermod's own Settings page - it is reserved
through WLED's PinManager so it won't silently clash with LEDs, relays or
other usermods. Configured `INPUT_PULLDOWN` and read as a plain digital
input - no library required.

## Usage

Add `sensor-hub-pir-provider` to `custom_usermods` next to the
[Sensor Hub](../sensor-hub/readme.md) itself.

## Usermod Settings

| Setting | Default | Description |
|---|---|---|
| Enabled | on | Master on/off switch (also auto-disabled until a pin is set) |
| Pin | unset | PIR output pin |
| Check interval | 100 ms | How often the pin is polled |
| Name prefix | `pir` | Sensor name becomes `<prefix>_motion` - must be unique across every provider registered with the hub |
