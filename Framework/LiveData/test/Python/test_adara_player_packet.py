"""
Test suite for Packet and ClientHelloPacket classes from adara_player module.
"""

import unittest
import struct
import hashlib
from io import BytesIO
import numpy as np
from adara_player import Packet, ClientHelloPacket, EPICS_EPOCH_OFFSET


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

        self.assertIsInstance(packet.SHA, str)
        self.assertEqual(len(packet.SHA), 64)  # SHA256 produces 64 hex characters

        self.assertIsInstance(packet.size, int)
        self.assertEqual(packet.size, self.expected_size)

        self.assertIsInstance(packet.packet_type, Packet.Type)
        self.assertEqual(packet.packet_type, self.packet_type)

        self.assertIsInstance(packet.timestamp, np.datetime64)

        self.assertIsInstance(packet.pulseid, int)

    def test_sha256(self):
        """Ensures hash calculation is consistent and correct given header/payload."""
        packet = Packet(header=self.header, payload=self.payload)

        # Calculate expected SHA256
        sha256 = hashlib.sha256()
        sha256.update(self.header + self.payload)
        expected_sha = sha256.hexdigest()

        # Verify packet SHA matches
        self.assertEqual(packet.SHA, expected_sha)

        # Test that same input produces same hash
        packet2 = Packet(header=self.header, payload=self.payload)
        self.assertEqual(packet.SHA, packet2.SHA)

        # Test that different payload produces different hash
        different_payload = b"\xff" * 8
        packet3 = Packet(header=self.header, payload=different_payload)
        self.assertNotEqual(packet.SHA, packet3.SHA)

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

    def test_iterfile_single_file(self):
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

    def test_to_file_and_from_file(self):
        """Round-trip serialize and parse: matching input/output for a given packet."""
        original_packet = Packet(header=self.header, payload=self.payload, source="original")

        # Write to BytesIO
        stream = BytesIO()
        Packet.to_file(stream, original_packet)

        # Read back
        stream.seek(0)
        restored_packet = Packet.from_file(stream, source="restored")

        # Verify all critical attributes match
        self.assertEqual(restored_packet._header, original_packet._header)
        self.assertEqual(restored_packet._payload, original_packet._payload)
        self.assertEqual(restored_packet._SHA, original_packet._SHA)
        self.assertEqual(restored_packet._size, original_packet._size)
        self.assertEqual(restored_packet._packet_type, original_packet._packet_type)
        self.assertEqual(restored_packet._timestamp, original_packet._timestamp)
        self.assertEqual(restored_packet._pulseid, original_packet._pulseid)

    def test_to_socket_and_from_socket(self):
        """Packet send/receive using sockets, ensuring transmission is lossless."""
        pass  # Will implement with I/O tests later

    def test_iterfiles_multi_file_ordering(self):
        """Tests that packets from multiple files are sorted chronologically as per first packet."""
        pass  # Will implement with I/O tests later

    def test_timestampfile_file_not_found(self):
        """Confirms proper error or fallback on missing file passed to `timestampfile`."""
        pass  # Will implement with I/O tests later

    def test_timestampfile_empty_malformed(self):
        """Verifies behavior when the file is empty or contains corrupt packet headers."""
        pass  # Will implement with I/O tests later


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
