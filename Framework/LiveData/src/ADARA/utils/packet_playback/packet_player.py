import collections
from enum import IntEnum, StrEnum
import errno
import itertools
import logging
import numpy as np
import os
from pathlib import Path
import re
import select
import signal
import socket
import struct
import sys
import time
from typing import Any, BinaryIO, Callable, Iterable, Iterator
import yaml
from zlib import crc32


# =========================================================
# ====== INITIALIZE the application's `Config` dict. ======
# =========================================================
TRACE_LEVEL = 5
logging.addLevelName(TRACE_LEVEL, "TRACE")


def trace(self, message, *args, **kws):
    if self.isEnabledFor(TRACE_LEVEL):
        self._log(TRACE_LEVEL, message, args, **kws)


logging.Logger.trace = trace

_config_path = Path(os.environ.get("adara_player_conf", "adara_player_conf.yml")).resolve()
if not _config_path.exists():
    print(f"Error during load of module '{__file__}' (as '{__name__}'):\n    config file '{_config_path}' not found on filesystem.")
    sys.exit(1)

Config = {}
with open(_config_path, "rt") as f:
    # Load the `Config` `dict`.
    Config = yaml.safe_load(f)

    # Initialize logging:
    
    ##################################################################################
    ## WARNING: in general, no console logging should originate from these classes. ##
    ##   Each client session runs in its own process, and at present we do not      ##
    ##   implement multi-process logging.                                           ##
    ##################################################################################
    
    log_file_path = Config["logging"]["filename"].format(PID=os.getpid())
    formatter = logging.Formatter(Config["logging"]["format"])

    _logger = logging.getLogger(__name__)
    _logger.setLevel(Config["logging"]["level"])

    if log_file_path:
        file_handler = logging.FileHandler(log_file_path)
        file_handler.setFormatter(formatter)
        _logger.addHandler(file_handler)
    else:
        console_handler = logging.StreamHandler()
        console_handler.setFormatter(formatter)
        _logger.addHandler(console_handler)

# =========================================================
# ====== end: `Config` initialization                ======
# =========================================================


EPICS_EPOCH_OFFSET = 631152000


class UnixGlob:
    # Parse a bash-style glob expression (optionally including braces)
    #   into a Python-compatible format: `(<base-directory path>, list[<glob str>])`.

    @classmethod
    def parse(cls, pattern) -> tuple[Path, list[str]]:
        # Split pattern into base dir and glob (find first non-path element)
        sep = os.sep
        parts = pattern.split(sep)
        base = []
        for part in parts:
            if any(ch in part for ch in "*?[]{},"):
                break
            base.append(part)
        base_dir = sep.join(base) if base else "."
        glob_pattern = sep.join(parts[len(base) :]) if base else pattern

        # Brace expansion: expand {a,b} to [a, b]
        def expand_braces(s):
            # Matches {...}
            match = re.search(r"\{([^{}]+)\}", s)
            if not match:
                return [s]
            choices = match.group(1).split(",")
            results = set()
            for choice in choices:
                replaced = s[: match.start()] + choice + s[match.end() :]
                for result in expand_braces(replaced):
                    results.add(result)
            return list(results)

        globs = expand_braces(glob_pattern)

        return (Path(base_dir), globs)

    @classmethod
    def multi_glob(cls, path: Path, patterns: str | list[str]) -> Iterator[Path]:
        if isinstance(patterns, str):
            patterns = [patterns]
        return itertools.chain.from_iterable(path.glob(p) for p in patterns)


