"""
Test suite for Player.play method from adara_player module.
"""
# ruff: noqa: F841

import numpy as np
from pathlib import Path
import signal
import socket
from tempfile import TemporaryDirectory

import player_config
from packet_player import Player, Packet, ClientHelloPacket
from adara_player_test_helpers import apply_test_config

import unittest
from unittest.mock import patch, MagicMock, call


class TimeoutException(Exception):
    """Raised when a test times out."""

    pass


def timeout_handler(signum, frame):
    raise TimeoutException("Test timed out - likely infinite loop")


class Test_Player_play(unittest.TestCase):
    """Test cases for Player.play method."""

    def setUp(self):
        super().setUp()
        self.temp_dir = self.enterContext(TemporaryDirectory(prefix=f"{__name__}_"))
        apply_test_config()

    def tearDown(self):
        player_config.reset()
        super().tearDown()

    def test_client_socket_init(self):
        """Checks that client socket is initialized correctly."""
        player = Player()

        # Mock socket creation and operations
        mock_client_socket = MagicMock(spec=socket.socket)

        with (
            patch.object(player, "stream_packets") as mock_stream,
        ):
            # Execute play
            files_iter = iter([Path("/fake/path/file1.adara"), Path("/fake/path/file2.adara")])
            player.play(mock_client_socket, files_iter)

            # Verify socket configuration
            mock_client_socket.settimeout.assert_called()
            mock_client_socket.setblocking.assert_called()

    def test_waits_for_clienthello(self):
        """Ensures handshake blocks until a CLIENTHELLO packet arrives."""
        apply_test_config({"playback.handshake": "client_hello"})
        player = Player()

        # Create mock CLIENT_HELLO packet
        start_time = np.datetime64(1, "s")
        mock_hello_packet = MagicMock(spec=ClientHelloPacket)
        mock_hello_packet.start_time = start_time

        # Mock socket operations
        mock_client_socket = MagicMock(spec=socket.socket)

        with (
            patch.object(ClientHelloPacket, "from_socket", return_value=mock_hello_packet),
            patch.object(player, "stream_packets") as mock_stream,
        ):
            # Execute play
            files_iter = iter([Path("/fake/path/file1.adara"), Path("/fake/path/file2.adara")])
            player.play(mock_client_socket, files_iter)

            # Verify ClientHelloPacket.from_socket was called
            ClientHelloPacket.from_socket.assert_called_once_with(mock_client_socket)

            # Verify start_time was set from the hello packet
            self.assertEqual(player._start_time, start_time)

            # Verify socket was configured for handshake (blocking mode with timeout)
            calls = mock_client_socket.settimeout.call_args_list
            # First call should be for handshake timeout
            self.assertIn(call(10.0), calls)

    def test_clienthello_timeout_handling(self):
        """Verifies timeout and error-handling if CLIENTHELLO doesn't arrive."""
        apply_test_config({"playback.handshake": "client_hello"})
        player = Player()

        # Mock socket operations
        mock_client_socket = MagicMock(spec=socket.socket)

        with (
            patch.object(ClientHelloPacket, "from_socket", side_effect=socket.timeout("Timeout")),
        ):
            # Should raise socket.timeout
            with self.assertRaises(socket.timeout):
                files_iter = iter([Path("/fake/path/file1.adara")])
                player.play(mock_client_socket, files_iter)

            # Cleanup should still be called
            self.assertFalse(player._running)

    def test_sets_starttime_from_packet(self):
        """Tests whether playback starttime is set correctly (handshake or default)."""
        # Test with client_hello handshake
        apply_test_config({"playback.handshake": "client_hello"})
        player = Player()

        custom_start_time = np.datetime64("2024-01-15T10:30:00")
        mock_hello_packet = MagicMock(spec=ClientHelloPacket)
        mock_hello_packet.start_time = custom_start_time

        mock_client_socket = MagicMock(spec=socket.socket)

        with (
            patch.object(ClientHelloPacket, "from_socket", return_value=mock_hello_packet),
            patch.object(player, "stream_packets"),
        ):
            files_iter = iter([Path("/fake/path/file1.adara"), Path("/fake/path/file2.adara")])
            player.play(mock_client_socket, files_iter)

            # Verify start_time was set from CLIENT_HELLO packet
            self.assertEqual(player._start_time, custom_start_time)

        # Test with no handshake (default start_time)
        apply_test_config({"playback.handshake": "none"})
        player = Player()

        mock_client_socket = MagicMock(spec=socket.socket)

        with patch.object(player, "stream_packets"):
            files_iter = iter([Path("/fake/path/file1.adara"), Path("/fake/path/file2.adara")])
            player.play(mock_client_socket, files_iter)

            # Verify default start_time (stream all data)
            self.assertEqual(player._start_time, np.datetime64(1, "s"))

    def test_streams_all_packets(self):
        """Ensures all packets matching patterns/rate are streamed."""
        player = Player()

        mock_client_socket = MagicMock(spec=socket.socket)

        files_iter = iter([Path("/test/data/file1.adara"), Path("/test/data/file2.adara")])
        with patch.object(player, "stream_packets") as mock_stream:
            player.play(mock_client_socket, files_iter)

        # The second arg must be the same iterator object we passed in.
        mock_stream.assert_called_once_with(mock_client_socket, files_iter, dry_run=False)

    def test_buffer_prefill_and_stream(self):
        """
        Validates buffer is filled before actual streaming starts.

        This test will FAIL if an INFINITE LOOP is detected.
        (Previously this defect occurred in the `stream_packets` fill-buffer handshaking system.)
        """
        apply_test_config({"server.buffer_MB": 2})
        player = Player()

        # Create mock packets with known sizes
        mock_packets = []
        for i in range(5):
            pkt = MagicMock(spec=Packet)
            pkt.timestamp = np.datetime64("2024-01-01T12:00:00") + np.timedelta64(i, "s")
            pkt.size = 1024 * 1024  # 1 MB each
            pkt.packet_type = Packet.Type.BANKED_EVENT_TYPE
            mock_packets.append(pkt)

        mock_socket = MagicMock(spec=socket.socket)

        # Set up timeout alarm to detect infinite loops
        old_handler = signal.signal(signal.SIGALRM, timeout_handler)
        signal.alarm(5)  # 5 second timeout

        try:
            # Mock Player.iter_files to return our test packets
            with (
                patch.object(Player, "iter_files", return_value=iter(mock_packets)),
                patch("select.select", return_value=([], [mock_socket], [])),
                patch.object(Packet, "to_socket") as mock_to_socket,
            ):
                # Run stream_packets
                player._running = True
                files_iter = iter([Path("/fake/file1.adara")])
                player.stream_packets(mock_socket, files_iter)

                # If we get here, the method exited gracefully
                # Verify all packets were sent
                self.assertEqual(mock_to_socket.call_count, 5, "Expected exactly 5 packets to be sent")

        except TimeoutException:
            # Test timed out!
            self.fail(
                "`stream_packets` entered infinite loop after iterator exhausted. "
                "Previous defect: next_packet is not set to None when StopIteration is caught, "
                "causing the refill loop to continuously try to append the same packet."
            )
        finally:
            # Cancel the alarm and restore old handler
            signal.alarm(0)
            signal.signal(signal.SIGALRM, old_handler)

    def test_handles_no_packets(self):
        """Checks for proper handling when source files are empty."""
        apply_test_config({"playback.rate": "normal"})
        player = Player()

        mock_socket = MagicMock(spec=socket.socket)

        # Set up timeout alarm to detect infinite loops
        old_handler = signal.signal(signal.SIGALRM, timeout_handler)
        signal.alarm(5)  # 5 second timeout

        try:
            # Mock empty packet iterator
            with patch.object(Player, "iter_files", return_value=iter([])):
                # Should handle gracefully (no exception)
                player._running = True
                files_iter = iter([Path("/fake/file1.adara")])
                player.stream_packets(mock_socket, files_iter)

            # Verify no packets were sent
            with (
                patch.object(Player, "iter_files", return_value=iter([])),
                patch.object(Packet, "to_socket") as mock_send,
            ):
                player._running = True
                files_iter = iter([Path("/fake/file1.adara")])
                player.stream_packets(mock_socket, files_iter)
                mock_send.assert_not_called()

        except TimeoutException:
            self.fail(
                "`stream_packets` entered infinite loop with empty packet iterator. The code should handle empty iterators gracefully."
            )

        finally:
            # Cancel the alarm and restore old handler
            signal.alarm(0)
            signal.signal(signal.SIGALRM, old_handler)

    def test_unexpected_client_packet(self):
        """Ensures warnings are logged for any packet received from the client unexpectedly."""
        player = Player()

        # Create a mock packet from source
        mock_packet = MagicMock(spec=Packet)
        mock_packet.timestamp = np.datetime64("2024-01-01T12:00:00")
        mock_packet.size = 1024
        mock_packet.packet_type = Packet.Type.BANKED_EVENT_TYPE

        # Create unexpected packet from client
        unexpected_packet = MagicMock(spec=Packet)
        unexpected_packet.packet_type = Packet.Type.CLIENT_HELLO_TYPE

        mock_socket = MagicMock(spec=socket.socket)

        # Set up timeout alarm to detect infinite loops
        old_handler = signal.signal(signal.SIGALRM, timeout_handler)
        signal.alarm(5)  # 5 second timeout

        try:
            # Mock select to indicate socket is readable (unexpected data)
            with (
                patch("select.select", return_value=([mock_socket], [mock_socket], [])),
                patch.object(Player, "iter_files", return_value=iter([mock_packet])),
                patch.object(Packet, "from_socket", return_value=unexpected_packet),
                patch.object(Packet, "to_socket"),
                patch("packet_player._logger") as mock_logger,
            ):
                player._running = True
                files_iter = iter([Path("/fake/file1.adara")])
                player.stream_packets(mock_socket, files_iter)

                # Verify warning was logged about unexpected packet
                self.assertTrue(mock_logger.warning.called)
                msg = mock_logger.warning.call_args[0][0]
                self.assertIn("RECV unexpected", msg)

        except TimeoutException:
            self.fail(
                "`stream_packets` entered infinite loop when handling unexpected client packet. Check the packet buffer management logic."
            )

        finally:
            # Cancel the alarm and restore old handler
            signal.alarm(0)
            signal.signal(signal.SIGALRM, old_handler)

    def test_socket_not_writable_retry(self):
        """Verifies retries if the client socket isn't writable."""
        player = Player()

        mock_packet = MagicMock(spec=Packet)
        mock_packet.timestamp = np.datetime64("2024-01-01T12:00:00")
        mock_packet.size = 1024
        mock_packet.packet_type = Packet.Type.BANKED_EVENT_TYPE

        mock_socket = MagicMock(spec=socket.socket)

        # First call: socket not writable, second call: socket writable
        select_results = [
            ([], [], []),  # Not writable
            ([], [mock_socket], []),  # Writable
        ]

        # Set up timeout alarm to detect infinite loops
        old_handler = signal.signal(signal.SIGALRM, timeout_handler)
        signal.alarm(5)  # 5 second timeout

        try:
            with (
                patch("select.select", side_effect=select_results),
                patch.object(Player, "iter_files", return_value=iter([mock_packet])),
                patch.object(Packet, "to_socket") as mock_send,
                patch("time.sleep") as mock_sleep,
            ):
                player._running = True
                files_iter = iter([Path("/fake/file1.adara")])
                player.stream_packets(mock_socket, files_iter)

                # Verify sleep was called (retry logic)
                mock_sleep.assert_called()

        except TimeoutException:
            self.fail(
                "`stream_packets` entered infinite loop during socket retry logic. Check the buffer management when socket is not writable."
            )

        finally:
            # Cancel the alarm and restore old handler
            signal.alarm(0)
            signal.signal(signal.SIGALRM, old_handler)

    def test_cleanup_and_disconnect(self):
        """Ensures server shutdown and all resources are handled after client disconnect."""
        player = Player()

        mock_client_socket = MagicMock(spec=socket.socket)

        with (
            patch.object(player, "stream_packets"),
            patch.object(player, "cleanup") as mock_cleanup,
        ):
            files_iter = iter([Path("/fake/path/file1.adara")])
            player.play(mock_client_socket, files_iter)

            # Verify cleanup was called
            mock_cleanup.assert_called_once()

            # Verify running flag was set correctly
            self.assertFalse(player._running)

    def test_play_exception_handling(self):
        """Checks robust handling of unexpected exceptions during play."""
        player = Player()

        mock_client_socket = MagicMock(spec=socket.socket)

        with (
            patch.object(player, "stream_packets", side_effect=RuntimeError("Test error")),
            patch.object(player, "cleanup") as mock_cleanup,
        ):
            # Should raise the exception but still call cleanup
            with self.assertRaises(RuntimeError):
                files_iter = iter([Path("/fake/path/file1.adara")])
                player.play(mock_client_socket, files_iter)

            # Verify cleanup was called even after exception
            mock_cleanup.assert_called_once()

            # Verify running flag is set to False
            self.assertFalse(player._running)


if __name__ == "__main__":
    unittest.main()
