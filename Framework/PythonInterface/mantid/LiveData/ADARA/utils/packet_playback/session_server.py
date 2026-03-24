import argparse
from itertools import islice
import logging
import multiprocessing
from pathlib import Path
import psutil
import os
import re
import socketserver
import signal
import sys
import threading
from typing import Iterator

import player_config
from player_config import yaml_load
import packet_player
from packet_player import Player, SocketAddress, UnixGlob

##
## ADARA-packet player is not implemented on Windows, due to lack of `fork` system call.
##
if sys.platform.startswith("win"):
    raise SystemExit("ADARA-packet player is not implemented for the Windows OS.")
##


# ---------------------------------------------------------------------------
# Module-level logger -- unified with the rest of the feature.
#   Handlers are attached in main(), after config has been loaded.
# ---------------------------------------------------------------------------
_logger = logging.getLogger(__name__)


class SessionServer:
    # Mixin class to control commandline argument and session-specific handling.
    def __init__(self, *args, commandline_args: argparse.Namespace, **kwargs):
        # do not forward `**kwargs`: `super()` may just be `object`
        super().__init__(*args)

        # Basic flags from CLI
        self._record = commandline_args.record
        self._source_address = commandline_args.source_address
        if self._record and self._source_address:
            _logger.debug(f"source: '{self._source_address}'")

        self._dry_run = commandline_args.dry_run
        self._summarize = commandline_args.summarize

        # Parsed glob, precomputed sessions, yaml_input_mode (filled by _parse_file_args)
        self._glob: tuple[Path, list[str]] | None = None
        self._sessions: Iterator[Iterator[Path]] | None = None
        self._yaml_input_mode: bool = False

        # Fill _glob and _sessions according to CLI
        SessionServer._parse_file_args(self, commandline_args)

        # Record-mode validation still uses _glob
        if self._record:
            base_path, patterns = self._glob
            if patterns and patterns[0]:
                raise RuntimeError(
                    f"When using record mode, the positional argument should be the target directory, not '{commandline_args.glob}'."
                )
            if base_path.exists():
                if any(p.is_dir() and re.fullmatch(r"\d{4}", p.name) for p in base_path.iterdir()):
                    raise RuntimeError(f"in record mode: session directory '{base_path}' must not contain existing sessions")
            if self._dry_run:
                raise RuntimeError("`dry_run` flag cannot be used with record mode.")
            if self._summarize:
                raise RuntimeError("`summarize` mode cannot be used with `record` mode.")

        if self._summarize and self._dry_run:
            raise RuntimeError("`dry_run` flag cannot be used with `summarize` mode.")

        # Initialize the shared IPC-value for the current session number:
        #   each session corresponds to a client connection to the server
        self._session_number = multiprocessing.Value("i", 1)
        self._parent_PID = os.getpid()

    @staticmethod
    def parse_args() -> argparse.Namespace:
        # Parse commandline arguments:
        #   allow overriding the most often used configuration args.

        parser = argparse.ArgumentParser(
            description="ADARA packet player.",
            usage="%(prog)s [-h] -c CONFIG [-r] [-s SOURCE_ADDRESS] [-a SERVER_ADDRESS] [-d] [-z] [-f FILES] [glob]",
        )
        parser.add_argument(
            "-c",
            "--config",
            type=str,
            required=True,
            help="Path to the ADARA player configuration YAML file (required)",
        )
        parser.add_argument("-r", "--record", action="store_true", help="Enable record mode")
        parser.add_argument("-s", "--source_address", type=str, help="Specify packet source address: used for record")
        parser.add_argument("-a", "--server_address", type=str, help="Specify server address")
        parser.add_argument("-d", "--dry_run", action="store_true", help="Dry run: used for play mode")
        parser.add_argument(
            "-z",
            "--summarize",
            action="store_true",
            help="Summarize packet files instead of streaming or recording",
        )
        parser.add_argument(
            "-f",
            "--files",
            type=str,
            help="YAML file with a list of packet files for each expected session for play/summarize modes",
        )
        parser.add_argument(
            "glob", nargs="?", help="Unix glob spec (in single quotes) for packet files, or an output-directory path in record mode"
        )
        args = parser.parse_args()

        return args

    @staticmethod
    def _parse_file_args(server: "SessionServer", args: argparse.Namespace) -> None:
        """
        Initialize `server._glob` and `server._sessions` from CLI args.

        Semantics:
        - Record mode:
            * Ignore --files (disallowed by validation in __init__).
            * Parse glob as usual to (_glob = (base_path, patterns)).
            * _sessions remains None (record uses output directories, not input files).
        - Play / summarize:
            * If --files is given:
                  YAML:
                    base_path: <base directory>
                    sessions:
                      - [file1, file2, ...]  # session 1
                      - [file3, file4, ...]  # session 2, ...
              - glob arg is ignored for file resolution.
              - _glob is set to (base_path, [""]) as a convenient carrier of base_path.
              - _sessions is an iterator[iterator[Path]], one inner iterator per YAML session.
            * If --files is NOT given:
              - Use UnixGlob.parse(glob) -> (base_path, patterns).
              - If patterns == [""]: multi-session root; each session is a subdir base_path/0001, 0002, ...
                    _sessions: one iterator per session dir, using PACKET_GLOB inside each.
              - Else: single logical session; _sessions has a single iterator over UnixGlob.multi_glob(base_path, patterns).
        """

        # IMPORTANT: `server._sessions` is an iterator-of-iterators (Iterator[Iterator[Path]]).
        # It MUST NOT be consumed in the parent process. Each forked child uses
        # `server.session_files` to recompute its session Iterator[Path] via `islice`,
        # using THE IPC-shared `_session_number` to pick its logical session index.

        # 1. Record mode: only glob matters here, sessions unused.
        if args.record:
            if getattr(args, "files", None):
                raise RuntimeError("`--files` cannot be used with record mode.")
            if not getattr(args, "glob", None):
                raise RuntimeError("record mode requires a target directory (glob positional argument).")
            base_path, patterns = UnixGlob.parse(args.glob)
            server._glob = (base_path, patterns)
            server._sessions = None
            return

        # 2. Play / summarize with --files: YAML dominates glob
        files_arg = getattr(args, "files", None)
        if getattr(args, "glob", None) and files_arg:
            raise RuntimeError("Usage error: only one of FILES or glob should be specified.")
        if files_arg:
            server._yaml_input_mode = True

            list_path = Path(files_arg)
            data = yaml_load(list_path)

            if not isinstance(data, dict):
                raise RuntimeError(f"`--files` YAML must be a mapping with keys 'base_path' and 'sessions', got {type(data)}")

            try:
                base_path_str = data["base_path"]
                sessions_data = data["sessions"]
            except KeyError as e:
                raise RuntimeError(f"`--files` YAML missing required key: {e!s}")

            if not isinstance(base_path_str, str):
                raise RuntimeError(f"`base_path` in `--files` YAML must be a string, got {type(base_path_str)}")
            base_path = Path(base_path_str)

            if not isinstance(sessions_data, list) or not sessions_data:
                raise RuntimeError("`sessions` in `--files` YAML must be a non-empty list of session file-lists")

            # For downstream code that still wants a "base glob", encode this as base-only.
            server._glob = (base_path, [""])

            # Build iterator-of-iterators: one inner iterator per session list.
            def _session_iter_factory(session_entries: list[str]) -> Iterator[Path]:
                for entry in session_entries:
                    p = Path(entry)
                    if p.is_absolute():
                        yield p
                    else:
                        yield base_path / p

            server._sessions = (_session_iter_factory(session_list) for session_list in sessions_data if isinstance(session_list, list))
            return

        # 3. Play / summarize without --files: glob determines sessions and files
        if not getattr(args, "glob", None):
            raise RuntimeError("glob argument is required when `--files` is not specified.")

        server._yaml_input_mode = False
        base_path, patterns = UnixGlob.parse(args.glob)
        server._glob = (base_path, patterns)

        # Directory-only glob => multi-session root: base_path/0001, base_path/0002, ...
        if patterns == [""]:
            session_dirs = sorted(p for p in base_path.iterdir() if p.is_dir() and re.fullmatch(r"\d{4}", p.name))

            # If no session directories exist, leave _sessions as an empty iterator.
            def _session_iter_for_dir(sp: Path) -> Iterator[Path]:
                return UnixGlob.multi_glob(sp, [Player.PACKET_GLOB])

            server._sessions = (_session_iter_for_dir(sp) for sp in session_dirs)
        else:
            # Single logical session: one iterator over all matching files.
            def _single_session_iter() -> Iterator[Path]:
                return UnixGlob.multi_glob(base_path, patterns)

            server._sessions = iter([_single_session_iter()])

    @property
    def record(self) -> bool:
        return self._record

    @property
    def source_address(self) -> Path | tuple[str, int]:
        return self._source_address

    @property
    def dry_run(self) -> bool:
        return self._dry_run

    @property
    def summarize(self) -> bool:
        return self._summarize

    @property
    def glob(self) -> tuple[Path, list[str]] | None:
        return self._glob

    @property
    def yaml_input_mode(self) -> bool:
        return self._yaml_input_mode

    @property
    def is_parent(self) -> bool:
        # return `True` if this is the parent process
        return os.getpid() == self._parent_PID

    @property
    def sessions(self) -> Iterator[Iterator[Path]] | None:
        if self.is_parent and not self.summarize:
            # 'summarize' mode does not fork
            raise RuntimeError("Usage error: `sessions` iterator should not be modified by the parent process.")
        return self._sessions

    @property
    def base_path(self) -> Path:
        return self._glob[0]

    @property
    def is_multi_session(self) -> bool:
        # glob includes only the base_path => multi-session
        return self._glob[1] == [""]

    def _claim_session_index(self) -> int:
        with self._session_number.get_lock():
            idx = self._session_number.value
            self._session_number.value += 1
        return idx

    def _session_files_for(self, idx: int) -> Iterator[Path]:
        if self.record:
            raise RuntimeError("Usage error: in 'record' mode, `session_files` is not used.")
        if self._sessions is None:
            raise RuntimeError("no sessions defined for play/summarize")
        try:
            return next(islice(self._sessions, idx - 1, idx))
        except StopIteration:
            raise RuntimeError(f"session index {idx} is out of range")

    def _session_path_for(self, idx: int) -> Path:
        if not self.record:
            raise RuntimeError("Usage error: `session_path` is only used in 'record' mode.")
        session_path = self.base_path / f"{idx:04d}"
        if session_path.exists() and any(session_path.iterdir()):
            raise RuntimeError(f"in record mode: session directory '{session_path}' is not empty")
        return session_path

    # Override: `ForkingMixIn` method.
    def process_request(self, request, client_address):
        _logger.info(f"Accepting client connection for session {self._session_number.value}.")
        super().process_request(request, client_address)

    def signal_handler(self, signum, frame):
        # handler for SIGINT, SIGTERM
        print(f"\nReceived signal {signum}, shutting down server...")

        pid = os.getpid()
        pgid = os.getpgid(os.getpid())

        # propagate the signal to the rest of the process group
        #   (e.g. child processes handling each session)
        for proc in psutil.process_iter(attrs=["pid"]):
            try:
                if os.getpgid(proc.pid) == pgid and proc.pid != pid:
                    os.kill(proc.pid, signum)
            except (psutil.NoSuchProcess, PermissionError):
                continue

        # shutdown the server
        shutdown_thread = threading.Thread(target=self.shutdown)
        shutdown_thread.start()

        # Set an alarm to force exit if cleanup takes too long.
        def _timeout_handler(signum, frame):
            print("*** Shutdown timeout exceeded, forcing exit ***")
            os._exit(1)

        if hasattr(signal, "SIGALRM"):
            # SIGALRM not available on Windows
            signal.signal(signal.SIGALRM, _timeout_handler)
            signal.alarm(20)  # 20-second timeout