class SocketAddress:
    # Parse a socket address string, specified as either a Unix-domain socket path, or an IP/host:port,
    #   into a Python-compatible format: `Path | tuple[str, int]`.

    @classmethod
    def parse(cls, address: str) -> Path | tuple[str, int]:
        # Match [IPv6]:port, IPv4:port, or hostname:port
        # Hostname spec (RFC 1123): letter/digit first, can contain '-', '.', letters, digits
        host_port_re = re.compile(
            r"""
            ^
            (?:\[(?P<ipv6>[0-9a-fA-F:]+)\]|      # [IPv6]
               (?P<host>[a-zA-Z0-9.\-]+)         # or hostname/domain, or IPv4
            )
            :
            (?P<port>\d{1,5})                    # Port
            $
            """,
            re.VERBOSE,
        )

        match = host_port_re.match(address)
        if match:
            ip = match.group("ipv6") or match.group("host")
            port = int(match.group("port"))

            if 0 < port <= 65535:
                return (ip, port)
            else:
                raise ValueError(f"Port out of range: {port}")

        # If not host:port, treat as Unix socket path (absolute only)
        if address.startswith("/"):
            return Path(address)

        raise ValueError(f"Invalid address format: {address}")

    @classmethod
    def isUDSSocket(cls, address: Path | tuple[str, int]):
        return isinstance(address, Path)


