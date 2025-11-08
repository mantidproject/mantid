import unittest
from unittest.mock import patch, MagicMock
from pathlib import Path
from adara_player import Player, Packet


class TestPlayerLimits(unittest.TestCase):
    def test_packet_filename_iteration(self):
        """
        Validates that successive calls to _packet_filename with increasing iteration values yield distinct filenames,
        and the naming scheme follows the expected pattern (i.e., no suffix for the first iteration, '-2', '-3', etc. for later copies).
        """
        dummy_packet = MagicMock(spec=Packet)
        dummy_packet.packet_type = Packet.Type.BANKED_EVENT_TYPE
        dummy_packet.timestamp = 123456
        expected_names = ["0x4000-123456.adara", "0x4000-123456-2.adara", "0x4000-123456-3.adara"]
        for i, expected in zip([1, 2, 3], expected_names):
            fname = Player._packet_filename(dummy_packet, i)
            self.assertEqual(fname, expected)

    @patch("adara_player.Path.exists")
    def test_packet_file_path_new_file(self, path_exists_mock):
        """
        Checks that _packet_file_path returns a path for a new (previously unwritten) packet,
        without triggering the iteration limit, when no files exist yet with the same base name.
        """
        path_exists_mock.return_value = False
        dummy_packet = MagicMock(spec=Packet)
        dummy_packet.packet_type = Packet.Type.BANKED_EVENT_TYPE
        dummy_packet.timestamp = 1000

        output_dir = Path("/fake/output")
        file_path = Player._packet_file_path(output_dir, dummy_packet)
        expected_fname = Player._packet_filename(dummy_packet, 1)
        self.assertEqual(file_path, output_dir / expected_fname)

    @patch("adara_player.Path.exists")
    def test_packet_file_path_multiple_iterations(self, path_exists_mock):
        """
        Simulates existing files and confirms that _packet_file_path increments the iteration number
        and returns the next available path, up to but not exceeding the iteration limit.
        """
        dummy_packet = MagicMock(spec=Packet)
        dummy_packet.packet_type = Packet.Type.BEAM_MONITOR_EVENT_TYPE
        dummy_packet.timestamp = 8888

        output_dir = Path("/fake/dir")
        # Simulate first two files exist, third does not
        path_exists_mock.side_effect = [True, True, False, False]
        file_path = Player._packet_file_path(output_dir, dummy_packet)
        expected_fname = Player._packet_filename(dummy_packet, 3)
        self.assertEqual(file_path, output_dir / expected_fname)

    @patch("adara_player.Path.exists")
    @patch("adara_player._logger")
    def test_packet_file_path_iteration_limit_exceeded(self, logger_mock, path_exists_mock):
        """
        Simulates all allowed iterations already existing and ensures _packet_file_path returns None
        and logs an error when the iteration limit is reached, preventing further writing.
        """
        dummy_packet = MagicMock(spec=Packet)
        dummy_packet.packet_type = Packet.Type.RUN_STATUS_TYPE
        dummy_packet.timestamp = 5555

        output_dir = Path("/dir/exists")
        limit = Player.ITERATION_LIMIT
        path_exists_mock.side_effect = [True] * limit
        result = Player._packet_file_path(output_dir, dummy_packet)
        self.assertIsNone(result)
        self.assertTrue(logger_mock.error.called)
        self.assertIn(str(limit), logger_mock.error.call_args[0][0])

    def test_impose_transfer_limit_stops_at_limit(self):
        """
        Checks that _impose_transfer_limit will set the appropriate state
          (e.g., stop the transfer, log an error, set relevant flags)
            when the byte transfer limit is reached or exceeded.
        """
        dummy_packet = MagicMock(spec=Packet)
        dummy_packet.size = 10 * 1024**2  # 10 MB

        with patch("adara_player.Player._get_server_address", return_value="/tmp/sock-test"):
            player = Player()
            player.TRANSFER_LIMIT_MB = 1  # set limit to 1 MB for test
            player._running = True
            player._transferred_bytes = 0

            with patch("adara_player._logger") as logger_mock:
                player._impose_transfer_limit(dummy_packet)
                self.assertFalse(player._running)  # should stop
                self.assertTrue(logger_mock.error.called)
                self.assertIn("Transfer limit", logger_mock.error.call_args[0][0])

    def test_impose_transfer_limit_under_limit(self):
        """
        Verifies that normal operation continues when _impose_transfer_limit is called,
          but the total transferred bytes are still below the defined limit.
        """
        dummy_packet = MagicMock(spec=Packet)
        dummy_packet.size = 512 * 1024  # 0.5 MB

        with patch("adara_player.Player._get_server_address", return_value="/tmp/sock-test"):
            player = Player()
            player.TRANSFER_LIMIT_MB = 1  # set limit to 1 MB for test
            player._running = True
            player._transferred_bytes = 0

            with patch("adara_player._logger") as logger_mock:
                player._impose_transfer_limit(dummy_packet)
                self.assertTrue(player._running)  # should keep running
                self.assertFalse(logger_mock.error.called)


if __name__ == "__main__":
    unittest.main()
