import socket
import struct
from typing import Optional

class DeviceConnector:
    """
    Client for communicating with scrcpy_server
    """

    # Command types (must be consistent with the server)
    CMD_QUERY_DEVICE_INFO = 1
    CMD_GET_SCREEN_FRAME = 2
    CMD_START_SCREEN_CAPUTRE = 3
    CMD_STOP_SCREEN_CAPUTRE = 4
    CMD_EXIT = 5

    # Packet types (server -> client)
    PKT_DEVICE_INFO = 1
    PKT_SCREEN_FRAME = 2
    PKT_ACK = 3
    PKT_ERROR = 4
    PKT_UNKNOWN = 255

    HEADER_FMT = "<BI"  # uint8_t type, uint32_t length (little-endian)

    def __init__(self, host: str = "127.0.0.1", port: int = 12345):
        self.host = host
        self.port = port
        self.sock: Optional[socket.socket] = None

    def connect(self):
        self.sock = socket.create_connection((self.host, self.port))

    def close(self):
        if self.sock:
            self.sock.close()
            self.sock = None

    def send_command(self, cmd_type: int, payload: bytes = b""):
        header = struct.pack(self.HEADER_FMT, cmd_type, len(payload))
        self.sock.sendall(header + payload)

    def recv_packet(self):
        # Receive packet header
        header = self._recv_all(struct.calcsize(self.HEADER_FMT))
        if not header:
            return None, None
        pkt_type, length = struct.unpack(self.HEADER_FMT, header)
        # Receive data body
        data = self._recv_all(length) if length > 0 else b""
        return pkt_type, data

    def _recv_all(self, size: int) -> bytes:
        buf = b""
        while len(buf) < size:
            chunk = self.sock.recv(size - len(buf))
            if not chunk:
                raise ConnectionError("Connection closed by server")
            buf += chunk
        return buf

    def query_device_info(self):
        self.send_command(self.CMD_QUERY_DEVICE_INFO)
        pkt_type, data = self.recv_packet()
        if pkt_type == self.PKT_DEVICE_INFO:
            # Parse DeviceInfo struct (must be consistent with the server struct)
            # Assume the struct format is as follows (adjust according to actual struct):
            fmt = "<32s32s32s32s32siiii16s"
            if len(data) >= struct.calcsize(fmt):
                unpacked = struct.unpack(fmt, data[:struct.calcsize(fmt)])
                return {
                    "model": unpacked[0].decode("utf-8", errors="ignore").rstrip('\x00'),
                    "brand": unpacked[1].decode("utf-8", errors="ignore").rstrip('\x00'),
                    "manufacturer": unpacked[2].decode("utf-8", errors="ignore").rstrip('\x00'),
                    "market_name": unpacked[3].decode("utf-8", errors="ignore").rstrip('\x00'),
                    "os_version": unpacked[4].decode("utf-8", errors="ignore").rstrip('\x00'),
                    "api_version": unpacked[5],
                    "dpi": unpacked[6],
                    "screen_width": unpacked[7],
                    "screen_height": unpacked[8],
                    "cpu_arch": unpacked[9].decode("utf-8", errors="ignore").rstrip('\x00'),
                }
        return None

    def start_screen_capture(self):
        print("Starting screen capture...")
        self.send_command(self.CMD_START_SCREEN_CAPUTRE)

    def stop_screen_capture(self):
        print("Stopping screen capture...")
        self.send_command(self.CMD_STOP_SCREEN_CAPUTRE)

    def exit(self):
        print("Exiting...")
        self.send_command(self.CMD_EXIT)