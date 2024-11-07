# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest.mock import patch
import tempfile
import os
import shutil
import numpy as np

from mantid.api import FileFinder
from mantid.kernel import V3D
from mantid.simpleapi import (
    CloneWorkspace,
    CreatePeaksWorkspace,
    DeleteWorkspace,
    AnalysisDataService,
    LoadEmptyInstrument,
    SetUB,
    SaveReflections,
    DeleteTableRows,
)


class SaveReflectionsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._test_dir = tempfile.mkdtemp()

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()
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
        run_num = 1
        for i in np.arange(0, 1, 0.1):
            for j in np.arange(-0.5, 0, 0.1):
                q = center_q.copy()
                q[1] += j
                q[2] += i
                # Add the peaks to the PeaksWorkspace with dummy values for intensity,
                # Sigma, and HKL
                peak = ws.createPeak(q)
                peak.setIntensity(100)
                peak.setSigmaIntensity(10)
                peak.setRunNumber(run_num)
                run_num += 1
                peak.setHKL(1, 1, 1)
                peak.setAbsorptionWeightedPathLength(1.0)
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
        lattice.setModVec2(V3D(0.333, 0.333, 0.0))
        for row, peak in enumerate(modulated):
            row_indices = m1m2[row]
            mnp = V3D(row_indices[0], row_indices[1], 0)
            peak.setIntMNP(mnp)
            # update hkl
            modvec = lattice.getModVec(0) * mnp[0] + lattice.getModVec(1) * mnp[1]
            hkl = peak.getHKL() + modvec
            peak.setHKL(hkl[0], hkl[1], hkl[2])

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

    def test_save_fullprof_format_separate_batch_numbers(self):
        # Arrange
        reference_result = self._get_reference_result("fullprof_format_separate_batch_nums.hkl")
        file_name = os.path.join(self._test_dir, "test_fullprof_separate_batch_nums.hkl")
        output_format = "Fullprof"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format, SeparateBatchNumbers=True)

        # Assert
        self._assert_file_content_equal(reference_result, file_name)

    def test_save_fullprof_format_scale_large_intensities(self):
        file_name = os.path.join(self._test_dir, "test_fullprof_scaled.hkl")
        lines = self._test_helper_scale_large_intensities(output_format="Fullprof", file_name=file_name)
        intensity = float(lines[-1].split()[3])  # last line (only peak)
        self.assertEqual(intensity, 99999999.99)

    def test_save_fullprof_format_constant_wavelength(self):
        # Arrange
        # leave only one peak (therefore a single wavelength in table)
        DeleteTableRows(TableWorkspace=self._workspace, Rows=f"1-{self._workspace.getNumberPeaks() - 1:.0f}")
        file_name = os.path.join(self._test_dir, "test_fullprof_cw.hkl")
        output_format = "Fullprof"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format)

        # Assert
        wavelength = 0
        with open(file_name, "r") as file:
            file.readline()  # skip
            file.readline()  # skip
            wavelength = float(file.readline().lstrip().split(" ")[0])
        self.assertAlmostEqual(wavelength, self._workspace.getPeak(0).getWavelength(), delta=1e-4)

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

    def test_save_jana_format_separate_batch_numbers(self):
        # Arrange
        reference_result = self._get_reference_result("jana_format_separate_batch_nums.hkl")
        file_name = os.path.join(self._test_dir, "test_jana_separate_batch_numbers.hkl")
        output_format = "Jana"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format, SeparateBatchNumbers=True)

        # Assert
        self._assert_file_content_equal(reference_result, file_name)

    def test_save_jana_format_scale_large_intensities(self):
        file_name = os.path.join(self._test_dir, "test_jana_scaled.hkl")
        lines = self._test_helper_scale_large_intensities(output_format="Jana", file_name=file_name)
        intensity = float(lines[-1].split()[3])  # last line (only peak)
        self.assertEqual(intensity, 99999999.99)

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
        reference_results = [self._get_reference_result("jana_format_modulated-m{}.hkl".format(i)) for i in range(1, 3)]
        file_name = os.path.join(self._test_dir, "test_jana_modulated.hkl")
        output_format = "Jana"
        split_files = True

        # Act
        SaveReflections(InputWorkspace=workspace, Filename=file_name, Format=output_format, SplitFiles=split_files)

        # Assert
        self._assert_file_content_equal(reference_results[0], os.path.join(self._test_dir, "test_jana_modulated-m1.hkl"))
        self._assert_file_content_equal(reference_results[1], os.path.join(self._test_dir, "test_jana_modulated-m2.hkl"))

    def test_save_jana_with_no_lattice_information(self):
        peaks = CloneWorkspace(self._workspace)
        peaks.sample().clearOrientedLattice()
        file_name = os.path.join(self._test_dir, "test_jana_no_lattice.hkl")

        # Act
        SaveReflections(InputWorkspace=peaks, Filename=file_name, Format="Jana", SplitFiles=False)

    def test_save_GSAS_format(self):
        # Arrange
        reference_result = self._get_reference_result("gsas_format.hkl")
        file_name = os.path.join(self._test_dir, "test_gsas.hkl")
        output_format = "GSAS"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format)

        # Assert
        self._assert_file_content_equal(reference_result, file_name)

    def test_save_GSAS_format_scale_large_intensities(self):
        file_name = os.path.join(self._test_dir, "test_gsas_scaled.hkl")
        lines = self._test_helper_scale_large_intensities(output_format="GSAS", file_name=file_name)
        intensity = float(lines[0].split()[3])  # first line (no header)
        self.assertEqual(intensity, 9999.99)

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
        self._assert_file_content_equal(reference_result, file_name)

    def test_save_SHELX_format_separate_batch_numbers(self):
        # Arrange
        reference_result = self._get_reference_result("shelx_format_separate_batch_nums.hkl")
        file_name = os.path.join(self._test_dir, "test_shelx_separate_batch_nums.hkl")
        output_format = "SHELX"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format, SeparateBatchNumbers=True)

        # Assert
        self._assert_file_content_equal(reference_result, file_name)

    def test_save_SHELX_format_scale_large_intensities(self):
        file_name = os.path.join(self._test_dir, "test_shelx_scaled.hkl")
        lines = self._test_helper_scale_large_intensities(output_format="SHELX", file_name=file_name)
        intensity = float(lines[0].split()[3])  # first line (no header)
        self.assertEqual(intensity, 9999.99)

    def test_save_SHELX_format_modulated(self):
        # Arrange
        workspace = self._create_modulated_peak_table()
        file_name = os.path.join(self._test_dir, "test_SHELX_modulated.hkl")
        output_format = "SHELX"

        # Act
        self.assertRaisesRegex(
            RuntimeError,
            "Cannot currently save modulated structures to GSAS or SHELX formats",
            SaveReflections,
            InputWorkspace=workspace,
            Filename=file_name,
            Format=output_format,
        )

    @patch("mantid.kernel.logger.warning")
    def test_MinIntensOverSigma(self, mock_log):
        file_name = os.path.join(self._test_dir, "test_shelx_MinIntensOverSigma.hkl")

        # set MinIntensOverSigma
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format="SHELX", MinIntensOverSigma=20)

        mock_log.assert_called_once_with(
            "There are no peaks with Intens/Sigma >= 20.0 in peak workspace ws. An empty file will be produced."
        )

    def test_save_invalid_format(self):
        # Arrange
        file_name = os.path.join(self._test_dir, "test_SHELX_modulated.hkl")
        output_format = "InvalidFormatName"

        # Act
        self.assertRaisesRegex(
            ValueError,
            'The value "InvalidFormatName" is not in the list of allowed values',
            SaveReflections,
            InputWorkspace=self._workspace,
            Filename=file_name,
            Format=output_format,
        )

    @patch("mantid.kernel.logger.warning")
    def test_save_empty_peak_table(self, mock_log):
        empty_peaks = CreatePeaksWorkspace(self._workspace, NumberOfPeaks=0)
        SaveReflections(InputWorkspace=empty_peaks, Filename="test.int", Format="Jana")
        mock_log.assert_called_once_with(
            "There are no peaks with Intens/Sigma >= 0.0 in peak workspace empty_peaks. An empty file will be produced."
        )

    def test_save_lean_peak_table_GSAS(self):
        reference_result = self._get_reference_result("lean_peak_GSAS.hkl")
        file_name = os.path.join(self._test_dir, "test_lean_peak_GSAS.hkl")
        empty_peaks = CreatePeaksWorkspace(self._workspace, NumberOfPeaks=1, OutputType="LeanElasticPeak")

        SaveReflections(InputWorkspace=empty_peaks, Filename=file_name, Format="GSAS")  # calls SaveHKLCW

        self._assert_file_content_equal(reference_result, file_name)

    # Private api
    def _assert_file_content_equal(self, reference_result, file_name):
        with open(reference_result, "r") as ref_file:
            reference = ref_file.read()
        with open(file_name, "r") as actual_file:
            actual = actual_file.read()
            self.maxDiff = 10000
        self.assertMultiLineEqual(reference, actual)

    def _test_helper_scale_large_intensities(self, output_format, file_name):
        # Arrange
        DeleteTableRows(TableWorkspace=self._workspace, Rows=f"1-{self._workspace.getNumberPeaks()-1}")  # only first pk
        self._workspace.getPeak(0).setIntensity(2.5e8)

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format)

        # Read lines from file to be asserted in individual tests
        with open(file_name, "r") as actual_file:
            lines = actual_file.readlines()

        return lines


if __name__ == "__main__":
    unittest.main()
