"""
Test suite for Player.record method from adara_player module.
"""
# ruff: noqa: F841

import signal
import socket
import tempfile
from pathlib import Path
from unittest.mock import patch, MagicMock, mock_open
import unittest

from packet_player import Player, Packet


class TimeoutException(Exception):
    """Raised when a test times out."""

    pass


def timeout_handler(signum, frame):
    raise TimeoutException("Test timed out - likely infinite loop")


class Test_Player_record(unittest.TestCase):
    """Test cases for Player.record method."""

    def test_connects_to_source_and_target_dir(self):
        """Ensures connection to packet source and existence/creation of target output directory."""
        with (
            patch(
                "packet_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
        ):
            player = Player()

            # Create temporary directory for output
            with tempfile.TemporaryDirectory() as tmpdir:
                output_path = Path(tmpdir) / "output"

                # Mock sockets
                mock_client_socket = MagicMock(spec=socket.socket)
                mock_source_socket = MagicMock(spec=socket.socket)

                # Use `select` mock `side_effect` to additionally control the
                #   `_running` flag.
                def stop_after_connect_select(*args, **kwargs):
                    """Stop loop after connection established."""
                    player._running = False
                    return ([], [], [])

                # Set up timeout alarm to prevent hanging
                old_handler = signal.signal(signal.SIGALRM, timeout_handler)
                signal.alarm(5)

                try:
                    with (
                        patch("socket.socket", return_value=mock_source_socket),
                        patch("select.select", side_effect=stop_after_connect_select),
                    ):
                        player.record(output_path, mock_client_socket)

                        # Verify directory was created
                        self.assertTrue(output_path.exists())
                        self.assertTrue(output_path.is_dir())

                        # Verify connection to source was attempted
                        mock_source_socket.connect.assert_called_once_with(player._source_address)

                finally:
                    signal.alarm(0)
                    signal.signal(signal.SIGALRM, old_handler)

    def test_missing_target_dir_error(self):
        """Verifies error if no output directory is provided."""
        with (
            patch(
                "packet_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
        ):
            player = Player()

            # Mock sockets
            mock_client_socket = MagicMock(spec=socket.socket)

            # Test with None - should raise TypeError or AttributeError
            with self.assertRaises(AttributeError):
                player.record(None, mock_client_socket)

    def test_file_and_socket_forwarding(self):
        """Tests packet forwarding to file and socket."""
        with (
            patch(
                "packet_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
        ):
            player = Player()

            # Create mock packet from source
            payload = b"\x01\x02\x03\x04\x05\x06\x07\x08"
            mock_packet = Packet(
                header=Packet.create_header(
                    payload_len=len(payload), packet_type=Packet.Type.BANKED_EVENT_TYPE, tv_sec=12345, tv_nsec=67890
                ),
                payload=payload,
            )

            with tempfile.TemporaryDirectory() as tmpdir:
                output_path = Path(tmpdir) / "output"

                # Mock sockets
                mock_client_socket = MagicMock(spec=socket.socket)
                mock_source_socket = MagicMock(spec=socket.socket)

                call_count = [0]

                def controlled_select(*args, **kwargs):
                    """Return data from source on first call, the client writable, then stop."""
                    call_count[0] += 1

                    match call_count[0]:
                        case 1:
                            return ([mock_source_socket], [], [])
                        case 2:
                            return ([], [mock_client_socket], [])
                        case _:
                            player._running = False
                            return ([], [], [])

                # Set up timeout alarm
                old_handler = signal.signal(signal.SIGALRM, timeout_handler)
                signal.alarm(5)

                try:
                    with (
                        patch("socket.socket", return_value=mock_source_socket),
                        patch("select.select", side_effect=controlled_select),
                        patch.object(Packet, "from_socket", return_value=mock_packet),
                        patch.object(mock_client_socket, "send") as mock_client_send,
                        patch("builtins.open", mock_open()) as mock_file,
                        patch.object(Packet, "to_file") as mock_to_file,
                    ):
                        mock_client_send.return_value = mock_packet.size

                        player.record(output_path, mock_client_socket)

                        # Verify packet was forwarded to client socket
                        mock_client_send.assert_called_once_with(mock_packet.header + mock_packet.payload)

                        # Verify packet was written to file
                        mock_to_file.assert_called_once()

                finally:
                    signal.alarm(0)
                    signal.signal(signal.SIGALRM, old_handler)

    def test_bidirectional_forwarding(self):
        """Checks handling of data in both directions between client and server."""
        with (
            patch(
                "packet_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
        ):
            player = Player()

            # Create mock packets
            payload = b"\x01\x02\x03\x04\x05\x06\x07\x08"
            source_packet = Packet(
                header=Packet.create_header(
                    payload_len=len(payload), packet_type=Packet.Type.BANKED_EVENT_TYPE, tv_sec=12345, tv_nsec=67890
                ),
                payload=payload,
            )

            payload = b"\x11\x22\x33\x44"
            client_packet = Packet(
                header=Packet.create_header(
                    payload_len=len(payload), packet_type=Packet.Type.CLIENT_HELLO_TYPE, tv_sec=12345, tv_nsec=78900
                ),
                payload=payload,
            )

            with tempfile.TemporaryDirectory() as tmpdir:
                output_path = Path(tmpdir) / "output"

                # Mock sockets
                mock_client_socket = MagicMock(spec=socket.socket)
                mock_source_socket = MagicMock(spec=socket.socket)

                # Use counter-based function for multiple iterations
                call_count = [0]

                def controlled_bidirectional_select(*args, **kwargs):
                    """Return client data, then source data, then stop."""
                    call_count[0] += 1
                    match call_count[0]:
                        case 1:
                            return ([mock_client_socket], [], [])
                        case 2:
                            return ([], [mock_source_socket], [])
                        case 3:
                            return ([mock_source_socket], [], [])
                        case 4:
                            return ([], [mock_client_socket], [])
                        case _:
                            player._running = False
                            return ([], [], [])

                packet_from_socket_calls = [client_packet, source_packet]

                # Set up timeout alarm
                old_handler = signal.signal(signal.SIGALRM, timeout_handler)
                signal.alarm(5)

                try:
                    with (
                        patch("socket.socket", return_value=mock_source_socket),
                        patch("select.select", side_effect=controlled_bidirectional_select),
                        patch.object(Packet, "from_socket", side_effect=packet_from_socket_calls),
                        patch.object(player, "_impose_transfer_limit"),
                        patch.object(mock_source_socket, "send") as mock_source_send,
                        patch.object(mock_client_socket, "send") as mock_client_send,
                        patch("builtins.open", mock_open()),
                        patch.object(Packet, "to_file"),
                    ):
                        mock_source_send.return_value = client_packet.size
                        mock_client_send.return_value = source_packet.size
                        player.record(output_path, mock_client_socket)

                        # Verify client->server forwarding
                        mock_source_send.assert_called_once_with(client_packet.header + client_packet.payload)

                        # Verify server->client forwarding
                        mock_client_send.assert_called_once_with(source_packet.header + source_packet.payload)

                finally:
                    signal.alarm(0)
                    signal.signal(signal.SIGALRM, old_handler)

    def test_socket_timeout_or_error(self):
        """Confirms resilience against timeouts and errors during socket operations."""
        with (
            patch(
                "packet_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
        ):
            player = Player()

            with tempfile.TemporaryDirectory() as tmpdir:
                output_path = Path(tmpdir) / "output"

                # Mock sockets
                mock_client_socket = MagicMock(spec=socket.socket)
                mock_source_socket = MagicMock(spec=socket.socket)

                call_count = [0]

                def select_with_data(*args, **kwargs):
                    """Return data available, which will trigger socket.timeout."""
                    call_count[0] += 1
                    if call_count[0] == 1:
                        return ([mock_source_socket], [], [])
                    else:
                        player._running = False
                        return ([], [], [])

                # Set up timeout alarm
                old_handler = signal.signal(signal.SIGALRM, timeout_handler)
                signal.alarm(5)

                try:
                    with (
                        patch("socket.socket", return_value=mock_source_socket),
                        patch("select.select", side_effect=select_with_data),
                        patch.object(Packet, "from_socket", side_effect=socket.timeout("Read timeout")),
                        patch("packet_player._logger") as mock_logger,
                    ):
                        player.record(output_path, mock_client_socket)

                        # Verify error was logged
                        error_logged = any(
                            "Socket error" in str(call_args) or "error" in str(call_args).lower()
                            for call_args in mock_logger.error.call_args_list
                        )
                        self.assertTrue(error_logged)

                finally:
                    signal.alarm(0)
                    signal.signal(signal.SIGALRM, old_handler)

    def test_control_packet_forwarding(self):
        """Tests that control packets are forwarded appropriately."""
        with (
            patch(
                "packet_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
        ):
            player = Player()

            # Create control packet from client
            payload = b"\x11\x22\x33\x44"
            control_packet = Packet(
                header=Packet.create_header(
                    payload_len=len(payload), packet_type=Packet.Type.CLIENT_HELLO_TYPE, tv_sec=12345, tv_nsec=78900
                ),
                payload=payload,
            )

            with tempfile.TemporaryDirectory() as tmpdir:
                output_path = Path(tmpdir) / "output"

                # Mock sockets
                mock_client_socket = MagicMock(spec=socket.socket)
                mock_source_socket = MagicMock(spec=socket.socket)

                call_count = [0]

                def controlled_control_select(*args, **kwargs):
                    """Return client data on first call, then source writable, then stop."""
                    call_count[0] += 1
                    match call_count[0]:
                        case 1:
                            return ([mock_client_socket], [], [])
                        case 2:
                            return ([], [mock_source_socket], [])
                        case _:
                            player._running = False
                            return ([], [], [])

                # Set up timeout alarm
                old_handler = signal.signal(signal.SIGALRM, timeout_handler)
                signal.alarm(5)

                try:
                    with (
                        patch("socket.socket", return_value=mock_source_socket),
                        patch("select.select", side_effect=controlled_control_select),
                        patch.object(Packet, "from_socket", return_value=control_packet),
                        patch.object(player, "_impose_transfer_limit"),
                        patch.object(mock_source_socket, "send") as mock_source_send,
                    ):
                        mock_source_send.return_value = control_packet.size

                        player.record(output_path, mock_client_socket)

                        # Verify control packet was forwarded to source (not saved to file)
                        mock_source_send.assert_called_once_with(control_packet.header + control_packet.payload)

                finally:
                    signal.alarm(0)
                    signal.signal(signal.SIGALRM, old_handler)

    def test_packet_file_naming(self):
        """Ensures output files are named based on packet metadata as expected."""
        with (
            patch(
                "packet_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
        ):
            player = Player()

            # Create mock packet with specific metadata
            payload = b"\x01\x02\x03\x04\x05\x06\x07\x08"
            mock_packet = Packet(
                header=Packet.create_header(
                    payload_len=len(payload), packet_type=Packet.Type.BANKED_EVENT_TYPE, tv_sec=12345, tv_nsec=67890
                ),
                payload=payload,
            )

            with tempfile.TemporaryDirectory() as tmpdir:
                output_path = Path(tmpdir) / "output"

                # Mock sockets
                mock_client_socket = MagicMock(spec=socket.socket)
                mock_source_socket = MagicMock(spec=socket.socket)

                call_count = [0]

                def controlled_naming_select(*args, **kwargs):
                    """Return source data on first call, then stop."""
                    call_count[0] += 1
                    if call_count[0] == 1:
                        return ([mock_source_socket], [], [])
                    else:
                        player._running = False
                        return ([], [], [])

                # Set up timeout alarm
                old_handler = signal.signal(signal.SIGALRM, timeout_handler)
                signal.alarm(5)

                try:
                    with (
                        patch("socket.socket", return_value=mock_source_socket),
                        patch("select.select", side_effect=controlled_naming_select),
                        patch.object(Packet, "from_socket", return_value=mock_packet),
                        patch.object(player, "_impose_transfer_limit"),
                        patch.object(mock_client_socket, "send") as mock_client_send,
                        patch("builtins.open", mock_open()) as mock_file,
                        patch.object(Packet, "to_file"),
                    ):
                        mock_client_send.return_value = mock_packet.size
                        player.record(output_path, mock_client_socket)

                        # Verify file was opened with expected name
                        expected_filename = output_path / f"{mock_packet.packet_type:#04x}-{mock_packet.timestamp}-000001.adara"
                        mock_file.assert_called()

                        # Check that the file path matches expected pattern
                        call_args = mock_file.call_args_list[0]
                        opened_path = Path(call_args[0][0])
                        self.assertEqual(opened_path.name, expected_filename.name)

                finally:
                    signal.alarm(0)
                    signal.signal(signal.SIGALRM, old_handler)

    def test_cleanup_sockets_post_record(self):
        """Verifies all sockets are closed and cleaned up after recording finishes."""
        with (
            patch(
                "packet_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
        ):
            player = Player()

            with tempfile.TemporaryDirectory() as tmpdir:
                output_path = Path(tmpdir) / "output"

                # Mock sockets
                mock_client_socket = MagicMock(spec=socket.socket)
                mock_source_socket = MagicMock(spec=socket.socket)

                def stop_running_and_return_empty_select(*args, **kwargs):
                    """Stop the loop by setting _running to False, then return empty select result."""
                    player._running = False
                    return ([], [], [])

                # Set up timeout alarm
                old_handler = signal.signal(signal.SIGALRM, timeout_handler)
                signal.alarm(5)

                try:
                    with (
                        patch("socket.socket", return_value=mock_source_socket),
                        # Use function instead of list to avoid StopIteration issue
                        patch("select.select", side_effect=stop_running_and_return_empty_select),
                    ):
                        player.record(output_path, mock_client_socket)

                        # Verify all sockets were closed
                        mock_client_socket.close.assert_called()
                        mock_source_socket.close.assert_called()

                        # Verify _running flag is False
                        self.assertFalse(player._running)

                finally:
                    signal.alarm(0)
                    signal.signal(signal.SIGALRM, old_handler)


if __name__ == "__main__":
    unittest.main()
