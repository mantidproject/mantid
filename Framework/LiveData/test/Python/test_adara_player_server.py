import argparse
import inspect
from pathlib import Path
import signal
import tempfile

from session_server import SessionServer, SessionHandler, SessionTCPServer, SessionUDSServer, main

import unittest
from unittest import mock


class TestSessionServer(unittest.TestCase):
    def setUp(self):
        self.manager = mock.Mock()
        self.args = argparse.Namespace(
            record=False,
            source_address=None,
            dry_run=False,
            glob="/tmp/sessions",
        )
        self.server = SessionServer(commandline_args=self.args, manager=self.manager)

    def test_init_initializes_server(self):
        """Test that SessionServer.__init__ properly initializes the server with given arguments."""
        self.assertEqual(self.server.record, self.args.record)
        self.assertEqual(self.server.dry_run, self.args.dry_run)
        self.assertIsNotNone(self.server.glob)

    def test_parseargs_returns_namespace(self):
        """Test that SessionServer.parseargs returns an argparse.Namespace with expected attributes."""
        args = SessionServer.parse_args()
        self.assertIsInstance(args, argparse.Namespace)
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
        self.assertEqual(self.server.glob, (Path("/tmp/sessions"), [""]))

    def test_property_basepath(self):
        """Test that the basepath property returns the expected base directory."""
        self.assertEqual(self.server.base_path, Path("/tmp/sessions"))

    def test_property_ismultisession(self):
        """Test that ismultisession property correctly indicates multi-session mode."""
        # Single-session glob is identified correctly.
        self.server._glob = (Path("/tmp/sessions"), ["*.pkt"])
        self.assertFalse(self.server.is_multi_session)

        # Multi-session glob is identified correctly.
        self.server._glob = (Path("/tmp/sessions"), [""])
        self.assertTrue(self.server.is_multi_session)

    def test_property_sessionnumber(self):
        """Test that sessionnumber property correctly tracks the current session number."""
        self.server._session_number = mock.MagicMock()
        self.server._session_number.value = 2
        self.assertEqual(self.server.session_number, 2)

    def test_nextsessionpath_returns_next_path(self):
        """Test that nextsessionpath generates or validates the appropriate session path."""
        with mock.patch.object(SessionServer, "is_multi_session", new_callable=mock.PropertyMock) as mock_is_multi_session:
            mock_is_multi_session.return_value = True
            self.server._session_number = mock.MagicMock()
            self.server._session_number.value = 54
            result = self.server.next_session_path()
            mock_is_multi_session.assert_called_once()
            self.assertIn(str(self.server._glob[0]), str(result))
            self.assertTrue(str(result).endswith("0054"))

    def test_nextsessionpath_error(self):
        """Test that nextsessionpath raises an exception if used in single-session mode."""
        with mock.patch.object(SessionServer, "is_multi_session", new_callable=mock.PropertyMock) as mock_is_multi_session:
            mock_is_multi_session.return_value = False
            with self.assertRaises(RuntimeError) as context:
                result = self.server.next_session_path()  # noqa: F841
            self.assertIn("a single session is expected", str(context.exception))

    def test_processrequest_accepts_and_logs_session(self):
        """Test that processrequest handles client requests and logs each new session."""
        with (
            mock.patch.object(super(SessionServer, self.server), "process_request") as mock_super,
            mock.patch.object(inspect.getModule(SessionServer), "console_logger") as mock_logger,
        ):
            self.server._session_number = mock.MagicMock()
            self.server._session_number.value = 1
            self.server.process_request(mock.sentinel.request, mock.sentinel.address)
            mock_logger.info.assert_called_once()
            self.assertIn("Accepting client connection for session", mock_logger.info.call_args_list[0][0][0])
            mock_super.assert_called_once_with(mock.sentinel.request, mock.sentinel.address)

    def test_signalhandler_handles_shutdown_signal(self):
        """Test that signalhandler processes shutdown signals gracefully."""
        with (
            mock.patch("os.getpid", return_value=mock.sentinel.PID) as mock_getpid,
            mock.patch("os.getpgid", return_value=mock.sentinel.PGID) as mock_getpgid,
            mock.patch("os.killpg") as killpg,
        ):
            self.server.signal_handler(signal.SIGINT, None)
            mock_getpid.assert_called_once()
            mock_getpgid.assert_called_once_with(mock.sentinel.PID)
            killpg.assert_called_once_with(mock.sentinel.PGID)


