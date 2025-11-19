import os
import socketserver
import signal
import threading

# `Player.play(<client socket>, <single-session glob path>, <single-session glob patterns>)`

# `Player.record(<single-session path>, <client socket>)`

class SessionServer:
    # Mixin class to control commandline argument and session-specific handling.
    def __init__(
        self, *,
        # kwargs only:
        args: argparse.Namespace,
        manager: multiprocessing.Manager
    ):
        super().__init__()
        
        # Validate commandline args
        self._record = args.record
        self._source_address = args.source_address    
        if self._record:
            _logger.debug(f"  source: '{self._source_address}'")
        self._dry_run = args.dry_run

        # Parse Unix-style glob to "internal" format: (<base-directory path>, list[<glob expr>]).
        base_path, patterns = UnixGlob.parse(
            args.glob
        )
        self._glob = base_path, patterns

        if self._record:
            if self.patterns and self.patterns[0]:
                raise RuntimeError(f"When using record mode, the positional argument should be the target directory, not '{args.glob}'.")
            if base_path.exists() and any(base_path.iterdir()):
                raise RuntimeError(f"in record mode: base directory '{base_path}' should be empty")
            if args.dry_run:
                raise RuntimeError("Dry run cannot be used with record mode.")

        # Initialize the shared IPC-value for the current session number:
        #   each session corresponds to a client connection to the server
        self._session_number = manager.Value('i', 1)

    @staticmethod
    def parse_args() -> argparse.Namespace:
        # Parse commandline arguments:
        #   allow overriding the most-often-used configuration args.

        parser = argparse.ArgumentParser(
            description="ADARA packet player.", usage="%(prog)s [-h] [-r] [-s SOURCE_ADDRESS] [-a SERVER_ADDRESS] [-d] 'glob'"
        )
        parser.add_argument("-r", "--record", action="store_true", help="Enable record mode")
        parser.add_argument("-s", "--source_address", type=str, help="Specify packet source address: used for record")
        parser.add_argument("-a", "--server_address", type=str, help="Specify server address")
        parser.add_argument("-d", "--dry_run", action="store_true", help="Dry run: used for play mode")
    
        # Default config-file location is overridden using the environment, NOT here:
        # example override: `adara_player_conf=<new config-file location> adara_player <options> <glob>`

        parser.add_argument("glob", help="Standard Unix glob spec (in single quotes), or an output-directory path (in record mode)")
        args = parser.parse_args()
        return args

    @staticmethod
    def serverIsUDSSocket(args: argeparse.Namespace) -> bool:
        # Before we can create the server, we need to know whether or not it serves from a UDS or INET socket.  How stupid!  :(
        return SocketAddress.parse(args.server_address if args.server_address else Player._get_server_address()).isUDSSocket()
         
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
    def glob(self) -> Path, list[str]:
        return self._glob
        
    @property
    def base_path(self) -> Path:
        return self._glob[0]

    @property
    def is_multi_session(self) -> bool:
        # glob includes only the base_path => multi-session
        return self._glob[1] == [""]

    @property
    def session_number(self) -> int:
        return self._session_number.value
                   
    def next_session_path(self) -> Path:
        if not self.is_multi_session:
            # in play mode: a complete glob string (i.e. not a base directory) implies only one session is expected
            raise RuntimeError("multiple client connections when only a single session is expected")
            
        with self._session_number.get_lock():
            session_path = self.base_path / f"{self.session_number.value:04d}")
            self._session_number.value += 1
        if self.record and session_path.exists() and any(session_path.iterdir()):
            raise RuntimeError(f"in record mode: session directory '{session_path}' should be empty")
        elif not session_path.exists():       
            raise RuntimeError(f"in play mode: required session directory '{session_path}' does not exist")
        return session_path

    def signal_handler(self, signum, frame):
        # handler for SIGINT, SIGTERM
        
        # propagate the signal to the entire process group
        #   (e.g. child processes handling each session)
        pgid = os.getpgid(os.getpid())
        os.killpg(pgid, signum)
        # shutdown the server
        shutdown_thread = threading.Thread(target=self.shutdown)
        shutdown_thread.start()

        # Set an alarm to force exit if cleanup takes too long.
        def _timeout_handler(signum, frame):
            _logger.error("Shutdown timeout exceeded, forcing exit")
            os._exit(1)

        signal.signal(signal.SIGALRM, _timeout_handler)
        signal.alarm(20)  # 20-second timeout

        
class SessionHandler(socketserver.BaseRequestHandler):
    def handle(self):
        player = Player(source_address=self.server.source_address)

        # Set up signal handlers for this child process
        signal.signal(signal.SIGINT, player.signal_handler)
        signal.signal(signal.SIGTERM, player.signal_handler)
        
        _console_logger.info(f"Accepted client connection for session {self.server.session_number}.")

        if not self.server.record:
            # `play` takes the client socket,
            #    and then a parsed Unix glob as (<base path>, <patterns>)
            
            path, patterns = self.server.glob
            if self.server.is_multi_session:
                path = self.server.next_session_path()
                patterns = [Config["playback"]["packet_glob"]]
            player.play(self.request, path, patterns, dry_run=self.server.dry_run)
        else:
            # `record` is always multi-session,
            #   taking the output directory <path> and then the client socket.
            path = self.server.next_session_path()
            player.record(path, self.request)


class SessionTCPServer(SessionServer, socketserver.ForkingMixIn, socketserver.TCPServer):
    def __init__(
        self,
        server_address: Path | tuple[str, int],
        args: argparse.Namespace,
        manager: multiprocessing.Manager
    ):
        super().__init__(server_address, SessionHandler, args=args, manager=manager)

class SessionUDSServer(SessionServer, socketserver.ForkingMixIn, socketserver.UnixStreamServer):
    def __init__(
        self,
        server_address: Path | tuple[str, int],
        args: argparse.Namespace,
        manager: multiprocessing.Manager
    ):
        super().__init__(str(server_address), SessionHandler, args=args, manager=manager)


if __name__ == "__main__":
    args = SessionServer.parse_args()
    server_address = SocketAddress.parse(
        args.server_address if args.server_address else Player._get_server_address()
    )
    manager = multiprocessing.Manager()
    
    if SocketAddress.isUDSSocket(server_address):
        with SessionUDSServer(server_address, commandline_args=args, manager=manager) as server:
            # Set up signal handlers
            signal.signal(signal.SIGINT, server.signal_handler)
            signal.signal(signal.SIGTERM, server.signal_handler)
            _console_logger.info(f"Waiting for client connection at {server_address}...")
            _console_logger.info("Type CNTL-C to exit.")
            server.serve_forever()
    else:
        with SessionTCPServer(server_address, commandline_args=args, manager=manager) as server:
            signal.signal(signal.SIGINT, server.signal_handler)
            signal.signal(signal.SIGTERM, server.signal_handler)
            _console_logger.info(f"Waiting for client connection at {server_address}...")
            _console_logger.info("Type CNTL-C to exit.")
            server.serve_forever()
