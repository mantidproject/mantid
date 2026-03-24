# ruff: noqa: C901  # `stream_packets`, `record` are too complex

import collections
from contextlib import contextmanager
from dataclasses import dataclass, field
from enum import IntEnum, nonmember, StrEnum
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
from typing import Any, BinaryIO, Callable, ClassVar, Iterable, Iterator
from zlib import crc32

import player_config
from player_config import get_config, yaml_load  # noqa: F401  (re-export yaml_load for existing callers)


##
## ADARA-packet player is not implemented on Windows, due to lack of `SIGALRM` signal.
##
if sys.platform.startswith("win"):
    raise SystemExit("ADARA-packet player is not implemented for the Windows OS.")
##


# ---------------------------------------------------------------------------
# Module-level logger
#   Handlers are attached later, after config has been loaded, via
#   ``init_process_logger()`` or ``player_config.configure_logger()``.
# ---------------------------------------------------------------------------
_logger = logging.getLogger(__name__)


def get_log_file_path_for_PID(pid: int) -> Path | None:
    """Delegate to player_config."""
    return player_config.get_log_file_path_for_PID(pid)


def init_process_logger() -> None:
    """Configure this module's logger for the current process."""
    player_config.init_process_logger(_logger)


# EPICS offset from the Unix epoch (seconds).
EPICS_EPOCH_OFFSET = 631152000


# =========================================================================
# initialize() -- populate class-level configuration constants
# =========================================================================
def initialize():
    """
    Populate class-level configuration constants from the loaded Config.

    Must be called exactly once, after ``player_config.load_config()``,
    and before any ``Player`` / ``Packet`` / ``_PacketFileSummary`` usage.
    Children inherit these values via fork().
    """
    cfg = get_config()

    # Packet
    Packet.STR_FORMAT = cfg["logging"]["packet_summary"]

    # _PacketFileSummary
    _PacketFileSummary.TIMESTAMP_RESOLUTION = cfg["summarize"]["timestamp_resolution"]

    # Player
    Player.SOCKET_TIMEOUT = cfg["server"]["socket_timeout"]
    Player.HANDSHAKE_TIMEOUT = cfg["playback"]["handshake_timeout"]
    Player.PLAYBACK_HANDSHAKE = cfg["playback"]["handshake"]
    Player.TRANSFER_LIMIT_MB = cfg["record"]["transfer_limit"]
    Player.PACKET_ORDERING_SCHEME = cfg["playback"]["packet_ordering"]
    Player.SUMMARIZE_TIMESTAMP_PER_TYPE = cfg["summarize"]["timestamp_per_type"]
    Player.PACKET_GLOB = cfg["playback"]["packet_glob"]

    # Module-level logger
    init_process_logger()


# =========================================================================


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
    """
    This is only a rudimentary implementation of a `Packet` class:
    - the objective here is primarily to access the packet header information.
    """

    HEADER_BYTES = 16
    STR_FORMAT: str = "type: {type:#06x}: {timestamp}, size: {size:>7}, CRC: {CRC:#010x}"  # set by initialize()

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

        # cache the `EVENT_TYPES` set
        _event_types_cache = nonmember(None)

        @classmethod
        def EVENT_TYPES(cls) -> frozenset["Packet.Type"]:
            """
            Consolidate packet types which include event data.
            """
            if cls._event_types_cache is None:
                # Define event-type subset once the enum is fully constructed
                cls._event_types_cache = frozenset(
                    {
                        Packet.Type.RAW_EVENT_TYPE,
                        Packet.Type.MAPPED_EVENT_TYPE,
                        Packet.Type.BANKED_EVENT_TYPE,
                        Packet.Type.BANKED_EVENT_STATE_TYPE,
                        Packet.Type.BEAM_MONITOR_EVENT_TYPE,
                    }
                )
            return cls._event_types_cache

        @classmethod
        def is_event_type(cls, t: "Packet.Type") -> bool:
            return t in cls.EVENT_TYPES()

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
    def create_header(cls, *, payload_len, packet_type, tv_sec, tv_nsec):
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
        tv_sec = struct.unpack("<I", self.payload[0:4])[0]
        self._start_time = np.datetime64(tv_sec + EPICS_EPOCH_OFFSET, "s")

    @property
    def start_time(self) -> np.datetime64:
        return self._start_time

    @property
    def is_StartOfRun_packet(self) -> bool:
        # `start_time` == 1000000000 ns from the EPICs epoch => replay all historical data
        return self.start_time - np.timedelta64(EPICS_EPOCH_OFFSET, "s") == np.datetime64(1, "s")


