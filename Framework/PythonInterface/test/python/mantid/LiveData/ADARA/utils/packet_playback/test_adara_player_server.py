import argparse
import inspect
from pathlib import Path
import signal
import sys
from tempfile import TemporaryDirectory

import player_config
from session_server import SessionServer, SessionHandler, SessionTCPServer, SessionUDSServer, main
from adara_player_test_helpers import apply_test_config

import unittest
from unittest import mock


class _DummyBase:
    def process_request(self, request, client_address):
        pass


class _SessionServer(SessionServer, _DummyBase):
    def __init__(self, *args, commandline_args):
        super().__init__(*args, commandline_args=commandline_args)


class TestSessionServer(unittest.TestCase):
    def setUp(self):
        super().setUp()
        self.temp_dir = self.enterContext(TemporaryDirectory(prefix=f"{__name__}_"))
        apply_test_config()

        # Patch UnixGlob.parse and multi_glob used by _parse_file_args
        self._parse_patcher = mock.patch(
            "session_server.UnixGlob.parse",
            return_value=(Path(f"{self.temp_dir}/sessions"), ["*.adara"]),
        )
        self._multiglob_patcher = mock.patch(
            "session_server.UnixGlob.multi_glob",
            return_value=iter([]),
        )
        self.mock_parse = self._parse_patcher.start()
        self.mock_multi = self._multiglob_patcher.start()
        self.addCleanup(self._parse_patcher.stop)
        self.addCleanup(self._multiglob_patcher.stop)

        self.args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=False,
            source_address=None,
            dry_run=False,
            glob=f"{self.temp_dir}/sessions",
            files=None,
            summarize=False,
        )

        self.server = _SessionServer(commandline_args=self.args)

    def tearDown(self):
        player_config.reset()
        super().tearDown()

    def test_parse_file_args_rejects_files_in_record_mode(self):
        """Test that _parse_file_args raises when --files is used together with record mode."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=True,
            source_address=None,
            dry_run=False,
            glob=f"{self.temp_dir}/sessions",
            files=f"{self.temp_dir}/list.yml",
            summarize=False,
        )

        with self.assertRaises(RuntimeError) as context:
            SessionServer._parse_file_args(self.server, args)

        self.assertIn("`--files` cannot be used with record mode.", str(context.exception))

    def test_parse_file_args_requires_glob_in_record_mode(self):
        """Test that _parse_file_args requires a target directory (glob) when in record mode."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=True,
            source_address=None,
            dry_run=False,
            glob=None,
            files=None,
            summarize=False,
        )

        with self.assertRaises(RuntimeError) as context:
            SessionServer._parse_file_args(self.server, args)

        self.assertIn("record mode requires a target directory", str(context.exception))

    def test_parse_file_args_rejects_both_glob_and_files(self):
        """Test that _parse_file_args raises when both a glob argument and --files are provided."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=False,
            source_address=None,
            dry_run=False,
            glob=f"{self.temp_dir}/sessions",
            files=f"{self.temp_dir}/list.yml",
            summarize=False,
        )

        with self.assertRaises(RuntimeError) as context:
            SessionServer._parse_file_args(self.server, args)

        self.assertIn("only one of FILES or glob should be specified", str(context.exception))

    def test_parse_file_args_files_yaml_must_be_mapping(self):
        """Test that _parse_file_args rejects a --files YAML that is not a mapping."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=False,
            source_address=None,
            dry_run=False,
            glob=None,
            files=f"{self.temp_dir}/list.yml",
            summarize=False,
        )

        with (
            mock.patch("builtins.open", mock.mock_open(read_data="dummy")),
            mock.patch("session_server.yaml_load", return_value=["not", "a", "dict"]),
        ):
            with self.assertRaises(RuntimeError) as context:
                SessionServer._parse_file_args(self.server, args)

        self.assertIn("YAML must be a mapping with keys 'base_path' and 'sessions'", str(context.exception))

    def test_parse_file_args_files_yaml_requires_base_path_and_sessions_keys(self):
        """Test that _parse_file_args raises if --files YAML is missing base_path or sessions keys."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=False,
            source_address=None,
            dry_run=False,
            glob=None,
            files=f"{self.temp_dir}/list.yml",
            summarize=False,
        )

        # Missing 'base_path' key
        yaml_data = {"sessions": []}

        with (
            mock.patch("builtins.open", mock.mock_open(read_data="dummy")),
            mock.patch("session_server.yaml_load", return_value=yaml_data),
        ):
            with self.assertRaises(RuntimeError) as context:
                SessionServer._parse_file_args(self.server, args)

        self.assertIn("missing required key", str(context.exception))
        self.assertIn("base_path", str(context.exception))

    def test_parse_file_args_files_yaml_requires_string_base_path(self):
        """Test that _parse_file_args raises if base_path in --files YAML is not a string."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=False,
            source_address=None,
            dry_run=False,
            glob=None,
            files=f"{self.temp_dir}/list.yml",
            summarize=False,
        )

        yaml_data = {"base_path": 123, "sessions": [["file1.adara"]]}

        with (
            mock.patch("builtins.open", mock.mock_open(read_data="dummy")),
            mock.patch("session_server.yaml_load", return_value=yaml_data),
        ):
            with self.assertRaises(RuntimeError) as context:
                SessionServer._parse_file_args(self.server, args)

        self.assertIn("`base_path` in `--files` YAML must be a string", str(context.exception))

    def test_parse_file_args_files_yaml_requires_non_empty_sessions_list(self):
        """Test that _parse_file_args raises if sessions in --files YAML is not a non-empty list."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=False,
            source_address=None,
            dry_run=False,
            glob=None,
            files=f"{self.temp_dir}/list.yml",
            summarize=False,
        )

        yaml_data = {"base_path": "/data/base", "sessions": []}

        with (
            mock.patch("builtins.open", mock.mock_open(read_data="dummy")),
            mock.patch("session_server.yaml_load", return_value=yaml_data),
        ):
            with self.assertRaises(RuntimeError) as context:
                SessionServer._parse_file_args(self.server, args)

        self.assertIn("`sessions` in `--files` YAML must be a non-empty list of session file-lists", str(context.exception))

    def test_parse_file_args_files_yaml_sets_glob_and_sessions_for_valid_input(self):
        """Test that _parse_file_args sets _glob and _sessions correctly for a valid --files YAML."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=False,
            source_address=None,
            dry_run=False,
            glob=None,
            files=f"{self.temp_dir}/list.yml",
            summarize=False,
        )

        yaml_data = {
            "base_path": "/data/base",
            "sessions": [
                ["s1_file1.adara"],
                ["s2_file1.adara", "s2_file2.adara"],
            ],
        }

        with (
            mock.patch("builtins.open", mock.mock_open(read_data="dummy")),
            mock.patch("session_server.yaml_load", return_value=yaml_data),
        ):
            SessionServer._parse_file_args(self.server, args)

        self.assertEqual(self.server.glob, (Path("/data/base"), [""]))
        outer = list(self.server._sessions)
        self.assertEqual(len(outer), 2)

    def test_parse_file_args_files_yaml_resolves_relative_and_absolute_paths(self):
        """Test that _parse_file_args resolves relative session entries under base_path and preserves absolute paths."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=False,
            source_address=None,
            dry_run=False,
            glob=None,
            files=f"{self.temp_dir}/list.yml",
            summarize=False,
        )

        yaml_data = {
            "base_path": "/data/base",
            "sessions": [
                ["rel1.adara", "/abs/other.adara"],
            ],
        }

        with (
            mock.patch("builtins.open", mock.mock_open(read_data="dummy")),
            mock.patch("session_server.yaml_load", return_value=yaml_data),
        ):
            SessionServer._parse_file_args(self.server, args)

        outer = list(self.server._sessions)
        self.assertEqual(len(outer), 1)
        first_session_files = list(outer[0])
        self.assertEqual(
            first_session_files,
            [Path("/data/base") / "rel1.adara", Path("/abs/other.adara")],
        )

    def test_parse_file_args_sets_yaml_input_mode(self):
        """Test that _parse_file_args sets `server._yaml_input_mode` when required."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=False,
            source_address=None,
            dry_run=False,
            glob=None,
            files=f"{self.temp_dir}/list.yml",
            summarize=False,
        )

        yaml_data = {"base_path": "/data/base", "sessions": [["file1.adara"]]}

        with (
            mock.patch("builtins.open", mock.mock_open(read_data="dummy")),
            mock.patch("session_server.yaml_load", return_value=yaml_data),
        ):
            SessionServer._parse_file_args(self.server, args)

        self.assertTrue(self.server.yaml_input_mode)

    def test_parse_file_args_clears_yaml_input_mode(self):
        """Test that _parse_file_args does not set `server._yaml_input_mode` unless required."""
        # Start with YAML input mode enabled to ensure the call without '--files' clears it.
        self.server._yaml_input_mode = True

        # No '--files' arg, only a glob: should leave yaml_input_mode False after parsing.
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=False,
            source_address=None,
            dry_run=False,
            glob=f"{self.temp_dir}/sessions",
            files=None,
            summarize=False,
        )

        SessionServer._parse_file_args(self.server, args)

        self.assertFalse(self.server.yaml_input_mode)

    def test_parse_file_args_requires_glob_when_files_not_specified(self):
        """Test that _parse_file_args raises if neither --files nor a glob argument is provided in play/summarize mode."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=False,
            source_address=None,
            dry_run=False,
            glob=None,
            files=None,
            summarize=False,
        )

        with self.assertRaises(RuntimeError) as context:
            SessionServer._parse_file_args(self.server, args)

        self.assertIn("glob argument is required when `--files` is not specified", str(context.exception))

    def test_parse_file_args_uses_multisession_root_when_patterns_empty(self):
        """Test that _parse_file_args treats an empty-pattern glob as a multi-session root and builds per-directory session iterators."""
        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
            base = Path(tmpdir)

            # Create real session directories that match the 4-digit pattern
            (base / "0001").mkdir()
            (base / "0002").mkdir()
            # A directory that should be ignored by the \d{4} filter
            (base / "abcd").mkdir()

            args = argparse.Namespace(
                config=f"{self.temp_dir}/config.yml",
                record=False,
                source_address=None,
                dry_run=False,
                glob=str(base),
                files=None,
                summarize=False,
            )

            with (
                # Force patterns == [""] so we enter the multi-session root branch
                mock.patch("session_server.UnixGlob.parse", return_value=(base, [""])),
                mock.patch("session_server.Player.PACKET_GLOB", "*.adara"),
                # Have multi_glob yield something per directory so we can see it is called
                mock.patch(
                    "session_server.UnixGlob.multi_glob",
                    side_effect=lambda sp, patterns: iter([sp / "dummy.adara"]),
                ) as mock_multi,
            ):
                SessionServer._parse_file_args(self.server, args)

                # _glob encodes the base-only multi-session root.
                self.assertEqual(self.server.glob, (base, [""]))

                # There should be one inner iterator per valid 4-digit directory: 0001 and 0002.
                outer = list(self.server._sessions)
                self.assertEqual(len(outer), 2)

                # Force consumption of inner iterators just to prove they are iterators over paths
                all_files = [list(it) for it in outer]
                self.assertEqual(
                    all_files,
                    [[base / "0001" / "dummy.adara"], [base / "0002" / "dummy.adara"]],
                )

            # multi_glob is invoked once per session directory with PACKET_GLOB.
            self.assertEqual(mock_multi.call_count, 2)
            mock_multi.assert_any_call(base / "0001", ["*.adara"])
            mock_multi.assert_any_call(base / "0002", ["*.adara"])

    def test_parse_file_args_uses_single_session_when_patterns_nonempty(self):
        """Test that _parse_file_args creates a single logical session iterator when glob patterns are non-empty."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=False,
            source_address=None,
            dry_run=False,
            glob=f"{self.temp_dir}/sessions",
            files=None,
            summarize=False,
        )

        with (
            mock.patch(
                "session_server.UnixGlob.parse",
                return_value=(Path(f"{self.temp_dir}/sessions"), ["*.adara"]),
            ) as mock_parse,
            mock.patch(
                "session_server.UnixGlob.multi_glob",
                return_value=iter([]),
            ) as mock_multi,
        ):
            SessionServer._parse_file_args(self.server, args)

        mock_parse.assert_called_once_with(f"{self.temp_dir}/sessions")
        self.assertEqual(self.server.glob, (Path(f"{self.temp_dir}/sessions"), ["*.adara"]))
        outer = list(self.server._sessions)
        self.assertEqual(len(outer), 1)
        mock_multi.assert_called_once_with(Path(f"{self.temp_dir}/sessions"), ["*.adara"])

    def test_init_initializes_server(self):
        """Test that SessionServer.__init__ properly initializes the server with given arguments."""
        self.assertEqual(self.server.record, self.args.record)
        self.assertEqual(self.server.dry_run, self.args.dry_run)
        self.assertIsNotNone(self.server.glob)

    def test_parseargs_returns_namespace(self):
        """Test that SessionServer.parseargs returns an argparse.Namespace with expected attributes."""
        with mock.patch.object(sys, "argv", ["adara_player", "-c", "dummy.yml"]):
            args = SessionServer.parse_args()

        self.assertIsInstance(args, argparse.Namespace)
        self.assertTrue(hasattr(args, "config"))
        self.assertTrue(hasattr(args, "record"))
        self.assertTrue(hasattr(args, "source_address"))
        self.assertTrue(hasattr(args, "dry_run"))

    def test_property_record(self):
        """Test that the record property accurately reflects the argument state."""
        self.server._record = True
        self.assertTrue(self.server.record)

    def test_property_sourceaddress(self):
        """Test that the sourceaddress property returns the proper Path or tuple."""
        self.server._source_address = Path("/some/path")
        self.assertEqual(self.server.source_address, Path("/some/path"))

    def test_property_dryrun(self):
        """Test that the dryrun property accurately reflects the argument state."""
        self.server._dry_run = True
        self.assertTrue(self.server.dry_run)

    def test_property_glob(self):
        """Test that the glob property returns the correct parsed glob tuple."""
        self.assertEqual(self.server.glob, (Path(f"{self.temp_dir}/sessions"), ["*.adara"]))

    def test_property_basepath(self):
        """Test that the basepath property returns the expected base directory."""
        self.assertEqual(self.server.base_path, Path(f"{self.temp_dir}/sessions"))

    def test_property_ismultisession(self):
        """Test that ismultisession property correctly indicates multi-session mode."""
        # Single-session glob is identified correctly.
        self.server._glob = (Path(f"{self.temp_dir}/sessions"), ["*.pkt"])
        self.assertFalse(self.server.is_multi_session)

        # Multi-session glob is identified correctly.
        self.server._glob = (Path(f"{self.temp_dir}/sessions"), [""])
        self.assertTrue(self.server.is_multi_session)

    def test_claim_session_index(self):
        """Test that _claim_session_index increments and returns the session index."""
        self.server._session_number = mock.MagicMock()
        self.server._session_number.value = 1

        # We need to simulate the lock context manager
        self.server._session_number.get_lock.return_value.__enter__.return_value = None

        idx1 = self.server._claim_session_index()
        self.assertEqual(idx1, 1)
        self.assertEqual(self.server._session_number.value, 2)
        self.server._session_number.get_lock.assert_called_once()

    def test_session_path_for_record_mode(self):
        """Test that _session_path_for validates record-mode session directories."""
        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
            base = Path(tmpdir)
            self.server._glob = (base, [""])  # base_path for record mode
            self.server._record = True

            # Positive: directory does not exist or is empty
            session_idx = 26
            expected = base / f"{session_idx:04d}"
            result = self.server._session_path_for(session_idx)
            self.assertEqual(result, expected)

        with TemporaryDirectory(prefix=f"{__name__}_") as tmpdir:
            base = Path(tmpdir)
            self.server._glob = (base, [""])
            self.server._record = True

            # Negative: directory exists and is non-empty
            session_idx = 42
            bad_path = base / f"{session_idx:04d}"
            bad_path.mkdir(parents=True, exist_ok=True)
            (bad_path / "something_in_the_directory.adara").touch()

            with self.assertRaises(RuntimeError) as context:
                _ = self.server._session_path_for(session_idx)
            self.assertIn("is not empty", str(context.exception))

    def test_session_files_for_play_mode(self):
        """Test that _session_files_for returns correct file iterator for play mode."""
        # Create a server with mocked sessions
        self.server._record = False
        with self.assertRaises(RuntimeError) as context:
            _ = self.server._session_path_for(1)
        self.assertIn("`session_path` is only used in 'record' mode", str(context.exception))

        # Create dummy session iterators
        session1_files = [Path("s1f1")]
        session2_files = [Path("s2f1"), Path("s2f2")]

        # We need _sessions to be an iterator that yields iterators
        self.server._sessions = iter([iter(session1_files), iter(session2_files)])

        # Retrieve session 1
        # Note: _session_files_for uses islice, which consumes the main iterator
        result1 = list(self.server._session_files_for(1))
        self.assertEqual(result1, session1_files)

        # Re-create server sessions iterator for next test since it was partially consumed
        self.server._sessions = iter([iter(session1_files), iter(session2_files)])

        # Retrieve session 2
        result2 = list(self.server._session_files_for(2))
        self.assertEqual(result2, session2_files)

    def test_processrequest_accepts_and_logs_session(self):
        """Test that processrequest handles client requests and logs each new session."""
        with (
            mock.patch.object(inspect.getmodule(SessionServer), "_logger") as mock_logger,
            mock.patch.object(_DummyBase, "process_request") as mock_super,
        ):
            self.server._session_number = mock.MagicMock()
            self.server._session_number.value = 1
            self.server.process_request(mock.sentinel.request, mock.sentinel.address)
            mock_logger.info.assert_called_once()
            self.assertIn("Accepting client connection for session", mock_logger.info.call_args_list[0][0][0])
            mock_super.assert_called_once_with(mock.sentinel.request, mock.sentinel.address)

    def test_signalhandler_handles_shutdown_signal(self):
        """Test that signal_handler processes shutdown signals gracefully."""
        with (
            mock.patch("os.getpid", return_value=1234) as mock_getpid,
            mock.patch("os.getpgid", return_value=9999) as mock_getpgid,
            mock.patch("psutil.process_iter") as mock_proc_iter,
            mock.patch("os.kill") as mock_kill,
            mock.patch("signal.signal"),
            mock.patch("signal.alarm"),
        ):
            # Two processes in the group: one is self, one is child
            proc_self = mock.Mock(pid=1234)
            proc_child = mock.Mock(pid=5678)
            mock_proc_iter.return_value = [proc_self, proc_child]

            self.server.shutdown = mock.Mock()
            self.server.signal_handler(signal.SIGINT, None)

            mock_getpid.assert_called()

            # getpgid should be called multiple times; ensure it was called for the server PID
            mock_getpgid.assert_any_call(1234)

            # ensure child in same group gets signalled
            mock_kill.assert_any_call(5678, signal.SIGINT)

            # ensure shutdown requested
            self.server.shutdown.assert_called_once()


