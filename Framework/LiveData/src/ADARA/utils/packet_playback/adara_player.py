import collections
import hashlib
import itertools
import numpy as np
import os
from pathlib import Path
import socket
import struct
import time
from typing import BinaryIO, Iterator
import yaml

Config = {}
with open(os.environ.get("player_conf", "player_conf.yml"), "rb") as f:
    Config = yaml.safe_load(os.environ.get("player_conf", "player_conf.yml"))


EPICS_EPOCH_OFFSET = 631152000

def multi_glob(path: Path, patterns: str | list[str]) -> Iterator[Path]:
    if isinstance(patterns, str):
        patterns = [patterns]
    return itertools.chain.from_iterable(path.glob(p) for p in patterns)

def sha256sum(path: Path) -> str:
    sha256 = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(65536), b""):
            sha256.update(block)
    return sha256.hexdigest()

class Packet:
    class Type(IntEnum):
        def __init__(self, value: int, version: int):
            self._value_ = value
            self.version = version
        
        RAW_EVENT_TYPE(0x0000, 0x01)
        RTDL_TYPE(0x0001, 0x01)
        SOURCE_LIST_TYPE(0x0002, 0x00)
        MAPPED_EVENT_TYPE(0x0003, 0x01)
        BANKED_EVENT_TYPE(0x4000, 0x01)
        BANKED_EVENT_STATE_TYPE(0x4100, 0x00)
        BEAM_MONITOR_EVENT_TYPE(0x4001, 0x01)
        PIXEL_MAPPING_TYPE(0x4002, 0x00)
        PIXEL_MAPPING_ALT_TYPE(0x4102, 0x01)
        RUN_STATUS_TYPE(0x4003, 0x01)
        RUN_INFO_TYPE(0x4004, 0x00)
        TRANS_COMPLETE_TYPE(0x4005, 0x00)
        CLIENT_HELLO_TYPE(0x4006, 0x01)
        STREAM_ANNOTATION_TYPE(0x4007, 0x00)
        SYNC_TYPE(0x4008, 0x00)
        HEARTBEAT_TYPE(0x4009, 0x00)
        GEOMETRY_TYPE(0x400A, 0x00)
        BEAMLINE_INFO_TYPE(0x400B, 0x01)
        DATA_DONE_TYPE(0x400C, 0x00)
        BEAM_MONITOR_CONFIG_TYPE(0x400D, 0x01)
        DETECTOR_BANK_SETS_TYPE(0x400E, 0x00)
        DEVICE_DESC_TYPE(0x8000, 0x00)
        VAR_VALUE_U32_TYPE(0x8001, 0x00)
        VAR_VALUE_DOUBLE_TYPE(0x8002, 0x00)
        VAR_VALUE_STRING_TYPE(0x8003, 0x00)
        VAR_VALUE_U32_ARRAY_TYPE(0x8004, 0x00)
        VAR_VALUE_DOUBLE_ARRAY_TYPE(0x8005, 0x00)
        MULT_VAR_VALUE_U32_TYPE(0x8101, 0x00)
        MULT_VAR_VALUE_DOUBLE_TYPE(0x8102, 0x00)
        MULT_VAR_VALUE_STRING_TYPE(0x8103, 0x00)
        MULT_VAR_VALUE_U32_ARRAY_TYPE(0x8104, 0x00)
        MULT_VAR_VALUE_DOUBLE_ARRAY_TYPE(0x8105, 0x00)       
        
        def asPacketField(self) -> int:
            return self._value_ << 8 | self.version
        
    def __init__(self, header: bytes, payload: bytes = b"", source: str = ""):
        self._header = header
        self._payload = payload
        self._source = source
        
        # Also provide the hex digest
        sha256 = hashlib.sha256()
        sha256.update(header + payload)
        self._SHA = sha256.hexdigest()
        
        # Use LITTLE-ENDIAN byte order here!
        payload_len = struct.unpack('<I', header[0:4])[0]
        self._size = len(header) + payload_len
        
        type_field = struct.unpack('<I', header[4:8])[0]
        self._packet_type = Packet.Type(type_field >> 8)
        
        tv_sec = struct.unpack('<I', header[8:12])[0]
        tv_nsec = struct.unpack('<I', header[12:16])[0]
        self._pulseid = tv_sec << 32 | tv_nsec
        self._timestamp = np.datetime64((tv_sec + EPICS_EPOCH_OFFSET) * 10**9 + tv_nsec, 'ns')

    @property
    def header(self) -> bytes:
        return self._header
        
    @property
    def payload(self) -> bytes:
        return self._payload
    
    @property
    def source(self) -> str:
        return self._source
    
    @property
    def SHA(self) -> str:
        return self._SHA
        
    @property
    def size(self) -> int:
        return self._size
    
    @property
    def packet_type(self) -> 'Packet.Type':
        return self._packet_type
        
    @property
    def timestamp(self) -> np.datetime64:
        return self._timestamp
    
    @property
    def pulseid(self) -> int:
        return self._pulseid
        
    @classmethod
    def from_file(cls, src: BinaryIO, header_only=False, source=None) -> "Iterator[Packet]":
        # Iterate over `Packet` from a file.
        # If `header_only=True`, only read the headers:
        #   for best efficiency in this case, open files as `open(<filename>, 'rb', buffering=0)`.
        while True:
            header = src.read(16)
            if not header:
                break
            payload_len = struct.unpack('<I', header[0:4])[0]
            payload = None
            if not header_only:
                payload = src.read(payload_len)
            else:
                src.seek(payload_len, 1)
            yield cls(header, payload, source)

    @classmethod
    def from_files(cls, path: Path, patterns: str | list[str], header_only=False) -> "Iterator[Packet]":
        # Iterate over `Packet` from a combination of `glob` expressions:
        #   individual source files will be iterated in order of the timestamp of their first `Packet`.
        def _timestamp(filePath: Path) -> np.datetime64:
            with open(filePath, 'rb', buffering=0) as src:
                return next(cls.from_file(src, header_only=True)).timestamp
        
        ps = sorted(multi_glob(path, patterns), key=_timestamp)
        for p in ps:
            with open(p, 'rb', buffering=0 if header_only else -1) as f:
                yield from cls.from_file(f, header_only=header_only, source=str(p))


