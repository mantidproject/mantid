"""

Test suite for Player class (excluding play and record methods) from adara_player module.

"""

from io import BytesIO
import numpy as np
import os
from pathlib import Path
import signal
import socket
import struct
from tempfile import NamedTemporaryFile, TemporaryDirectory
import time

import player_config
from packet_player import Player, Packet, EPICS_EPOCH_OFFSET
from adara_player_test_helpers import apply_test_config

import unittest
from unittest.mock import Mock, patch, MagicMock


class Test_Player(unittest.TestCase):
    """Test cases for Player class (general methods)."""

    def setUp(self):
        super().setUp()
        self.temp_dir = self.enterContext(TemporaryDirectory(prefix=f"{__name__}_"))
        apply_test_config()

    def tearDown(self):
        player_config.reset()
        super().tearDown()

    def test_init_defaults_and_overrides(self):
        """Tests initialization with default configuration values and with explicit overrides."""
        # Test with defaults (using Config values)
        player = Player()

        # Check defaults
        self.assertIsNotNone(player._source_address)
        self.assertIsNotNone(player._rate_filter)
        self.assertIsNotNone(player._packet_filter)
        self.assertEqual(player._buffer_MB, 64)
        self.assertFalse(player._running)

        # Test with explicit overrides
        custom_source = "192.168.1.1:8888"
        apply_test_config({"playback.rate": "normal"})
        player = Player(source_address=custom_source)

        # Check overrides
        self.assertEqual(player._source_address, ("192.168.1.1", 8888))

    def test_getratefilter_normal_unlimited(self):
        """
        Verifies valid creation and function of rate filters for NORMAL and UNLIMITED,
          and fails on unknown values.
        """
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
        apply_test_config({"server.address": "{XDG_RUNTIME_DIR}/sock-{name}", "server.name": "default_player"})

        # Test with environment variables set
        with patch.dict(os.environ, {"XDG_RUNTIME_DIR": "/run/user/1000", "adara_player_name": "custom_player"}):
            address = Player.get_server_address()
            self.assertEqual(address, "/run/user/1000/sock-custom_player")

        # Test with only XDG_RUNTIME_DIR (use default name from Config)
        with patch.dict(os.environ, {"XDG_RUNTIME_DIR": "/run/user/1000"}, clear=True):
            address = Player.get_server_address()
            self.assertEqual(address, "/run/user/1000/sock-default_player")

        # Test fallback to TMPDIR on macOS
        with patch.dict(os.environ, {"TMPDIR": f"{self.temp_dir}"}, clear=True):
            address = Player.get_server_address()
            self.assertEqual(address, f"{self.temp_dir}/sock-default_player")

    def test_iter_file_single_file(self):
        """Validates packet iteration from a single file stream (including edge cases)."""
        # Create a BytesIO with multiple packets
        stream = BytesIO()

        # Write first packet
        header1 = Packet.create_header(payload_len=8, packet_type=Packet.Type.HEARTBEAT_TYPE, tv_sec=1000000000, tv_nsec=500000000)
        payload1 = b"\x01\x02\x03\x04\x05\x06\x07\x08"
        stream.write(header1)
        stream.write(payload1)

        # Create second packet with different data
        header2 = Packet.create_header(payload_len=4, packet_type=Packet.Type.SYNC_TYPE, tv_sec=2000000000, tv_nsec=0)
        payload2 = b"\xaa\xbb\xcc\xdd"
        stream.write(header2)
        stream.write(payload2)

        # Reset stream to beginning
        stream.seek(0)

        # Iterate and collect packets
        packets = list(Player.iter_file(stream, source="test_stream"))

        self.assertEqual(len(packets), 2)
        self.assertEqual(packets[0].payload, payload1)
        self.assertEqual(packets[1].payload, payload2)
        self.assertEqual(packets[0].source, "test_stream")
        self.assertEqual(packets[1].source, "test_stream")

    def test_iter_file_empty_file(self):
        """Test iteration over empty file yields no packets."""
        stream = BytesIO(b"")
        packets = list(Player.iter_file(stream))
        self.assertEqual(len(packets), 0)

    def test_iter_file_incomplete_header(self):
        """Test that iter_file stops cleanly when encountering incomplete header."""
        # Create stream with one complete packet, then incomplete header
        complete_header = Packet.create_header(payload_len=4, packet_type=Packet.Type.HEARTBEAT_TYPE, tv_sec=1000000000, tv_nsec=0)
        complete_payload = b"\x01\x02\x03\x04"

        # Write complete packet, then only 10 bytes of next header (need 16)
        stream = BytesIO(complete_header + complete_payload + b"\x00" * 10)

        # Should yield first packet successfully, then stop (not raise error)
        packets = list(Player.iter_file(stream))

        # Should get exactly one packet (incomplete header is silently skipped)
        self.assertEqual(len(packets), 1)
        self.assertEqual(packets[0].payload, complete_payload)

    def test_iter_file_corrupt_packet_type(self):
        """Test that iter_file raises ValueError for corrupt packet type."""
        # Create complete header (16 bytes) but with invalid packet type
        corrupt_header = struct.pack("<I", 0)  # payload_len = 0
        corrupt_header += struct.pack("<I", 0xFFFFFFFF)  # Invalid type field
        corrupt_header += struct.pack("<I", 1000000000)  # tv_sec
        corrupt_header += struct.pack("<I", 0)  # tv_nsec

        stream = BytesIO(corrupt_header)

        # Should raise ValueError for invalid packet type
        with self.assertRaises(ValueError) as context:
            list(Player.iter_file(stream))

        # Error should mention invalid packet type
        error_msg = str(context.exception)
        self.assertIn("Invalid ADARA packet type", error_msg)
        self.assertIn("0xFFFFFF", error_msg)  # Type value

    def test_iter_files_multi_file_ordering(self):
        """Tests that packets from multiple files are sorted by file modification time."""
        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
            tmppath = Path(tmpdir)

            # Create file 3 with MIDDLE timestamp (THIRD sequence number): (FIRST mtime)
            file3 = tmppath / "data-000003.adara"
            header3 = Packet.create_header(
                payload_len=8,
                packet_type=Packet.Type.HEARTBEAT_TYPE,
                tv_sec=1500000000,  # Middle timestamp
                tv_nsec=0,
            )
            payload3 = b"MIDDLE--"
            with open(file3, "wb") as f:
                f.write(header3 + payload3)
            t0 = time.time()
            os.utime(file3, (t0, t0))  # set `mtime == t0`

            # Create file 2 with EARLIER timestamp (SECOND sequence number): (SECOND mtime)
            file2 = tmppath / "data-000002.adara"
            header2 = Packet.create_header(
                payload_len=8,
                packet_type=Packet.Type.HEARTBEAT_TYPE,
                tv_sec=1000000000,  # Earlier timestamp
                tv_nsec=0,
            )
            payload2 = b"EARLIER-"
            with open(file2, "wb") as f:
                f.write(header2 + payload2)
            os.utime(file2, (t0 + 1, t0 + 1))  # set `mtime == t0 + 1`

            # Create file 1 with LATER timestamp (FIRST sequence number): (LAST mtime)
            file1 = tmppath / "data-000001.adara"
            header1 = Packet.create_header(
                payload_len=8,
                packet_type=Packet.Type.HEARTBEAT_TYPE,
                tv_sec=2000000000,  # Later timestamp
                tv_nsec=0,
            )
            payload1 = b"LATER---"
            with open(file1, "wb") as f:
                f.write(header1 + payload1)
            os.utime(file1, (t0 + 2, t0 + 2))  # set `mtime == t0 + 2`

            # iter_files should yield packets in chronological order
            # regardless of filename order
            player = Player(packet_ordering_scheme="timestamp")
            packets = list(player.iter_files(tmppath.glob("*.adara")))

            self.assertEqual(len(packets), 3)
            # Verify chronological ordering by payload content
            self.assertEqual(packets[0].payload, payload2)  # EARLIER (1000000000)
            self.assertEqual(packets[1].payload, payload3)  # MIDDLE (1500000000)
            self.assertEqual(packets[2].payload, payload1)  # LATER (2000000000)

            # Verify source is set correctly (file path)
            self.assertIn(str(file2), packets[0].source)
            self.assertIn(str(file3), packets[1].source)
            self.assertIn(str(file1), packets[2].source)

            # iter_files should yield packets in order of the sequence numbers
            # regardless of timestamp order, or modification-time order
            player = Player(packet_ordering_scheme="sequence_number")
            packets = list(player.iter_files(tmppath.glob("*.adara")))

            self.assertEqual(len(packets), 3)
            # Verify chronological ordering by payload content
            self.assertEqual(packets[0].payload, payload1)  # Sequence number: 1
            self.assertEqual(packets[1].payload, payload2)  # Sequence number: 2
            self.assertEqual(packets[2].payload, payload3)  # Sequence number: 3

            # Verify source is set correctly (file path)
            self.assertIn(str(file1), packets[0].source)
            self.assertIn(str(file2), packets[1].source)
            self.assertIn(str(file3), packets[2].source)

            # iter_files should yield packets in order of the modification times
            # regardless of timestamp order, or sequence-number order
            player = Player(packet_ordering_scheme="mtime")
            packets = list(player.iter_files(tmppath.glob("*.adara")))

            self.assertEqual(len(packets), 3)
            # Verify chronological ordering by payload content
            self.assertEqual(packets[0].payload, payload3)  # modification time: 1
            self.assertEqual(packets[1].payload, payload2)  # modification time: 2
            self.assertEqual(packets[2].payload, payload1)  # modification time: 3

            # Verify source is set correctly (file path)
            self.assertIn(str(file3), packets[0].source)
            self.assertIn(str(file2), packets[1].source)
            self.assertIn(str(file1), packets[2].source)

    def test__file_timestamp_file_not_found(self):
        """Confirms proper error on missing file passed to `_file_timestamp`."""
        nonexistent_path = Path("/nonexistent/path/to/file.adara")

        with self.assertRaises(FileNotFoundError):
            Player._file_timestamp(nonexistent_path)

    def test__file_timestamp_empty_malformed(self):
        """Verifies behavior when the file is empty or contains corrupt packet headers."""
        # Test 1: Empty file should return epoch timestamp
        with NamedTemporaryFile(delete=False, suffix=".adara") as f:
            empty_filepath = Path(f.name)

        try:
            # Empty file should catch StopIteration and return epoch
            timestamp = Player._file_timestamp(empty_filepath)
            self.assertEqual(timestamp, np.datetime64(0, "ns"))
        finally:
            empty_filepath.unlink()

        # Test 2: File with corrupt/invalid packet type (complete header)
        with NamedTemporaryFile(delete=False, suffix=".adara") as f:
            # Write complete header (16 bytes) with invalid packet type
            corrupt_header = struct.pack("<I", 0)  # payload_len = 0
            corrupt_header += struct.pack("<I", 0xFFFFFFFF)  # Invalid type field
            corrupt_header += struct.pack("<I", 1000000000)  # tv_sec
            corrupt_header += struct.pack("<I", 0)  # tv_nsec
            f.write(corrupt_header)
            corrupt_filepath = Path(f.name)

        try:
            # Should raise ValueError for invalid packet type
            with self.assertRaises(ValueError) as context:
                Player._file_timestamp(corrupt_filepath)

            # Verify error message mentions invalid packet type
            error_msg = str(context.exception)
            self.assertIn("Invalid ADARA packet type", error_msg)
        finally:
            corrupt_filepath.unlink()

    def test__file_timestamp_incomplete_header(self):
        """Test _file_timestamp behavior with incomplete header in file."""
        with NamedTemporaryFile(delete=False, suffix=".adara") as f:
            # Write incomplete header (only 10 bytes, need 16)
            f.write(b"\x00" * 10)
            incomplete_filepath = Path(f.name)

        try:
            # iter_file should handle this gracefully, returning no packets
            # _file_timestamp catches StopIteration and returns epoch
            timestamp = Player._file_timestamp(incomplete_filepath)
            self.assertEqual(timestamp, np.datetime64(0, "ns"))
        finally:
            incomplete_filepath.unlink()

    def test__file_timestamp_valid_file(self):
        """Test _file_timestamp returns correct timestamp from valid file."""
        with NamedTemporaryFile(delete=False, suffix=".adara") as f:
            # Create valid packet with known timestamp
            test_tv_sec = 1234567890
            test_tv_nsec = 123456789
            header = Packet.create_header(payload_len=4, packet_type=Packet.Type.HEARTBEAT_TYPE, tv_sec=test_tv_sec, tv_nsec=test_tv_nsec)
            f.write(header + b"\x00\x00\x00\x00")
            filepath = Path(f.name)

        try:
            timestamp = Player._file_timestamp(filepath)

            # Verify timestamp matches the packet's timestamp
            expected_timestamp = np.datetime64(test_tv_sec + EPICS_EPOCH_OFFSET, "s") + np.timedelta64(test_tv_nsec, "ns")
            self.assertEqual(timestamp, expected_timestamp)
        finally:
            filepath.unlink()

    def test_packet_filename_sequence_number(self):
        """
        Validates that successive calls to _packet_filename with increasing sequence numbers
        yield distinct filenames, and the naming scheme follows the expected pattern.
        """
        dummy_packet = MagicMock(spec=Packet)
        dummy_packet.packet_type = Packet.Type.BANKED_EVENT_TYPE
        dummy_packet.timestamp = 123456
        expected_names = ["0x4000-123456-000001.adara", "0x4000-123456-000002.adara", "0x4000-123456-000003.adara"]
        for i, expected in zip([1, 2, 3], expected_names, strict=True):
            fname = Player._packet_filename(dummy_packet, i)
            self.assertEqual(fname, expected)

    def test_cleanup_all_sockets_closed(self):
        """Verifies closure and cleanup logic for all held sockets."""
        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
            test_socket_path = Path(tmpdir) / "test_adara_socket"

            player = Player()

            # Create mock sockets
            mock_source = MagicMock(spec=socket.socket)
            mock_client = MagicMock(spec=socket.socket)

            # Create a fake socket file
            test_socket_path.touch()
            self.assertTrue(test_socket_path.exists())

            try:
                # Call cleanup
                player.cleanup(sockets=[mock_source, mock_client], addresses=[test_socket_path])

                # Verify all sockets were closed
                mock_source.close.assert_called_once()
                mock_client.close.assert_called_once()

                # Verify UDS file was removed
                self.assertFalse(test_socket_path.exists())
            finally:
                # Cleanup
                if test_socket_path.exists():
                    test_socket_path.unlink()

    def test_signalhandler_sets_running_false(self):
        """Tests the handler response: sets flag and prepares for shutdown."""
        apply_test_config({"server.buffer_MB": 2, "playback.rate": "normal"})
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
