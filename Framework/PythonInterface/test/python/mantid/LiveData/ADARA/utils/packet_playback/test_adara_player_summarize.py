"""
Test suite for Player.summarize method from adara_player module.
"""

import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import patch, MagicMock

import numpy as np

import player_config
from packet_player import Player, Packet, _PacketFileSummary
from adara_player_test_helpers import apply_test_config


class TimeoutException(Exception):
    """Raised when a test times out."""

    pass


def timeout_handler(signum, frame):
    """Signal handler used to detect potential infinite loops in tests."""
    raise TimeoutException("Test timed out - likely infinite loop")


class Test_Player_summarize(unittest.TestCase):
    """Test cases for Player.summarize method."""

    def setUp(self):
        super().setUp()
        self.temp_dir = self.enterContext(TemporaryDirectory(prefix=f"{__name__}_"))
        apply_test_config()

    def tearDown(self):
        player_config.reset()
        super().tearDown()

    def test_iter_files_called_with_callbacks(self):
        """Ensures summarize drives Player.iter_files with file and packet callbacks to accumulate per-file statistics."""
        player = Player()

        def fake_iter_files(*args, **kwargs):
            # Simulate no packets; we only care that callbacks are wired up.
            return iter([])

        with patch.object(Player, "iter_files", side_effect=fake_iter_files) as mock_iter:
            files_iter = iter([Path("/fake/file1.adara")])

            player.summarize(files_iter)

            mock_iter.assert_called_once()
            _args, kwargs = mock_iter.call_args
            # Verify header_only and that callbacks were supplied
            self.assertIn("header_only", kwargs)
            self.assertFalse(kwargs["header_only"])
            self.assertIn("file_callback", kwargs)
            self.assertIsNotNone(kwargs["file_callback"])
            self.assertIn("packet_callback", kwargs)
            self.assertIsNotNone(kwargs["packet_callback"])

    def test_logs_basic_summary_for_single_file(self):
        """
        Verifies that summarize logs a header, overall packet count, overall timestamp range,
        and per-type packet counts for a single input file with packets.
        """
        player = Player()

        p = Path("/fake/path/file1.adara")

        # Two packets of the same event-bearing type, with increasing timestamps.
        pkt1 = MagicMock(spec=Packet)
        pkt1.timestamp = np.datetime64("2024-01-01T00:00:00")
        pkt1.packet_type = Packet.Type.BANKED_EVENT_TYPE

        pkt2 = MagicMock(spec=Packet)
        pkt2.timestamp = np.datetime64("2024-01-01T00:00:10")
        pkt2.packet_type = Packet.Type.BANKED_EVENT_TYPE

        # Make stat() succeed with a deterministic size.
        MB = 1024 * 1024
        fake_stat = MagicMock()
        fake_stat.st_size = 5 * MB

        with (
            patch("packet_player._logger") as mock_logger,
            patch.object(Path, "stat", return_value=fake_stat),
        ):

            def fake_iter_files(*args, **kwargs):
                file_callback = kwargs.get("file_callback")
                packet_callback = kwargs.get("packet_callback")
                if file_callback:
                    file_callback(p)
                if packet_callback:
                    for pkt in (pkt1, pkt2):
                        packet_callback(pkt)
                # Drive the for-loop once; the yielded values are ignored.
                return iter([None])

            with patch.object(Player, "iter_files", side_effect=fake_iter_files):
                files_iter = iter([p])
                player.summarize(files_iter)

        # Collect logged messages
        messages = [call_args[0][0] for call_args in mock_logger.info.call_args_list]

        # Header line should mention the file name
        self.assertTrue(any(p.name in m for m in messages))

        # Overall packet count should be 2
        self.assertTrue(any("packets: 2" in m for m in messages))

        # Overall timestamp range should match the packet timestamps
        self.assertTrue(any("timestamps: 2024-01-01T00:00:00 .. 2024-01-01T00:00:10" in m for m in messages))

        # Per-type count line for BANKED_EVENT_TYPE should exist
        self.assertTrue(any("0x4000" in m and "BANKED_EVENT_TYPE" in m and ": 2" in m for m in messages))

    def test_logs_no_packets_for_empty_file(self):
        """
        Checks that summarize logs a '(no packets)' message when a file has no packets,
        and does not attempt to log timestamp or per-type details.
        """
        player = Player()
        p = Path("/fake/path/empty.adara")

        def fake_iter_files(*args, **kwargs):
            file_callback = kwargs.get("file_callback")
            if file_callback:
                file_callback(p)
            # Do not call packet_callback -> zero packets
            return iter([None])

        with (
            patch("packet_player._logger") as mock_logger,
            patch.object(Player, "iter_files", side_effect=fake_iter_files),
        ):
            files_iter = iter([p])
            player.summarize(files_iter)

        messages = [call_args[0][0] for call_args in mock_logger.info.call_args_list]

        # Should log the "(no packets)" line
        self.assertTrue(any("(no packets)" in m for m in messages))

        # No overall timestamp line should be logged for this file
        self.assertFalse(any("timestamps:" in m and "(events" not in m for m in messages))

    def test_logs_size_unknown_when_stat_fails(self):
        """
        Confirms that summarize falls back to a 'size: unknown' header line when Path.stat()
        fails for an input file, without raising an exception.
        """
        player = Player()
        p = Path("/fake/path/bad_stat.adara")

        # Force stat() to raise OSError for all Paths
        with (
            patch("packet_player._logger") as mock_logger,
            patch.object(Path, "stat", side_effect=OSError("stat failed")),
        ):

            def fake_iter_files(*args, **kwargs):
                file_callback = kwargs.get("file_callback")
                if file_callback:
                    file_callback(p)
                return iter([None])

            with patch.object(Player, "iter_files", side_effect=fake_iter_files):
                files_iter = iter([p])
                # Should not raise
                player.summarize(files_iter)

        messages = [call_args[0][0] for call_args in mock_logger.info.call_args_list]
        # Header line with "size: unknown" should be present
        self.assertTrue(any(p.name in m and "size: unknown" in m for m in messages))

    def test_per_type_timestamp_ranges_respect_flag(self):
        """
        Verifies that per-type timestamp ranges are logged only when
        Config['summarize']['timestamp_per_type'] is True and are omitted when False.
        """
        p = Path("/fake/path/ts_ranges.adara")

        # Common packet: single type with two timestamps
        pkt1 = MagicMock(spec=Packet)
        pkt1.timestamp = np.datetime64("2024-01-01T00:00:00")
        pkt1.packet_type = Packet.Type.BANKED_EVENT_TYPE

        pkt2 = MagicMock(spec=Packet)
        pkt2.timestamp = np.datetime64("2024-01-01T00:01:00")
        pkt2.packet_type = Packet.Type.BANKED_EVENT_TYPE

        def fake_iter_files(*args, **kwargs):
            file_callback = kwargs.get("file_callback")
            packet_callback = kwargs.get("packet_callback")
            if file_callback:
                file_callback(p)
            if packet_callback:
                for pkt in (pkt1, pkt2):
                    packet_callback(pkt)
            return iter([None])

        # Case 1: timestamp_per_type == False (default)
        apply_test_config({"summarize.timestamp_per_type": False})
        with (
            patch("packet_player._logger") as mock_logger,
            patch.object(Player, "iter_files", side_effect=fake_iter_files),
        ):
            player = Player()
            files_iter = iter([p])
            player.summarize(files_iter)

            msgs = [c[0][0] for c in mock_logger.info.call_args_list]
            # Count all "timestamps:" lines that are not the "(events)" variant
            ts_lines = [m for m in msgs if "timestamps:" in m and "timestamps(events" not in m]
            # Overall timestamp line only; no per-type lines
            self.assertEqual(len(ts_lines), 1)

        # Case 2: timestamp_per_type == True
        apply_test_config({"summarize.timestamp_per_type": True})
        with (
            patch("packet_player._logger") as mock_logger2,
            patch.object(Player, "iter_files", side_effect=fake_iter_files),
        ):
            player = Player()
            files_iter = iter([p])
            player.summarize(files_iter)

            msgs2 = [c[0][0] for c in mock_logger2.info.call_args_list]
            ts_lines2 = [m for m in msgs2 if "timestamps:" in m and "timestamps(events" not in m]
            # Now expect overall + per-type timestamp line(s)
            self.assertGreater(len(ts_lines2), 1)

    def test_suggested_playback_ordering_logged(self):
        """
        Ensures that after processing all files, summarize logs a 'Suggested playback ordering'
        header and one line per file using _PacketFileSummary.sorted_for_playback.
        """
        player = Player()

        p1 = Path("/fake/path/file1.adara")
        p2 = Path("/fake/path/file2.adara")

        def fake_iter_files(*args, **kwargs):
            file_callback = kwargs.get("file_callback")
            if file_callback:
                # Create two summaries (two files)
                file_callback(p1)
                file_callback(p2)
            return iter([None])

        # Two fake summaries in a particular order
        s1 = MagicMock()
        s1.file_path = p2
        s1.is_events_file = True

        s2 = MagicMock()
        s2.file_path = p1
        s2.is_events_file = False

        with (
            patch("packet_player._logger") as mock_logger,
            patch.object(Player, "iter_files", side_effect=fake_iter_files),
            patch("packet_player._PacketFileSummary.sorted_for_playback", return_value=[s1, s2]),
        ):
            files_iter = iter([p1, p2])
            player.summarize(files_iter)

        msgs = [c[0][0] for c in mock_logger.info.call_args_list]

        # Header for suggested playback ordering
        self.assertTrue(any("Suggested playback ordering" in m for m in msgs))

        # Ensure both file names from the sorted list appear in the log
        self.assertTrue(any(p2.name in m for m in msgs))
        self.assertTrue(any(p1.name in m for m in msgs))

    def test_handles_no_input_files_gracefully(self):
        """
        Checks that summarize handles an empty iterable of files without raising,
        and emits only the expected warning(s) from iter_files (if any).
        """
        player = Player()

        # iter_files returns an empty iterator; callbacks are never invoked.
        with (
            patch("packet_player._logger") as mock_logger,
            patch.object(Player, "iter_files", return_value=iter([])),
        ):
            files_iter = iter([])
            # Should not raise
            player.summarize(files_iter)

        # With no files and no summaries, there should be no "Suggested playback ordering" section.
        msgs = [c[0][0] for c in mock_logger.info.call_args_list]
        self.assertFalse(any("Suggested playback ordering" in m for m in msgs))

    # ---- sorting-focused tests for suggested playback ordering ----

    def test_ordering_uses_primary_timestamp_key(self):
        """
        Verifies that sorted_for_playback orders summaries by the primary timestamp key
        (sort_ts), so a file with an earlier sort_ts appears before one with a later sort_ts,
        regardless of file name.
        """
        t1 = np.datetime64("2024-01-01T00:00:00")
        t2 = np.datetime64("2024-01-01T00:00:10")

        s_early = _PacketFileSummary(Path("z_late.adara"))
        s_late = _PacketFileSummary(Path("a_early.adara"))

        # Both event-bearing: BANKED_EVENT_TYPE is in EVENT_TYPES
        pkt_early = MagicMock(spec=Packet)
        pkt_early.timestamp = t1
        pkt_early.packet_type = Packet.Type.BANKED_EVENT_TYPE

        pkt_late = MagicMock(spec=Packet)
        pkt_late.timestamp = t2
        pkt_late.packet_type = Packet.Type.BANKED_EVENT_TYPE

        s_early.accumulate(pkt_early)
        s_late.accumulate(pkt_late)

        ordered = _PacketFileSummary.sorted_for_playback([s_late, s_early])

        self.assertEqual(
            [s.file_path.name for s in ordered],
            [s_early.file_path.name, s_late.file_path.name],
        )

    def test_metadata_files_before_event_files(self):
        """
        Ensures that when two files share the same primary timestamp, a metadata-only file
        (is_events_file == False) is ordered before an events file (is_events_file == True)
        due to the meta_flag secondary sort key.
        """
        t = np.datetime64("2024-01-01T00:00:00")

        # Metadata-only file: RUN_INFO_TYPE (non-event, but controlling metadata)
        s_meta = _PacketFileSummary(Path("meta.adara"))
        pkt_meta = MagicMock(spec=Packet)
        pkt_meta.timestamp = t
        pkt_meta.packet_type = Packet.Type.RUN_INFO_TYPE
        s_meta.accumulate(pkt_meta)

        # Events file: BANKED_EVENT_TYPE
        s_events = _PacketFileSummary(Path("events.adara"))
        pkt_events = MagicMock(spec=Packet)
        pkt_events.timestamp = t
        pkt_events.packet_type = Packet.Type.BANKED_EVENT_TYPE
        s_events.accumulate(pkt_events)

        ordered = _PacketFileSummary.sorted_for_playback([s_events, s_meta])

        self.assertEqual(
            [s.file_path.name for s in ordered],
            [s_meta.file_path.name, s_events.file_path.name],
        )

    def test_filename_used_as_tiebreaker(self):
        """
        Confirms that when both primary timestamp and metadata/event classification are equal,
        sorted_for_playback uses the file_path.name as a tertiary key so filenames are ordered
        lexicographically.
        """
        t = np.datetime64("2024-01-01T00:00:00")

        # Two metadata-only files with same timestamp/type
        s_a = _PacketFileSummary(Path("a_first.adara"))
        pkt_a = MagicMock(spec=Packet)
        pkt_a.timestamp = t
        pkt_a.packet_type = Packet.Type.RUN_INFO_TYPE
        s_a.accumulate(pkt_a)

        s_b = _PacketFileSummary(Path("b_second.adara"))
        pkt_b = MagicMock(spec=Packet)
        pkt_b.timestamp = t
        pkt_b.packet_type = Packet.Type.RUN_INFO_TYPE
        s_b.accumulate(pkt_b)

        ordered = _PacketFileSummary.sorted_for_playback([s_b, s_a])

        self.assertEqual(
            [s.file_path.name for s in ordered],
            ["a_first.adara", "b_second.adara"],
        )

    def test_ordering_reflected_in_logged_summary(self):
        """
        Checks that the sequence of file names logged under 'Suggested playback ordering'
        matches the order returned by _PacketFileSummary.sorted_for_playback, exercising the
        integration between the sorter and the summarize logger.
        """
        player = Player()

        p = Path("/fake/path/input.adara")

        def fake_iter_files(*args, **kwargs):
            file_callback = kwargs.get("file_callback")
            if file_callback:
                file_callback(p)
            return iter([None])

        # Two mock summaries in a known order
        s1 = MagicMock()
        s1.file_path = Path("zeta.adara")
        s1.is_events_file = True

        s2 = MagicMock()
        s2.file_path = Path("alpha.adara")
        s2.is_events_file = False

        with (
            patch("packet_player._logger") as mock_logger,
            patch.object(Player, "iter_files", side_effect=fake_iter_files),
            patch(
                "packet_player._PacketFileSummary.sorted_for_playback",
                return_value=[s1, s2],
            ),
        ):
            files_iter = iter([p])
            player.summarize(files_iter)

        msgs = [c[0][0] for c in mock_logger.info.call_args_list]

        # Find positions of the two names in the log
        idx_s1 = next(i for i, m in enumerate(msgs) if "zeta.adara" in m)
        idx_s2 = next(i for i, m in enumerate(msgs) if "alpha.adara" in m)

        # Order in logs should reflect order from sorted_for_playback
        self.assertLess(idx_s1, idx_s2)

    def test__summarize_returns_summaries_without_logging(self):
        """The internal method `_summarize` should drive iter_files, build summaries, and not log."""
        player = Player()

        p = Path("/fake/path/file1.adara")

        pkt1 = MagicMock(spec=Packet)
        pkt1.timestamp = np.datetime64("2024-01-01T00:00:00")
        pkt1.packet_type = Packet.Type.BANKED_EVENT_TYPE

        def fake_iter_files(*args, **kwargs):
            file_callback = kwargs.get("file_callback")
            packet_callback = kwargs.get("packet_callback")
            if file_callback:
                file_callback(p)
            if packet_callback:
                packet_callback(pkt1)
            return iter([None])

        with (
            patch("packet_player._logger") as mock_logger,
            patch.object(Player, "iter_files", side_effect=fake_iter_files),
        ):
            files_iter = iter([p])
            summaries = player._summarize(files_iter)

        self.assertEqual(len(summaries), 1)
        self.assertEqual(summaries[0].file_path, p)
        # _summarize itself should not have emitted any info logs
        self.assertEqual(mock_logger.info.call_count, 0)


if __name__ == "__main__":
    unittest.main()