class ClientHelloPacket(Packet):
    def __init__(self, header: bytes, payload: bytes):
        super.__init__(header, payload)
        if self.type != Packet.Type.CLIENT_HELLO_TYPE:
            raise ValueError(f"Expecting 'CLIENT_HELLO_TYPE' not '{self.type}'.")
        self._start_time = np.datetime64(struct.unpack('<I', self.payload[0:4])[0], 's')

    @property
    def start_time(self) -> np.datetime64:
        return self._start_time
        
    def fromStartOfRun(self):
        # `start_time` == 1000000000 ns from the EPICs epoch => replay all historical data
        return self.start_time == np.datetime64(1, 's')

        
class Player:
    class Rate(StrEnum):
        NORMAL: 'normal'
        UNLIMITED: 'unlimited'
    
    def __init__(
        self,
        socket_address = None,
        rate = None,
        ignore_packets = []
        ):
        self._rate_filter = Player._get_rate_filter(rate if rate else Config['playback']['rate'])
        self._socket_address = socket_address if socket_address else Player.get_socket_address()
        self._packet_filter = Player._get_packet_filter(ignore_packets if ignore_packets else Config['playback']['ignore_packets'])

        # Miscellaneous settings from `Config`
        self._buffer_MB = Config['server']['buffer_MB']
            
    @classmethod
    def _get_rate_filter(cls, rate: str) -> Callable[[Packet, np.datetime64], bool]:
        rate = Packet.Rate(rate)
        rate_filter = lambda _packet, _start_time: True
        match rate:
            case Rate.NORMAL:
                # Send packets at their original source rate
                rate_filter = lambda packet, start_time: packet.timestamp <= (np.datetime64('now') - start_time)
            case Rate.UNLIMITED:
                pass
            case _:
                raise ValueError(f"unrecognized `Player.Rate` value '{rate}'")
        return rate_filter
    
    @classmethod
    def _get_packet_filter(cls, ignore_packets: Iterable[str]) -> Callable[[Packet], bool]:
        ignore_packets = [Packet.Type(t) for t in ignore_packets]
        if not ignore_packets:
            return lambda _packet: True
        return lambda packet: packet.type not in ignore_packets 
    
    @classmethod
    def _get_socket_address(cls) -> str:
        # Assemble the socket address using information from the from `Config` and from `os.environ`:
        # -- unless overridden, this path will be: ${XDG_RUNTIME_DIR}/sock-${name};
        # -- alternatively: ${TMPDIR} is used to support macOS.
        address = None
        try:
            address_fmt = Config['server']['socket_address']
            runtime_dir = os.environ["XDG_RUNTIME_DIR"] if "XDG_RUNTIME_DIR" in os.environ else os.environ["TMPDIR"]
            name = os.environ.get('adara_player_name', Config['server']['name']) # environment overrides `Config`
            address = address_fmt.format(XDG_RUNTIME_DIR=runtime_dir, name=name))
        except KeyError:
            # Github runners may not have either of the temporary directories: we do not care.
            pass
        return address
    
    def stream_packets(self, path: Path, patterns: str | list[str], socket):
        MB = 1024 * 1024
        buffer_bytes = self._buffer_MB * MB
        packet_buffer = collections.deque()
        current_buffer_bytes = 0
        start_time = np.datetime64('now')

        packets = Packet.from_files(path, patterns)
        try:
            next_packet = next(packets)
        except StopIteration:
            return

        # Pre-fill buffer by MB
        while current_buffer_bytes < buffer_bytes:
            packet_buffer.append(next_packet)
            current_buffer_bytes += next_packet.size
            try:
                next_packet = next(packets)
            except StopIteration:
                break

        while packet_buffer:
            now = np.datetime64('now')
            # Send all packets whose timestamp is ready
            while packet_buffer and packet_buffer[0].timestamp <= (now - start_time):
                pkt = packet_buffer.popleft()

                # Check for unexpected incoming packets
                try:
                    unexpected_pkt = Packet.fromSocket(socket)
                    if unexpected_pkt:
                        logger.warning(f"WARNING: Unexpected packet received from client: '{unexpected_pkt.type}'")
                except BlockingIOError:
                    # No data available - this is expected
                    pass
                except Exception as e:
                    logger.error(f"Exception while checking for incoming data: {e}")
                    raise
                    
                # Send the packet
                try:
                    socket.sendall(pkt.header + pkt.payload)
                except Exception as e:
                    logger.error(f"ERROR: Failed to send packet: {e}")
                    return
                current_buffer_bytes -= pkt.size

            # Fill buffer if room
            while current_buffer_bytes < buffer_bytes:
                try:
                    packet_buffer.append(next_packet)
                    current_buffer_bytes += next_packet.size
                    next_packet = next(packets)
                except StopIteration:
                    break

            # Sleep if nothing to send
            if not (packet_buffer and packet_buffer[0].timestamp <= (now - start_time)):
                time.sleep(0.01)

    def start(self, path: Path, patterns: str | list[str]):
        address = self._socket_address
        use_unix_socket = isinstance(address, (str, os.PathLike))
        if use_unix_socket:
            srv_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            srv_sock.bind(address)
            srv_sock.listen(1)
            logger.info(f"Listening for Unix socket connection at '{address}'...")
            conn, _ = srv_sock.accept()
        else:
            INET_address = tuple(address)
            if not isinstance(INET_address[0], str) and isinstance(INET_address[1], int):
                raise ValueError(f"Expecting socket address in the form `['127.0.0.1': str, port: int]`, not {address}.")
            srv_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            srv_sock.bind(INET_address)
            srv_sock.listen(1)
            logger.info(f"Listening for TCP connection at {INET_address[0]}:{INET_address[1]}...")
            conn, _ = srv_sock.accept()
        
        try: 
            self._start_time = np.datetime64(1, 's') # default: stream all of the data
            if Config['playback']['handshake'] == 'client_hello':
                # Wait for "hello" packet before streaming
                logger.info("Waiting for 'CLIENT_HELLO_PACKET'...")
                try:
                    client_hello = ClientHelloPacket.fromSocket(conn)
                    self._start_time = client_hello.start_time
                    logger.info("Received 'CLIENT_HELLO_PACKET'. Starting stream...")
                except ValueError as e:
                    logger.error(f"{e}")
                    raise

            self.stream_packets(path, patterns, conn)
        finally:
            logger.info(f"Disconnecting server.")
            conn.close()
            srv_sock.close()

if __name__ == "__main__":
    # Usage
    start_server_and_send_streaming_mb('your_large_packets.dat', use_unix_socket=False, port=12345, buffer_mb=10)
