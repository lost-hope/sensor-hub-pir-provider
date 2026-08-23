#include "wled.h"
#include "sensor_bus.h"

/*
 * PIR motion sensor provider.
 *
 * Reads a simple digital PIR module (HC-SR501 and similar - active HIGH)
 * on a single GPIO pin and pushes the state into the Sensor Hub (see
 * ../sensor-hub/usermod_sensor_hub.cpp and ../sensor-hub/sensor_bus.h) as
 * a single binary "<prefix>_motion" sensor. This usermod never talks to
 * MQTT, the JSON API or the Info tab itself - the hub takes care of all of
 * that once a sensor is registered here.
 *
 * The pin is not a shared WLED global - it is configured (and reserved via
 * WLED's PinManager, to avoid clashing with LEDs/relays/other usermods)
 * right here in this usermod's own settings.
 */

REGISTER_SENSOR_SLOT(_slotMotion, "_motion", SensorTypes::Motion, 1, 100);

class PIRSensorUsermod : public Usermod {
  private:
    SensorHub* hub = nullptr;
    uint8_t motionHandle = SENSOR_HANDLE_INVALID;

    bool enabled = true;
    bool initDone = false;

    unsigned long lastRead = 0;

    // config
    int8_t pin = -1;                // PIR output pin, unset by default
    uint16_t checkIntervalMs = 100; // how often the pin is polled
    String namePrefix = "pir";      // sensor name becomes "<prefix>_motion"
    uint8_t priority = 100;         // getValueBinary() selection priority - lower wins among sensors of the same SensorType (see sensor_bus.h)

    static const char _name[];
    static const char _enabled[];
    static const char _pin[];
    static const char _checkInterval[];
    static const char _namePrefix[];
    static const char _priority[];

    void registerSensors() {
      if (!hub || motionHandle != SENSOR_HANDLE_INVALID) return; // already registered
      motionHandle = hub->attachSensor(&_slotMotion, namePrefix.c_str(), 1, priority);
    }

  public:
    void setup() override {
      // Neither branch touches 'enabled' (the user's own on/off switch,
      // persisted to config) - initDone (left false here) is what actually
      // gates loop(), so a later pin fix takes effect on the next boot
      // instead of staying stuck disabled.
      if (pin < 0) return;
      if (!PinManager::allocatePin(pin, false, PinOwner::UM_Unspecified)) {
        pin = -1; // conflicts with another pin owner - force reconfiguration
        return;
      }
#ifdef ESP8266
      pinMode(pin, pin == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP); // ESP8266 has INPUT_PULLDOWN on GPIO16 only
#else
      pinMode(pin, INPUT_PULLDOWN);
#endif
      initDone = true;
    }

    void loop() override {
      if (!enabled || !initDone) return;

      if (!hub) hub = getSensorHub(); // Sensor Hub usermod may finish init after us
      if (hub) registerSensors();

      unsigned long now = millis();
      if (now - lastRead < (unsigned long)checkIntervalMs) return;
      lastRead = now;

      bool state = digitalRead(pin);
      if (hub && motionHandle != SENSOR_HANDLE_INVALID) {
        hub->setSensorAvailable(motionHandle, true);
        hub->updateSensorBinary(motionHandle, state);
      }
    }

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_enabled)] = enabled;
      top[FPSTR(_pin)] = pin;
      top[FPSTR(_checkInterval)] = checkIntervalMs;
      top[FPSTR(_namePrefix)] = namePrefix;
      top[FPSTR(_priority)] = priority;
    }

    bool readFromConfig(JsonObject& root) override {
      int8_t oldPin = pin;

      JsonObject top = root[FPSTR(_name)];
      bool configComplete = !top.isNull();
      configComplete &= getJsonValue(top[FPSTR(_enabled)], enabled);
      configComplete &= getJsonValue(top[FPSTR(_pin)], pin);
      configComplete &= getJsonValue(top[FPSTR(_checkInterval)], checkIntervalMs);
      configComplete &= getJsonValue(top[FPSTR(_namePrefix)], namePrefix);
      configComplete &= getJsonValue(top[FPSTR(_priority)], priority);

      if (initDone && pin != oldPin) {
        // pin changed at runtime via the Settings UI - release the old one and re-init on the new one
        if (oldPin >= 0) PinManager::deallocatePin(oldPin, PinOwner::UM_Unspecified);
        initDone = false;
        setup();
      }
      return configComplete;
    }

    void appendConfigData(Print& settingsScript) override {
      settingsScript.print(F("addInfo('PIRSensor:pin',1,'PIR output pin (active HIGH)');"));
      settingsScript.print(F("addInfo('PIRSensor:checkInterval',1,'milliseconds between pin reads');"));
      settingsScript.print(F("addInfo('PIRSensor:namePrefix',1,'sensor name becomes &lt;prefix&gt;_motion - must be unique across all sensor providers');"));
      settingsScript.print(F("addInfo('PIRSensor:priority',1,'getValueBinary() selection priority - lower wins if another provider also registers a Motion sensor');"));
    }
};

const char PIRSensorUsermod::_name[]          PROGMEM = "PIRSensor";
const char PIRSensorUsermod::_enabled[]       PROGMEM = "enabled";
const char PIRSensorUsermod::_pin[]           PROGMEM = "pin";
const char PIRSensorUsermod::_checkInterval[] PROGMEM = "checkInterval";
const char PIRSensorUsermod::_namePrefix[]    PROGMEM = "namePrefix";
const char PIRSensorUsermod::_priority[]      PROGMEM = "priority";

static PIRSensorUsermod pir_sensor;
REGISTER_USERMOD(pir_sensor);
