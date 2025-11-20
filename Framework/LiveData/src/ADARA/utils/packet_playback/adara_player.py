import multiprocessing
import signal

from packet_player import Player, SocketAddress
from session_server import console_logger, SessionServer, SessionTCPServer, SessionUDSServer


if __name__ == "__main__":
    args = SessionServer.parse_args()
    server_address = SocketAddress.parse(args.server_address if args.server_address else Player.get_server_address())
    manager = multiprocessing.Manager()

    if SocketAddress.isUDSSocket(server_address):
        with SessionUDSServer(server_address, commandline_args=args, manager=manager) as server:
            # Set up signal handlers
            signal.signal(signal.SIGINT, server.signal_handler)
            signal.signal(signal.SIGTERM, server.signal_handler)
            console_logger.info(f"Waiting for client connection at {server_address}...")
            console_logger.info("Type CNTL-C to exit.")
            server.serve_forever()
    else:
        with SessionTCPServer(server_address, commandline_args=args, manager=manager) as server:
            signal.signal(signal.SIGINT, server.signal_handler)
            signal.signal(signal.SIGTERM, server.signal_handler)
            console_logger.info(f"Waiting for client connection at {server_address}...")
            console_logger.info("Type CNTL-C to exit.")
            server.serve_forever()
