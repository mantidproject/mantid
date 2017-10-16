import unittest
import tempfile
import filecmp
import os
import shutil
import numpy as np

from mantid.api import FileFinder
from mantid.simpleapi import SaveReflections, DeleteWorkspace, LoadEmptyInstrument, CreatePeaksWorkspace, SetUB


class SaveReflectionsTest(unittest.TestCase):

    def setUp(self):
        self._workspace = self._create_peaks_workspace()
        self._test_dir = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self._test_dir)
        DeleteWorkspace(self._workspace)

    def _create_peaks_workspace(self):
        """Create a dummy peaks workspace"""
        path = FileFinder.getFullPath("IDFs_for_UNIT_TESTING/MINITOPAZ_Definition.xml")
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
        self.assertTrue(filecmp.cmp(reference_result, file_name))

    def test_save_jana_format(self):
        # Arrange
        reference_result = self._get_reference_result("jana_format.hkl")
        file_name = os.path.join(self._test_dir, "test_jana.hkl")
        output_format = "Jana"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format)

        # Assert
        self.assertTrue(filecmp.cmp(reference_result, file_name))

    def test_save_GSAS_format(self):
        # Arrange
        reference_result = self._get_reference_result("gsas_format.hkl")
        file_name = os.path.join(self._test_dir, "test_gsas.hkl")
        output_format = "GSAS"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format)

        # Assert
        self.assertTrue(filecmp.cmp(reference_result, file_name))

    def test_save_SHELX_format(self):
        # Arrange
        reference_result = self._get_reference_result("shelx_format.hkl")
        file_name = os.path.join(self._test_dir, "test_shelx.hkl")
        output_format = "SHELX"

        # Act
        SaveReflections(InputWorkspace=self._workspace, Filename=file_name, Format=output_format)

        # Assert
        self.assertTrue(filecmp.cmp(reference_result, file_name))


if __name__ == '__main__':
    unittest.main()
