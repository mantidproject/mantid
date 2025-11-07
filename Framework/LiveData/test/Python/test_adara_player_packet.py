"""
Test suite for Packet and ClientHelloPacket classes from adara_player module.
"""

from io import BytesIO
import numpy as np
from pathlib import Path
import socket
import struct
import tempfile
from zlib import crc32

from adara_player import Packet, ClientHelloPacket, EPICS_EPOCH_OFFSET

import unittest
from unittest.mock import Mock


class Test_Packet(unittest.TestCase):
    """Test cases for Packet class."""

    def setUp(self):
        """Create sample packet data for testing."""
        # Create a valid ADARA packet header (16 bytes)
        # Format: payloadlen(4), typefield(4), tvsec(4), tvnsec(4)
        self.payload_len = 8
        self.packet_type = Packet.Type.HEARTBEAT_TYPE
        self.tv_sec = 1000000000  # Seconds since EPICS epoch
        self.tv_nsec = 500000000  # Nanoseconds

        # Pack the header using little-endian format
        self.header = struct.pack("<I", self.payload_len)  # payload length
        self.header += struct.pack("<I", self.packet_type.asPacketField())  # type field
        self.header += struct.pack("<I", self.tv_sec)  # seconds
        self.header += struct.pack("<I", self.tv_nsec)  # nanoseconds

        # Create a simple payload
        self.payload = b"\x01\x02\x03\x04\x05\x06\x07\x08"

        # Expected calculated values
        self.expected_size = Packet.HEADER_BYTES + self.payload_len
        self.expected_pulseid = (self.tv_sec << 32) | self.tv_nsec
        self.expected_timestamp = np.datetime64(self.tv_sec + EPICS_EPOCH_OFFSET, "s") + np.timedelta64(self.tv_nsec, "ns")

    def _create_header(self, payload_len=0, packet_type=None, tv_sec=0, tv_nsec=0):
        """Helper method to create packet headers for testing."""
        if packet_type is None:
            packet_type = Packet.Type.HEARTBEAT_TYPE

        header = struct.pack("<I", payload_len)
        header += struct.pack("<I", packet_type.asPacketField())
        header += struct.pack("<I", tv_sec)
        header += struct.pack("<I", tv_nsec)
        return header

    def test_init_valid_header_payload(self):
        """Creates Packet objects with valid inputs and verifies all attribute calculations."""
        # With explicit source
        packet = Packet(header=self.header, payload=self.payload, source="test_source")

        # Verify basic attributes
        self.assertEqual(packet._header, self.header)
        self.assertEqual(packet._payload, self.payload)
        self.assertEqual(packet._source, "test_source")

        # Verify calculated attributes
        self.assertEqual(packet._size, self.expected_size)
        self.assertEqual(packet._packet_type, self.packet_type)
        self.assertEqual(packet._pulseid, self.expected_pulseid)
        self.assertEqual(packet._timestamp, self.expected_timestamp)

        # Without source (should be None, not '')
        packet2 = Packet(header=self.header, payload=self.payload)
        self.assertIsNone(packet2._source)

    def test_init_invalid_type(self):
        """Asserts error is raised if the packet type is not recognized."""
        # Create header with invalid type field (0xFFFF with version 0xFF)
        invalid_type_value = 0xFFFF
        invalid_typefield = (invalid_type_value << 8) | 0xFF
        invalid_header = struct.pack("<I", 0)  # payloadlen
        invalid_header += struct.pack("<I", invalid_typefield)  # invalid type
        invalid_header += struct.pack("<I", 0)  # tvsec
        invalid_header += struct.pack("<I", 0)  # tvnsec

        # This should raise ValueError when trying to create Packet.Type
        with self.assertRaises(ValueError) as context:
            packet = Packet(header=invalid_header, payload=b"")  # noqa: F841

        # Verify the error message contains useful information
        error_msg = str(context.exception)
        self.assertIn("Invalid ADARA packet type", error_msg)
        self.assertIn("0xFFFF", error_msg)  # Check for hex type value

    def test_attribute_accessors(self):
        """Confirms all accessors (size, payload, header, timestamp, etc.) return expected values."""
        packet = Packet(header=self.header, payload=self.payload, source="test")

        # Test all property accessors
        self.assertIsInstance(packet.header, bytes)
        self.assertEqual(len(packet.header), Packet.HEADER_BYTES)

        self.assertIsInstance(packet.payload, bytes)
        self.assertEqual(len(packet.payload), self.payload_len)

        self.assertIsInstance(packet.source, str)
        self.assertEqual(packet.source, "test")

        self.assertIsInstance(packet.CRC, int)
        self.assertEqual(packet.CRC, packet.CRC & 0xffffffff)  # CRC32 produces 32-bit integers

        self.assertIsInstance(packet.size, int)
        self.assertEqual(packet.size, self.expected_size)

        self.assertIsInstance(packet.packet_type, Packet.Type)
        self.assertEqual(packet.packet_type, self.packet_type)

        self.assertIsInstance(packet.timestamp, np.datetime64)

        self.assertIsInstance(packet.pulseid, int)

    def test_CRC32(self):
        """Ensures hash calculation is consistent and correct given header/payload."""
        packet = Packet(header=self.header, payload=self.payload)

        # Calculate expected CRC32
        expected_CRC = crc32(self.header + self.payload)

        # Verify packet CRC matches
        self.assertEqual(packet.CRC, expected_CRC)

        # Test that same input produces same hash
        packet2 = Packet(header=self.header, payload=self.payload)
        self.assertEqual(packet.CRC, packet2.CRC)

        # Test that different payload produces different hash
        different_payload = b"\xff" * 8
        packet3 = Packet(header=self.header, payload=different_payload)
        self.assertNotEqual(packet.CRC, packet3.CRC)

    def test_as_packet_field(self):
        """Tests that the packed representation (asPacketField) is correct for the type/version."""
        # Test various packet types
        test_cases = [
            (Packet.Type.HEARTBEAT_TYPE, (0x4009 << 8) | 0x00),
            (Packet.Type.CLIENT_HELLO_TYPE, (0x4006 << 8) | 0x01),
            (Packet.Type.RAW_EVENT_TYPE, (0x0000 << 8) | 0x01),
        ]

        for packet_type, expected_field in test_cases:
            self.assertEqual(packet_type.asPacketField(), expected_field, f"Failed for {packet_type.name}")

    def test_iter_file_single_file(self):
        """Validates packet iteration from a single file stream (including edge cases)."""
        # Create a BytesIO with multiple packets
        stream = BytesIO()

        # Write first packet
        stream.write(self.header)
        stream.write(self.payload)

        # Create second packet with different data
        header2 = struct.pack("<I", 4)  # shorter payload
        header2 += struct.pack("<I", Packet.Type.SYNC_TYPE.asPacketField())
        header2 += struct.pack("<I", 2000000000)
        header2 += struct.pack("<I", 0)
        payload2 = b"\xaa\xbb\xcc\xdd"
        stream.write(header2)
        stream.write(payload2)

        # Reset stream to beginning
        stream.seek(0)

        # Iterate and collect packets
        packets = list(Packet.iter_file(stream, source="test_stream"))

        self.assertEqual(len(packets), 2)
        self.assertEqual(packets[0].payload, self.payload)
        self.assertEqual(packets[1].payload, payload2)
        self.assertEqual(packets[0].source, "test_stream")
        self.assertEqual(packets[1].source, "test_stream")

    def test_iter_file_empty_file(self):
        """Test iteration over empty file yields no packets."""
        stream = BytesIO(b"")
        packets = list(Packet.iter_file(stream))
        self.assertEqual(len(packets), 0)

    def test_iter_file_incomplete_header(self):
        """Test that iter_file stops cleanly when encountering incomplete header."""
        # Create stream with one complete packet, then incomplete header
        complete_header = self._create_header(payload_len=4, tv_sec=1000000000)
        complete_payload = b"\x01\x02\x03\x04"

        # Write complete packet, then only 10 bytes of next header (need 16)
        stream = BytesIO(complete_header + complete_payload + b"\x00" * 10)

        # Should yield first packet successfully, then stop (not raise error)
        packets = list(Packet.iter_file(stream))

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
            list(Packet.iter_file(stream))

        # Error should mention invalid packet type
        error_msg = str(context.exception)
        self.assertIn("Invalid ADARA packet type", error_msg)
        self.assertIn("0xFFFFFF", error_msg)  # Type value

    def test_to_file_and_from_file(self):
        """Round-trip serialize and parse: matching input/output for a given packet."""
        original_packet = Packet(header=self.header, payload=self.payload, source="original")

        # Write to BytesIO
        stream = BytesIO()
        Packet.to_file(stream, original_packet)
        self.assertEqual(original_packet._source, "original")

        # Read back
        stream.seek(0)
        restored_packet = Packet.from_file(stream, source="restored")

        # Verify all critical attributes match
        self.assertEqual(restored_packet._header, original_packet._header)
        self.assertEqual(restored_packet._payload, original_packet._payload)
        self.assertEqual(restored_packet._CRC, original_packet._CRC)
        self.assertEqual(restored_packet._size, original_packet._size)
        self.assertEqual(restored_packet._packet_type, original_packet._packet_type)
        self.assertEqual(restored_packet._timestamp, original_packet._timestamp)
        self.assertEqual(restored_packet._pulseid, original_packet._pulseid)
        self.assertEqual(restored_packet._source, "restored")

    def test_from_file_incomplete_header(self):
        """Test EOFError when header is truncated."""
        # Create stream with only 4 bytes (need 16 for complete header)
        stream = BytesIO(b"\x00\x00\x00\x00")

        with self.assertRaises(EOFError) as context:
            Packet.from_file(stream)

        # Error message should mention incomplete header
        self.assertIn("Incomplete header", str(context.exception))

    def test_from_file_incomplete_payload(self):
        """Test EOFError when payload is truncated."""
        # Create header that claims 100 bytes of payload
        header = self._create_header(payload_len=100, tv_sec=1000000000)
        # But only provide 50 bytes
        stream = BytesIO(header + b"\x00" * 50)

        with self.assertRaises(EOFError) as context:
            Packet.from_file(stream)

        # Error message should mention incomplete payload
        self.assertIn("Incomplete payload", str(context.exception))

    def test_to_socket_and_from_socket(self):
        """Packet send/receive using sockets, ensuring transmission is lossless."""
        # Test to_socket
        mock_send_socket = Mock()
        # Mock send to accept all data at once
        mock_send_socket.send.return_value = len(self.header + self.payload)

        original_packet = Packet(header=self.header, payload=self.payload)
        Packet.to_socket(mock_send_socket, original_packet)

        # Verify send was called with correct data
        self.assertTrue(mock_send_socket.send.called)
        # Get all data that was sent
        sent_calls = mock_send_socket.send.call_args_list
        sent_data = b"".join([call[0][0] for call in sent_calls])
        self.assertEqual(sent_data, self.header + self.payload)

        # Test from_socket with complete reads
        mock_recv_socket = Mock()
        # Mock recv to return complete data in two calls
        mock_recv_socket.recv.side_effect = [
            self.header,  # First recv: complete 16-byte header
            self.payload,  # Second recv: complete payload
        ]

        received_packet = Packet.from_socket(mock_recv_socket)

        # Verify lossless transmission
        self.assertEqual(received_packet.header, original_packet.header)
        self.assertEqual(received_packet.payload, original_packet.payload)
        self.assertEqual(received_packet.CRC, original_packet.CRC)
        self.assertEqual(received_packet.size, original_packet.size)
        self.assertEqual(received_packet.packet_type, original_packet.packet_type)

    def test_from_socket_partial_reads(self):
        """Test that recvexact handles partial socket reads correctly."""
        mock_socket = Mock()

        # Simulate partial reads (common with TCP sockets)
        mock_socket.recv.side_effect = [
            self.header[:8],  # First recv: only 8 bytes of 16-byte header
            self.header[8:],  # Second recv: remaining 8 bytes of header
            self.payload[:4],  # Third recv: partial payload (4 bytes)
            self.payload[4:],  # Fourth recv: rest of payload (4 bytes)
        ]

        packet = Packet.from_socket(mock_socket)

        # Should assemble correctly despite fragmentation
        self.assertEqual(packet.header, self.header)
        self.assertEqual(packet.payload, self.payload)
        self.assertEqual(packet.CRC, crc32(self.header + self.payload))

        # Verify recv was called multiple times
        self.assertEqual(mock_socket.recv.call_count, 4)

    def test_from_socket_connection_closed(self):
        """Test ConnectionError when socket closes mid-read."""
        mock_socket = Mock()
        # Simulate connection closed during header read
        mock_socket.recv.side_effect = [
            self.header[:8],  # Partial header (8 bytes)
            b"",  # Empty bytes = connection closed
        ]

        with self.assertRaises(ConnectionError) as context:
            Packet.from_socket(mock_socket)

        # Error message should indicate connection was closed
        error_msg = str(context.exception).lower()
        self.assertTrue("connection" in error_msg or "closed" in error_msg)

    def test_from_socket_timeout(self):
        """Test socket timeout handling."""
        mock_socket = Mock()
        # Simulate socket timeout
        mock_socket.recv.side_effect = socket.timeout("Socket operation timed out")

        with self.assertRaises(socket.timeout):
            Packet.from_socket(mock_socket)

    def test_to_socket_partial_sends(self):
        """Test that sendall handles partial socket sends correctly."""
        mock_socket = Mock()

        data = self.header + self.payload

        # Simulate partial sends
        sent_chunks = []

        def fake_send(buf):
            # Record the portion of the buffer that was claimed sent
            if not sent_chunks:
                n = 10  # first call: send 10 bytes
            else:
                n = len(buf)  # second call: send remainder
            sent_chunks.append(buf[:n])  # record only what is 'sent'
            return n

        mock_socket.send.side_effect = fake_send

        packet = Packet(header=self.header, payload=self.payload)
        Packet.to_socket(mock_socket, packet)

        # Verify send was called multiple times
        self.assertEqual(mock_socket.send.call_count, 2)

        # Verify all of the data was sent

        sent_data = b"".join(sent_chunks)
        self.assertEqual(sent_data, data)

    def test_iter_files_multi_file_ordering(self):
        """Tests that packets from multiple files are sorted chronologically as per first packet."""
        with tempfile.TemporaryDirectory() as tmpdir:
            tmppath = Path(tmpdir)

            # Create file 1 with LATER timestamp (should be read second)
            file1 = tmppath / "data_001.adara"
            header1 = self._create_header(
                payload_len=8,
                packet_type=Packet.Type.HEARTBEAT_TYPE,
                tv_sec=2000000000,  # Later timestamp
                tv_nsec=0,
            )
            payload1 = b"LATER---"
            with open(file1, "wb") as f:
                f.write(header1 + payload1)

            # Create file 2 with EARLIER timestamp (should be read first)
            file2 = tmppath / "data_002.adara"
            header2 = self._create_header(
                payload_len=8,
                packet_type=Packet.Type.HEARTBEAT_TYPE,
                tv_sec=1000000000,  # Earlier timestamp
                tv_nsec=0,
            )
            payload2 = b"EARLIER-"
            with open(file2, "wb") as f:
                f.write(header2 + payload2)

            # Create file 3 with MIDDLE timestamp
            file3 = tmppath / "data_003.adara"
            header3 = self._create_header(
                payload_len=8,
                packet_type=Packet.Type.HEARTBEAT_TYPE,
                tv_sec=1500000000,  # Middle timestamp
                tv_nsec=0,
            )
            payload3 = b"MIDDLE--"
            with open(file3, "wb") as f:
                f.write(header3 + payload3)

            # iter_files should yield packets in chronological order
            # regardless of filename order
            packets = list(Packet.iter_files(tmppath, "*.adara"))

            self.assertEqual(len(packets), 3)
            # Verify chronological ordering by payload content
            self.assertEqual(packets[0].payload, payload2)  # EARLIER (1000000000)
            self.assertEqual(packets[1].payload, payload3)  # MIDDLE (1500000000)
            self.assertEqual(packets[2].payload, payload1)  # LATER (2000000000)

            # Verify source is set correctly (file path)
            self.assertTrue(str(file2) in packets[0].source)
            self.assertTrue(str(file3) in packets[1].source)
            self.assertTrue(str(file1) in packets[2].source)

    def test__file_timestamp_file_not_found(self):
        """Confirms proper error on missing file passed to `_file_timestamp`."""
        nonexistent_path = Path("/nonexistent/path/to/file.adara")

        with self.assertRaises(FileNotFoundError):
            Packet._file_timestamp(nonexistent_path)

    def test__file_timestamp_empty_malformed(self):
        """Verifies behavior when the file is empty or contains corrupt packet headers."""
        # Test 1: Empty file should return epoch timestamp
        with tempfile.NamedTemporaryFile(delete=False, suffix=".adara") as f:
            empty_filepath = Path(f.name)

        try:
            # Empty file should catch StopIteration and return epoch
            timestamp = Packet._file_timestamp(empty_filepath)
            self.assertEqual(timestamp, np.datetime64(0, "ns"))
        finally:
            empty_filepath.unlink()

        # Test 2: File with corrupt/invalid packet type (complete header)
        with tempfile.NamedTemporaryFile(delete=False, suffix=".adara") as f:
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
                Packet._file_timestamp(corrupt_filepath)

            # Verify error message mentions invalid packet type
            error_msg = str(context.exception)
            self.assertIn("Invalid ADARA packet type", error_msg)
        finally:
            corrupt_filepath.unlink()

    def test__file_timestamp_incomplete_header(self):
        """Test _file_timestamp behavior with incomplete header in file."""
        with tempfile.NamedTemporaryFile(delete=False, suffix=".adara") as f:
            # Write incomplete header (only 10 bytes, need 16)
            f.write(b"\x00" * 10)
            incomplete_filepath = Path(f.name)

        try:
            # iter_file should handle this gracefully, returning no packets
            # _file_timestamp catches StopIteration and returns epoch
            timestamp = Packet._file_timestamp(incomplete_filepath)
            self.assertEqual(timestamp, np.datetime64(0, "ns"))
        finally:
            incomplete_filepath.unlink()

    def test__file_timestamp_valid_file(self):
        """Test _file_timestamp returns correct timestamp from valid file."""
        with tempfile.NamedTemporaryFile(delete=False, suffix=".adara") as f:
            # Create valid packet with known timestamp
            test_tv_sec = 1234567890
            test_tv_nsec = 123456789
            header = self._create_header(payload_len=4, tv_sec=test_tv_sec, tv_nsec=test_tv_nsec)
            f.write(header + b"\x00\x00\x00\x00")
            filepath = Path(f.name)

        try:
            timestamp = Packet._file_timestamp(filepath)

            # Verify timestamp matches the packet's timestamp
            expected_timestamp = np.datetime64(test_tv_sec + EPICS_EPOCH_OFFSET, "s") + np.timedelta64(test_tv_nsec, "ns")
            self.assertEqual(timestamp, expected_timestamp)
        finally:
            filepath.unlink()


