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
from mantid.kernel import V3D
from mantid.simpleapi import (CloneWorkspace, CreatePeaksWorkspace, DeleteWorkspace,
                              LoadEmptyInstrument, SetUB, SaveReflections)


class SaveReflectionsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._test_dir = tempfile.mkdtemp()

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls._test_dir)

    def setUp(self):
        self._workspace = self._create_peaks_workspace()

    def tearDown(self):
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
        center_q = np.array([-5.1302, 2.5651, 3.71809])
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
        # alternating pattern of (1,0),(0,-1) and a (0,0)
        nrows = self._workspace.rowCount()
        m1m2 = np.zeros((nrows, 2))
        m1m2[::2, 0] = 1
        m1m2[1::2, 1] = -1
        m1m2[int(nrows / 2)] = [0, 0]
        return self._create_indexed_workspace(self._workspace, m1m2)

    def _create_indexed_workspace(self, fractional_peaks, m1m2):
        # Create table with the number of columns we need
        modulated = CloneWorkspace(fractional_peaks)
        lattice = modulated.sample().getOrientedLattice()
        lattice.setModVec1(V3D(0.5, 0.0, 0.5))
        lattice.setModVec2(V3D(0.333, 0.333, 0.))
        for row, peak in enumerate(modulated):
            row_indices = m1m2[row]
            peak.setIntMNP(V3D(row_indices[0], row_indices[1], 0))

        return modulated

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
        self._assert_file_content_equal(reference_result, file_name)

    def test_save_fullprof_format_modulated(self):
        # Arrange
        workspace = self._create_modulated_peak_table()
        reference_result = self._get_reference_result("fullprof_format_modulated.hkl")
        file_name = os.path.join(self._test_dir, "test_fullprof_modulated.hkl")
        output_format = "Fullprof"

        # Act
        SaveReflections(InputWorkspace=workspace, Filename=file_name, Format=output_format)

        # Assert
        self._assert_file_content_equal(reference_result, file_name)

    def test_save_jana_format(self):
        # Arrange
        reference_result = self._get_reference_result("jana_format.hkl")
        file_name = os.path.join(self._test_dir, "test_jana.hkl")
        output_format = "Jana"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format)

        # Assert
        self._assert_file_content_equal(reference_result, file_name)

    def test_save_jana_format_modulated_single_file(self):
        # Arrange
        workspace = self._create_modulated_peak_table()
        reference_result = self._get_reference_result("jana_format_modulated.hkl")
        file_name = os.path.join(self._test_dir, "test_jana_modulated.hkl")
        output_format = "Jana"

        # Act
        SaveReflections(InputWorkspace=workspace, Filename=file_name, Format=output_format)

        # Assert
        self._assert_file_content_equal(reference_result, file_name)

    def test_save_jana_format_modulated_separate_files(self):
        # Arrange
        workspace = self._create_modulated_peak_table()
        reference_results = [
            self._get_reference_result("jana_format_modulated-m{}.hkl".format(i))
            for i in range(1, 3)
        ]
        file_name = os.path.join(self._test_dir, "test_jana_modulated.hkl")
        output_format = "Jana"
        split_files = True

        # Act
        SaveReflections(InputWorkspace=workspace,
                        Filename=file_name,
                        Format=output_format,
                        SplitFiles=split_files)

        # Assert
        self._assert_file_content_equal(reference_results[0],
                                        os.path.join(self._test_dir, "test_jana_modulated-m1.hkl"))
        self._assert_file_content_equal(reference_results[1],
                                        os.path.join(self._test_dir, "test_jana_modulated-m2.hkl"))

    def test_save_GSAS_format(self):
        # Arrange
        reference_result = self._get_reference_result("gsas_format.hkl")
        file_name = os.path.join(self._test_dir, "test_gsas.hkl")
        output_format = "GSAS"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format)

        # Assert
        self._assert_file_content_equal(reference_result, file_name)

    def test_save_GSAS_format_modulated(self):
        # Arrange
        workspace = self._create_modulated_peak_table()
        file_name = os.path.join(self._test_dir, "test_GSAS_modulated.hkl")
        output_format = "GSAS"

        # Act
        self.assertRaises(RuntimeError,
                          SaveReflections,
                          InputWorkspace=workspace,
                          Filename=file_name,
                          Format=output_format)

    def test_save_SHELX_format(self):
        # Arrange
        reference_result = self._get_reference_result("shelx_format.hkl")
        file_name = os.path.join(self._test_dir, "test_shelx.hkl")
        output_format = "SHELX"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format)

        # Assert
        self._assert_file_content_equal(reference_result, file_name)

    def test_save_SHELX_format_modulated(self):
        # Arrange
        workspace = self._create_modulated_peak_table()
        file_name = os.path.join(self._test_dir, "test_SHELX_modulated.hkl")
        output_format = "SHELX"

        # Act
        self.assertRaises(RuntimeError,
                          SaveReflections,
                          InputWorkspace=workspace,
                          Filename=file_name,
                          Format=output_format)

    def test_save_invalid_format(self):
        # Arrange
        file_name = os.path.join(self._test_dir, "test_SHELX_modulated.hkl")
        output_format = "InvalidFormatName"

        # Act
        self.assertRaises(ValueError,
                          SaveReflections,
                          InputWorkspace=self._workspace,
                          Filename=file_name,
                          Format=output_format)

    # Private api
    def _assert_file_content_equal(self, reference_result, file_name):
        with open(reference_result, 'r') as ref_file:
            reference = ref_file.read()
        with open(file_name, 'r') as actual_file:
            actual = actual_file.read()
            self.maxDiff = 10000
        self.assertMultiLineEqual(reference, actual)


if __name__ == '__main__':
    unittest.main()
