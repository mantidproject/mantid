"""
Unit tests for player_config module and the `packet_player.initialize()` pattern.
"""

import logging
import multiprocessing
import os
from pathlib import Path
import sys
import tempfile
import textwrap
import unittest
from unittest.mock import patch

import player_config
import packet_player
from packet_player import Packet, Player
from session_server import SessionServer
from adara_player_test_helpers import apply_test_config


# ---------------------------------------------------------------------------
# Minimal config YAML used across multiple test classes.
# ---------------------------------------------------------------------------
MINIMAL_CONFIG = textwrap.dedent("""\
    server:
      name: 'test_player'
      address: '127.0.0.1:31415'
      buffer_MB: 64
      socket_timeout: 1.0
    source:
      address: '127.0.0.1:31415'
    playback:
      handshake: none
      handshake_timeout: 10.0
      rate: 'unlimited'
      timestamps:
        offset: none
      ignore_packets: []
      packet_ordering: yaml
      packet_glob: '*.adara'
    record:
      transfer_limit: 16000
      file_output: multi_packet
    summarize:
      timestamp_per_type: false
      timestamp_resolution: 0.017
    logging:
      format: '%(asctime)s: %(levelname)s: %(message)s'
      level: INFO
      filename: ''
      packet_summary: "type: {type:#06x}: {timestamp}, size: {size:>7}, CRC: {CRC:#010x}"
""")


def _write_config(tmpdir: str, content: str, name: str = "config.yml") -> Path:
    """Write config content to a file inside *tmpdir* and return its Path."""
    path = Path(tmpdir) / name
    path.write_text(content)
    return path


class TestPlayerConfigLifecycle(unittest.TestCase):
    """Tests for player_config.load_config / get_config lifecycle."""

    def setUp(self):
        player_config.reset()
        self._tmpdir = tempfile.TemporaryDirectory()

    def tearDown(self):
        player_config.reset()
        self._tmpdir.cleanup()

    def test_load_config_success(self):
        """load_config populates get_config() and returns the dict."""
        path = _write_config(self._tmpdir.name, MINIMAL_CONFIG)
        result = player_config.load_config(path)
        self.assertIs(result, player_config.get_config())
        self.assertEqual(result["server"]["name"], "test_player")

    def test_load_config_nonexistent_file(self):
        """load_config raises SystemExit for missing file."""
        with self.assertRaises(SystemExit):
            player_config.load_config(Path(self._tmpdir.name) / "nonexistent.yml")

    def test_load_config_double_load_raises(self):
        """load_config raises RuntimeError if called twice."""
        path = _write_config(self._tmpdir.name, MINIMAL_CONFIG)
        player_config.load_config(path)
        with self.assertRaises(RuntimeError):
            player_config.load_config(path)

    def test_get_config_before_load_raises(self):
        """get_config raises RuntimeError if load_config not called."""
        with self.assertRaises(RuntimeError):
            player_config.get_config()

    def test_is_loaded(self):
        """is_loaded reflects whether config has been loaded."""
        self.assertFalse(player_config.is_loaded())
        path = _write_config(self._tmpdir.name, MINIMAL_CONFIG)
        player_config.load_config(path)
        self.assertTrue(player_config.is_loaded())

    def test_reset(self):
        """reset() allows a subsequent load_config to succeed."""
        path = _write_config(self._tmpdir.name, MINIMAL_CONFIG)
        player_config.load_config(path)
        self.assertTrue(player_config.is_loaded())
        player_config.reset()
        self.assertFalse(player_config.is_loaded())
        # Should be able to load again
        player_config.load_config(path)
        self.assertTrue(player_config.is_loaded())


