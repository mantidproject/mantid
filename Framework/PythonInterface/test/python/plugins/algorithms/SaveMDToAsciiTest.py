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

    def test_bin_centres_correct_for_bin_count_prone_to_float_drift(self):
        # 7 bins over a 0-1 extent gives a step (1/7) that is not exactly representable
        # in binary floating point; np.arange(start, stop, step) can drop or add an
        # extra element here due to accumulated rounding, producing the wrong number
        # of bin centres (see _dim2array).
        nbins = 7
        ws = CreateMDHistoWorkspace(
            Dimensionality=1,
            Extents="0,1",
            SignalInput=list(range(nbins)),
            ErrorInput=[1] * nbins,
            NumberOfBins=str(nbins),
            Names="A",
            Units="U",
        )

        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name)

        data = np.loadtxt(self.tmp_file.name)
        self.assertEqual(data.shape[0], nbins)
        halfstep = 0.5 / nbins
        np.testing.assert_allclose(data[0, 0], halfstep)
        np.testing.assert_allclose(data[-1, 0], 1 - halfstep)

    def test_extra_header_default_empty_adds_nothing(self):
        ws = CreateMDHistoWorkspace(
            Dimensionality=1,
            Extents="0,2",
            SignalInput=[1, 2],
            ErrorInput=[1, 1],
            NumberOfBins="2",
            Names="A",
            Units="U",
        )
        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name)

        with open(self.tmp_file.name) as f:
            header_lines = [line for line in f if line.startswith("#")]
        self.assertEqual(len(header_lines), 2)
        self.assertIn("A Intensity Error", header_lines[0])

    def test_extra_header_written_as_first_header_line(self):
        ws = CreateMDHistoWorkspace(
            Dimensionality=1,
            Extents="0,2",
            SignalInput=[1, 2],
            ErrorInput=[1, 1],
            NumberOfBins="2",
            Names="A",
            Units="U",
        )
        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name, ExtraHeader="Collected on 2026-07-17, run 12345")

        with open(self.tmp_file.name) as f:
            header_lines = [line for line in f if line.startswith("#")]
        self.assertEqual(len(header_lines), 3)
        self.assertIn("Collected on 2026-07-17, run 12345", header_lines[0])
        self.assertIn("A Intensity Error", header_lines[1])

    def test_extra_header_multiline_each_line_prefixed(self):
        ws = CreateMDHistoWorkspace(
            Dimensionality=1,
            Extents="0,2",
            SignalInput=[1, 2],
            ErrorInput=[1, 1],
            NumberOfBins="2",
            Names="A",
            Units="U",
        )
        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name, ExtraHeader="Sample: NaCl powder\nRun: 12345")

        with open(self.tmp_file.name) as f:
            header_lines = [line for line in f if line.startswith("#")]
        self.assertEqual(len(header_lines), 4)
        self.assertEqual(header_lines[0].strip(), "# Sample: NaCl powder")
        self.assertEqual(header_lines[1].strip(), "# Run: 12345")
        self.assertIn("A Intensity Error", header_lines[2])

    def test_extra_header_literal_backslash_n_treated_as_newline(self):
        # A literal backslash-n (as opposed to an embedded newline character) is what a user
        # typing into a single-line text field would produce; it should be treated the same way.
        ws = CreateMDHistoWorkspace(
            Dimensionality=1,
            Extents="0,2",
            SignalInput=[1, 2],
            ErrorInput=[1, 1],
            NumberOfBins="2",
            Names="A",
            Units="U",
        )
        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name, ExtraHeader=r"test1\ntest2")

        with open(self.tmp_file.name) as f:
            header_lines = [line for line in f if line.startswith("#")]
        self.assertEqual(len(header_lines), 4)
        self.assertEqual(header_lines[0].strip(), "# test1")
        self.assertEqual(header_lines[1].strip(), "# test2")
        self.assertIn("A Intensity Error", header_lines[2])

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

    def test_normalization_volume_normalization_divides_by_bin_volume(self):
        # 2x2 bins over a 2x2 extent: each bin is 1x1, so inverse volume is 1 and this
        # normalization is a no-op here; see test_normalization_volume_normalization_scales_by_inverse_volume
        # for a case where the bin volume is not 1.
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

    def test_normalization_volume_normalization_scales_by_inverse_volume(self):
        # 2 bins over a 4-wide extent: each bin has volume 2, so inverse volume is 0.5.
        ws = CreateMDHistoWorkspace(
            Dimensionality=1,
            Extents="0,4",
            SignalInput=[2, 4],
            ErrorInput=[2, 4],
            NumberOfBins="2",
            Names="A",
            Units="U",
        )

        SaveMDToAscii(InputWorkspace=ws, Filename=self.tmp_file.name, Normalization="VolumeNormalization")

        data = np.loadtxt(self.tmp_file.name)
        np.testing.assert_allclose(data[:, 1], [1, 2])
        np.testing.assert_allclose(data[:, 2], [1, 2])

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

    def test_header_column_names_use_configured_separator(self):
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

                with open(self.tmp_file.name) as f:
                    header_lines = [line for line in f if line.startswith("#")]
                column_header = header_lines[0].lstrip("#").strip()
                self.assertEqual(column_header.split(char), ["A", "Intensity", "Error"])

    def test_header_column_names_use_custom_separator(self):
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

        with open(self.tmp_file.name) as f:
            header_lines = [line for line in f if line.startswith("#")]
        column_header = header_lines[0].lstrip("#").strip()
        self.assertEqual(column_header.split("|"), ["A", "Intensity", "Error"])

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
