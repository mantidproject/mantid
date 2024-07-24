import unittest
from unittest import mock

from mantidqt.dialogs.errorreports.gdb_async_runner import GDBAsync


class GDBAscyncTest(unittest.TestCase):
    def setUp(self):
        self.mock_presenter = mock.MagicMock()
        self.runner = GDBAsync(parent_presenter=self.mock_presenter)

    def test_trim_core_dump_file_with_empty_string(self):
        self.assertEqual(self.runner._trim_core_dump_file(""), "")

    def test_trim_core_dump_with_no_trace_line(self):
        text = "Welcome to gdb etc\n" "some information ..."
        self.assertEqual(self.runner._trim_core_dump_file(text), "")

    def test_trim_core_dump_file(self):
        intro_text = "Welcome to gdb etc\n" "version 1234 etc\n"
        trace_text = (
            "type of exception or fault ...\n" "# my error happened here\n" "# and previously here\n" "full stack below\n" "# more and more"
        )
        full_text = intro_text + trace_text
        self.assertEqual(self.runner._trim_core_dump_file(full_text), trace_text)


if __name__ == "__main__":
    unittest.main()
