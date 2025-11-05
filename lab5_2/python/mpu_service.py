#!/usr/bin/env python3
"""
mpu_service.py

Subscribe to an MQTT topic and plot real-time graphs for MPU accelerometer data.
Expected payload format: JSON string like: {"ax": -10.00, "ay": 3.90, "az": -9.50}

Usage:
    python mpu_service.py --host test.mosquitto.org --port 1883 --topic esp32/0ad3/tx

The script uses paho-mqtt for MQTT and matplotlib for plotting.
"""

import argparse
import threading
from collections import deque
from datetime import datetime
import json
import math

import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from matplotlib.animation import FuncAnimation
import paho.mqtt.client as mqtt
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401 (needed for 3D projection)


def parse_payload_json(payload: str):
    """Parse JSON payload with keys ax, ay, az -> return tuple(ax, ay, az) or None."""
    try:
        obj = json.loads(payload)
        ax = float(obj.get("ax"))
        ay = float(obj.get("ay"))
        az = float(obj.get("az"))
        return ax, ay, az
    except Exception:
        return None


class MPUPlotter:
    def __init__(self, host, port, topic, bufsize=500, user: str = None, password: str = None):
        self.host = host
        self.port = port
        self.topic = topic
        self.user = user
        self.password = password

        # Buffers
        self.times = deque(maxlen=bufsize)
        self.axs = deque(maxlen=bufsize)
        self.ays = deque(maxlen=bufsize)
        self.azs = deque(maxlen=bufsize)

        self.lock = threading.Lock()

        # MQTT client
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message

        # Matplotlib setup
        preferred_styles = ['seaborn-darkgrid', 'seaborn', 'ggplot', 'dark_background']
        for s in preferred_styles:
            if s in plt.style.available:
                plt.style.use(s)
                break

        # 2x2 grid: 3D trajectory top-left (large), three time-series on right or below
        self.fig = plt.figure(figsize=(12, 8))
        # 3D trajectory
        self.ax3d = self.fig.add_subplot(2, 2, 1, projection='3d')
        self.ax3d.set_title('3D Acceleration Trajectory (ax, ay, az)')
        self.line3d, = self.ax3d.plot([], [], [], lw=1, marker='o', markersize=2, alpha=0.7)

        # Time series for ax, ay, az
        self.ax_ax = self.fig.add_subplot(2, 2, 2)
        self.ax_ay = self.fig.add_subplot(2, 2, 3, sharex=self.ax_ax)
        self.ax_az = self.fig.add_subplot(2, 2, 4, sharex=self.ax_ax)

        self.line_ax, = self.ax_ax.plot([], [], label='ax (m/s²)', color='tab:red')
        self.line_ay, = self.ax_ay.plot([], [], label='ay (m/s²)', color='tab:blue')
        self.line_az, = self.ax_az.plot([], [], label='az (m/s²)', color='tab:orange')

        self.ax_ax.set_ylabel('ax')
        self.ax_ay.set_ylabel('ay')
        self.ax_az.set_ylabel('az')
        self.ax_az.set_xlabel('Time')

        for ax in (self.ax_ax, self.ax_ay, self.ax_az):
            ax.legend(loc='upper left')
            ax.grid(True)

        # Date formatting on x axis
        self.xfmt = mdates.DateFormatter('%H:%M:%S')
        self.ax_az.xaxis.set_major_formatter(self.xfmt)

        # 3D axis labels
        self.ax3d.set_xlabel('ax')
        self.ax3d.set_ylabel('ay')
        self.ax3d.set_zlabel('az')

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
        parsed = parse_payload_json(payload)
        if parsed is None:
            print(f"Received malformed payload on {msg.topic}: {payload!r}")
            return
        axv, ayv, azv = parsed
        now = datetime.now()
        with self.lock:
            self.times.append(now)
            self.axs.append(axv)
            self.ays.append(ayv)
            self.azs.append(azv)
        # concise log
        print(f"{now.strftime('%H:%M:%S')} - ax={axv:.2f} ay={ayv:.2f} az={azv:.2f}")

    def start_mqtt(self):
        try:
            if self.user is not None:
                self.client.username_pw_set(self.user, self.password)
            self.client.connect(self.host, self.port, keepalive=60)
        except Exception as e:
            print(f"Error connecting to MQTT broker: {e}")
            raise
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
                return (self.line3d, self.line_ax, self.line_ay, self.line_az)
            xs_time = list(self.times)
            ys_ax = list(self.axs)
            ys_ay = list(self.ays)
            ys_az = list(self.azs)

        # Update time-series
        self.line_ax.set_data(xs_time, ys_ax)
        self.line_ay.set_data(xs_time, ys_ay)
        self.line_az.set_data(xs_time, ys_az)

        # Autoscale time-series axes
        for ax in (self.ax_ax, self.ax_ay, self.ax_az):
            ax.relim()
            ax.autoscale_view()

        # Set x-axis limits to show latest interval (if enough points)
        try:
            xmin = xs_time[0]
            xmax = xs_time[-1]
            self.ax_az.set_xlim(xmin, xmax)
        except Exception:
            pass

        # 3D trajectory: use ax, ay, az as X,Y,Z
        # For better visual stability, downsample if too many points
        max_points_3d = 1000
        if len(ys_ax) > max_points_3d:
            step = max(1, len(ys_ax) // max_points_3d)
            xs3 = ys_ax[::step]
            ys3 = ys_ay[::step]
            zs3 = ys_az[::step]
        else:
            xs3 = ys_ax
            ys3 = ys_ay
            zs3 = ys_az

        # Update 3D line
        self.line3d.set_data(xs3, ys3)
        self.line3d.set_3d_properties(zs3)

        # Adjust 3D axes limits with a small margin
        try:
            def _range_with_margin(data):
                if not data:
                    return (-1, 1)
                mn = min(data)
                mx = max(data)
                if math.isclose(mn, mx):
                    return (mn - 0.5, mx + 0.5)
                margin = (mx - mn) * 0.1
                return (mn - margin, mx + margin)

            xlim = _range_with_margin(xs3)
            ylim = _range_with_margin(ys3)
            zlim = _range_with_margin(zs3)
            self.ax3d.set_xlim(*xlim)
            self.ax3d.set_ylim(*ylim)
            self.ax3d.set_zlim(*zlim)
        except Exception:
            pass

        # Rotate x-tick labels for readability
        for label in self.ax_az.get_xticklabels():
            label.set_rotation(30)
            label.set_ha('right')

        return (self.line3d, self.line_ax, self.line_ay, self.line_az)

    def run(self, interval_ms=1000):
        self.start_mqtt()
        self.ani = FuncAnimation(self.fig, self._update_plot, interval=interval_ms)

        try:
            plt.tight_layout()
            plt.show()
        except KeyboardInterrupt:
            print('Interrupted by user')
        finally:
            self.stop_mqtt()


def main():
    parser = argparse.ArgumentParser(description='MPU MQTT real-time plotter (ax/ay/az)')
    parser.add_argument('--host', required=True, help='MQTT broker hostname')
    parser.add_argument('--port', type=int, default=1883, help='MQTT broker port (default: 1883)')
    parser.add_argument('--topic', required=True, help='MQTT topic to subscribe to')
    parser.add_argument('--buffer', type=int, default=500, help='Number of points to keep in graph')
    parser.add_argument('--interval', type=int, default=500, help='Plot update interval in milliseconds')
    parser.add_argument('--user', help='MQTT username (optional)')
    parser.add_argument('--password', help='MQTT password (optional)')

    args = parser.parse_args()

    plotter = MPUPlotter(args.host, args.port, args.topic, bufsize=args.buffer,
                         user=args.user, password=args.password)
    try:
        plotter.run(interval_ms=args.interval)
    except Exception as e:
        print(f"Fatal error: {e}")


if __name__ == '__main__':
    main()