class TestSessionHandler(unittest.TestCase):
    def setUp(self):
        super().setUp()
        self.temp_dir = self.enterContext(TemporaryDirectory(prefix=f"{__name__}_"))
        apply_test_config()

    def tearDown(self):
        player_config.reset()
        super().tearDown()

    def test_handle_calls_player_play_or_record(self):
        """Test that SessionHandler.handle invokes Player.play or Player.record as needed."""
        mock_request = mock.Mock()

        # Play mode
        mock_server = mock.Mock()
        mock_server.record = False
        mock_server.source_address = None
        mock_server.yaml_input_mode = False
        mock_server._claim_session_index.return_value = 1
        mock_server._session_files_for.return_value = mock.sentinel.session_files

        mock_server.dry_run = False

        with mock.patch("session_server.Player") as PlayerMock:
            player_inst = PlayerMock.return_value
            player_inst.play = mock.Mock()
            player_inst.record = mock.Mock()

            _handler = SessionHandler(mock_request, mock.sentinel.client_address, mock_server)

            player_inst.play.assert_called_once_with(mock_request, mock.sentinel.session_files, dry_run=False)
            player_inst.record.assert_not_called()
            mock_server._claim_session_index.assert_called_once()
            mock_server._session_files_for.assert_called_once_with(1)

        # Record mode
        mock_server = mock.Mock()
        mock_server.record = True
        mock_server.source_address = None
        mock_server.yaml_input_mode = False
        mock_server._claim_session_index.return_value = 2
        mock_server._session_path_for.return_value = mock.sentinel.session_path

        with mock.patch("session_server.Player") as PlayerMock:
            player_inst = PlayerMock.return_value
            player_inst.play = mock.Mock()
            player_inst.record = mock.Mock()

            _handler = SessionHandler(mock_request, mock.sentinel.client_address, mock_server)

            player_inst.record.assert_called_once_with(mock.sentinel.session_path, mock_request)
            player_inst.play.assert_not_called()
            mock_server._claim_session_index.assert_called_once()
            mock_server._session_path_for.assert_called_once_with(2)


