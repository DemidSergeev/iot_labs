#!/usr/bin/env python3
"""
dht_service.py

Subscribe to an MQTT topic and plot real-time graphs for DHT sensor data.
Expected payload format: "temp:humidity:heatindex" where each value is a float.

Usage:
    python dht_service.py --host test.mosquitto.org --port 1883 --topic esp32/0ad3/tx

The script uses paho-mqtt for MQTT and matplotlib for plotting.
"""

import argparse
import threading
from collections import deque
from datetime import datetime

import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from matplotlib.animation import FuncAnimation
import paho.mqtt.client as mqtt


def parse_payload(payload: str):
    """Parse payload like 'x:y:z' into tuple of floats (temp, hum, heatindex).
    Returns None on parse error.
    """
    try:
        parts = payload.strip().split(":")
        if len(parts) != 3:
            return None
        t, h, hi = map(float, parts)
        return t, h, hi
    except Exception:
        return None


class DHTPlotter:
    def __init__(self, host, port, topic, bufsize=300, user: str = None, password: str = None):
        self.host = host
        self.port = port
        self.topic = topic
        self.user = user
        self.password = password

        # Buffers
        self.times = deque(maxlen=bufsize)
        self.temps = deque(maxlen=bufsize)
        self.humids = deque(maxlen=bufsize)
        self.heatidx = deque(maxlen=bufsize)

        self.lock = threading.Lock()

        # MQTT client
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message

        # Matplotlib setup - pick a style that's available (seaborn may not be installed)
        preferred_styles = ['seaborn-darkgrid', 'seaborn', 'ggplot', 'dark_background']
        for s in preferred_styles:
            if s in plt.style.available:
                plt.style.use(s)
                break
        self.fig, (self.ax_t, self.ax_h, self.ax_hi) = plt.subplots(3, 1, sharex=True, figsize=(10, 8))

        self.line_t, = self.ax_t.plot([], [], label='Temperature (°C)', color='tab:red')
        self.line_h, = self.ax_h.plot([], [], label='Humidity (%)', color='tab:blue')
        self.line_hi, = self.ax_hi.plot([], [], label='Heat Index (°C)', color='tab:orange')

        self.ax_t.set_ylabel('°C')
        self.ax_h.set_ylabel('%')
        self.ax_hi.set_ylabel('°C')
        self.ax_hi.set_xlabel('Time')

        for ax in (self.ax_t, self.ax_h, self.ax_hi):
            ax.legend(loc='upper left')
            ax.grid(True)

        # Date formatting on x axis
        self.xfmt = mdates.DateFormatter('%H:%M:%S')
        self.ax_hi.xaxis.set_major_formatter(self.xfmt)

    # MQTT callbacks
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print(f"Connected to MQTT broker {self.host}:{self.port}")
            client.subscribe(self.topic)
            print(f"Subscribed to topic: {self.topic}")
        else:
            print(f"Failed to connect to MQTT broker, rc={rc}")

    def on_message(self, client, userdata, msg):
        payload = msg.payload.decode(errors='ignore')
        parsed = parse_payload(payload)
        if parsed is None:
            print(f"Received malformed payload on {msg.topic}: {payload!r}")
            return
        t, h, hi = parsed
        now = datetime.now()
        with self.lock:
            self.times.append(now)
            self.temps.append(t)
            self.humids.append(h)
            self.heatidx.append(hi)
        # Also print a short log
        print(f"{now.strftime('%H:%M:%S')} - T={t:.2f}°C H={h:.2f}% HI={hi:.2f}°C")

    def start_mqtt(self):
        try:
            # If username provided, set credentials
            if self.user is not None:
                # password may be None which is permitted
                self.client.username_pw_set(self.user, self.password)

            self.client.connect(self.host, self.port, keepalive=60)
        except Exception as e:
            print(f"Error connecting to MQTT broker: {e}")
            raise
        # Run network loop in background thread
        self.client.loop_start()

    def stop_mqtt(self):
        try:
            self.client.loop_stop()
            self.client.disconnect()
        except Exception:
            pass

    def _update_plot(self, frame):
        with self.lock:
            if len(self.times) == 0:
                return self.line_t, self.line_h, self.line_hi

            xs = list(self.times)
            ys_t = list(self.temps)
            ys_h = list(self.humids)
            ys_hi = list(self.heatidx)

        # Update data
        self.line_t.set_data(xs, ys_t)
        self.line_h.set_data(xs, ys_h)
        self.line_hi.set_data(xs, ys_hi)

        # Adjust x limits
        self.ax_t.relim()
        self.ax_h.relim()
        self.ax_hi.relim()

        self.ax_t.autoscale_view()
        self.ax_h.autoscale_view()
        self.ax_hi.autoscale_view()

        # Improve x-axis range: show last N seconds based on data
        try:
            xmin = xs[0]
            xmax = xs[-1]
            self.ax_hi.set_xlim(xmin, xmax)
        except Exception:
            pass

        # Rotate and redraw date labels
        for label in self.ax_hi.get_xticklabels():
            label.set_rotation(30)
            label.set_ha('right')

        return self.line_t, self.line_h, self.line_hi

    def run(self, interval_ms=1000):
        self.start_mqtt()
        # Keep a reference to the animation to prevent it being garbage collected
        self.ani = FuncAnimation(self.fig, self._update_plot, interval=interval_ms)

        try:
            plt.tight_layout()
            plt.show()
        except KeyboardInterrupt:
            print('Interrupted by user')
        finally:
            self.stop_mqtt()


def main():
    parser = argparse.ArgumentParser(description='DHT MQTT real-time plotter')
    parser.add_argument('--host', required=True, help='MQTT broker hostname')
    parser.add_argument('--port', type=int, default=1883, help='MQTT broker port (default: 1883)')
    parser.add_argument('--topic', required=True, help='MQTT topic to subscribe to')
    parser.add_argument('--buffer', type=int, default=300, help='Number of points to keep in graph')
    parser.add_argument('--interval', type=int, default=1000, help='Plot update interval in milliseconds')
    parser.add_argument('--user', help='MQTT username (optional)')
    parser.add_argument('--password', help='MQTT password (optional)')

    args = parser.parse_args()

    plotter = DHTPlotter(args.host, args.port, args.topic, bufsize=args.buffer,
                         user=args.user, password=args.password)
    try:
        plotter.run(interval_ms=args.interval)
    except Exception as e:
        print(f"Fatal error: {e}")


if __name__ == '__main__':
    main()