class Packet:
    HEADER_BYTES = 16
    STR_FORMAT = Config["logging"]["packet_summary"]

    class Type(IntEnum):
        RAW_EVENT_TYPE = (0x0000, 0x01)
        RTDL_TYPE = (0x0001, 0x01)
        SOURCE_LIST_TYPE = (0x0002, 0x00)
        MAPPED_EVENT_TYPE = (0x0003, 0x01)
        BANKED_EVENT_TYPE = (0x4000, 0x01)
        BANKED_EVENT_STATE_TYPE = (0x4100, 0x00)
        BEAM_MONITOR_EVENT_TYPE = (0x4001, 0x01)
        PIXEL_MAPPING_TYPE = (0x4002, 0x00)
        PIXEL_MAPPING_ALT_TYPE = (0x4102, 0x01)
        RUN_STATUS_TYPE = (0x4003, 0x01)
        RUN_INFO_TYPE = (0x4004, 0x00)
        TRANS_COMPLETE_TYPE = (0x4005, 0x00)
        CLIENT_HELLO_TYPE = (0x4006, 0x01)
        STREAM_ANNOTATION_TYPE = (0x4007, 0x00)
        SYNC_TYPE = (0x4008, 0x00)
        HEARTBEAT_TYPE = (0x4009, 0x00)
        GEOMETRY_TYPE = (0x400A, 0x00)
        BEAMLINE_INFO_TYPE = (0x400B, 0x01)
        DATA_DONE_TYPE = (0x400C, 0x00)
        BEAM_MONITOR_CONFIG_TYPE = (0x400D, 0x01)
        DETECTOR_BANK_SETS_TYPE = (0x400E, 0x00)
        DEVICE_DESC_TYPE = (0x8000, 0x00)
        VAR_VALUE_U32_TYPE = (0x8001, 0x00)
        VAR_VALUE_DOUBLE_TYPE = (0x8002, 0x00)
        VAR_VALUE_STRING_TYPE = (0x8003, 0x00)
        VAR_VALUE_U32_ARRAY_TYPE = (0x8004, 0x00)
        VAR_VALUE_DOUBLE_ARRAY_TYPE = (0x8005, 0x00)
        MULT_VAR_VALUE_U32_TYPE = (0x8101, 0x00)
        MULT_VAR_VALUE_DOUBLE_TYPE = (0x8102, 0x00)
        MULT_VAR_VALUE_STRING_TYPE = (0x8103, 0x00)
        MULT_VAR_VALUE_U32_ARRAY_TYPE = (0x8104, 0x00)
        MULT_VAR_VALUE_DOUBLE_ARRAY_TYPE = (0x8105, 0x00)

        def __new__(cls, value, version):
            obj = int.__new__(cls, value)
            obj._value_ = value
            obj.version = version
            return obj

        def asPacketField(self) -> int:
            return self._value_ << 8 | self.version

    def __init__(self, header: bytes, payload: bytes = b"", source: str | None = None):
        self._header = header
        self._payload = payload
        self._source = source

        self._CRC = crc32(header + payload)

        # Use LITTLE-ENDIAN byte order here!
        payload_len = struct.unpack("<I", header[0:4])[0]
        self._size = len(header) + payload_len
        type_field = struct.unpack("<I", header[4:8])[0]

        # Extract type value before creating enum
        type_value = type_field >> 8
        version = type_field & 0xFF

        try:
            self._packet_type = Packet.Type(type_value)
        except ValueError:
            raise ValueError(f"Invalid ADARA packet type: 0x{type_value:04X} (version {version}). Type field: 0x{type_field:08X}")

        tv_sec = struct.unpack("<I", header[8:12])[0]
        tv_nsec = struct.unpack("<I", header[12:16])[0]
        self._pulseid = (tv_sec << 32) | tv_nsec
        self._timestamp = np.datetime64(tv_sec + EPICS_EPOCH_OFFSET, "s") + np.timedelta64(tv_nsec, "ns")

    @property
    def header(self) -> bytes:
        return self._header

    @property
    def payload(self) -> bytes:
        return self._payload

    @property
    def source(self) -> str | None:
        return self._source

    @property
    def CRC(self) -> int:
        return self._CRC

    @property
    def size(self) -> int:
        return self._size

    @property
    def packet_type(self) -> "Packet.Type":
        return self._packet_type

    @property
    def timestamp(self) -> np.datetime64:
        return self._timestamp

    @property
    def pulseid(self) -> int:
        return self._pulseid

    def __str__(self) -> str:
        KB = 1024
        MB = 1024 * 1024
        if self.size >= MB:
            size_str = f"{self.size / MB:6.2f} MB"
        elif self.size >= KB:
            size_str = f"{self.size // KB:5d} KB"
        else:
            size_str = f"{self.size:5d}  B"
        return self.STR_FORMAT.format(type=self.packet_type, timestamp=self.timestamp, size=size_str, CRC=self.CRC)

    @classmethod
    def create_header(self, *, payload_len, packet_type, tv_sec, tv_nsec):
        """Helper method to create packet headers for testing."""
        header = struct.pack("<I", payload_len)
        header += struct.pack("<I", packet_type.asPacketField())
        header += struct.pack("<I", tv_sec)
        header += struct.pack("<I", tv_nsec)
        return header

    @classmethod
    def to_file(cls, dest: BinaryIO, packet: "Packet"):
        dest.write(packet.header + packet.payload)

    @classmethod
    def from_file(cls, src: BinaryIO, source: str = None) -> "Packet":
        """Read a single packet from a file object."""
        header = src.read(cls.HEADER_BYTES)
        if not header:
            raise EOFError("No data available to read")
        if len(header) < cls.HEADER_BYTES:
            raise EOFError(f"Incomplete header: got {len(header)} bytes, expected {cls.HEADER_BYTES}")

        payload_len = struct.unpack("<I", header[0:4])[0]

        if payload_len > 0:
            payload = src.read(payload_len)
            if len(payload) < payload_len:
                raise EOFError(f"Incomplete payload: got {len(payload)} bytes, expected {payload_len}")
        else:
            payload = b""

        return cls(header=header, payload=payload, source=source)

    @classmethod
    def to_socket(cls, dest: socket.socket, packet: "Packet"):
        cls._send_all(dest, packet.header + packet.payload)

    @classmethod
    def from_socket(cls, src: socket.socket) -> "Packet":
        # Read the header:
        header = cls._recv_exact(src, cls.HEADER_BYTES)
        payload_len = struct.unpack("<I", header[0:4])[0]

        # Read the payload
        if payload_len > 0:
            payload = cls._recv_exact(src, payload_len)
        else:
            payload = b""

        return cls(header=header, payload=payload)

    ## ====== HELPER methods to WRAP socket partial transfers. ======

    @classmethod
    def _recv_exact(cls, src: socket.socket, num_bytes: int) -> bytes:
        _logger.trace(f"Receiving {num_bytes} bytes from socket")
        data = b""
        while len(data) < num_bytes:
            try:
                chunk = src.recv(num_bytes - len(data))
                if not chunk:
                    raise ConnectionError(f"Socket connection closed after receiving {len(data)} of {num_bytes} bytes")
                data += chunk
            except socket.error as e:
                if e.errno in (errno.EAGAIN, errno.EWOULDBLOCK):
                    continue  # Just retry, DO NOT break or lose track
                else:
                    raise
            except socket.timeout:
                # Re-raise timeout with context about partial transfer
                raise socket.timeout(f"Socket timeout after receiving {len(data)} of {num_bytes} bytes")
        return data

    @staticmethod
    def _send_all(sock: socket.socket, data: bytes) -> int:
        _logger.trace(f"Sending {len(data)} bytes to socket")
        total_sent = 0
        while total_sent < len(data):
            try:
                sent = sock.send(data[total_sent:])
                if sent == 0:
                    raise ConnectionError("Socket connection broken")
                total_sent += sent
            except socket.error as e:
                if e.errno in (errno.EAGAIN, errno.EWOULDBLOCK):
                    continue  # Retry the send, DO NOT break or lose track
                else:
                    raise
            except socket.timeout:
                # Re-raise timeout with context about partial transfer
                raise socket.timeout(f"Socket timeout after sending {total_sent} of {len(data)} bytes")
        return total_sent

    ## ====== end: HELPER methods.                              ======


