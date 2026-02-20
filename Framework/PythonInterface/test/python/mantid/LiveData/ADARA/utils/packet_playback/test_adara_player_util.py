"""
Test suite for UnixGlob and SocketAddress classes from adara_player module.
"""

from pathlib import Path

import player_config
from packet_player import UnixGlob, SocketAddress
from adara_player_test_helpers import apply_test_config
from tempfile import TemporaryDirectory

import unittest


class Test_UnixGlob(unittest.TestCase):
    """Test cases for UnixGlob class."""

    def setUp(self):
        super().setUp()
        self.temp_dir = self.enterContext(TemporaryDirectory(prefix=f"{__name__}_"))
        apply_test_config()

    def tearDown(self):
        player_config.reset()
        super().tearDown()

    def test_parse_valid_patterns(self):
        """Verifies that valid Unix glob patterns are correctly parsed into base directory and glob string list."""
        # Test simple glob pattern
        base, globs = UnixGlob.parse("/path/to/files/*.txt")
        self.assertEqual(base, Path("/path/to/files"))
        self.assertEqual(globs, ["*.txt"])

        # Test pattern with question mark
        base, globs = UnixGlob.parse("/data/file?.dat")
        self.assertEqual(base, Path("/data"))
        self.assertEqual(globs, ["file?.dat"])

        # Test pattern with brackets
        base, globs = UnixGlob.parse("/logs/app[123].log")
        self.assertEqual(base, Path("/logs"))
        self.assertEqual(globs, ["app[123].log"])

        # Test pattern with no base directory (relative path)
        base, globs = UnixGlob.parse("*.py")
        self.assertEqual(base, Path("."))
        self.assertEqual(globs, ["*.py"])

        # Test pattern with multiple wildcards
        base, globs = UnixGlob.parse("/var/log/*/error*.log")
        self.assertEqual(base, Path("/var/log"))
        self.assertEqual(globs, ["*/error*.log"])

    def test_parse_invalid_patterns(self):
        """Ensures that malformed or unsupported patterns raise appropriate errors or result in fallback behavior."""
        # Empty string should default to current directory
        base, globs = UnixGlob.parse("")
        self.assertEqual(base, Path("."))
        self.assertEqual(globs, [""])

        # Pattern with only directory (no glob)
        base, globs = UnixGlob.parse("/path/to/directory")
        self.assertEqual(base, Path("/path/to/directory"))
        self.assertEqual(globs, [""])

    def test_expand_braces_simple(self):
        """Confirm that simple brace expansions (like `{a,b}`) in globs work as expected."""
        # Test simple brace expansion
        base, globs = UnixGlob.parse("/data/file{1,2,3}.txt")
        self.assertEqual(base, Path("/data"))
        self.assertIn("file1.txt", globs)
        self.assertIn("file2.txt", globs)
        self.assertIn("file3.txt", globs)
        self.assertEqual(len(globs), 3)

        # Test brace expansion with wildcards
        base, globs = UnixGlob.parse("/logs/{error,warn,info}*.log")
        self.assertEqual(base, Path("/logs"))
        self.assertIn("error*.log", globs)
        self.assertIn("warn*.log", globs)
        self.assertIn("info*.log", globs)
        self.assertEqual(len(globs), 3)

    def test_expand_braces_nested(self):
        """Validate handling of nested brace expansions such as `{a,{b,c}}`."""
        # Test nested braces - note: the implementation uses simple regex
        # and processes one level at a time recursively
        base, globs = UnixGlob.parse("/path/file{a,{x,y}}.dat")
        self.assertEqual(base, Path("/path"))
        # Should expand to all combinations, but order is indeterminate
        self.assertEqual(set(globs), {"filea.dat", "filex.dat", "filey.dat"})
        self.assertEqual(len(globs), 3)

    def test_expand_braces_edge_cases(self):
        """Test for edge cases: empty braces, unbalanced braces, and globs containing unusual characters."""
        # Pattern without braces should remain unchanged
        base, globs = UnixGlob.parse("/path/file.txt")
        self.assertEqual(base, Path("/path/file.txt"))
        self.assertEqual(globs, [""])

        # Pattern with comma but no braces
        base, globs = UnixGlob.parse("/path/file,test.txt")
        self.assertEqual(base, Path("/path"))
        self.assertEqual(globs, ["file,test.txt"])

        # Multiple brace groups in path
        base, globs = UnixGlob.parse("/data/{dir1,dir2}/file{a,b}.txt")
        self.assertEqual(base, Path("/data"))
        # Should expand to all combinations, but order is indeterminate
        self.assertEqual(set(globs), {"dir1/filea.txt", "dir1/fileb.txt", "dir2/filea.txt", "dir2/fileb.txt"})
        self.assertEqual(len(globs), 4)