class TestYAMLEnvSubstitution(unittest.TestCase):
    """Tests for ${ENV_VAR} substitution in YAML loading."""

    def setUp(self):
        self._tmpdir = tempfile.TemporaryDirectory()

    def tearDown(self):
        self._tmpdir.cleanup()

    def test_env_substitution(self):
        """${ENV_VAR} in YAML is replaced from os.environ."""
        os.environ["_TEST_ADARA_VAR"] = "substituted_value"
        try:
            content = "test_key: ${_TEST_ADARA_VAR}\n"
            path = _write_config(self._tmpdir.name, content)
            result = player_config.yaml_load(path)
            self.assertEqual(result["test_key"], "substituted_value")
        finally:
            os.environ.pop("_TEST_ADARA_VAR", None)

    def test_env_substitution_missing_var_raises(self):
        """${UNDEFINED_VAR} in YAML raises RuntimeError."""
        os.environ.pop("_MISSING_ADARA_VAR_XYZ", None)
        content = "test_key: ${_MISSING_ADARA_VAR_XYZ}\n"
        path = _write_config(self._tmpdir.name, content)
        with self.assertRaises(RuntimeError):
            player_config.yaml_load(path)

    def test_env_substitution_multiple_vars(self):
        """Multiple ${ENV_VAR} references in one value are all replaced."""
        os.environ["_TEST_A"] = "hello"
        os.environ["_TEST_B"] = "world"
        try:
            content = "test_key: ${_TEST_A}_${_TEST_B}\n"
            path = _write_config(self._tmpdir.name, content)
            result = player_config.yaml_load(path)
            self.assertEqual(result["test_key"], "hello_world")
        finally:
            os.environ.pop("_TEST_A", None)
            os.environ.pop("_TEST_B", None)


class TestLoggingHelpers(unittest.TestCase):
    """Tests for player_config logging configuration helpers."""

    def setUp(self):
        player_config.reset()
        self._tmpdir = tempfile.TemporaryDirectory()

    def tearDown(self):
        player_config.reset()
        self._tmpdir.cleanup()

    def _load_minimal_config(self):
        apply_test_config()

    def test_configure_logger_console(self):
        """configure_logger with console=True attaches StreamHandler."""
        self._load_minimal_config()
        logger = logging.getLogger("test_console_logger")
        player_config.configure_logger(logger, console=True)
        self.assertTrue(any(isinstance(h, logging.StreamHandler) for h in logger.handlers))

    def test_configure_logger_replaces_handlers(self):
        """configure_logger removes pre-existing handlers."""
        self._load_minimal_config()
        logger = logging.getLogger("test_replace_logger")
        old_handler = logging.StreamHandler()
        logger.addHandler(old_handler)
        player_config.configure_logger(logger, console=True)
        self.assertNotIn(old_handler, logger.handlers)

    def test_configure_logger_sets_level(self):
        """configure_logger sets the logger level from config."""
        self._load_minimal_config()
        logger = logging.getLogger("test_level_logger")
        player_config.configure_logger(logger, console=True)
        self.assertEqual(logger.level, logging.INFO)

    def test_get_log_file_path_for_PID_empty_template(self):
        """get_log_file_path_for_PID returns None when template is empty."""
        self._load_minimal_config()
        self.assertIsNone(player_config.get_log_file_path_for_PID(12345))

    def test_get_log_file_path_for_PID_with_template(self):
        """get_log_file_path_for_PID substitutes {PID} in template."""
        # Use a config with a filename template pointing to our temp directory
        apply_test_config({"logging.filename": f"{self._tmpdir.name}/test_{{PID}}.log"})

        result = player_config.get_log_file_path_for_PID(99999)
        self.assertIsNotNone(result)
        self.assertIn("99999", str(result))

    def test_get_log_formatter(self):
        """get_log_formatter returns a Formatter with the configured format."""
        self._load_minimal_config()
        formatter = player_config.get_log_formatter()
        self.assertIsInstance(formatter, logging.Formatter)

    def test_logging_helpers_before_config_raises(self):
        """Logging helpers raise RuntimeError before config is loaded."""
        player_config.reset()
        with self.assertRaises(RuntimeError):
            player_config.get_log_formatter()
        with self.assertRaises(RuntimeError):
            player_config.get_log_file_path_for_PID(1)
        with self.assertRaises(RuntimeError):
            player_config.configure_logger(logging.getLogger("test"), console=True)