@dataclass(frozen=True)
class _PacketFileSummary:
    """
    Accumulated information about the content of a file of packets:
    - this information is used by `Player.summarize` both to provide information to the user,
      and to suggest how the packet files should be ordered during playback.
    """

    TIMESTAMP_RESOLUTION: ClassVar[float] = 0.017  # set by initialize()

    file_path: Path
    type_counts: collections.Counter[Packet.Type] = field(default_factory=collections.Counter)
    ts_ranges: dict[Packet.Type, tuple[np.datetime64, np.datetime64]] = field(default_factory=dict)

    def accumulate(self, pkt: Packet):
        ts = pkt.timestamp
        type_ = pkt.packet_type

        # Update counts
        self.type_counts[type_] += 1

        # Update timestamp ranges
        min_ts, max_ts = self.ts_ranges.get(type_, (ts, ts))

        self.ts_ranges[type_] = (min(min_ts, ts), max(max_ts, ts))

    @property
    def packet_count(self) -> int:
        # packet count over all packet types
        return sum(self.type_counts[type_] for type_ in self.type_counts)

    @property
    def ts_range(self) -> tuple[np.datetime64, np.datetime64] | None:
        # timestamp range over all packet types
        return self._get_ts_range()

    @property
    def events_ts_range(self) -> tuple[np.datetime64, np.datetime64] | None:
        # timestamp range over all event-packet types
        return self._get_ts_range(keys=Packet.Type.EVENT_TYPES())

    def _get_ts_range(self, keys: Iterable[Packet.Type] | None = None) -> tuple[np.datetime64, np.datetime64] | None:
        # calculate packet-type filtered timestamp ranges
        items = self.ts_ranges.values() if not keys else [self.ts_ranges[k] for k in keys if k in self.ts_ranges]

        if not items:
            return None

        return min(r[0] for r in items), max(r[1] for r in items)

    # ---- classification helpers for ordering ----

    @property
    def is_events_file(self) -> bool:
        """True if this file contains any event-bearing packet types."""
        return any(t in Packet.Type.EVENT_TYPES() and self.type_counts.get(t, 0) > 0 for t in self.type_counts)

    @property
    def events_start(self) -> np.datetime64 | None:
        """Start time of the event-only range (if any)."""
        r = self.events_ts_range
        return r[0] if r is not None else None

    @property
    def metadata_start(self) -> np.datetime64 | None:
        """
        Start time for metadata ordering.

        For metadata-only files, choose the earliest timestamp of a
        'controlling' packet type, falling back to the overall earliest
        timestamp if needed.
        """
        if self.is_events_file:
            return None

        for t in self._timestamp_control_priority:
            if t in self.ts_ranges:
                return self.ts_ranges[t][0]

        # Fallback: earliest timestamp over all packet types in this file
        r = self.ts_range
        return r[0] if r is not None else None

    @property
    def sort_ts(self) -> np.datetime64:
        """
        Primary timestamp used for suggested playback ordering.

        - For event-bearing files: use events_start.
        - For metadata-only files: use metadata_start.
        - Fallback to overall earliest timestamp if neither is available.
        - Fallback to epoch if the file has no timestamps at all.
        """
        epoch = np.datetime64(0, "ns")
        ts = epoch  # fallback (for completely empty/invalid summaries)

        # Events first: for event files, events_start is the primary timestamp.
        if self.events_start is not None:
            ts = self.events_start
        elif self.metadata_start is not None:
            # Metadata-only file: use metadata controlling timestamp.
            ts = self.metadata_start
        else:
            # Fallback: earliest over all packet types in the file.
            r = self.ts_range
            if r is not None:
                ts = r[0]

        # Round timestamp to required resolution before comparison:
        #   ADARA timestamps jitter slightly.  The default value for
        #   resolution is the pulse-time interval.
        return self.round_datetime64(ts, self.TIMESTAMP_RESOLUTION)

    @staticmethod
    def round_datetime64(t, dt):
        """
        Round np.datetime64 to nearest dt seconds, zeroing sub-interval digits.
        """
        epoch = np.datetime64(0, "ns")
        # epoch = np.datetime64('1970-01-01T00:00:00')

        # Convert to nanoseconds since epoch
        ns = (t - epoch) / np.timedelta64(1, "ns")
        ns = ns.astype("int64")

        # Quantize to nearest dt interval (zeroing sub-dt precision)
        dt_ns = int(round(dt * 1e9))  # Nearest integer nanoseconds
        ns_quantized = (ns // dt_ns) * dt_ns  # Explicit quantization

        return epoch + ns_quantized.astype("timedelta64[ns]")

    # Priority list for metadata-timestamp "controlling" types:
    _timestamp_control_priority: tuple[Packet.Type, ...] = (
        Packet.Type.RUN_INFO_TYPE,
        Packet.Type.STREAM_ANNOTATION_TYPE,
        Packet.Type.GEOMETRY_TYPE,
        Packet.Type.BEAMLINE_INFO_TYPE,
        Packet.Type.BEAM_MONITOR_CONFIG_TYPE,
        Packet.Type.PIXEL_MAPPING_ALT_TYPE,
        Packet.Type.DEVICE_DESC_TYPE,
        Packet.Type.VAR_VALUE_U32_TYPE,
        Packet.Type.VAR_VALUE_DOUBLE_TYPE,
        Packet.Type.VAR_VALUE_STRING_TYPE,
        Packet.Type.VAR_VALUE_U32_ARRAY_TYPE,
        Packet.Type.VAR_VALUE_DOUBLE_ARRAY_TYPE,
        Packet.Type.MULT_VAR_VALUE_U32_TYPE,
        Packet.Type.MULT_VAR_VALUE_DOUBLE_TYPE,
        Packet.Type.MULT_VAR_VALUE_STRING_TYPE,
        Packet.Type.MULT_VAR_VALUE_U32_ARRAY_TYPE,
        Packet.Type.MULT_VAR_VALUE_DOUBLE_ARRAY_TYPE,
    )

    @classmethod
    def sorted_for_playback(cls, summaries: list["_PacketFileSummary"]) -> list["_PacketFileSummary"]:
        """
        Suggested playback ordering.

        New scheme:
        - Primary key: timestamp
            * For event-bearing files: events_start
            * For metadata-only files: metadata_start (control-packet priority)
              WARNING: current implementation does not check for duplicate device-descriptor sections!
            * Fallback to earliest overall timestamp, then epoch if needed
        - Secondary key: metadata before events files
        - Tertiary key: filename (for tie-breaking)
        """

        def sort_key(s: "_PacketFileSummary") -> tuple:
            # Primary: unified timestamp
            ts = s.sort_ts

            # Secondary: metadata first (0) then event files (1)
            meta_flag = 0 if not s.is_events_file else 1

            # Tertiary: filename
            name = s.file_path.name

            return (ts, meta_flag, name)

        return sorted(summaries, key=sort_key)


class Player:
    # ------ Config constants (set by initialize()) ------
    SOCKET_TIMEOUT: float = 1.0  # set by initialize()
    HANDSHAKE_TIMEOUT: float = 10.0  # set by initialize()
    PLAYBACK_HANDSHAKE: str = "client_hello"  # set by initialize()
    TRANSFER_LIMIT_MB: int = 16000  # set by initialize()
    PACKET_ORDERING_SCHEME: str = "yaml"  # set by initialize()
    SUMMARIZE_TIMESTAMP_PER_TYPE: bool = False  # set by initialize()
    PACKET_GLOB: str = "*.adara"  # set by initialize()
    # ------

    class Rate(StrEnum):
        NORMAL = "normal"
        UNLIMITED = "unlimited"

    class FileOutputMode(StrEnum):
        SINGLE_PACKET = "single_packet"
        MULTI_PACKET = "multi_packet"

    def __init__(self, *, source_address=None, packet_ordering_scheme=None):
        cfg = get_config()

        self._source_address: Path | tuple[str, int] = SocketAddress.parse(source_address or cfg["source"]["address"])

        # Initialize control callbacks
        self._rate_filter = Player._get_rate_filter(cfg["playback"]["rate"])
        self._packet_filter = Player._get_packet_filter(cfg["playback"]["ignore_packets"])

        # Miscellaneous settings from config
        self._file_output_mode = Player.FileOutputMode(cfg["record"]["file_output"])
        self._buffer_MB = cfg["server"]["buffer_MB"]
        self._packet_ordering_scheme = packet_ordering_scheme or self.PACKET_ORDERING_SCHEME

        self._running = False

        self._transferred_bytes = 0  # total transferred bytes: for current `record` session
        self._sequence_number = 0  # packet file sequence number: for current `record` session
        self._session_number = 0  # `record` session number

    @contextmanager
    def _open_record_file(
        self,
        output_path: Path,
        packet: Packet,
        sequence_number: int,
    ) -> Iterator[BinaryIO]:
        """
        Context manager that yields a writable file object for a recorded packet.

        - In SINGLE_PACKET mode: opens a new file per packet.
        - In MULTI_PACKET mode: appends this packet to a single session file.
        """
        file_path = self._packet_file_path(output_path, packet, sequence_number)

        # In MULTI_PACKET mode: this is inefficient, as we re-open once per packet, however:
        # - this allows us to keep the output section in one place;
        # - the overhead should be negligable in comparison to network socket I/O.
        mode = "wb" if self._file_output_mode is Player.FileOutputMode.SINGLE_PACKET else "ab"

        with open(file_path, mode) as f:
            yield f

    @classmethod
    def init_process_logger(cls):
        # Call the module-level method:
        # - this initializes a file-based logging handler,
        #   depending on the PID of the client session.
        init_process_logger()

    @classmethod
    def _get_rate_filter(cls, rate: str) -> Callable[[Packet, np.datetime64, np.datetime64], bool]:
        """
        Constructs a callable to test whether or not a given packet should be immediately sent:
        args:
        `packet`: the packet to test
        `packets_start_time`: the timestamp from the first packet in the stream
        `start_time`: the time that the stream was started by the player (real time)
        """
        rate = Player.Rate(rate)
        match rate:
            case cls.Rate.NORMAL:
                # Send packets at their original source rate
                return lambda packet, packets_start_time, start_time: (
                    (packet.timestamp - packets_start_time) <= (np.datetime64("now") - start_time)
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

    def _packet_ordering(self) -> Callable[[Path], Any]:
        match self._packet_ordering_scheme:
            case "yaml":
                # Return a constant key; sort is stable, so original input order is preserved
                return lambda _path: 0
            case "sequence_number":
                return self._file_sequence_number
            case "timestamp":
                return self._file_timestamp
            case "mtime":
                return self._file_mtime
            case _:
                raise ValueError(f"unrecognized packet ordering '{self._packet_ordering_scheme}'")

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
        # Assemble the socket address using information from config and from `os.environ`:
        # -- unless overridden, this path will be: ${XDG_RUNTIME_DIR}/sock-${name};
        # -- alternatively: ${TMPDIR} is used to support macOS.
        cfg = get_config()
        address = cfg["server"]["address"]

        # replace any tokens that may have been specified
        runtime_dir = os.environ.get("XDG_RUNTIME_DIR", os.environ.get("TMPDIR"))
        name = os.environ.get("adara_player_name", cfg["server"]["name"])  # environment overrides config
        address = address.format(XDG_RUNTIME_DIR=runtime_dir, name=name)

        # Note: this method may raise `KeyError`: e.g. github runners may not have either of the temporary directories,
        #   or `ValueError` (any parsing error).
        return address

    @classmethod
    def iter_file(
        cls,
        src: BinaryIO,
        header_only: bool = False,
        source: str | None = None,
        packet_callback: Callable[[Packet], None] | None = None,
    ) -> Iterator[Packet]:
        """Iterate over packets in a single file."""
        # If `header_only=True`, only read the headers:
        #   for best efficiency in this case, open files as `open(<filename>, 'rb', buffering=0)`.
        while True:
            header = src.read(Packet.HEADER_BYTES)
            if not header:
                break
            if len(header) < Packet.HEADER_BYTES:
                # Incomplete header at EOF: do not yield, just stop iteration.
                break

            payload_len = struct.unpack("<I", header[0:4])[0]

            payload = b""
            if header_only:
                if payload_len > 0:
                    src.seek(payload_len, 1)
            else:
                if payload_len > 0:
                    payload = src.read(payload_len)
                    if len(payload) < payload_len:
                        # Incomplete payload at EOF: stop iteration.
                        break

            packet = Packet(header=header, payload=payload, source=source)
            if packet_callback is not None:
                packet_callback(packet)

            yield packet

    def iter_files(
        self,
        files: Iterable[Path],
        header_only: bool = False,
        file_callback: Callable[[Path], None] | None = None,
        packet_callback: Callable[[Packet], None] | None = None,
    ) -> Iterator[Packet]:
        """Iterate over packets across multiple files."""

        # Source files themselves will be iterated in order of cls.PACKET_ORDERING_SCHEME
        # applied using the file-path itself, or implicitly to the file's first packet.

        try:
            files_list = list(files)
            if not files_list:
                _logger.warning("No files found ...")
                return

            ps = files_list
            if self._packet_ordering_scheme != "yaml":
                _logger.info(f"Found {len(files_list)} files, sorting by {self._packet_ordering_scheme} ...")
                ps = sorted(files_list, key=self._packet_ordering())
            else:
                _logger.info(f"Found {len(files_list)} files ...")

        except Exception as e:
            _logger.error(f"Error sorting files: {e}")
            raise

        for p in ps:
            _logger.debug(f"Processing file: {p}")
            if file_callback is not None:
                file_callback(p)
            with open(p, "rb", buffering=0 if header_only else -1) as f:
                yield from self.iter_file(
                    f,
                    header_only=header_only,
                    source=str(p),
                    packet_callback=packet_callback,
                )

    def stream_packets(self, dest: socket.socket, files: Iterable[Path], dry_run: bool = False):
        MB = 1024 * 1024
        buffer_bytes = self._buffer_MB * MB
        packet_buffer = collections.deque()
        current_buffer_bytes = 0
        start_time = np.datetime64("now")

        # Get packet iterator
        packets = self.iter_files(files)

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
        _logger.info(f"Buffered {len(packet_buffer)} packets ({current_buffer_bytes / MB:.2f} MB).")

        stall_start = None  # track any client-socket write stall

        try:
            while packet_buffer:
                if not self._running:
                    break

                # Send all packets whose timestamp is ready
                while packet_buffer and self._rate_filter(packet_buffer[0], packets_start_time, start_time):
                    if not self._running:
                        break
                    pkt = packet_buffer.popleft()
                    current_buffer_bytes -= pkt.size  # always track `popleft` or `append`

                    if not dry_run:
                        readable, writable, exceptional = select.select([dest], [dest], [dest], 0)

                        if dest in exceptional:
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
                            if stall_start is None:
                                stall_start = time.monotonic()  # mark start of stall
                            _logger.debug("Client socket not ready for writing, retrying...")
                            packet_buffer.appendleft(pkt)  # Put packet back
                            current_buffer_bytes += pkt.size  # restore the bytes count
                            time.sleep(0.01)  # packet rate is 60 Hz: 0.017 seconds
                            continue

                        # socket is writable; if we were in a stall, log its duration
                        if stall_start is not None:
                            stall_duration = time.monotonic() - stall_start
                            if stall_duration > self.SOCKET_TIMEOUT:
                                _logger.warning(
                                    "Client socket write stall lasted %.3f s (SOCKET_TIMEOUT=%.3f s).",
                                    stall_duration,
                                    self.SOCKET_TIMEOUT,
                                )
                            stall_start = None

                        # Send the packet
                        try:
                            Packet.to_socket(dest, pkt)
                            _logger.debug(f"SEND: {pkt}")
                        except (socket.error, socket.timeout) as e:
                            _logger.error(f"Failed to send packet: {e}")
                            return
                    else:
                        # dry run: log the packet that would have been sent
                        _logger.debug(f"[{Path(pkt.source).name if pkt.source else '<unknown>'}]: SEND: {pkt}")

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

                # Sleep if nothing is ready to send
                if not (packet_buffer and self._rate_filter(packet_buffer[0], packets_start_time, start_time)):
                    time.sleep(0.01)  # packet rate is 60 Hz: 0.017 seconds

        finally:
            # if we exit while still in a stall, log that stall duration once
            if stall_start is not None:
                stall_duration = time.monotonic() - stall_start
                if stall_duration > self.SOCKET_TIMEOUT:
                    _logger.warning(
                        "Client socket write stall lasted %.3f s at disconnect (SOCKET_TIMEOUT=%.3f s).",
                        stall_duration,
                        self.SOCKET_TIMEOUT,
                    )
            if packet_buffer:
                _logger.warning(f"Client disconnected before end of stream: {len(packet_buffer)} packets remain in stream.")

        _logger.info("Finished streaming all packets")

    def play(self, client: socket.socket, files: Iterable[Path], dry_run: bool = False):
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
                        client_hello = ClientHelloPacket.from_socket(client)
                        self._start_time = client_hello.start_time
                        _logger.info("Received 'CLIENT_HELLO_PACKET'. Starting stream...")
                    except ValueError as e:
                        _logger.error(f"{e}")
                        raise
                    finally:
                        # Restore non-blocking mode for streaming
                        client.settimeout(self.SOCKET_TIMEOUT)
                        client.setblocking(False)

            self.stream_packets(client, files, dry_run=dry_run)
        finally:
            _logger.info("Disconnecting from server.")
            self._running = False
            self.cleanup(sockets=[client])

    def record(self, output_path: Path, client: socket.socket):
        # Ensure target directory exists
        output_path.mkdir(parents=True, exist_ok=True)

        # If the logger is attached to a file: make a symlink to it in the target directory.
        _log_file_path = get_log_file_path_for_PID(os.getpid())
        if _log_file_path:
            (output_path / "session.log").symlink_to(_log_file_path)

        # Connect to ADARA packet server.
        source = None
        stall_start_client = None  # track client-direction write stall
        stall_start_server = None  # track server-direction write stall

        try:
            self._running = True

            _logger.info(f"Connecting to ADARA packet server at {self._source_address}.")
            source = None
            if SocketAddress.isUDSSocket(self._source_address):
                source = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            else:
                source = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            source.settimeout(self.SOCKET_TIMEOUT)
            source.connect(str(self._source_address) if isinstance(self._source_address, Path) else self._source_address)
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

                now = time.monotonic()  # common timestamp for stall tracking

                # update client-direction stall state
                if to_client_queue and client not in writable:
                    if stall_start_client is None:
                        stall_start_client = now
                else:
                    if stall_start_client is not None:
                        stall_duration = now - stall_start_client
                        if stall_duration > self.SOCKET_TIMEOUT:
                            _logger.warning(
                                "Client socket write stall lasted %.3f s (SOCKET_TIMEOUT=%.3f s).",
                                stall_duration,
                                self.SOCKET_TIMEOUT,
                            )
                        stall_start_client = None

                # update server-direction stall state
                if to_server_queue and source not in writable:
                    if stall_start_server is None:
                        stall_start_server = now
                else:
                    if stall_start_server is not None:
                        stall_duration = now - stall_start_server
                        if stall_duration > self.SOCKET_TIMEOUT:
                            _logger.warning(
                                "Server socket write stall lasted %.3f s (SOCKET_TIMEOUT=%.3f s).",
                                stall_duration,
                                self.SOCKET_TIMEOUT,
                            )
                        stall_start_server = None

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
                            with self._open_record_file(output_path, packet, sequence_number) as f:
                                Packet.to_file(f, packet)

                            # `sequence_number` will only be used for SINGLE_PACKET files,
                            #   but there's no harm in maintaining it for both SINGLE_PACKET and
                            #   MULTI_PACKET cases.
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
            _logger.error(f"Error in proxy loop: {e}.", exc_info=True)
        finally:
            # log any stall still active at disconnect
            if stall_start_client is not None:
                stall_duration = time.monotonic() - stall_start_client
                if stall_duration > self.SOCKET_TIMEOUT:
                    _logger.warning(
                        "Client socket write stall lasted %.3f s at disconnect (SOCKET_TIMEOUT=%.3f s).",
                        stall_duration,
                        self.SOCKET_TIMEOUT,
                    )

            if stall_start_server is not None:
                stall_duration = time.monotonic() - stall_start_server
                if stall_duration > self.SOCKET_TIMEOUT:
                    _logger.warning(
                        "Server socket write stall lasted %.3f s at disconnect (SOCKET_TIMEOUT=%.3f s).",
                        stall_duration,
                        self.SOCKET_TIMEOUT,
                    )

            _logger.info("Disconnecting from server.")
            self._running = False
            self.cleanup(sockets=[client, source])

    def _summarize(self, files: Iterable[Path]) -> list[_PacketFileSummary]:
        """
        Internal helper for 'summarize' mode:
        - drives iter_files with file and packet callbacks
        - builds one _PacketFileSummary per input file, in playback order
        - performs no logging
        """
        summaries: list[_PacketFileSummary] = []
        current_summary: _PacketFileSummary | None = None

        def _flush_current() -> None:
            nonlocal current_summary
            if current_summary is None:
                return
            summaries.append(current_summary)
            current_summary = None

        def _start_file(p: Path) -> None:
            nonlocal current_summary
            # Finish previous file, if any
            _flush_current()
            current_summary = _PacketFileSummary(p)

        def _accumulate(pkt: Packet) -> None:
            nonlocal current_summary
            if current_summary is None:
                # Defensive guard; in normal use start_file is always called first.
                return
            current_summary.accumulate(pkt)

        # Drive the iteration; all work is done in callbacks.
        for _ in self.iter_files(
            files,
            header_only=False,
            file_callback=_start_file,
            packet_callback=_accumulate,
        ):
            pass

        # Flush final file
        _flush_current()

        return summaries

    def summarize(self, files: Iterable[Path]) -> None:
        """
        Log a summary for each source-file of packets, in the order
        that the files would be played back.
        """
        MB = 1024 * 1024

        # Collect summaries without logging
        summaries = self._summarize(files)

        # Per-file logging
        for summary in summaries:
            p = summary.file_path

            # Header line with size or "size: unknown"
            try:
                size_bytes = p.stat().st_size
                size_mb = size_bytes / MB
                header_line = f"---- {p.name} ({size_mb:.2f} MB) ----"
            except OSError as e:
                _logger.error(f"Failed to stat '{p}': {e}")
                header_line = f"---- {p.name} (size: unknown) ----"

            _logger.info(header_line)

            packet_count = summary.packet_count
            ts_range = summary.ts_range
            events_ts_range = summary.events_ts_range

            if packet_count == 0:
                _logger.info("  (no packets)")
            else:
                # overall packet count
                _logger.info(f"  packets: {packet_count}")

                # overall timestamp range
                first_ts, last_ts = ts_range
                _logger.info(f"    timestamps: {first_ts} .. {last_ts}")

                # events-only timestamp range
                if events_ts_range:
                    events_first_ts, events_last_ts = events_ts_range
                    _logger.info(f"    timestamps(events): {events_first_ts} .. {events_last_ts}")

                # packet counts by type (and optional per-type ranges)
                for pkt_type, count in sorted(summary.type_counts.items(), key=lambda kv: int(kv[0])):
                    _logger.info(f"  {int(pkt_type):#06x} {pkt_type.name}: {count}")

                    # optionally log per-type timestamp range, additionally indented
                    if self.SUMMARIZE_TIMESTAMP_PER_TYPE:
                        ts_range_type = summary.ts_ranges.get(pkt_type)
                        if ts_range_type is not None:
                            ts_min, ts_max = ts_range_type
                            _logger.info(f"    timestamps: {ts_min} .. {ts_max}")

            _logger.info("-" * len(header_line))

        # Suggested playback ordering based on collected summaries
        if summaries:
            header = "Suggested playback ordering:"
            _logger.info(header)
            _logger.info("-" * len(header))

            ordered = _PacketFileSummary.sorted_for_playback(summaries)
            max_name_len = max(len(s.file_path.name) for s in ordered)

            for s in ordered:
                name = s.file_path.name
                role = "events" if s.is_events_file else "metadata"
                _logger.info(f" {name:<{max_name_len}} ({role}: {s.sort_ts})")

    def _packet_file_path(self, output_path: Path, packet: Packet, sequence_number: int) -> Path:
        if self._file_output_mode is Player.FileOutputMode.SINGLE_PACKET:
            return output_path / self._packet_filename(packet, sequence_number)
        else:
            return output_path / "session.adara"

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
            except Exception as exc:
                # Best-effort cleanup: log and ignore errors while closing sockets
                _logger.debug("Ignoring exception while closing socket %r: %s", sock, exc)
        for address in addresses:
            try:
                # Remove any UDS path from filesystem
                if SocketAddress.isUDSSocket(address) and address.exists():
                    address.unlink()
            except Exception as exc:
                # Best-effort cleanup: log and ignore errors while removing UDS path
                _logger.debug("Ignoring exception while cleaning up address %r: %s", address, exc)

    def signal_handler(self, signum, frame):
        """Handle shutdown signals"""
        print(f"\nReceived signal {signum}, shutting down client session...")
        self._running = False

        # Set an alarm to force exit if cleanup takes too long.
        def _timeout_handler(signum, frame):
            print("*** Shutdown timeout exceeded, forcing exit ***")
            os._exit(1)

        if hasattr(signal, "SIGALRM"):
            # SIGALRM not available on Windows
            signal.signal(signal.SIGALRM, _timeout_handler)
            signal.alarm(10)  # 10-second timeout
