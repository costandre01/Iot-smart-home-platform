# IoT Smart Home Platform

Academic smart-home monitoring and automation project integrating a web dashboard, REST APIs, Raspberry Pi, Arduino-compatible hardware, sensors and actuators.

> **Portfolio note:** the repository keeps the original hardware-oriented implementation, while `docs/` contains a browser-based **Demo Mode** that simulates the physical devices. This makes the project fully explorable without owning the original hardware.

## Live Demo

**GitHub Pages:** https://costandre01.github.io/Iot-smart-home-platform/

The public demo runs entirely in the browser and includes:

- live simulated telemetry;
- temperature and humidity monitoring;
- air-quality values;
- virtual movement/distance readings;
- remote light, door, buzzer and fan controls;
- simulated camera capture;
- telemetry history;
- persistent demo state using browser storage.

No Raspberry Pi, Arduino board, backend server or login is required for the public demo.

## Original Project

The original implementation was developed as an academic IoT project at ESTG. It connected physical devices to a PHP web application through REST endpoints.

### Original architecture

```text
Web Dashboard ↔ PHP REST API ↔ Raspberry Pi / Arduino ↔ Sensors & Actuators
```

The hardware implementation includes:

- Raspberry Pi GPIO integration;
- HC-SR04 distance sensing;
- buzzer control;
- remote webcam capture;
- Arduino/MKR Wi-Fi communication;
- DHT11 temperature and humidity sensing;
- MQ-135-based air-quality measurements;
- servo-controlled door;
- remotely controlled lighting;
- GET/POST communication with the web API.

## Tech Stack

**Web**
- HTML5
- CSS3
- JavaScript
- PHP
- REST API

**IoT / Edge**
- Python
- Raspberry Pi GPIO
- Arduino / MKR
- DHT11
- MQ-135
- HC-SR04
- Servo motor
- Webcam integration

**Portfolio Demo**
- HTML5
- CSS3
- Vanilla JavaScript
- LocalStorage
- GitHub Pages

## Repository Structure

```text
.
├── docs/                       # Public static demo for GitHub Pages
│   ├── index.html
│   ├── styles.css
│   └── app.js
│
└── Projeto/                    # Original academic implementation
    ├── arduino/                # Arduino/MKR firmware
    ├── code/                   # PHP UI components
    ├── config/                 # PHP configuration
    ├── css/                    # Original dashboard styles
    ├── files/                  # Device state and historical sample data
    ├── js/                     # Original dashboard logic
    ├── query/                  # REST-style PHP endpoints
    └── scripts/                # Raspberry Pi Python integration
```

## Run the Public Demo Locally

Because the demo is static, any simple HTTP server works.

```bash
cd docs
python -m http.server 8000
```

Then open `http://localhost:8000`.

## Run the Original PHP Application Locally

The original application requires PHP and write access to `Projeto/files/`.

1. Create a local credentials file from the safe example:

```bash
cp Projeto/files/credenciais.example.txt Projeto/files/credenciais.txt
```

The example account is:

```text
username: demo
password: demo123
```

2. Start the PHP development server from the `Projeto` directory:

```bash
cd Projeto
php -S localhost:8080
```

3. Open `http://localhost:8080`.

Physical sensor updates require the original Raspberry Pi / Arduino hardware. For portfolio purposes, use the static Demo Mode instead.

## GitHub Pages Setup

In the GitHub repository:

1. Open **Settings → Pages**.
2. Under **Build and deployment**, select **Deploy from a branch**.
3. Select branch **main** and folder **/docs**.
4. Save.

GitHub will publish the portfolio demo from `docs/`.

## Security

Hardware network credentials are intentionally excluded from the public source. Replace the placeholders in the Arduino sketches only in your private/local environment.

`Projeto/files/credenciais.txt` is also ignored by Git. `credenciais.example.txt` exists only as a local-development example.

If a secret was previously committed to Git history, removing it from the latest version does not erase the old commit. Treat previously published credentials as exposed and rotate them when applicable.

## Why Demo Mode?

The project was originally built around real hardware. The public Demo Mode replaces only the physical-device layer with simulated telemetry, allowing recruiters and other visitors to interact with the system immediately while keeping the original implementation available for review.

## Authors

- André Costa
- Luís Bento

Academic project · ESTG · 2024/2025
