# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import tempfile
import unittest

import numpy as np
from mantid.api import MDNormalization
from mantid.simpleapi import CreateMDHistoWorkspace, CreateSampleWorkspace, SaveMDToAscii


class SaveMDToAsciiTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp_file = tempfile.NamedTemporaryFile(suffix=".dat", delete=False)
        self.tmp_file.close()

    def tearDown(self) -> None:
        os.remove(self.tmp_file.name)

    def _read_data_lines(self):
        with open(self.tmp_file.name) as f:
            return [line for line in f if not line.startswith("#")]

    def test_column_order_and_values(self):
        # SignalInput/ErrorInput fill the underlying array with dimension 0 varying fastest,
        # so for a 2x2 workspace bin (A=0,B=0)->1, (A=1,B=0)->2, (A=0,B=1)->3, (A=1,B=1)->4.
        ws = CreateMDHistoWorkspace(
            Dimensionality=2,
            Extents="0,2,0,2",
            SignalInput=[1, 2, 3, 4],
            ErrorInput=[1, 1, 1, 1],
            NumberOfBins="2,2",
            Names="A,B",
            Units="U,V",
        )

        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name)

        data = np.loadtxt(self.tmp_file.name)
        expected = np.array(
            [
                [0.5, 0.5, 1, 1],
                [0.5, 1.5, 3, 1],
                [1.5, 0.5, 2, 1],
                [1.5, 1.5, 4, 1],
            ]
        )
        np.testing.assert_allclose(data, expected)

    def test_exclude_integrated_dimensions_true_by_default(self):
        ws = CreateMDHistoWorkspace(
            Dimensionality=3,
            Extents="0,2,0,2,10,20",
            SignalInput=[1, 2, 3, 4],
            ErrorInput=[1, 1, 1, 1],
            NumberOfBins="2,2,1",
            Names="A,B,C",
            Units="U,V,W",
        )

        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name)

        data = np.loadtxt(self.tmp_file.name)
        # dimension C is integrated (1 bin) and excluded by default: same shape as the 2D case
        expected = np.array(
            [
                [0.5, 0.5, 1, 1],
                [0.5, 1.5, 3, 1],
                [1.5, 0.5, 2, 1],
                [1.5, 1.5, 4, 1],
            ]
        )
        np.testing.assert_allclose(data, expected)

    def test_exclude_integrated_dimensions_false(self):
        ws = CreateMDHistoWorkspace(
            Dimensionality=3,
            Extents="0,2,0,2,10,20",
            SignalInput=[1, 2, 3, 4],
            ErrorInput=[1, 1, 1, 1],
            NumberOfBins="2,2,1",
            Names="A,B,C",
            Units="U,V,W",
        )

        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name, ExcludeIntegratedDimensions=False)

        data = np.loadtxt(self.tmp_file.name)
        expected = np.array(
            [
                [0.5, 0.5, 15.0, 1, 1],
                [0.5, 1.5, 15.0, 3, 1],
                [1.5, 0.5, 15.0, 2, 1],
                [1.5, 1.5, 15.0, 4, 1],
            ]
        )
        np.testing.assert_allclose(data, expected)

    def test_all_dimensions_integrated_raises(self):
        ws = CreateMDHistoWorkspace(
            Dimensionality=2,
            Extents="0,2,0,2",
            SignalInput=[1],
            ErrorInput=[1],
            NumberOfBins="1,1",
            Names="A,B",
            Units="U,V",
        )

        self.assertRaises(RuntimeError, SaveMDToAscii, InputWorkspace=ws, Filename=self.tmp_file.name)

    def test_normalization_from_workspace_uses_num_events(self):
        ws = CreateMDHistoWorkspace(
            Dimensionality=2,
            Extents="0,2,0,2",
            SignalInput=[2, 4, 6, 8],
            ErrorInput=[2, 2, 2, 2],
            NumberOfEvents=[2, 2, 2, 2],
            NumberOfBins="2,2",
            Names="A,B",
            Units="U,V",
        )
        ws.setDisplayNormalization(MDNormalization.NumEventsNormalization)

        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name)

        data = np.loadtxt(self.tmp_file.name)
        np.testing.assert_allclose(data[:, 2], [1, 3, 2, 4])
        np.testing.assert_allclose(data[:, 3], [1, 1, 1, 1])

    def test_normalization_override_no_normalization(self):
        ws = CreateMDHistoWorkspace(
            Dimensionality=2,
            Extents="0,2,0,2",
            SignalInput=[2, 4, 6, 8],
            ErrorInput=[2, 2, 2, 2],
            NumberOfEvents=[2, 2, 2, 2],
            NumberOfBins="2,2",
            Names="A,B",
            Units="U,V",
        )
        ws.setDisplayNormalization(MDNormalization.NumEventsNormalization)

        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name, Normalization="NoNormalization")

        data = np.loadtxt(self.tmp_file.name)
        np.testing.assert_allclose(data[:, 2], [2, 6, 4, 8])
        np.testing.assert_allclose(data[:, 3], [2, 2, 2, 2])

    def test_normalization_volume_normalization_does_not_divide_by_events(self):
        ws = CreateMDHistoWorkspace(
            Dimensionality=2,
            Extents="0,2,0,2",
            SignalInput=[2, 4, 6, 8],
            ErrorInput=[2, 2, 2, 2],
            NumberOfEvents=[2, 2, 2, 2],
            NumberOfBins="2,2",
            Names="A,B",
            Units="U,V",
        )

        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name, Normalization="VolumeNormalization")

        data = np.loadtxt(self.tmp_file.name)
        np.testing.assert_allclose(data[:, 2], [2, 6, 4, 8])
        np.testing.assert_allclose(data[:, 3], [2, 2, 2, 2])

    def test_separator_choices(self):
        separators = {"CSV": ",", "Tab": "\t", "Space": " ", "Colon": ":", "SemiColon": ";"}
        for name, char in separators.items():
            with self.subTest(separator=name):
                ws = CreateMDHistoWorkspace(
                    Dimensionality=1,
                    Extents="0,2",
                    SignalInput=[1, 2],
                    ErrorInput=[1, 1],
                    NumberOfBins="2",
                    Names="A",
                    Units="U",
                )
                SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name, Separator=name)

                data_lines = self._read_data_lines()
                self.assertEqual(len(data_lines), 2)
                fields = data_lines[0].strip().split(char)
                self.assertEqual(len(fields), 3)

    def test_separator_userdefined_with_custom(self):
        ws = CreateMDHistoWorkspace(
            Dimensionality=1,
            Extents="0,2",
            SignalInput=[1, 2],
            ErrorInput=[1, 1],
            NumberOfBins="2",
            Names="A",
            Units="U",
        )
        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name, Separator="UserDefined", CustomSeparator="|")

        data_lines = self._read_data_lines()
        fields = data_lines[0].strip().split("|")
        self.assertEqual(len(fields), 3)

    def test_separator_userdefined_without_custom_raises(self):
        ws = CreateMDHistoWorkspace(
            Dimensionality=1,
            Extents="0,2",
            SignalInput=[1, 2],
            ErrorInput=[1, 1],
            NumberOfBins="2",
            Names="A",
            Units="U",
        )
        self.assertRaises(RuntimeError, SaveMDToAscii, InputWorkspace=ws, Filename=self.tmp_file.name, Separator="UserDefined")

    def test_precision(self):
        ws = CreateMDHistoWorkspace(
            Dimensionality=1,
            Extents="0,2",
            SignalInput=[1, 2],
            ErrorInput=[1, 1],
            NumberOfBins="2",
            Names="A",
            Units="U",
        )

        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name, Precision=2)
        data_lines = self._read_data_lines()
        for value in data_lines[0].split():
            self.assertRegex(value, r"^\d\.\d{2}e[+-]\d+$")

        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name)
        data_lines = self._read_data_lines()
        for value in data_lines[0].split():
            self.assertRegex(value, r"^\d\.\d{6}e[+-]\d+$")

    def test_wrong_workspace_type_raises(self):
        ws = CreateSampleWorkspace()
        self.assertRaises(ValueError, SaveMDToAscii, InputWorkspace=ws, Filename=self.tmp_file.name)


if __name__ == "__main__":
    unittest.main()
