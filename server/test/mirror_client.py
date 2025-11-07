import os
import queue
import socket
import subprocess
import sys
import threading
import time

import av
from PySide6.QtCore import Qt, Signal, QThread
from PySide6.QtGui import QAction, QImage, QPixmap
from PySide6.QtWidgets import QApplication, QMainWindow, QSlider, QLabel

from device_connector import DeviceConnector

openharmony = True

class StreamReader(QThread):
    frame_ready = Signal(QImage)
    def __init__(self):
        super().__init__()
        self.connector = None
        self.stop_request = False
        self.frame_queue = queue.Queue(maxsize=10)

    def run(self):
        global openharmony
        if openharmony:
            print("Using OpenHarmony device connection")
            self.connector = oh_start_mirror_server_and_connect()
        else:
            self.connector = start_mirror_server_and_connect()
        if not self.connector:
            print("Could not connect to mirror_server")
            return

        # Get device information
        device_info = self.connector.query_device_info()
        if device_info:
            print(f"Device information: {device_info}")

        self.frame_thread = threading.Thread(target=self._process_frames, daemon=True)
        self.frame_thread.start()

        self._poll_stream()


    def stop(self):
        print("Stopping read thread")
        self.stop_request = True
        if self.connector:
            self.connector.stop_screen_capture()
            self.connector.exit()

    def _poll_stream(self):
        # Send start capture command
        self.connector.start_screen_capture()

        """Parse H264 stream to video frames"""
        codec = av.CodecContext.create("h264", "r")
        while not self.stop_request:
            try:
                pkt_type, data = self.connector.recv_packet()
                if pkt_type == DeviceConnector.PKT_SCREEN_FRAME:
                    # Handle screen frame data
                    if not data:
                        break
                    print(f"Received screen frame data: {len(data)} bytes, type={pkt_type}")
                    # print(data[:16])
                    for packet in codec.parse(data):
                        for frame in codec.decode(packet):
                            self.frame_queue.put(frame)
                elif pkt_type is None:
                    print("Connection lost")
                    break
            except Exception as e:
                print(f"Error receiving frame: {str(e)}")
                break


    def _process_frames(self):
        """Process frames to convert to QImage and emit signal"""
        while not self.stop_request:
            try:
                frame = self.frame_queue.get(timeout=0.5)
                height, width = frame.height, frame.width
                rgb_frame = frame.reformat(format='rgb24')
                rgb_data = rgb_frame.to_ndarray(format='rgb24')

                image = QImage(
                    rgb_data.data,
                    width,
                    height,
                    width * 3,
                    QImage.Format.Format_RGB888
                )
                self.frame_ready.emit(image)
            except queue.Empty:
                continue
            except Exception as e:
                print(f"Frame processing error: {str(e)}")

class VideoPlayerWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Screen Recorder Player")
        self.setGeometry(100, 100, 800, 600)

        self.video_label = QLabel()
        self.video_label.setMinimumSize(640, 480)
        self.video_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.video_label.setScaledContents(False)
        self.setCentralWidget(self.video_label)

        self.toolbar = self.addToolBar("Controls")
        self.start_action = QAction("Start")
        self.start_action.triggered.connect(self.start_playback)
        self.toolbar.addAction(self.start_action)
        self.stop_action = QAction("Stop")
        self.stop_action.triggered.connect(self.stop_playback)
        self.stop_action.setEnabled(False)
        self.toolbar.addAction(self.stop_action)
        self.toolbar.addSeparator()
        self.slider = QSlider(Qt.Orientation.Horizontal)
        self.slider.setRange(0, 100)
        self.toolbar.addWidget(self.slider)
        self.statusBar().showMessage("Ready")


    def start_playback(self):
        self.stream_reader = StreamReader()
        self.stream_reader.frame_ready.connect(self.handle_frame)
        self.stream_reader.start()

        self.statusBar().showMessage("Connecting device...")
        QApplication.processEvents()

        self.statusBar().showMessage(f"Playing")
        self.start_action.setEnabled(False)
        self.stop_action.setEnabled(True)


    def stop_playback(self):
        self.stream_reader.stop()
        self.statusBar().showMessage("Stopped")
        self.stop_action.setEnabled(False)
        self.start_action.setEnabled(True)

    def handle_frame(self, image):
        pixmap = QPixmap.fromImage(image)
        scaled_pixmap = pixmap.scaled(
            self.video_label.size(),
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation
        )
        self.video_label.setPixmap(scaled_pixmap)
        if not self.slider.isSliderDown():
            current_pos = self.slider.value() + 1
            if current_pos > 100:
                current_pos = 0
            self.slider.setValue(current_pos)

    def closeEvent(self, event):
        self.stream_reader.stop()
        super().closeEvent(event)


