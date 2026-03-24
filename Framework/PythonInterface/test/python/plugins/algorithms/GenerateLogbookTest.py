# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import mtd, ITableWorkspace
from mantid.simpleapi import config, GenerateLogbook
import os
from tempfile import TemporaryDirectory


class GenerateLogbookTest(unittest.TestCase):
    _data_directory = None

    def setUp(self):
        data_dirs = config["datasearch.directories"].split(";")
        unit_test_data_dir = [p for p in data_dirs if "UnitTest" in p][0]
        d7_dir = "ILL/D7"
        if "ILL" in unit_test_data_dir:
            d7_dir = "D7"
        self._data_directory = os.path.abspath(os.path.join(unit_test_data_dir, d7_dir))

    def tearDown(self):
        mtd.clear()

    @classmethod
    def setUpClass(cls):
        cls.TEMP_DIR = TemporaryDirectory()
        cls.TEMP_FILE_NAME = os.path.join(cls.TEMP_DIR.name, "logbook.csv")

    @classmethod
    def tearDownClass(cls):
        mtd.clear()
        cls.TEMP_DIR.cleanup()

    def test_instrument_does_not_exist(self):
        self.assertTrue(os.path.exists(self._data_directory))
        with self.assertRaisesRegex(RuntimeError, "There is no parameter file for nonexistent instrument."):
            GenerateLogbook(Directory=self._data_directory, OutputWorkspace="__unused", Facility="ISIS", Instrument="nonexistent")

    def test_d7_default(self):
        self.assertTrue(os.path.exists(self._data_directory))
        GenerateLogbook(
            Directory=self._data_directory, OutputWorkspace="default_logbook", Facility="ILL", Instrument="D7", NumorRange="396990:396993"
        )
        self._check_output("default_logbook", numberEntries=3, numberColumns=7)

    def test_d7_optional(self):
        self.assertTrue(os.path.exists(self._data_directory))
        GenerateLogbook(
            Directory=self._data_directory,
            OutputWorkspace="optional_logbook",
            Facility="ILL",
            Instrument="D7",
            NumorRange="396990:396993",
            OptionalHeaders="TOF",
        )
        self._check_output("optional_logbook", numberEntries=3, numberColumns=8)

    def test_d7_custom(self):
        self.assertTrue(os.path.exists(self._data_directory))
        GenerateLogbook(
            Directory=self._data_directory,
            OutputWorkspace="custom_logbook",
            Facility="ILL",
            Instrument="D7",
            NumorRange="396990:396993",
            CustomEntries="/entry0/acquisition_mode",
        )
        self._check_output("custom_logbook", numberEntries=3, numberColumns=8)

    def test_d7_custom_with_summing(self):
        self.assertTrue(os.path.exists(self._data_directory))
        GenerateLogbook(
            Directory=self._data_directory,
            OutputWorkspace="custom_logbook_w_summing",
            Facility="ILL",
            Instrument="D7",
            NumorRange="396990:396993",
            CustomEntries="/entry0/acquisition_mode",
            OptionalHeaders="wavelength",
        )
        self._check_output("custom_logbook_w_summing", numberEntries=3, numberColumns=9)

    def test_d7_save_csv(self):
        self.assertTrue(os.path.exists(self._data_directory))
        GenerateLogbook(
            Directory=self._data_directory,
            OutputWorkspace="__unused",
            Facility="ILL",
            Instrument="D7",
            NumorRange="396990:396993",
            OutputFile=self.TEMP_FILE_NAME,
        )
        self.assertTrue(os.path.exists(self.TEMP_FILE_NAME))

    def _check_output(self, ws, numberEntries, numberColumns):
        self.assertTrue(mtd[ws])
        self.assertTrue(isinstance(mtd[ws], ITableWorkspace))
        self.assertEqual(len(mtd[ws].row(0)), numberColumns)
        self.assertEqual(len(mtd[ws].column(0)), numberEntries)


if __name__ == "__main__":
    unittest.main()
