# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import tempfile
import os
import shutil
import numpy as np

from mantid.api import FileFinder
from mantid.simpleapi import SaveReflections, DeleteWorkspace, LoadEmptyInstrument, CreatePeaksWorkspace, SetUB, CreateEmptyTableWorkspace


class SaveReflectionsTest(unittest.TestCase):

    def setUp(self):
        self._workspace = self._create_peaks_workspace()
        self._test_dir = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self._test_dir)
        DeleteWorkspace(self._workspace)

    def _create_peaks_workspace(self):
        """Create a dummy peaks workspace"""
        path = FileFinder.getFullPath("unit_testing/MINITOPAZ_Definition.xml")
        inst = LoadEmptyInstrument(Filename=path)
        ws = CreatePeaksWorkspace(inst, 0)
        DeleteWorkspace(inst)
        SetUB(ws, 1, 1, 1, 90, 90, 90)

        # Add a bunch of random peaks that happen to fall on the
        # detetor bank defined in the IDF
        center_q = np.array([-5.1302,2.5651,3.71809])
        qs = []
        for i in np.arange(0, 1, 0.1):
            for j in np.arange(-0.5, 0, 0.1):
                q = center_q.copy()
                q[1] += j
                q[2] += i
                qs.append(q)

        # Add the peaks to the PeaksWorkspace with dummy values for intensity,
        # Sigma, and HKL
        for q in qs:
            peak = ws.createPeak(q)
            peak.setIntensity(100)
            peak.setSigmaIntensity(10)
            peak.setHKL(1, 1, 1)
            ws.addPeak(peak)

        return ws

    def _create_modulated_peak_table(self):
        hklm = np.ones((self._workspace.rowCount(), 2))
        return self._create_indexed_workspace(self._workspace, 5, hklm)

    def _create_indexed_workspace(self, fractional_peaks, ndim, hklm):
        # Create table with the number of columns we need
        indexed = CreateEmptyTableWorkspace()
        names = fractional_peaks.getColumnNames()
        types = fractional_peaks.columnTypes()

        # Insert the extra columns for the addtional indicies
        for i in range(ndim - 3):
            names.insert(5 + i, 'm{}'.format(i + 1))
            types.insert(5 + i, 'double')

        names = np.array(names)
        types = np.array(types)

        # Create columns in the table workspace
        for name, column_type in zip(names, types):
            indexed.addColumn(column_type, name)

        # Copy all columns from original workspace, ignoring HKLs
        column_data = []
        idx = np.arange(0, names.size)
        hkl_mask = (idx < 5) | (idx > 4 + (ndim - 3))
        for name in names[hkl_mask]:
            column_data.append(fractional_peaks.column(name))

        # Insert the addtional HKL columns into the data
        for i, col in enumerate(hklm.T.tolist()):
            column_data.insert(i + 2, col)

        # Insert the columns into the table workspace
        for i in range(fractional_peaks.rowCount()):
            row = [column_data[j][i] for j in range(indexed.columnCount())]
            indexed.addRow(row)

        return indexed

    def _get_reference_result(self, name):
        path = FileFinder.getFullPath(name)
        if path is None or path == "":
            raise RuntimeError("Could not find unit test data: {}".format(name))
        return path

    def test_save_fullprof_format(self):
        # Arrange
        reference_result = self._get_reference_result("fullprof_format.hkl")
        file_name = os.path.join(self._test_dir, "test_fullprof.hkl")
        output_format = "Fullprof"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format)

        # Assert
        self.assertTrue(compare_file(reference_result, file_name))

    def test_save_fullprof_format_modulated(self):
        # Arrange
        workspace = self._create_modulated_peak_table()
        reference_result = self._get_reference_result("fullprof_format_modulated.hkl")
        file_name = os.path.join(self._test_dir, "test_fullprof_modulated.hkl")
        output_format = "Fullprof"

        # Act
        SaveReflections(InputWorkspace=workspace, Filename=file_name, Format=output_format)

        # Assert
        self.assertTrue(compare_file(reference_result, file_name))

    def test_save_jana_format(self):
        # Arrange
        reference_result = self._get_reference_result("jana_format.hkl")
        file_name = os.path.join(self._test_dir, "test_jana.hkl")
        output_format = "Jana"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format)

        # Assert
        self.assertTrue(compare_file(reference_result, file_name))

    def test_save_jana_format_modulated(self):
        # Arrange
        workspace = self._create_modulated_peak_table()
        reference_result = self._get_reference_result("jana_format_modulated.hkl")
        file_name = os.path.join(self._test_dir, "test_jana_modulated.hkl")
        output_format = "Jana"

        # Act
        SaveReflections(InputWorkspace=workspace, Filename=file_name, Format=output_format)

        # Assert
        self.assertTrue(compare_file(reference_result, file_name))

    def test_save_GSAS_format(self):
        # Arrange
        reference_result = self._get_reference_result("gsas_format.hkl")
        file_name = os.path.join(self._test_dir, "test_gsas.hkl")
        output_format = "GSAS"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format)

        # Assert
        self.assertTrue(compare_file(reference_result, file_name))

    def test_save_GSAS_format_modulated(self):
        # Arrange
        workspace = self._create_modulated_peak_table()
        file_name = os.path.join(self._test_dir, "test_GSAS_modulated.hkl")
        output_format = "GSAS"

        # Act
        self.assertRaises(RuntimeError, SaveReflections, InputWorkspace=workspace, Filename=file_name, Format=output_format)

    def test_save_SHELX_format(self):
        # Arrange
        reference_result = self._get_reference_result("shelx_format.hkl")
        file_name = os.path.join(self._test_dir, "test_shelx.hkl")
        output_format = "SHELX"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format)

        # Assert
        self.assertTrue(compare_file(reference_result, file_name))

    def test_save_SHELX_format_modulated(self):
        # Arrange
        workspace = self._create_modulated_peak_table()
        file_name = os.path.join(self._test_dir, "test_SHELX_modulated.hkl")
        output_format = "SHELX"

        # Act
        self.assertRaises(RuntimeError, SaveReflections, InputWorkspace=workspace, Filename=file_name, Format=output_format)

    def test_save_invalid_format(self):
        # Arrange
        file_name = os.path.join(self._test_dir, "test_SHELX_modulated.hkl")
        output_format = "InvalidFormatName"

        # Act
        self.assertRaises(ValueError, SaveReflections, InputWorkspace=self._workspace, Filename=file_name, Format=output_format)


def compare_file(reference_result, file_name):
    with open(reference_result, 'r') as ref_file:
        with open(file_name, 'r') as actual_file:
            ref_lines = ref_file.readlines()
            actual_lines = actual_file.readlines()
            ref_lines = map(lambda x: x.strip(), ref_lines)
            actual_lines = map(lambda x: x.strip(), actual_lines)
            for ref_line, actual_line in zip(ref_lines, actual_lines):
                if ref_line != actual_line:
                    return False

    return True


if __name__ == '__main__':
    unittest.main()