class TestSessionTCPServer(unittest.TestCase):
    def setUp(self):
        super().setUp()
        self.temp_dir = self.enterContext(TemporaryDirectory(prefix=f"{__name__}_"))
        apply_test_config()

    def tearDown(self):
        player_config.reset()
        super().tearDown()

    def test_init_sets_up_tcp_server(self):
        """Test that SessionTCPServer is initialized with correct arguments and handler."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=False,
            summarize=False,
            source_address=None,
            dry_run=False,
            glob=f"{self.temp_dir}/sessions",
            files=None,
        )

        with (
            mock.patch("session_server.UnixGlob.parse", return_value=(Path(f"{self.temp_dir}/sessions"), ["*.pkt"])),
            mock.patch("session_server.UnixGlob.multi_glob", return_value=iter([])),
        ):
            server = SessionTCPServer(("localhost", 9999), commandline_args=args)

        self.assertIsInstance(server, SessionTCPServer)


class TestSessionUDSServer(unittest.TestCase):
    def setUp(self):
        super().setUp()
        self.temp_dir = self.enterContext(TemporaryDirectory(prefix=f"{__name__}_"))
        apply_test_config()

    def tearDown(self):
        player_config.reset()
        super().tearDown()

    def test_init_sets_up_uds_server(self):
        """Test that SessionUDSServer is initialized with correct arguments and handler."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            record=False,
            summarize=False,
            source_address=None,
            dry_run=False,
            glob=f"{self.temp_dir}/sessions",
            files=None,
        )

        with (
            TemporaryDirectory(prefix=f"{__name__}_") as tmpdir,
            mock.patch("session_server.UnixGlob.parse", return_value=(Path(f"{self.temp_dir}/sessions"), ["*.pkt"])),
            mock.patch("session_server.UnixGlob.multi_glob", return_value=iter([])),
        ):
            UDS_path = Path(tmpdir) / "sock"
            server = SessionUDSServer(UDS_path, commandline_args=args)
            self.assertIsInstance(server, SessionUDSServer)