class TestSessionHandler(unittest.TestCase):
    def test_handle_calls_player_play_or_record(self):
        """Test that SessionHandler.handle invokes Player.play or Player.record as needed."""
        mock_server = mock.Mock(record=False, source_address=None, glob=(Path("/tmp/sessions"), [""]))
        mock_request = mock.Mock()
        handler = SessionHandler(mock_request, mock.sentinel.client_address, mock_server)
        with mock.patch.object(inspect.getmodule(SessionHandler), "Player") as PlayerMock:
            player_inst = PlayerMock.return_value
            player_inst.play = mock.Mock()
            player_inst.record = mock.Mock()
            handler.handle()
            player_inst.play.assert_called_once()
            player_inst.record.assert_not_called()
        mock_server.reset_mock()
        mock_request.reset_mock()

        mock_server.record = True
        with mock.patch.object(inspect.getmodule(SessionHandler), "Player") as PlayerMock:
            player_inst = PlayerMock.return_value
            player_inst.play = mock.Mock()
            player_inst.record = mock.Mock()
            handler.handle()
            player_inst.record.assert_called_once()
            player_inst.play.assert_not_called()


class TestSessionTCPServer(unittest.TestCase):
    def test_init_sets_up_tcp_server(self):
        """Test that SessionTCPServer is initialized with correct arguments and handler."""
        args = argparse.Namespace(record=False, source_address=None, dry_run=False, glob="/tmp/sessions")
        manager = mock.Mock()
        server = SessionTCPServer(("localhost", 9999), args, manager)
        self.assertIsInstance(server, SessionTCPServer)


class TestSessionUDSServer(unittest.TestCase):
    def test_init_sets_up_uds_server(self):
        """Test that SessionUDSServer is initialized with correct arguments and handler."""
        args = argparse.Namespace(record=False, source_address=None, dry_run=False, glob="/tmp/sessions")
        manager = mock.Mock()
        with tempfile.NamedTemporaryFile() as tf:
            server = SessionUDSServer(tf.name, args, manager)
            self.assertIsInstance(server, SessionUDSServer)


class TestMainFunction(unittest.TestCase):
    @mock.patch("session_server.SessionTCPServer")
    @mock.patch("session_server.SessionUDSServer")
    @mock.patch("session_server.SessionServer.parse_args")
    @mock.patch("session_server.SocketAddress")
    @mock.patch("session_server.Player")
    def test_main_starts_correct_server_based_on_args(self, Player, SocketAddress, parse_args, UDSServer, TCPServer):
        """Test that main() chooses UDS/TCP server based on arguments and calls serve_forever."""
        args = argparse.Namespace(server_address=None, record=False, source_address=None, dry_run=False, glob="/tmp/sessions")
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
    def test_main_signal_handlers_registered(self, SocketAddress, parse_args, TCPServer, sig_signal):
        """Test that main() sets up signal handlers for SIGINT and SIGTERM."""
        args = argparse.Namespace(server_address=None, record=False, source_address=None, dry_run=False, glob="/tmp/sessions")
        parse_args.return_value = args
        SocketAddress.isUDSSocket.return_value = False
        TCPServer.return_value.__enter__.return_value.serve_forever = mock.Mock()
        main()
        self.assertTrue(sig_signal.called)
        calls = [call[0][0] for call in sig_signal.call_args_list]
        self.assertIn(signal.SIGINT, calls)
        self.assertIn(signal.SIGTERM, calls)

    @mock.patch("session_server.SessionTCPServer")
    @mock.patch("session_server.SessionServer.parse_args")
    @mock.patch("session_server.SocketAddress")
    def test_main_exits_gracefully_on_keyboard_interrupt(self, SocketAddress, parse_args, TCPServer):
        """Test that main() exits gracefully when interrupted (CTRL-C)."""
        args = argparse.Namespace(server_address=None, record=False, source_address=None, dry_run=False, glob="/tmp/sessions")
        parse_args.return_value = args
        SocketAddress.isUDSSocket.return_value = False
        TCPServer.return_value.__enter__.side_effect = KeyboardInterrupt
        try:
            main()
        except KeyboardInterrupt:
            self.fail("main() should handle KeyboardInterrupt gracefully")


if __name__ == "__main__":
    unittest.main()