class SessionHandler(socketserver.BaseRequestHandler):
    def handle(self):
        # Ensure per-process logging uses this child PID
        Player.init_process_logger()
        # Also configure *this* module's logger for the forked child
        player_config.init_process_logger(_logger)

        player = Player(
            source_address=self.server.source_address,
            # `yaml_input_mode` overrides packet-ordering from config
            packet_ordering_scheme="yaml" if self.server.yaml_input_mode else None,
        )

        # Set up signal handlers for this child process
        signal.signal(signal.SIGINT, player.signal_handler)
        signal.signal(signal.SIGTERM, player.signal_handler)

        # Note that 'summarize' mode does not use `handle`.

        try:
            if not self.server.record:
                # Each client gets one logical session's files.
                session_idx = self.server._claim_session_index()
                session_files = self.server._session_files_for(session_idx)
                player.play(self.request, session_files, dry_run=self.server.dry_run)
            else:
                session_idx = self.server._claim_session_index()
                path = self.server._session_path_for(session_idx)
                player.record(path, self.request)
        except RuntimeError as e:
            _logger.error(str(e))
            try:
                self.request.close()
            finally:
                return


class SessionTCPServer(SessionServer, socketserver.ForkingMixIn, socketserver.TCPServer):
    def __init__(self, server_address: tuple[str, int], commandline_args: argparse.Namespace):
        super().__init__(server_address, SessionHandler, commandline_args=commandline_args)