class TestMainFunction(unittest.TestCase):
    def setUp(self):
        super().setUp()
        self.temp_dir = self.enterContext(TemporaryDirectory(prefix=f"{__name__}_"))

    def tearDown(self):
        player_config.reset()
        super().tearDown()

    @mock.patch("session_server.SessionTCPServer")
    @mock.patch("session_server.SessionUDSServer")
    @mock.patch("session_server.SessionServer.parse_args")
    @mock.patch("session_server.SocketAddress")
    @mock.patch("session_server.Player")
    @mock.patch("session_server.packet_player.initialize")
    @mock.patch("session_server.player_config.load_config")
    @mock.patch("session_server.player_config.configure_logger")
    def test_main_starts_correct_server_based_on_args(
        self, mock_configure_logger, mock_load_config, mock_initialize, Player, SocketAddress, parse_args, UDSServer, TCPServer
    ):
        """Test that main() chooses UDS/TCP server based on arguments and calls serve_forever."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            server_address=None,
            record=False,
            summarize=False,
            source_address=None,
            dry_run=False,
            glob=f"{self.temp_dir}/sessions",
            files=None,
        )
        parse_args.return_value = args

        # INET socket:
        SocketAddress.isUDSSocket.return_value = False
        UDSServer.return_value.__enter__.return_value.serve_forever = mock.Mock()
        TCPServer.return_value.__enter__.return_value.serve_forever = mock.Mock()
        main()
        self.assertFalse(UDSServer.return_value.__enter__.return_value.serve_forever.called)
        self.assertTrue(TCPServer.return_value.__enter__.return_value.serve_forever.called)

        # UDS socket:
        SocketAddress.isUDSSocket.return_value = True
        UDSServer.return_value.__enter__.return_value.serve_forever = mock.Mock()
        TCPServer.return_value.__enter__.return_value.serve_forever = mock.Mock()
        main()
        self.assertTrue(UDSServer.return_value.__enter__.return_value.serve_forever.called)
        self.assertFalse(TCPServer.return_value.__enter__.return_value.serve_forever.called)

    @mock.patch("signal.signal")
    @mock.patch("session_server.SessionTCPServer")
    @mock.patch("session_server.SessionServer.parse_args")
    @mock.patch("session_server.SocketAddress")
    @mock.patch("session_server.Player.get_server_address", return_value="127.0.0.1:31415")
    @mock.patch("session_server.packet_player.initialize")
    @mock.patch("session_server.player_config.load_config")
    @mock.patch("session_server.player_config.configure_logger")
    def test_main_signal_handlers_registered(
        self,
        mock_configure_logger,
        mock_load_config,
        mock_initialize,
        mock_get_server_address,
        mock_SocketAddress,
        mock_parse_args,
        mock_TCPServer,
        mock_sig_signal,
    ):
        """Test that main() sets up signal handlers for SIGINT and SIGTERM."""
        args = argparse.Namespace(
            config=f"{self.temp_dir}/config.yml",
            server_address=None,
            record=False,
            summarize=False,
            source_address=None,
            dry_run=False,
            glob=f"{self.temp_dir}/sessions",
            files=None,
        )
        mock_parse_args.return_value = args
        mock_SocketAddress.isUDSSocket.return_value = False
        mock_TCPServer.return_value.__enter__.return_value.serve_forever = mock.Mock()
        main()
        self.assertTrue(mock_sig_signal.called)
        calls = [call[0][0] for call in mock_sig_signal.call_args_list]
        self.assertIn(signal.SIGINT, calls)
        self.assertIn(signal.SIGTERM, calls)


if __name__ == "__main__":
    unittest.main()
