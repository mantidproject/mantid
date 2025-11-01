"""
Test suite for Player.record method from adara_player module.
"""
# ruff: noqa: F841

import signal
import socket
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import patch, MagicMock, mock_open
import unittest

import player_config
from packet_player import Player, Packet
from adara_player_test_helpers import apply_test_config


class TimeoutException(Exception):
    """Raised when a test times out."""

    pass


def timeout_handler(signum, frame):
    raise TimeoutException("Test timed out - likely infinite loop")


class Test_Player_record(unittest.TestCase):
    """Test cases for Player.record method."""

    def setUp(self):
        super().setUp()
        self.temp_dir = self.enterContext(TemporaryDirectory(prefix=f"{__name__}_"))
        apply_test_config()

    def tearDown(self):
        player_config.reset()
        super().tearDown()

    def test_connects_to_source_and_target_dir(self):
        """Ensures connection to packet source and existence/creation of target output directory."""
        player = Player()

        # Create temporary directory for output
        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
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
        player = Player()

        # Mock sockets
        mock_client_socket = MagicMock(spec=socket.socket)

        # Test with None - should raise TypeError or AttributeError
        with self.assertRaises(AttributeError):
            player.record(None, mock_client_socket)

    def test_file_and_socket_forwarding(self):
        """Tests packet forwarding to file and socket."""
        player = Player()

        # Create mock packet from source
        payload = b"\x01\x02\x03\x04\x05\x06\x07\x08"
        mock_packet = Packet(
            header=Packet.create_header(payload_len=len(payload), packet_type=Packet.Type.BANKED_EVENT_TYPE, tv_sec=12345, tv_nsec=67890),
            payload=payload,
        )

        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
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
        player = Player()

        # Create mock packets
        payload = b"\x01\x02\x03\x04\x05\x06\x07\x08"
        source_packet = Packet(
            header=Packet.create_header(payload_len=len(payload), packet_type=Packet.Type.BANKED_EVENT_TYPE, tv_sec=12345, tv_nsec=67890),
            payload=payload,
        )

        payload = b"\x11\x22\x33\x44"
        client_packet = Packet(
            header=Packet.create_header(payload_len=len(payload), packet_type=Packet.Type.CLIENT_HELLO_TYPE, tv_sec=12345, tv_nsec=78900),
            payload=payload,
        )

        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
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
        player = Player()

        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
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
        player = Player()

        # Create control packet from client
        payload = b"\x11\x22\x33\x44"
        control_packet = Packet(
            header=Packet.create_header(payload_len=len(payload), packet_type=Packet.Type.CLIENT_HELLO_TYPE, tv_sec=12345, tv_nsec=78900),
            payload=payload,
        )

        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
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
        apply_test_config({"record.file_output": "single_packet"})
        player = Player()

        # Create mock packet with specific metadata
        payload = b"\x01\x02\x03\x04\x05\x06\x07\x08"
        mock_packet = Packet(
            header=Packet.create_header(
                payload_len=len(payload),
                packet_type=Packet.Type.BANKED_EVENT_TYPE,
                tv_sec=12345,
                tv_nsec=67890,
            ),
            payload=payload,
        )

        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
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
                # --- SINGLE_PACKET case ---
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

                    # Verify file was opened with expected per-packet name
                    expected_filename = output_path / f"{mock_packet.packet_type:#06x}-{mock_packet.timestamp}-000001.adara"
                    mock_file.assert_called()
                    call_args = mock_file.call_args_list[0]
                    opened_path = Path(call_args[0][0])
                    self.assertEqual(opened_path.name, expected_filename.name)

                # --- MULTI_PACKET case ---
                # Reuse same player and sockets, but force MULTI_PACKET mode and reset the select counter.
                player._file_output_mode = Player.FileOutputMode.MULTI_PACKET
                call_count[0] = 0

                with (
                    patch("socket.socket", return_value=mock_source_socket),
                    patch("select.select", side_effect=controlled_naming_select),
                    patch.object(Packet, "from_socket", return_value=mock_packet),
                    patch.object(player, "_impose_transfer_limit"),
                    patch.object(mock_client_socket, "send") as mock_client_send,
                    patch("builtins.open", mock_open()) as mock_file_multi,
                    patch.object(Packet, "to_file"),
                ):
                    mock_client_send.return_value = mock_packet.size

                    player.record(output_path, mock_client_socket)

                    # In MULTI_PACKET mode all packets go to a single session file.
                    expected_session_path = output_path / "session.adara"
                    mock_file_multi.assert_called()
                    call_args_multi = mock_file_multi.call_args_list[0]
                    opened_path_multi = Path(call_args_multi[0][0])
                    self.assertEqual(opened_path_multi, expected_session_path)
            finally:
                signal.alarm(0)
                signal.signal(signal.SIGALRM, old_handler)

    def test_single_packet_mode_creates_distinct_files(self):
        """Ensures SINGLE_PACKET file output creates a distinct file per packet using the sequence number."""
        apply_test_config({"record.file_output": "single_packet"})
        player = Player()

        payload = b"\x01\x02\x03\x04\x05\x06\x07\x08"
        packet = Packet(
            header=Packet.create_header(
                payload_len=len(payload),
                packet_type=Packet.Type.BANKED_EVENT_TYPE,
                tv_sec=12345,
                tv_nsec=67890,
            ),
            payload=payload,
        )

        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
            output_path = Path(tmpdir) / "output"

            first_path = player._packet_file_path(output_path, packet, sequence_number=1)
            second_path = player._packet_file_path(output_path, packet, sequence_number=2)

            # Distinct filenames, driven by the sequence number suffix.
            self.assertNotEqual(first_path.name, second_path.name)
            self.assertTrue(first_path.name.endswith("000001.adara"))
            self.assertTrue(second_path.name.endswith("000002.adara"))

    def test_multi_packet_mode_uses_single_session_file(self):
        """Ensures MULTI_PACKET file output records all packets into a single 'session.adara' file per session."""
        apply_test_config({"record.file_output": "multi_packet"})
        player = Player()

        payload = b"\x01\x02\x03\x04\x05\x06\x07\x08"
        packet = Packet(
            header=Packet.create_header(
                payload_len=len(payload),
                packet_type=Packet.Type.BANKED_EVENT_TYPE,
                tv_sec=12345,
                tv_nsec=67890,
            ),
            payload=payload,
        )

        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
            output_path = Path(tmpdir) / "output"

            first_path = player._packet_file_path(output_path, packet, sequence_number=1)
            second_path = player._packet_file_path(output_path, packet, sequence_number=2)

            expected = output_path / "session.adara"
            self.assertEqual(first_path, expected)
            self.assertEqual(second_path, expected)

    def test_multi_packet_mode_appends_to_existing_session_file(self):
        """Verifies MULTI_PACKET file output appends multiple packets to the same session file without overwriting earlier data."""
        apply_test_config({"record.file_output": "multi_packet"})
        player = Player()

        payload = b"\x01\x02\x03\x04\x05\x06\x07\x08"
        packet = Packet(
            header=Packet.create_header(
                payload_len=len(payload),
                packet_type=Packet.Type.BANKED_EVENT_TYPE,
                tv_sec=12345,
                tv_nsec=67890,
            ),
            payload=payload,
        )

        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
            output_path = Path(tmpdir) / "output"

            with patch("builtins.open", mock_open()) as mock_file:
                # Open twice via the context manager used in record()
                with player._open_record_file(output_path, packet, sequence_number=1):
                    pass
                with player._open_record_file(output_path, packet, sequence_number=2):
                    pass

            # Both opens should target the same session file and use append mode.
            self.assertGreaterEqual(len(mock_file.call_args_list), 2)
            first_call = mock_file.call_args_list[0]
            second_call = mock_file.call_args_list[1]

            first_path = Path(first_call[0][0])
            second_path = Path(second_call[0][0])
            self.assertEqual(first_path.name, "session.adara")
            self.assertEqual(second_path.name, "session.adara")
            self.assertEqual(first_call[0][1], "ab")
            self.assertEqual(second_call[0][1], "ab")

    def test_multi_packet_mode_integrates_with_record_loop(self):
        """
        Checks that in MULTI_PACKET mode, packets received from the server during record()
          are all written via a single session file path.
        """
        apply_test_config({"record.file_output": "multi_packet"})
        player = Player()

        # Two packets coming from the server during a single record session.
        payload1 = b"\x01\x02\x03\x04"
        payload2 = b"\x05\x06\x07\x08"
        packet1 = Packet(
            header=Packet.create_header(
                payload_len=len(payload1),
                packet_type=Packet.Type.BANKED_EVENT_TYPE,
                tv_sec=12345,
                tv_nsec=100,
            ),
            payload=payload1,
        )
        packet2 = Packet(
            header=Packet.create_header(
                payload_len=len(payload2),
                packet_type=Packet.Type.BANKED_EVENT_TYPE,
                tv_sec=12346,
                tv_nsec=200,
            ),
            payload=payload2,
        )

        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
            output_path = Path(tmpdir) / "output"

            mock_client_socket = MagicMock(spec=socket.socket)
            mock_source_socket = MagicMock(spec=socket.socket)

            call_count = [0]

            def controlled_multi_select(*args, **kwargs):
                """Alternate between server-readable and client-writable to process two packets."""
                call_count[0] += 1
                match call_count[0]:
                    case 1:
                        # First packet available from server
                        return ([mock_source_socket], [], [])
                    case 2:
                        # Client writable: send first packet
                        return ([], [mock_client_socket], [])
                    case 3:
                        # Second packet available from server
                        return ([mock_source_socket], [], [])
                    case 4:
                        # Client writable: send second packet
                        return ([], [mock_client_socket], [])
                    case _:
                        # Stop the loop
                        player._running = False
                        return ([], [], [])

            old_handler = signal.signal(signal.SIGALRM, timeout_handler)
            signal.alarm(5)

            try:
                with (
                    patch("socket.socket", return_value=mock_source_socket),
                    patch("select.select", side_effect=controlled_multi_select),
                    patch.object(
                        Packet,
                        "from_socket",
                        side_effect=[packet1, packet2],
                    ),
                    patch.object(player, "_impose_transfer_limit"),
                    patch.object(mock_client_socket, "send") as mock_client_send,
                    patch("builtins.open", mock_open()) as mock_file,
                    patch.object(Packet, "to_file"),
                ):
                    # Simulate successful sends for both packets
                    mock_client_send.return_value = max(packet1.size, packet2.size)

                    player.record(output_path, mock_client_socket)

                    # All writes should target the shared session file in MULTI_PACKET mode.
                    mock_file.assert_called()
                    opened_paths = [Path(call[0][0]) for call in mock_file.call_args_list]
                    self.assertGreaterEqual(len(opened_paths), 2)
                    unique_names = {p.name for p in opened_paths}
                    self.assertEqual(unique_names, {"session.adara"})
            finally:
                signal.alarm(0)
                signal.signal(signal.SIGALRM, old_handler)

    def test_cleanup_sockets_post_record(self):
        """Verifies all sockets are closed and cleaned up after recording finishes."""
        player = Player()

        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
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
