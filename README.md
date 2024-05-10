## <br/>👉 [Check out](https://islandmagic.co/products/B-B-Link-Adapter-p629580960?utm_campaign=diy&utm_medium=web&utm_source=github) the new edition that plugs directly into your iPhone 15! 🔥 <br/>&nbsp;

# B.B. Link, the BLE to Bluetooth Classic adapter for Kenwood TH-D74/5 Radios

Some devices, like the Kenwood TH-D74 radio only support Bluetooth Classic serial profile. iOS devices only support Bluetooth Low Energy (BLE). They are not compatible and as such, you can't pair those devices together. This code provides a way to create an adapter that can interface a device that exposes a serial profile over Bluetooth Classic, to an iOS device via BLE. Its main purpose is to enable iOS app that supports AX.25 packet like RadioMail or APRS.fi to use the TNC built in the radio as a modem.

For a detailed "how-to build" this adapter, watch this video: https://www.youtube.com/watch?v=xLze6qDOLww. After you're done, come back and follow the instructions here as they are continually updated.

## Hardware

The adapter is based on the ESP32 microcontroller, which provides support for both Bluetooth Classic and Bluetooth Low Energy (BLE). There is specialized code tailored for the [TinyPICO](https://www.tinypico.com) development board, which serves as the standard implementation. This code can be altered to work with other ESP32-based boards as needed.

**Note: Compatibility is limited to ESP32 microcontroller. Boards like the TinyS3, which use the ESP32-S3 chip, do not support Bluetooth Classic. If you don't use the TinyPICO, make sure to choose a board that features the ESP32 microcontroller.**

### Material

1. TinyPICO [Buy](https://unexpectedmaker.com/shop.html#!/TinyPICO/p/577111313/category=154494282)
2. 600 mAh LiPo Battery model 602248 (6x22x48mm) [Buy](https://www.aliexpress.us/item/2251832520607268.html)
3. 3D Printed case
4. Brad fastener as touch button (8mm head)

### Power Options

The adapter can be powered via USB, such as through a USB adapter or a portable power bank, or alternatively, it can be connected to a LiPo battery for use on the go. The TinyPICO will charge the battery when plugged into USB.

### Capacitive Touch Button

The installation of a capacitive touch button is not required but recommended if you need an on/off switch while using a LiPo battery. To install, connect a wire to pin number 4 on the TinyPICO board; this will serve as the on/off control.

### Cases

You can 3D print a case to house the battery and the board. Here are a couple of option that attach via the mounting holes for the belt clip.

1. Horizontal

This is the orignal design from WH6AZ, functional but primitive. Can work as a stand alone box if you don't feel like attaching to the radio.

https://github.com/islandmagic/bb-link/tree/master/enclosure

2. Vertical

This design is a beautiful contribution from F4LFJ. It features a slick design that allows the case to attach vertically. There is enough clearance to insert and remove the battery from the radio.

https://github.com/islandmagic/bb-link/tree/master/enclosure/contribs/F4LFJ

## Software

1. Install Arduino IDE 2.x [https://www.arduino.cc](https://www.arduino.cc)
1. Add additional library source in Arduino IDE. Settings > Additional board manager URLs: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
1. Install the esp32 by Espressif Systems board library. This code has been tested with version 2.0.15.
1. Install the TinyPICO helper library
1. Install FreeRTOS library
1. Install ArduinoQueue library
1. Install ArduinoLog library
1. Clone this repo
1. Flash the code to the TinyPICO board
1. Download [B.B. Link Configurator](https://apps.apple.com/us/app/b-b-link-configurator/id6476163710) app on your phone
1. Download RadioMail on your phone [https://radiomail.app](https://radiomail.app)

### Rig Control

By default, the adapter will set the radio to KISS mode and automatically respond to RadioMail's instructions to switch frequencies, enabling seamless operation. If you prefer the adapter not to alter radio settings during use, toggle off the 'Control Frequency' option in the configurator app.

## Operating Instructions

### Powering On

1. Wake the adapter by briefly pressing the touch button. An amber light will indicate that it is awake.

### Pairing with a Radio (one time only)

1. Turn on adapter
1. Open B.B. Link Configurator app on your  phone
1. After a few seconds, `B.B. Link` appears in the list of nearby adapters. Select it.
1. Set the radio in Bluetooth discovery mode. Navigate to 'Menu -> Bluetooth -> Pairing Mode' on the radio.
1. Tap 'Paired Radio' on B.B. Link app
1. Wait a few seconds, the name of your radio should appear in the list. Select it.
1. After a few seconds a PIN prompt should appear on the radio. Press 'OK' to accept. This step is only necessary once; afterwards, the adapter will automatically try to reconnect with the radio.
1. Successful pairing is indicated by a breathing blue LED on the adapter.

### Pairing with iPhone or iPad

1. Make sure B.B. Link Configurator app is fully closed
1. Open the RadioMail app. Proceed to 'Settings -> KISS TNC Modem -> Default TNC'.
1. `B.B. Link` should be visible in the discovery screen.
1. Select `B.B. Link` and tap 'Done'.
1. Navigate to the connection screen and choose a packet station.
1. A solid blue LED on the adapter signals that RadioMail has connected. Red and green LEDs will flash to indicate data transmission and reception.

### Powering Off

- The adapter will shut off automatically after 5 minutes when not in use on battery power. It stays on indefinitely if USB power is detected.
- To manually turn off the adapter, press and hold the touch button until the LED blinks amber, then release.

### User Interface

#### Touch Button Functions

- **Short Press**: Wake the adapter or check the battery status when idle.
- **Long Press**: Turn off the adapter.

#### LED Indicator

The dotstar tri-color LED is used to indicate various states:

- 🟠: Idle, adapter waiting to pair
- 🔵 (slow flash): Adapter scanning for radio
- 🔵 (breathing): Idle, paired with radio
- 🔵: Ready, radio and iOS device paired
- 🟠 (fast blink): Shutting down

Data Activity:

- 🟢: Rx (Receiving)
- 🔴: Tx (Transmitting)
- 🟣: Rx/Tx (Both Receiving and Transmitting)

Error Conditions:

- 🔴 (fast blink): Low battery immediate shutdown
- 🔴 (slow flash): Fatal error, must reset

Battery Status Indicators:

- 🟢: Battery full
- 🟢 (fast blink): Battery low

#### Battery Indicator

The charge level of a LiPO battery can be difficult to determine without additional circuitry due to their non-linear discharge curve. A basic battery indicator is provided based on the battery voltage level:

1. Upon waking up, the adapter displays the battery level for a few seconds. A solid green light indicates the battery is considered full, while a flashing green light suggests the battery needs charging.
1. When the adapter is idle (indicated by an amber LED), a short tap will activate the battery level display.

### Battery Saver

To save energy, the adapter automatically enters a low-power sleep mode after a certain period of not being used, for instance, when no phone is connected, during which it consumes approximately 40 µA. While active, the adapter's power consumption is around 140 mA, allowing for continuous operation for up to 4 hours on a 600 mAh battery. In standby mode, with no active bluetooth connection, it uses about 80 mA. Additionally, the sleep feature is disabled when the adapter is connected to USB power, ensuring that the adapter can stay on for longer durations and can be used while it is being charged.

If the battery voltage falls to a critical level, the adapter will automatically shut down to prevent damage to the circuit.

### Factory Reset

You can reset the adapter to its default configuration. This will clear the list of previously paired devices and restoring default settings. Simply tap 'Reset Adapter' in the configurator app.

Alternatively, you can reset the adapter by connecting it to a computer. Here's how to do it:

1. Launch the Arduino IDE on your PC.
1. Connect the adapter to your PC using a USB cable.
1. Navigate to the Serial Monitor within the IDE.
1. Type `R` into the Serial Monitor and send the command.
1. Monitor the response in the Serial Monitor which will confirm the clearing of the previously paired devices.
1. After the process is complete, disconnect the adapter from your PC.

## Troubleshooting

* If the adapter connects to the radio but the radio does not transmit, check the TNC settings. Go to Menu > Configuration > Interface > KISS (983) and set it to Bluetooth.

## Known Issues

1. The ESP32 library is limited to discovering Bluetooth Classic devices exclusively prior to establishing any connections. After connecting with a device, the discovery capability is no longer available. To initiate a new scan, the adapter must be rebooted. If you intend to pair with a different radio, ensure that the radio previously paired with is turned off before powering on the adapter. If not, the adapter will automatically re-establish a connection with the previously paired radio, preventing it from discovering new radios.

## How to Contribute

This project is open source, so everyone's contribution is welcome. Here's a quick guide to get started:

* **Share**: If you build your own adapter, share it online! Post photos, write a blog post, or create a tutorial video to show others how it's done.
* **Update Documentation**: Help improve or correct the documentation. Fork the repo, make your updates, and submit a pull request.
* **Submit Change Requests**: If you're developing a new feature or bug fix, fork the repository, create a new branch for your changes, and submit a pull request with a clear description of your modifications.
* **Write Good Issue Reports**: If you encounter bugs or have feature suggestions, please submit an issue report with a clear title, a detailed description, and steps to reproduce the issue if it's a bug.

The source code for the [Configurator](https://github.com/islandmagic/ios-bblink-config) app is available as well.

