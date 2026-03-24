"""
Test suite for Packet and ClientHelloPacket classes from adara_player module.
"""

from io import BytesIO
import numpy as np
import socket
import struct
from zlib import crc32

import player_config
from packet_player import Packet, ClientHelloPacket, EPICS_EPOCH_OFFSET
from adara_player_test_helpers import apply_test_config

import unittest
from unittest.mock import Mock


class Test_Packet(unittest.TestCase):
    """Test cases for Packet class."""

    def setUp(self):
        """Create sample packet data for testing."""
        apply_test_config()

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

    def tearDown(self):
        player_config.reset()

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
        self.assertEqual(packet.CRC, packet.CRC & 0xFFFFFFFF)  # CRC32 produces 32-bit integers

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
        header = Packet.create_header(payload_len=100, packet_type=Packet.Type.HEARTBEAT_TYPE, tv_sec=1000000000, tv_nsec=0)
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


class Test_ClientHelloPacket(unittest.TestCase):
    """Test cases for ClientHelloPacket class."""

    def setUp(self):
        """Create sample ClientHelloPacket data."""
        apply_test_config()

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

    def tearDown(self):
        player_config.reset()

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
        expected_start_time = np.datetime64(self.start_time_value + EPICS_EPOCH_OFFSET, "s")
        self.assertEqual(packet.start_time, expected_start_time)

        # Verify it's accessible via property
        self.assertIsInstance(packet.start_time, np.datetime64)

    def test_is_StartOfRun_packet(self):
        """Confirms that `is_StartOfRun_packet` property reflects the correct time semantics."""
        packet = ClientHelloPacket(header=self.header, payload=self.start_of_run_payload)

        # fromStartOfRun should return True
        self.assertTrue(packet.is_StartOfRun_packet)

        # Verify the calculation is consistent
        self.assertEqual(packet.start_time, np.datetime64(1 + EPICS_EPOCH_OFFSET, "s"))


if __name__ == "__main__":
    unittest.main()