class SessionUDSServer(SessionServer, socketserver.ForkingMixIn, socketserver.UnixStreamServer):
    def __init__(self, server_address: Path, commandline_args: argparse.Namespace):
        super().__init__(str(server_address), SessionHandler, commandline_args=commandline_args)


def main():
    # Application `main` for commandline `adara_player`.
    args = SessionServer.parse_args()

    # 1. Load configuration from the mandatory --config path.
    player_config.load_config(args.config)

    # 2. Populate class-level constants from config.
    packet_player.initialize()

    # 3. Configure this module's logger (console + file).
    player_config.configure_logger(_logger, pid=os.getpid(), console=True)

    # Summarize mode: no server or client connection.
    if getattr(args, "summarize", False):
        # Reuse `SessionServer`'s parsing / validation logic, but without starting any server.
        server = SessionServer(commandline_args=args)

        # Construct player
        player = Player(
            # `yaml_input_mode` overrides packet-ordering from config
            packet_ordering_scheme="yaml" if server.yaml_input_mode else None
        )

        sessions = server.sessions
        if sessions is None:
            _logger.warning("No sessions/files found to summarize.")
        else:
            _logger.info("Summarizing content of ADARA-packet files ...")
            for idx, session_files in enumerate(sessions, start=1):
                _logger.info(f"Session {idx:04d}:")
                player.summarize(session_files)

        _logger.info(f"Summary available at:\n    '{player_config.get_log_file_path_for_PID(os.getpid())}'.")
        return  # Exit without waiting for any client

    server_address = SocketAddress.parse(args.server_address or Player.get_server_address())
    if SocketAddress.isUDSSocket(server_address) and server_address.exists():
        server_address.unlink()

    if SocketAddress.isUDSSocket(server_address):
        with SessionUDSServer(server_address, commandline_args=args) as server:
            # Set up signal handlers
            signal.signal(signal.SIGINT, server.signal_handler)
            signal.signal(signal.SIGTERM, server.signal_handler)
            _logger.info(f"Waiting for client connection at {server_address}...")
            _logger.info("Type CNTL-C to exit.")
            server.serve_forever()
    else:
        with SessionTCPServer(server_address, commandline_args=args) as server:
            signal.signal(signal.SIGINT, server.signal_handler)
            signal.signal(signal.SIGTERM, server.signal_handler)
            _logger.info(f"Waiting for client connection at {server_address}...")
            _logger.info("Type CNTL-C to exit.")
            server.serve_forever()