class TestInitializePattern(unittest.TestCase):
    """
    Tests for the packet_player.initialize() pattern:
    verify that class-level constants are populated from config.
    """

    def setUp(self):
        player_config.reset()

    def tearDown(self):
        player_config.reset()

    def test_initialize_sets_packet_str_format(self):
        """initialize() sets Packet.STR_FORMAT from config."""
        apply_test_config()

        expected = "type: {type:#06x}: {timestamp}, size: {size:>7}, CRC: {CRC:#010x}"
        self.assertEqual(Packet.STR_FORMAT, expected)

    def test_initialize_sets_player_constants(self):
        """initialize() sets all Player class-level constants."""
        apply_test_config()

        self.assertEqual(Player.SOCKET_TIMEOUT, 1.0)
        self.assertEqual(Player.HANDSHAKE_TIMEOUT, 10.0)
        self.assertEqual(Player.PLAYBACK_HANDSHAKE, "none")
        self.assertEqual(Player.TRANSFER_LIMIT_MB, 16000)
        self.assertEqual(Player.PACKET_ORDERING_SCHEME, "none")
        self.assertFalse(Player.SUMMARIZE_TIMESTAMP_PER_TYPE)
        self.assertEqual(Player.PACKET_GLOB, "*.adara")

    def test_initialize_sets_summary_timestamp_resolution(self):
        """initialize() sets _PacketFileSummary.TIMESTAMP_RESOLUTION."""
        apply_test_config()

        summary_cls = packet_player._PacketFileSummary
        self.assertAlmostEqual(summary_cls.TIMESTAMP_RESOLUTION, 0.017)

    def test_initialize_before_config_raises(self):
        """initialize() raises RuntimeError if config is not loaded."""
        player_config.reset()
        with self.assertRaises(RuntimeError):
            packet_player.initialize()


class TestConfigCLIIntegration(unittest.TestCase):
    """
    Integration test: verify that --config is required and
    properly wired through parse_args.
    """

    def setUp(self):
        self._tmpdir = tempfile.TemporaryDirectory()

    def tearDown(self):
        self._tmpdir.cleanup()

    def test_parse_args_requires_config(self):
        """parse_args exits if --config is not provided."""
        with patch.object(sys, "argv", ["prog", "some_glob"]):
            with self.assertRaises(SystemExit):
                SessionServer.parse_args()

    def test_parse_args_accepts_config(self):
        """parse_args succeeds when --config is provided."""
        config_path = str(Path(self._tmpdir.name) / "test.yml")
        with patch.object(sys, "argv", ["prog", "-c", config_path, "some_glob"]):
            args = SessionServer.parse_args()
            self.assertEqual(args.config, config_path)

    def test_parse_args_config_with_all_options(self):
        """parse_args accepts --config alongside other flags."""
        config_path = str(Path(self._tmpdir.name) / "test.yml")
        with patch.object(sys, "argv", ["prog", "-c", config_path, "-r", "-s", "127.0.0.1:9999", "output_dir"]):
            args = SessionServer.parse_args()
            self.assertEqual(args.config, config_path)
            self.assertTrue(args.record)
            self.assertEqual(args.source_address, "127.0.0.1:9999")
            self.assertEqual(args.glob, "output_dir")


class TestConfigMultiProcessSafety(unittest.TestCase):
    """
    Verify that config and initialized class attributes
    are accessible in a forked child process.

    These tests explicitly use the "fork" start method because
    they verify that module-level state is inherited across fork().
    On macOS (which defaults to "spawn"), "spawn" requires pickling
    the target function — but that is not the semantics we are testing.
    """

    def setUp(self):
        player_config.reset()

    def tearDown(self):
        player_config.reset()

    @unittest.skipIf(
        "fork" not in multiprocessing.get_all_start_methods(),
        "fork start method not available on this platform",
    )
    def test_config_survives_fork(self):
        """Config loaded in parent is readable in forked child."""
        apply_test_config()

        def child_check(result_queue):
            try:
                cfg = player_config.get_config()
                result_queue.put(cfg.get("server", {}).get("name"))
            except Exception as e:
                result_queue.put(f"ERROR: {e}")

        ctx = multiprocessing.get_context("fork")
        q = ctx.Queue()
        p = ctx.Process(target=child_check, args=(q,))
        p.start()
        p.join(timeout=5)

        result = q.get_nowait()
        self.assertEqual(result, "adara_player")

    @unittest.skipIf(
        "fork" not in multiprocessing.get_all_start_methods(),
        "fork start method not available on this platform",
    )
    def test_initialized_constants_survive_fork(self):
        """Class constants set by initialize() are available in forked child."""
        apply_test_config()

        def child_check(result_queue):
            try:
                result_queue.put(Player.SOCKET_TIMEOUT)
            except Exception as e:
                result_queue.put(f"ERROR: {e}")

        ctx = multiprocessing.get_context("fork")
        q = ctx.Queue()
        p = ctx.Process(target=child_check, args=(q,))
        p.start()
        p.join(timeout=5)

        result = q.get_nowait()
        self.assertEqual(result, 1.0)


if __name__ == "__main__":
    unittest.main()
