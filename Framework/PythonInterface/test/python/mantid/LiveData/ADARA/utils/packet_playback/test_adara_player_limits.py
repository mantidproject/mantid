from tempfile import TemporaryDirectory

import player_config
from packet_player import Player, Packet
from adara_player_test_helpers import apply_test_config

import unittest
from unittest.mock import patch, MagicMock


class TestPlayerLimits(unittest.TestCase):
    def setUp(self):
        super().setUp()
        self.temp_dir = self.enterContext(TemporaryDirectory(prefix=f"{__name__}_"))
        apply_test_config()

    def tearDown(self):
        player_config.reset()
        super().tearDown()

    def test_impose_transfer_limit_stops_at_limit(self):
        """
        Checks that _impose_transfer_limit will set the appropriate state
          (e.g., stop the transfer, log an error, set relevant flags)
            when the byte transfer limit is reached or exceeded.
        """
        dummy_packet = MagicMock(spec=Packet)
        dummy_packet.size = 10 * 1024**2  # 10 MB

        player = Player()
        player.TRANSFER_LIMIT_MB = 1  # set limit to 1 MB for test
        player._running = True
        player._transferred_bytes = 0

        with patch("packet_player._logger") as logger_mock:
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

        player = Player()
        player.TRANSFER_LIMIT_MB = 1  # set limit to 1 MB for test
        player._running = True
        player._transferred_bytes = 0

        with patch("packet_player._logger") as logger_mock:
            player._impose_transfer_limit(dummy_packet)
            self.assertTrue(player._running)  # should keep running
            self.assertFalse(logger_mock.error.called)


if __name__ == "__main__":
    unittest.main()
