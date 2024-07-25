import unittest
from unittest import mock
from tempfile import TemporaryDirectory
from time import sleep

from mantidqt.dialogs.errorreports.gdb_async_runner import GDBAsync


class GDBAscyncTest(unittest.TestCase):
    RUNNER_CLS_PATH = "mantidqt.dialogs.errorreports.gdb_async_runner"

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

    def test_no_core_dump_file_from_empty_dir(self):
        with TemporaryDirectory() as tempdir:
            self.assertIsNone(self.runner._latest_core_dump(tempdir))

    def test_latest_core_dump_gets_latest_file(self):
        file_names = ["first", "second", "third"]
        with TemporaryDirectory() as tempdir:
            for name in file_names:
                open(f"{tempdir}/{name}", "a").close()
                sleep(0.1)  # not consistently ordered without a sleep
            latest_file = self.runner._latest_core_dump(tempdir)
            self.assertEqual(latest_file.name, file_names[-1])

    @mock.patch(f"{RUNNER_CLS_PATH}.CORE_DUMP_RECENCY_LIMIT", 0.5)
    def test_no_core_dump_file_if_files_too_old(self):
        with TemporaryDirectory() as tempdir:
            open(f"{tempdir}/my_test_file", "a").close()
            sleep(1)
            self.assertIsNone(self.runner._latest_core_dump(tempdir))


if __name__ == "__main__":
    unittest.main()
