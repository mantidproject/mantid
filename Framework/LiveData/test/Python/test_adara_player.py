"""

Test suite for Player class (excluding play and record methods) from adara_player module.

"""

import os
from pathlib import Path
import signal
import socket
import tempfile

from adara_player import Player, Packet

import unittest
from unittest.mock import Mock, patch, MagicMock


class Test_Player(unittest.TestCase):
    """Test cases for Player class (general methods)."""

    def test_init_defaults_and_overrides(self):
        """Tests initialization with default configuration values and with explicit overrides."""
        # Test with defaults (using Config values)
        with patch(
            "adara_player.Config",
            {
                "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                "source": {"address": "127.0.0.1:31415"},
                "playback": {"rate": "normal", "ignore_packets": []},
            },
        ):
            with patch("adara_player.Player._get_server_address", return_value="/tmp/sock-test"):
                player = Player()

                # Check defaults
                self.assertIsNotNone(player._server_address)
                self.assertIsNotNone(player._source_address)
                self.assertIsNotNone(player._rate_filter)
                self.assertIsNotNone(player._packet_filter)
                self.assertEqual(player._buffer_MB, 64)
                self.assertFalse(player._running)
                self.assertIsNone(player._source)
                self.assertIsNone(player._server)
                self.assertIsNone(player._client)

        # Test with explicit overrides
        custom_server = "127.0.0.1:9999"
        custom_source = "192.168.1.1:8888"

        with patch(
            "adara_player.Config",
            {
                "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                "source": {"address": "127.0.0.1:31415"},
                "playback": {"rate": "normal", "ignore_packets": []},
            },
        ):
            player = Player(server_address=custom_server, source_address=custom_source)

            # Check overrides
            self.assertEqual(player._server_address, ("127.0.0.1", 9999))
            self.assertEqual(player._source_address, ("192.168.1.1", 8888))

    def test_getratefilter_normal_unlimited(self):
        """Verifies valid creation and function of rate filters for NORMAL and UNLIMITED, and fails on unknown values."""
        import numpy as np

        # Test NORMAL rate filter
        rate_filter_normal = Player._get_rate_filter("normal")
        self.assertIsNotNone(rate_filter_normal)

        # Create test packets and timestamps
        packets_start_time = np.datetime64("2024-01-01T12:00:00")
        mock_packet = Mock()
        mock_packet.timestamp = np.datetime64("2024-01-01T12:00:10")
        mock_packet2 = Mock()
        mock_packet2.timestamp = np.datetime64("2024-01-01T12:00:20")

        # Test that packet in the past should pass
        start_time = np.datetime64("now") - np.timedelta64(15, "s")
        self.assertTrue(rate_filter_normal(mock_packet, packets_start_time, start_time))
        # Test that packet in the future should not pass
        self.assertFalse(rate_filter_normal(mock_packet2, packets_start_time, start_time))

        # Test UNLIMITED rate filter
        rate_filter_unlimited = Player._get_rate_filter("unlimited")
        self.assertIsNotNone(rate_filter_unlimited)

        # Unlimited should always return True
        start_time = np.datetime64("now") - np.timedelta64(15, "s")
        self.assertTrue(rate_filter_unlimited(mock_packet, packets_start_time, start_time))
        self.assertTrue(rate_filter_unlimited(mock_packet2, packets_start_time, start_time))

        # Test invalid rate value
        with self.assertRaises(ValueError) as context:
            Player._get_rate_filter("invalid_rate")
        self.assertIn("invalid_rate", str(context.exception))

    def test_getpacketfilter_ignore_list(self):
        """Checks that ignored packet types are properly filtered out."""
        # Define packets to ignore
        ignore_list = [0x4009, 0x8000]  # HEARTBEAT_TYPE, DEVICE_DESC_TYPE

        packet_filter = Player._get_packet_filter(ignore_list)

        # Create mock packets
        heartbeat_packet = Mock()
        heartbeat_packet.packet_type = Packet.Type.HEARTBEAT_TYPE

        device_desc_packet = Mock()
        device_desc_packet.packet_type = Packet.Type.DEVICE_DESC_TYPE

        banked_event_packet = Mock()
        banked_event_packet.packet_type = Packet.Type.BANKED_EVENT_TYPE

        # Test filtering
        self.assertFalse(packet_filter(heartbeat_packet))
        self.assertFalse(packet_filter(device_desc_packet))
        self.assertTrue(packet_filter(banked_event_packet))

    def test_getpacketfilter_empty(self):
        """Ensures no filtering occurs if an empty list is provided."""
        # Test with empty list
        packet_filter_empty = Player._get_packet_filter([])

        # Create mock packets
        heartbeat_packet = Mock()
        heartbeat_packet.packet_type = Packet.Type.HEARTBEAT_TYPE

        device_desc_packet = Mock()
        device_desc_packet.packet_type = Packet.Type.DEVICE_DESC_TYPE

        banked_event_packet = Mock()
        banked_event_packet.packet_type = Packet.Type.BANKED_EVENT_TYPE

        # Test filtering: should pass all packets
        self.assertTrue(packet_filter_empty(heartbeat_packet))
        self.assertTrue(packet_filter_empty(device_desc_packet))
        self.assertTrue(packet_filter_empty(banked_event_packet))

    def test_getserveraddress_config_env(self):
        """Tests config/environment variable substitutions in address formatting."""
        with patch("adara_player.Config", {"server": {"address": "{XDG_RUNTIME_DIR}/sock-{name}", "name": "default_player"}}):
            # Test with environment variables set
            with patch.dict(os.environ, {"XDG_RUNTIME_DIR": "/run/user/1000", "adara_player_name": "custom_player"}):
                address = Player._get_server_address()
                self.assertEqual(address, "/run/user/1000/sock-custom_player")

            # Test with only XDG_RUNTIME_DIR (use default name from Config)
            with patch.dict(os.environ, {"XDG_RUNTIME_DIR": "/run/user/1000"}, clear=True):
                address = Player._get_server_address()
                self.assertEqual(address, "/run/user/1000/sock-default_player")

            # Test fallback to TMPDIR on macOS
            with patch.dict(os.environ, {"TMPDIR": "/tmp"}, clear=True):
                address = Player._get_server_address()
                self.assertEqual(address, "/tmp/sock-default_player")

    def test_create_server_socket_tcp_uds(self):
        """Ensures correct creation and binding of TCP and Unix domain sockets."""
        # Test TCP socket creation
        tcp_address = ("127.0.0.1", 12345)
        tcp_socket = Player._create_server_socket(tcp_address)

        try:
            self.assertIsNotNone(tcp_socket)
            self.assertEqual(tcp_socket.family, socket.AF_INET)
            self.assertEqual(tcp_socket.type, socket.SOCK_STREAM)

            # Check that socket is bound and listening
            socket_address = tcp_socket.getsockname()
            self.assertEqual(socket_address[0], "127.0.0.1")
            self.assertEqual(socket_address[1], 12345)
        finally:
            tcp_socket.close()

        with tempfile.TemporaryDirectory() as tmpdir:
            # Test UDS socket creation
            uds_path = Path(tmpdir) / "test_adara_socket"

            # Clean up if exists
            if uds_path.exists():
                uds_path.unlink()

            uds_socket = Player._create_server_socket(uds_path)

            try:
                self.assertIsNotNone(uds_socket)
                self.assertEqual(uds_socket.family, socket.AF_UNIX)
                self.assertEqual(uds_socket.type, socket.SOCK_STREAM)

                # Check that UDS file was created
                self.assertTrue(uds_path.exists())
            finally:
                uds_socket.close()
                if uds_path.exists():
                    uds_path.unlink()

    def test_cleanup_all_sockets_closed(self):
        """Verifies closure and cleanup logic for all held sockets."""
        with tempfile.TemporaryDirectory() as tmpdir:
            test_socket_path = Path(tmpdir) / "test_adara_socket"

            with patch(
                "adara_player.Config",
                {
                    "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": []},
                },
            ):
                with patch("adara_player.Player._get_server_address", return_value=str(test_socket_path)):
                    player = Player()

            # Create mock sockets
            mock_source = MagicMock(spec=socket.socket)
            mock_client = MagicMock(spec=socket.socket)
            mock_server = MagicMock(spec=socket.socket)

            player._source = mock_source
            player._client = mock_client
            player._server = mock_server

            # Create a fake socket file
            test_socket_path.touch()
            self.assertTrue(test_socket_path.exists())

            try:
                # Call cleanup
                player._cleanup(close_server=True)

                # Verify all sockets were closed
                mock_source.close.assert_called_once()
                mock_client.close.assert_called_once()
                mock_server.close.assert_called_once()

                # Verify socket references are None
                self.assertIsNone(player._source)
                self.assertIsNone(player._client)
                self.assertIsNone(player._server)

                # Verify UDS file was removed
                self.assertFalse(test_socket_path.exists())
            finally:
                # Cleanup
                if test_socket_path.exists():
                    test_socket_path.unlink()

        with tempfile.TemporaryDirectory() as tmpdir:
            test_socket_path = Path(tmpdir) / "test_adara_socket"

            # Test cleanup with close_server=False
            with patch(
                "adara_player.Config",
                {
                    "server": {"address": "/tmp/test-sock", "socket_timeout": 1.0, "buffer_MB": 64},
                    "source": {"address": "127.0.0.1:31415"},
                    "playback": {"rate": "normal", "ignore_packets": []},
                },
            ):
                with patch("adara_player.Player._get_server_address", return_value=str(test_socket_path)):
                    player = Player()

            mock_server2 = MagicMock(spec=socket.socket)
            player._server = mock_server2

            # Create a fake socket file
            test_socket_path.touch()
            self.assertTrue(test_socket_path.exists())

            try:
                # Call cleanup
                player._cleanup(close_server=False)

                # Server should not be closed
                mock_server2.close.assert_not_called()

                # Server path should still exist
                self.assertTrue(test_socket_path.exists())
            finally:
                # Cleanup
                if test_socket_path.exists():
                    test_socket_path.unlink()

    def test_signalhandler_sets_running_false(self):
        """Tests the handler response: sets flag and prepares for shutdown."""
        with patch(
            "adara_player.Config",
            {
                "server": {"address": "/tmp/sock-test", "socket_timeout": 1.0, "buffer_MB": 64},
                "source": {"address": "127.0.0.1:31415"},
                "playback": {"rate": "normal", "ignore_packets": []},
            },
        ):
            # `player._server` should not be created during this test, so we don't care what its path is!
            with patch("adara_player.Player._get_server_address", return_value="/tmp/sock-test"):
                player = Player()

        # Set running to True
        player._running = True

        # Mock signal.signal and signal.alarm
        with patch("signal.signal") as mock_signal, patch("signal.alarm") as mock_alarm:
            # Call signal handler
            player.signal_handler(signal.SIGINT, None)

            # Verify running flag was set to False
            self.assertFalse(player._running)

            # Verify alarm was set
            mock_alarm.assert_called_once_with(10)

            # Verify SIGALRM handler was registered
            self.assertEqual(mock_signal.call_count, 1)
            args = mock_signal.call_args[0]
            self.assertEqual(args[0], signal.SIGALRM)


if __name__ == "__main__":
    unittest.main()
