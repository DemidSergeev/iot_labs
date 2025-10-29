DHT MQTT real-time plotter

This small Python service subscribes to an MQTT topic and plots real-time graphs
for DHT sensor data arriving as `temp:humidity:heatindex` (floats).

Requirements

- Python 3.8+
- Install dependencies:

```bash
pip install -r requirements.txt
```

Usage

```bash
python dht_service.py --host <broker_host> --port 1883 --topic <topic>
```

Example

```bash
python dht_service.py --host test.mosquitto.org --port 1883 --topic esp32/0ad3/tx
```

Authentication

If your broker requires username/password authentication, pass the flags `--user` and `--password`:

```bash
python dht_service.py --host <broker> --port 1883 --topic <topic> --user myuser --password mypass
```

Notes

- The script uses paho-mqtt and matplotlib. It runs the MQTT client network loop in a background thread
  and uses matplotlib animation to update plots in the main thread.
- Payload must be exactly three colon-separated float values, e.g. `23.4:45.1:24.8`.
- Use Ctrl+C or close the plot window to exit.
