# Bike computer

ESP32 based bike computer that calculates your speed and sends it to [dedicated Android app](https://github.com/AlexeiTrofimov/Bike-Computer-App) using nimBLE stack.

## Working principle

When computer is powered it start advertising right away. When client disconnects, advertising resumes.

Magnet connected to the spoke of the bikes wheel passes reed switch which closes circuit and send signal to GPIO port on ESP32.
Time difference of two reading is used to calculate current speed of your bike.

When speed value changes, computer sends it to the app using notifications if they are enabled by client.

BLE functionality based on [ESP32 NimBLE prph-example](https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/nimble/bleprph)