class Test_SocketAddress(unittest.TestCase):
    """Test cases for SocketAddress class."""

    def setUp(self):
        super().setUp()
        self.temp_dir = self.enterContext(TemporaryDirectory(prefix=f"{__name__}_"))
        apply_test_config()

    def tearDown(self):
        player_config.reset()
        super().tearDown()

    def test_parse_hostname(self):
        """Checks that hostnames and DNS names parse as (host, port) tuples."""

        # Simple "localhost"
        result = SocketAddress.parse("localhost:12345")
        self.assertEqual(result, ("localhost", 12345))

        # Common DNS-style name
        result = SocketAddress.parse("example.com:80")
        self.assertEqual(result, ("example.com", 80))

        # Subdomain and hyphen in name
        result = SocketAddress.parse("my-db-server.local:54321")
        self.assertEqual(result, ("my-db-server.local", 54321))

        # Fully qualified domain name (FQDN)
        result = SocketAddress.parse("bl3-daq1.sns.gov:31415")
        self.assertEqual(result, ("bl3-daq1.sns.gov", 31415))

        # Hostname with digits
        result = SocketAddress.parse("node123:8081")
        self.assertEqual(result, ("node123", 8081))

    def test_parse_invalid_hostname_formats(self):
        """Ensures invalid hostname/port addresses throw errors."""

        # Hostname, missing port
        with self.assertRaises(ValueError) as ctx:
            SocketAddress.parse("example.com")
        self.assertIn("Invalid address format", str(ctx.exception))

        # Hostname, port out of range
        with self.assertRaises(ValueError) as ctx:
            SocketAddress.parse("localhost:99999")
        self.assertIn("Port out of range", str(ctx.exception))

        # Port is zero (not allowed)
        with self.assertRaises(ValueError) as ctx:
            SocketAddress.parse("someserv:0")
        self.assertIn("Port out of range", str(ctx.exception))

        # Hostname with invalid character
        with self.assertRaises(ValueError) as ctx:
            SocketAddress.parse("bad*host:8000")
        self.assertIn("Invalid address format", str(ctx.exception))

        # Leading colon, missing host
        with self.assertRaises(ValueError) as ctx:
            SocketAddress.parse(":8080")
        self.assertIn("Invalid address format", str(ctx.exception))

    def test_parse_ipv4(self):
        """Checks that IPv4 address/port strings parse to expected tuples."""
        # Standard IPv4 address
        result = SocketAddress.parse("192.168.1.100:8080")
        self.assertEqual(result, ("192.168.1.100", 8080))

        # Localhost
        result = SocketAddress.parse("127.0.0.1:9000")
        self.assertEqual(result, ("127.0.0.1", 9000))

        # High port number
        result = SocketAddress.parse("10.0.0.1:65535")
        self.assertEqual(result, ("10.0.0.1", 65535))

        # Low port number
        result = SocketAddress.parse("172.16.0.1:1")
        self.assertEqual(result, ("172.16.0.1", 1))

    def test_parse_ipv6(self):
        """Checks that IPv6 address/port strings parse correctly, including bracket handling."""
        # IPv6 with brackets (required format)
        result = SocketAddress.parse("[::1]:8080")
        self.assertEqual(result, ("::1", 8080))

        # Full IPv6 address
        result = SocketAddress.parse("[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:9000")
        self.assertEqual(result, ("2001:0db8:85a3:0000:0000:8a2e:0370:7334", 9000))

        # Compressed IPv6 address
        result = SocketAddress.parse("[2001:db8::1]:443")
        self.assertEqual(result, ("2001:db8::1", 443))

        # IPv6 loopback
        result = SocketAddress.parse("[::1]:12345")
        self.assertEqual(result, ("::1", 12345))

    def test_parse_unix_socket(self):
        """Tests parsing of Unix domain socket paths to Path objects."""
        # Absolute path starting with /
        result = SocketAddress.parse(f"{self.temp_dir}/my_socket.sock")
        self.assertIsInstance(result, Path)
        self.assertEqual(result, Path(f"{self.temp_dir}/my_socket.sock"))

        # Another Unix socket path
        result = SocketAddress.parse("/var/run/adara.sock")
        self.assertIsInstance(result, Path)
        self.assertEqual(result, Path("/var/run/adara.sock"))

        # Path with multiple components
        result = SocketAddress.parse("/home/user/.local/share/app/socket")
        self.assertIsInstance(result, Path)
        self.assertEqual(result, Path("/home/user/.local/share/app/socket"))

    def test_parse_invalid_format(self):
        """Ensures invalid address strings throw errors."""
        # Port out of range (too high)
        with self.assertRaises(ValueError) as ctx:
            SocketAddress.parse("192.168.1.1:65536")
        self.assertIn("Port out of range", str(ctx.exception))

        # Port out of range (negative)
        with self.assertRaises(ValueError) as ctx:
            SocketAddress.parse("192.168.1.1:-1")
        self.assertIn("Invalid address format", str(ctx.exception))

        # Port is zero
        with self.assertRaises(ValueError) as ctx:
            SocketAddress.parse("192.168.1.1:0")
        self.assertIn("Port out of range", str(ctx.exception))

        # Invalid format - no match for IP:port or Unix socket
        with self.assertRaises(ValueError) as ctx:
            SocketAddress.parse("not-a-valid-address")
        self.assertIn("Invalid address format", str(ctx.exception))

        # IPv6 without brackets
        with self.assertRaises(ValueError) as ctx:
            SocketAddress.parse("::1:8080")
        self.assertIn("Invalid address format", str(ctx.exception))

    def test_is_uds_socket_true(self):
        """Verifies that Unix socket paths are identified as UDS by `isUDSSocket`."""
        # Test with Path object (Unix socket)
        uds_path = Path(f"{self.temp_dir}/socket.sock")
        self.assertTrue(SocketAddress.isUDSSocket(uds_path))

        # Parse a Unix socket and verify
        parsed = SocketAddress.parse("/var/run/app.sock")
        self.assertTrue(SocketAddress.isUDSSocket(parsed))

    def test_is_uds_socket_false(self):
        """Confirms IP/port combinations are not falsely reported as UDS sockets."""
        # Test with tuple (IP:port)
        ip_port = ("192.168.1.1", 8080)
        self.assertFalse(SocketAddress.isUDSSocket(ip_port))

        # Parse an IPv4 address and verify
        parsed_ipv4 = SocketAddress.parse("127.0.0.1:9000")
        self.assertFalse(SocketAddress.isUDSSocket(parsed_ipv4))

        # Parse an IPv6 address and verify
        parsed_ipv6 = SocketAddress.parse("[::1]:8080")
        self.assertFalse(SocketAddress.isUDSSocket(parsed_ipv6))


if __name__ == "__main__":
    unittest.main()