class ClientHelloPacket(Packet):
    def __init__(self, header: bytes, payload: bytes):
        super().__init__(header, payload)
        if self.packet_type != Packet.Type.CLIENT_HELLO_TYPE:
            raise ValueError(f"Expecting 'CLIENT_HELLO_TYPE' not '{self._packet_type}'.")
        self._start_time = np.datetime64(struct.unpack("<I", self.payload[0:4])[0], "s")

    @property
    def start_time(self) -> np.datetime64:
        return self._start_time

    @property
    def is_StartOfRun_packet(self) -> bool:
        # `start_time` == 1000000000 ns from the EPICs epoch => replay all historical data
        return self.start_time == np.datetime64(1, "s")


class Player:
    SOCKET_TIMEOUT = Config["server"]["socket_timeout"]
    HANDSHAKE_TIMEOUT = Config["playback"]["handshake_timeout"]
    PLAYBACK_HANDSHAKE = Config["playback"]["handshake"]
    TRANSFER_LIMIT_MB = Config["record"]["transfer_limit"]
    PACKET_ORDERING_SCHEME = Config["playback"]["packet_ordering"]
    
    # "generic" glob (for use in multi-session playback): all packets in a directory
    PACKET_GLOB = Config["playback"]["packet_glob"]

    class Rate(StrEnum):
        NORMAL = "normal"
        UNLIMITED = "unlimited"

    def __init__(self, *, source_address=None):
        self._source_address = SocketAddress.parse(source_address if source_address else Config["source"]["address"])

        # Initialize control callbacks
        self._rate_filter = Player._get_rate_filter(Config["playback"]["rate"])
        self._packet_filter = Player._get_packet_filter(Config["playback"]["ignore_packets"])

        # Miscellaneous settings from `Config`
        self._buffer_MB = Config["server"]["buffer_MB"]

        self._running = False

        self._transferred_bytes = 0  # total transferred bytes: for current `record` session
        self._sequence_number = 0  # packet file sequence number: for current `record` session
        self._session_number = 0  # `record` session number

        # packet-source socket:
        self._source = None

        # listener socket:
        self._server = None

        # client (i.e. connection) socket:
        self._client = None

    @classmethod
    def _get_rate_filter(cls, rate: str) -> Callable[[Packet, np.datetime64], bool]:
        rate = Player.Rate(rate)
        match rate:
            case cls.Rate.NORMAL:
                # Send packets at their original source rate
                return lambda packet, packets_start_time, start_time: (packet.timestamp - packets_start_time) <= (
                    np.datetime64("now") - start_time
                )
            case cls.Rate.UNLIMITED:
                return lambda _packet, _packets_start_time, _start_time: True
            case _:
                raise ValueError(f"unrecognized `Player.Rate` value '{rate}'")

    @classmethod
    def _get_packet_filter(cls, ignore_packets: Iterable[str]) -> Callable[[Packet], bool]:
        ignore_packets = [Packet.Type(t) for t in ignore_packets]
        if not ignore_packets:
            return lambda _packet: True
        return lambda packet: packet.packet_type not in ignore_packets

    @classmethod
    def _packet_ordering(cls) -> Callable[[Path], Any]:
        match cls.PACKET_ORDERING_SCHEME:
            case "sequence_number":
                return cls._file_sequence_number
            case "timestamp":
                return cls._file_timestamp
            case "mtime":
                return cls._file_mtime
            case _:
                raise ValueError(f"unrecognized packet ordering '{cls.PACKET_ORDERING_SCHEME}'")

    @classmethod
    def _file_timestamp(cls, filePath: Path) -> np.datetime64:
        # Sorting key for ADARA packet files, using the timestamp of the first packet.
        try:
            with open(filePath, "rb", buffering=0) as src:
                first_packet = next(cls.iter_file(src, header_only=True))
                return first_packet.timestamp
        except FileNotFoundError:
            _logger.error(f"File not found: {filePath}")
            raise
        except PermissionError:
            _logger.error(f"Permission denied reading file: {filePath}")
            raise
        except StopIteration:
            _logger.error(f"Empty or invalid file (no packets): {filePath}, using epoch as timestamp")
            return np.datetime64(0, "ns")  # Use epoch for empty files
        except struct.error as e:
            _logger.error(f"Corrupt packet header in file: {filePath}: {e}")
            raise ValueError(f"Corrupt file: {filePath}")
        except Exception as e:
            _logger.error(f"Unexpected error reading timestamp from {filePath}: {e}")
            raise

    @classmethod
    def _file_mtime(cls, filePath: Path) -> np.datetime64:
        # Sorting key for ADARA packet files, using the file's modification time.
        try:
            mtime_ns = int(filePath.stat().st_mtime * 1e9)  # nanoseconds since epoch
            return np.datetime64(mtime_ns, "ns")
        except FileNotFoundError:
            _logger.error(f"File not found: {filePath}")
            raise
        except PermissionError:
            _logger.error(f"Permission denied reading file: {filePath}")
            raise
        except StopIteration:
            _logger.error(f"Empty or invalid file (no packets): {filePath}, using epoch as timestamp")
            return np.datetime64(0, "ns")  # Use epoch for empty files
        except struct.error as e:
            _logger.error(f"Corrupt packet header in file: {filePath}: {e}")
            raise ValueError(f"Corrupt file: {filePath}")
        except Exception as e:
            _logger.error(f"Unexpected error reading timestamp from {filePath}: {e}")
            raise

    @classmethod
    def _file_sequence_number(cls, filePath: Path) -> int:
        # Sorting key for ADARA packet files, using the sequence number from the filename.

        # Match the last hyphen-separated group before '.adara'
        match = re.search(r"-(\d{6})\.adara$", filePath.name)
        if match is None:
            _logger.error(f"Cannot extract sequence number from '{filePath}': using 0 as sequence number")
            return 0
        return int(match.group(1))

    @classmethod
    def get_server_address(cls) -> str:
        # Assemble the socket address using information from the from `Config` and from `os.environ`:
        # -- unless overridden, this path will be: ${XDG_RUNTIME_DIR}/sock-${name};
        # -- alternatively: ${TMPDIR} is used to support macOS.
        address = Config["server"]["address"]

        # replace any tokens that may have been specified
        runtime_dir = os.environ.get("XDG_RUNTIME_DIR", os.environ.get("TMPDIR"))
        name = os.environ.get("adara_player_name", Config["server"]["name"])  # environment overrides `Config`
        address = address.format(XDG_RUNTIME_DIR=runtime_dir, name=name)

        # Note: this method may raise `KeyError`: e.g. github runners may not have either of the temporary directories,
        #   or `ValueError` (any parsing error).
        return address

    @classmethod
    def iter_file(cls, src: BinaryIO, header_only=False, source=None) -> Iterator[Packet]:
        """Iterate over packets in a single file."""
        # If `header_only=True`, only read the headers:
        # for best efficiency in this case, open files as `open(<filename>, 'rb', buffering=0)`.
        while True:
            header = src.read(Packet.HEADER_BYTES)
            if not header:
                break
            if len(header) < Packet.HEADER_BYTES:
                # Incomplete header at EOF: do not yield, just stop iteration.
                break
            payload_len = struct.unpack("<I", header[0:4])[0]
            payload = b""
            if not header_only:
                payload = src.read(payload_len)
            else:
                src.seek(payload_len, 1)
            yield Packet(header, payload, source)

    @classmethod
    def iter_files(cls, path: Path, patterns: str | list[str], header_only=False) -> Iterator[Packet]:
        """Iterate over packets across multiple files."""

        # Source files are specified using glob expressions.
        # Source files themselves will be iterated in order of the timestamp of their first `Packet`.

        try:
            files = list(UnixGlob.multi_glob(path, patterns))
            if not files:
                _logger.warning(f"No files found matching pattern: {patterns} in '{path}'")
                return

            _logger.info(f"Found {len(files)} files, sorting by {cls.PACKET_ORDERING_SCHEME}...")
            ps = sorted(files, key=cls._packet_ordering())
        except Exception as e:
            _logger.error(f"Error sorting files: {e}")
            raise

        for p in ps:
            _logger.debug(f"Processing file: {p}")
            with open(p, "rb", buffering=0 if header_only else -1) as f:
                yield from cls.iter_file(f, header_only=header_only, source=str(p))

    def stream_packets(self, dest: socket.socket, path: Path, patterns: str | list[str], dry_run=False):
        MB = 1024 * 1024
        buffer_bytes = self._buffer_MB * MB
        packet_buffer = collections.deque()
        current_buffer_bytes = 0
        start_time = np.datetime64("now")

        # Get packet iterator
        packets = self.iter_files(path, patterns)

        try:
            next_packet = next(packets)
        except StopIteration:
            _logger.warning("No packets available to stream")
            return
        packets_start_time = next_packet.timestamp

        # Pre-fill buffer by MB
        while current_buffer_bytes < buffer_bytes:
            packet_buffer.append(next_packet)
            current_buffer_bytes += next_packet.size
            try:
                next_packet = next(packets)
            except StopIteration:
                # IMPORTANT: clear `next_packet` when iterator is exhausted!
                next_packet = None
                break
        _logger.debug(f"Buffered {len(packet_buffer)} packets ({current_buffer_bytes / MB:.2f} MB).")

        while packet_buffer:
            if not self._running:
                break

            # Send all packets whose timestamp is ready
            now = np.datetime64("now")
            while packet_buffer and (packet_buffer[0].timestamp - packets_start_time) <= (now - start_time):
                if not self._running:
                    break
                pkt = packet_buffer.popleft()

                if not dry_run:
                    readable, writable, exceptional = select.select([dest], [dest], [dest], 0)

                    if socket in exceptional:
                        _logger.error("Socket error detected")
                        return

                    # Check for UNEXPECTED incoming packets
                    if dest in readable:
                        try:
                            unexpected_pkt = Packet.from_socket(dest)
                            _logger.warning(f"RECV unexpected: {unexpected_pkt}")
                        except Exception as e:
                            _logger.error(f"Unable to read unexpected packet: {e}")
                            return

                    # Verify socket is writable before sending
                    if dest not in writable:
                        _logger.warning("Socket not ready for writing, retrying...")
                        packet_buffer.appendleft(pkt)  # Put packet back
                        current_buffer_bytes += pkt.size
                        time.sleep(0.01)  # packet rate is 60 Hz: 0.017 seconds
                        continue

                    # Send the packet
                    try:
                        Packet.to_socket(dest, pkt)
                        _logger.debug(f"SEND: {pkt}")
                    except (socket.error, socket.timeout) as e:
                        _logger.error(f"Failed to send packet: {e}")
                        return
                    current_buffer_bytes -= pkt.size
                else:
                    # dry run: log the packet that would have been sent
                    _logger.debug(f"[{Path(pkt.source).name}]: SEND: {pkt}")
                    current_buffer_bytes -= pkt.size

            # Fill buffer if room available
            if current_buffer_bytes < buffer_bytes and next_packet:
                while current_buffer_bytes < buffer_bytes:
                    if not self._running:
                        break
                    if next_packet is None:
                        break
                    try:
                        packet_buffer.append(next_packet)
                        current_buffer_bytes += next_packet.size
                        next_packet = next(packets)
                    except StopIteration:
                        next_packet = None
                        break
                _logger.debug(f"Buffered {len(packet_buffer)} packets ({current_buffer_bytes / MB:.2f} MB).")

            # Sleep if nothing to send
            if not (packet_buffer and (packet_buffer[0].timestamp - packets_start_time) <= (now - start_time)):
                time.sleep(0.01)  # packet rate is 60 Hz: 0.017 seconds

        _logger.info("Finished streaming all packets")

    def play(self, client: socket.socket, path: Path, patterns: str | list[str], dry_run=False):
        try:
            self._running = True

            if not dry_run:
                # Make this as symmetric with `record()` as possible -- use non-blocking mode.
                client.settimeout(self.SOCKET_TIMEOUT)
                client.setblocking(False)

                self._start_time = np.datetime64(1, "s")  # default: stream all of the data

                if self.PLAYBACK_HANDSHAKE == "client_hello":
                    # Wait for "hello" packet before streaming
                    _logger.info("Waiting for 'CLIENT_HELLO_PACKET'...")

                    # For handshake, temporarily use blocking mode with a longer timeout.
                    client.settimeout(self.HANDSHAKE_TIMEOUT)
                    client.setblocking(True)
                    try:
                        client_hello = ClientHelloPacket.from_socket(self._client)
                        self._start_time = client_hello.start_time
                        _logger.info("Received 'CLIENT_HELLO_PACKET'. Starting stream...")
                    except ValueError as e:
                        _logger.error(f"{e}")
                        raise
                    finally:
                        # Restore non-blocking mode for streaming
                        client.settimeout(self.SOCKET_TIMEOUT)
                        client.setblocking(False)

            self.stream_packets(client, path, patterns, dry_run=dry_run)
        finally:
            _logger.info("Disconnecting from server.")
            self._running = False
            self._cleanup(sockets=[client])

    def record(self, *, output_path: Path, client: socket.socket):
        # Ensure target directory exists
        output_path.mkdir(parents=True, exist_ok=True)

        # Connect to ADARA packet server.
        try:
            _logger.info(f"Connecting to ADARA packet server at {self._source_address}.")
            source = None
            if SocketAddress.isUDSSocket(self._source_address):
                source = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            else:
                source = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            source.settimeout(self.SOCKET_TIMEOUT)
            source.connect(self._source_address)
            _logger.info(f"Successfully connected to ADARA packet server at {self._source_address}.")

            # Start a new session for each connection to the packet server:
            #   this will write all output to a single directory, named as the session number.
            sequence_number = 1
            self._transferred_bytes = 0

            # Set sockets to non-blocking mode for use with select
            client.setblocking(False)
            source.setblocking(False)

            # Queue all data to allow source and client transfer-rate mismatch!
            to_client_queue = []
            to_server_queue = []

            while self._running:
                readables = []
                if not to_server_queue:
                    readables.append(client)
                if not to_client_queue:
                    readables.append(source)

                writables = []
                if to_client_queue:
                    writables.append(client)
                if to_server_queue:
                    writables.append(source)

                exceptionals = [client, source]
                readable, writable, exceptional = select.select(readables, writables, exceptionals, self.SOCKET_TIMEOUT)

                # Check for errors first
                if exceptional:
                    _logger.error("Socket error detected: select clause")
                    break

                loop_broken = False  # Used to propagate any 'break'

                # Process readable sockets: one packet per iteration per socket.
                for sock in readable:
                    try:
                        packet = Packet.from_socket(sock)

                        # limit total number of bytes transferred
                        self._impose_transfer_limit(packet)
                        if not self._running:
                            loop_broken = True
                            break

                        # Determine direction and forward
                        if sock is source:
                            # Data from server: log packet and forward to client.
                            file_path = self._packet_file_path(output_path, packet, sequence_number)
                            with open(file_path, "wb") as f:
                                Packet.to_file(f, packet)
                            sequence_number += 1

                            data = packet.header + packet.payload
                            to_client_queue.append(data)

                            # The packet hasn't actually been sent yet, but it's easiest to log it here.
                            _logger.debug(f"server SENDs client -> {packet}")
                        else:
                            # Data from client: forward to server (e.g. control packets)
                            data = packet.header + packet.payload
                            to_server_queue.append(data)
                            _logger.debug(f"client SENDs server <- {packet}")
                    except (socket.timeout, socket.error) as e:
                        _logger.error(f"Socket error during RECV: {e}")
                        loop_broken = True
                        break
                else:
                    # Process writable sockets: one packet per iteration per socket.
                    for sock in writable:
                        if not self._running:
                            loop_broken = True
                            break
                        queue = to_client_queue if sock is client else to_server_queue
                        if queue:
                            data = queue[0]
                            try:
                                sent = sock.send(data)
                                if sent < len(data):
                                    queue[0] = data[sent:]  # Partial write; keep unsent part
                                else:
                                    queue.pop(0)

                            except (socket.timeout, socket.error) as e:
                                _logger.error(f"Socket error during SEND: {e}")
                                loop_broken = True
                                break

                if loop_broken:
                    break
        except Exception as e:
            _logger.error(f"Error in proxy loop: {e}.")
        finally:
            self.cleanup(sockets=[client, source])
    
    @classmethod
    def _packet_file_path(cls, output_path: Path, packet: Packet, sequence_number: int) -> Path | None:
        return output_path / cls._packet_filename(packet, sequence_number)

    @classmethod
    def _packet_filename(cls, packet: Packet, sequence_number: int) -> str:
        # Device descriptor (and other) packet types may have a non-unique timestamp,
        #   so the sequence number is used as an additional suffix in the filename.
        return f"{packet.packet_type:#06x}-{packet.timestamp}-{sequence_number:06d}" + ".adara"

    def _impose_transfer_limit(self, pkt: Packet):
        # impose absolute limit on number of bytes transferred
        MB = 1024**2
        self._transferred_bytes += pkt.size
        if self._transferred_bytes / MB > self.TRANSFER_LIMIT_MB:
            _logger.error(f"Transfer limit of {self.TRANSFER_LIMIT_MB} MB exceeded.")
            self._running = False

    @classmethod
    def cleanup(cls, *, sockets: Iterable[socket.socket], addresses: Iterable[Path | tuple[str, int]] = []):
        """Clean up sockets and associated files"""
        for sock in sockets:
            try:
                sock.close()
            except:
                pass
        for address in addresses:
            try:
                # Remove any UDS path from filesystem
                if SocketAddress.isUDSSocket(address) and address.exists():
                    address.unlink()
            except:
                pass

    def signal_handler(self, signum, frame):
        """Handle shutdown signals"""
        _logger.info(f"\nReceived signal {signum}, shutting down client session...")
        self._running = False

        # Set an alarm to force exit if cleanup takes too long.
        def _timeout_handler(signum, frame):
            _logger.error("Shutdown timeout exceeded, forcing exit")
            os._exit(1)

        signal.signal(signal.SIGALRM, _timeout_handler)
        signal.alarm(10)  # 10-second timeout
