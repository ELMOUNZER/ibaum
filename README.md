# ESP32 Irrigation System

This project utilizes ESP32 microcontroller to create a smart irrigation system. The system monitors humidity using a DHT sensor and controls a water pump to maintain optimal soil moisture levels. Additionally, it features an LED indicator for status feedback.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Getting Started](#getting-started)
  - [Hardware Setup](#hardware-setup)
  - [Software Setup](#software-setup)
- [Configuration](#configuration)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Prerequisites

Before you begin, ensure you have the following:

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) installed on your development machine.
- [DHT Library](https://github.com/adafruit/DHT-sensor-library) for ESP32 installed in your project.

## Getting Started

### Hardware Setup

1. Connect the DHT sensor to GPIO pin `DHT_GPIO_PIN`.
2. Connect the relay to GPIO pin `RELAY_GPIO_PIN`.
3. Connect the LED to GPIO pin `LED_GPIO_PIN`.

### Software Setup

1. Clone the repository:

    ```bash
    git clone https://github.com/your-username/esp32-irrigation-system.git
    ```

2. Navigate to the project directory:

    ```bash
    cd esp32-irrigation-system
    ```

3. Configure your project:

    ```bash
    idf.py menuconfig
    ```

    Set the GPIO pin configurations for the DHT sensor, relay, and LED.

4. Build and flash the project:

    ```bash
    idf.py build flash
    ```

## Configuration

Adjust the configuration parameters in `main.c` according to your requirements:

- `HUMIDITY_THRESHOLD`: The target humidity level.
- `HYSTERESIS`: The acceptable variation around the target humidity.
- `PUMP_DURATION_SECONDS`: The duration for which the pump is active.
- `BLINK_INTERVAL_MS`: The interval for LED blinking.

## Usage

Run the project on your ESP32, and it will monitor the humidity levels, control the water pump, and provide status feedback through the LED.

## Contributing

Contributions are welcome! Feel free to open issues or pull requests.

## License

This project is licensed under the [MIT License](LICENSE).
