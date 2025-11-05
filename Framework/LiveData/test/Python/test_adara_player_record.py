"""
Test suite for Player.record method from adara_player module.
"""
# ruff: noqa: F841

import numpy as np
import signal
import socket
import tempfile
from pathlib import Path
from unittest.mock import patch, MagicMock, mock_open
import unittest

from adara_player import Player, Packet


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
                "adara_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
            patch("adara_player.Player._get_server_address", return_value="/tmp/sock-test"),
        ):
            player = Player()

            # Create temporary directory for output
            with tempfile.TemporaryDirectory() as tmpdir:
                output_path = Path(tmpdir) / "output"

                # Mock sockets
                mock_server_socket = MagicMock(spec=socket.socket)
                mock_client_socket = MagicMock(spec=socket.socket)
                mock_source_socket = MagicMock(spec=socket.socket)

                # Configure accept to return client socket once then timeout
                mock_server_socket.accept.return_value = (mock_client_socket, None)

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
                        patch.object(Player, "_create_server_socket", return_value=mock_server_socket),
                        patch("socket.socket", return_value=mock_source_socket),
                        patch("select.select", side_effect=stop_after_connect_select),
                    ):
                        player.record(output_path, player._source_address)

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
                "adara_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
            patch("adara_player.Player._get_server_address", return_value="/tmp/sock-test"),
        ):
            player = Player()

            # Test with None - should raise TypeError or AttributeError
            with self.assertRaises(AttributeError):
                player.record(None, player._source_address)

    def test_file_and_socket_forwarding(self):
        """Tests packet forwarding to file and socket."""
        with (
            patch(
                "adara_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
            patch("adara_player.Player._get_server_address", return_value="/tmp/sock-test"),
        ):
            player = Player()

            # Create mock packet from source
            mock_packet = MagicMock(spec=Packet)
            mock_packet.packet_type = Packet.Type.BANKED_EVENT_TYPE
            mock_packet.timestamp = np.datetime64("2024-01-01T12:00:00")
            mock_packet.size = 1024

            with tempfile.TemporaryDirectory() as tmpdir:
                output_path = Path(tmpdir) / "output"

                # Mock sockets
                mock_server_socket = MagicMock(spec=socket.socket)
                mock_client_socket = MagicMock(spec=socket.socket)
                mock_source_socket = MagicMock(spec=socket.socket)

                mock_server_socket.accept.return_value = (mock_client_socket, None)

                call_count = [0]

                def controlled_select(*args, **kwargs):
                    """Return data from source on first call, then stop."""
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
                        patch.object(Player, "_create_server_socket", return_value=mock_server_socket),
                        patch("socket.socket", return_value=mock_source_socket),
                        patch("select.select", side_effect=controlled_select),
                        patch.object(Packet, "from_socket", return_value=mock_packet),
                        patch.object(Packet, "to_socket") as mock_to_socket,
                        patch("builtins.open", mock_open()) as mock_file,
                        patch.object(Packet, "to_file") as mock_to_file,
                    ):
                        player.record(output_path, player._source_address)

                        # Verify packet was forwarded to client socket
                        mock_to_socket.assert_called_once_with(mock_client_socket, mock_packet)

                        # Verify packet was written to file
                        mock_to_file.assert_called_once()

                finally:
                    signal.alarm(0)
                    signal.signal(signal.SIGALRM, old_handler)

    def test_bidirectional_forwarding(self):
        """Checks handling of data in both directions between client and server."""
        with (
            patch(
                "adara_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
            patch("adara_player.Player._get_server_address", return_value="/tmp/sock-test"),
        ):
            player = Player()

            # Create mock packets
            server_packet = MagicMock(spec=Packet)
            server_packet.packet_type = Packet.Type.BANKED_EVENT_TYPE
            server_packet.timestamp = np.datetime64("2024-01-01T12:00:00")

            client_packet = MagicMock(spec=Packet)
            client_packet.packet_type = Packet.Type.CLIENT_HELLO_TYPE

            with tempfile.TemporaryDirectory() as tmpdir:
                output_path = Path(tmpdir) / "output"

                # Mock sockets
                mock_server_socket = MagicMock(spec=socket.socket)
                mock_client_socket = MagicMock(spec=socket.socket)
                mock_source_socket = MagicMock(spec=socket.socket)

                mock_server_socket.accept.return_value = (mock_client_socket, None)

                # FIX: Use counter-based function for multiple iterations
                call_count = [0]

                def controlled_bidirectional_select(*args, **kwargs):
                    """Return source data, then client data, then stop."""
                    call_count[0] += 1
                    if call_count[0] == 1:
                        return ([mock_source_socket], [], [])
                    elif call_count[0] == 2:
                        return ([mock_client_socket], [], [])
                    else:
                        player._running = False
                        return ([], [], [])

                packet_from_socket_calls = [server_packet, client_packet]

                # Set up timeout alarm
                old_handler = signal.signal(signal.SIGALRM, timeout_handler)
                signal.alarm(5)

                try:
                    with (
                        patch.object(Player, "_create_server_socket", return_value=mock_server_socket),
                        patch("socket.socket", return_value=mock_source_socket),
                        patch("select.select", side_effect=controlled_bidirectional_select),
                        patch.object(Packet, "from_socket", side_effect=packet_from_socket_calls),
                        patch.object(Packet, "to_socket") as mock_to_socket,
                        patch("builtins.open", mock_open()),
                        patch.object(Packet, "to_file"),
                    ):
                        player.record(output_path, player._source_address)

                        # Verify both directions were forwarded
                        self.assertEqual(mock_to_socket.call_count, 2)

                        # Verify server->client forwarding
                        mock_to_socket.assert_any_call(mock_client_socket, server_packet)

                        # Verify client->server forwarding
                        mock_to_socket.assert_any_call(mock_source_socket, client_packet)

                finally:
                    signal.alarm(0)
                    signal.signal(signal.SIGALRM, old_handler)

    def test_socket_timeout_or_error(self):
        """Confirms resilience against timeouts and errors during socket operations."""
        with (
            patch(
                "adara_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
            patch("adara_player.Player._get_server_address", return_value="/tmp/sock-test"),
        ):
            player = Player()

            with tempfile.TemporaryDirectory() as tmpdir:
                output_path = Path(tmpdir) / "output"

                # Mock sockets
                mock_server_socket = MagicMock(spec=socket.socket)
                mock_client_socket = MagicMock(spec=socket.socket)
                mock_source_socket = MagicMock(spec=socket.socket)

                mock_server_socket.accept.return_value = (mock_client_socket, None)

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
                        patch.object(Player, "_create_server_socket", return_value=mock_server_socket),
                        patch("socket.socket", return_value=mock_source_socket),
                        patch("select.select", side_effect=select_with_data),
                        patch.object(Packet, "from_socket", side_effect=socket.timeout("Read timeout")),
                        patch("adara_player._logger") as mock_logger,
                    ):
                        player.record(output_path, player._source_address)

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
                "adara_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
            patch("adara_player.Player._get_server_address", return_value="/tmp/sock-test"),
        ):
            player = Player()

            # Create control packet from client
            control_packet = MagicMock(spec=Packet)
            control_packet.packet_type = Packet.Type.CLIENT_HELLO_TYPE
            control_packet.timestamp = np.datetime64("2024-01-01T12:00:00")

            with tempfile.TemporaryDirectory() as tmpdir:
                output_path = Path(tmpdir) / "output"

                # Mock sockets
                mock_server_socket = MagicMock(spec=socket.socket)
                mock_client_socket = MagicMock(spec=socket.socket)
                mock_source_socket = MagicMock(spec=socket.socket)

                mock_server_socket.accept.return_value = (mock_client_socket, None)

                call_count = [0]

                def controlled_control_select(*args, **kwargs):
                    """Return client data on first call, then stop."""
                    call_count[0] += 1
                    if call_count[0] == 1:
                        return ([mock_client_socket], [], [])
                    else:
                        player._running = False
                        return ([], [], [])

                # Set up timeout alarm
                old_handler = signal.signal(signal.SIGALRM, timeout_handler)
                signal.alarm(5)

                try:
                    with (
                        patch.object(Player, "_create_server_socket", return_value=mock_server_socket),
                        patch("socket.socket", return_value=mock_source_socket),
                        patch("select.select", side_effect=controlled_control_select),
                        patch.object(Packet, "from_socket", return_value=control_packet),
                        patch.object(Packet, "to_socket") as mock_to_socket,
                    ):
                        player.record(output_path, player._source_address)

                        # Verify control packet was forwarded to source (not saved to file)
                        mock_to_socket.assert_called_once_with(mock_source_socket, control_packet)

                finally:
                    signal.alarm(0)
                    signal.signal(signal.SIGALRM, old_handler)

    def test_packet_file_naming(self):
        """Ensures output files are named based on packet metadata as expected."""
        with (
            patch(
                "adara_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
            patch("adara_player.Player._get_server_address", return_value="/tmp/sock-test"),
        ):
            player = Player()

            # Create mock packet with specific metadata
            mock_packet = MagicMock(spec=Packet)
            mock_packet.packet_type = Packet.Type.BANKED_EVENT_TYPE
            timestamp = np.datetime64("2024-01-15T10:30:45")
            mock_packet.timestamp = timestamp

            with tempfile.TemporaryDirectory() as tmpdir:
                output_path = Path(tmpdir) / "output"

                # Mock sockets
                mock_server_socket = MagicMock(spec=socket.socket)
                mock_client_socket = MagicMock(spec=socket.socket)
                mock_source_socket = MagicMock(spec=socket.socket)

                mock_server_socket.accept.return_value = (mock_client_socket, None)

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
                        patch.object(Player, "_create_server_socket", return_value=mock_server_socket),
                        patch("socket.socket", return_value=mock_source_socket),
                        patch("select.select", side_effect=controlled_naming_select),
                        patch.object(Packet, "from_socket", return_value=mock_packet),
                        patch.object(Packet, "to_socket"),
                        patch("builtins.open", mock_open()) as mock_file,
                        patch.object(Packet, "to_file"),
                    ):
                        player.record(output_path, player._source_address)

                        # Verify file was opened with expected name
                        expected_filename = output_path / f"{mock_packet.packet_type}-{mock_packet.timestamp}.adara"
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
                "adara_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": [], "handshake": "none", "handshake_timeout": 10.0},
                },
            ),
            patch("adara_player.Player._get_server_address", return_value="/tmp/sock-test"),
        ):
            player = Player()

            with tempfile.TemporaryDirectory() as tmpdir:
                output_path = Path(tmpdir) / "output"

                # Mock sockets
                mock_server_socket = MagicMock(spec=socket.socket)
                mock_client_socket = MagicMock(spec=socket.socket)
                mock_source_socket = MagicMock(spec=socket.socket)

                mock_server_socket.accept.return_value = (mock_client_socket, None)

                def stop_running_and_return_empty_select(*args, **kwargs):
                    """Stop the loop by setting _running to False, then return empty select result."""
                    player._running = False
                    return ([], [], [])

                # Set up timeout alarm
                old_handler = signal.signal(signal.SIGALRM, timeout_handler)
                signal.alarm(5)

                try:
                    with (
                        patch.object(Player, "_create_server_socket", return_value=mock_server_socket),
                        patch("socket.socket", return_value=mock_source_socket),
                        # Use function instead of list to avoid StopIteration issue
                        patch("select.select", side_effect=stop_running_and_return_empty_select),
                    ):
                        player.record(output_path, player._source_address)

                        # Verify all sockets were closed
                        mock_server_socket.close.assert_called()
                        mock_client_socket.close.assert_called()
                        mock_source_socket.close.assert_called()

                        # Verify _running flag is False
                        self.assertFalse(player._running)

                finally:
                    signal.alarm(0)
                    signal.signal(signal.SIGALRM, old_handler)


if __name__ == "__main__":
    unittest.main()