def start_mirror_server_and_connect(server_path="../android/release/mirror_server", local_port=12346, remote_port=12345):
    # 1. Get device serial number
    result = subprocess.run(["adb", "devices"], capture_output=True, text=True)
    lines = result.stdout.strip().splitlines()
    serial = None
    for line in lines[1:]:
        parts = line.strip().split()
        if len(parts) >= 2 and parts[1] == "device":
            serial = parts[0]
            break
    if not serial:
        print("No available adb device detected")
        return None

    print(f"Detected device: {serial}")

    # 2. Push server to device
    server_path = os.path.abspath(server_path)
    remote_path = "/data/local/tmp/mirror_server"
    # Kill old process first
    subprocess.run(["adb", "-s", serial, "shell", "pkill", "-9", "mirror_server"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(["adb", "-s", serial, "push", server_path, remote_path], check=True)
    subprocess.run(["adb", "-s", serial, "shell", "chmod", "755", remote_path], check=True)

    # 3. Start server
    # Start new process (background)
    subprocess.Popen(["adb", "-s", serial, "shell", f"{remote_path} &"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(1)  # Wait for server to start

    # 4. tcp port forwarding
    subprocess.run(["adb", "-s", serial, "forward", f"tcp:{local_port}", f"tcp:{remote_port}"], check=True)

    # 5. Connect to server
    connector = DeviceConnector("127.0.0.1", local_port)
    for _ in range(10):
        try:
            connector.connect()
            print("Connected to mirror_server")
            break
        except (ConnectionRefusedError, socket.error):
            time.sleep(0.5)
    else:
        print("Failed to connect to mirror_server")
        return None

    return connector


def oh_start_mirror_server_and_connect(server_path="../ohos/release/mirror_server", local_port=12345, remote_port=12345):
    # 1. Get device serial number
    result = subprocess.run(["hdc", "list", "targets"], capture_output=True, text=True)
    lines = result.stdout.strip().splitlines()
    serial = None
    for line in lines:
        parts = line.strip().split()
        serial = parts[0]
    if not serial:
        print("No available hdc device detected")
        return None

    print(f"Detected device: {serial}")

    # 2. Push server to device
    server_path = os.path.abspath(server_path)
    remote_path = "/data/local/tmp/mirror_server"
    # Kill old process first
    subprocess.run(["hdc", "-t", serial, "shell", "pkill", "-9", "mirror_server"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(["hdc", "-t", serial, "file", "send", server_path, remote_path], check=True)
    subprocess.run(["hdc", "-t", serial, "shell", "chmod", "755", remote_path], check=True)

    # 3. Start server
    # Start new process (background)
    subprocess.Popen(["hdc", "-t", serial, "shell", f"{remote_path} &"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(1)  # Wait for server to start

    # 4. tcp port forwarding
    subprocess.run(["hdc", "-t", serial, "fport", f"tcp:{local_port}", f"tcp:{remote_port}"], check=True)

    # 5. Connect to server
    connector = DeviceConnector("127.0.0.1", local_port)
    for _ in range(10):
        try:
            connector.connect()
            print("Connected to mirror_server")
            break
        except (ConnectionRefusedError, socket.error):
            time.sleep(0.5)
    else:
        print("Failed to connect to mirror_server")
        return None

    return connector

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "ohos":
        openharmony = True
    else:
        openharmony = False

    app = QApplication(sys.argv)
    window = VideoPlayerWindow()
    window.show()

    sys.exit(app.exec())