class Test_ClientHelloPacket(unittest.TestCase):
    """Test cases for ClientHelloPacket class."""

    def setUp(self):
        """Create sample ClientHelloPacket data."""
        # Create CLIENT_HELLO_TYPE packet
        self.start_time_value = 1234567890  # seconds
        self.payload = struct.pack("<I", self.start_time_value)

        self.start_of_run_marker = 1  # seconds from the EPICS epoch
        self.start_of_run_payload = struct.pack("<I", self.start_of_run_marker)

        # Create header with CLIENT_HELLO_TYPE
        self.header = struct.pack("<I", len(self.payload))
        self.header += struct.pack("<I", Packet.Type.CLIENT_HELLO_TYPE.asPacketField())
        self.header += struct.pack("<I", 1000000000)
        self.header += struct.pack("<I", 0)

    def test_init_type_check(self):
        """Ensures only packets of CLIENT_HELLO_TYPE are accepted."""
        # Valid CLIENT_HELLO_TYPE should work
        packet = ClientHelloPacket(header=self.header, payload=self.payload)
        self.assertEqual(packet.packet_type, Packet.Type.CLIENT_HELLO_TYPE)

        # Wrong packet type should raise ValueError
        wrong_header = struct.pack("<I", 0)
        wrong_header += struct.pack("<I", Packet.Type.HEARTBEAT_TYPE.asPacketField())
        wrong_header += struct.pack("<I", 0)
        wrong_header += struct.pack("<I", 0)

        with self.assertRaises(ValueError) as context:
            ClientHelloPacket(header=wrong_header, payload=b"")
        self.assertIn("CLIENT_HELLO_TYPE", str(context.exception))

    def test_start_time_extraction(self):
        """Validates extraction of start_time from packet payload."""
        packet = ClientHelloPacket(header=self.header, payload=self.payload)

        # Verify start_time is correctly extracted
        expected_start_time = np.datetime64(self.start_time_value, "s")
        self.assertEqual(packet.start_time, expected_start_time)

        # Verify it's accessible via property
        self.assertIsInstance(packet.start_time, np.datetime64)

    def test_from_start_of_run(self):
        """Confirms that `fromStartOfRun` property reflects the correct time semantics."""
        packet = ClientHelloPacket(header=self.header, payload=self.start_of_run_payload)

        # fromStartOfRun should return True
        self.assertTrue(packet.fromStartOfRun())

        # Verify the calculation is consistent
        self.assertEqual(packet.start_time, np.datetime64(1, "s"))


if __name__ == "__main__":
    unittest.main()
