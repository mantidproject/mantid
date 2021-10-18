# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import numpy

from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace
from plugins.algorithms.WorkflowAlgorithms.ReflectometryISISSumBanks import ReflectometryISISSumBanks


class ReflectometryISISSumBanksTest(unittest.TestCase):

    def test_mask_workspace(self):
        roi_detector_id = 200
        test_ws = CreateSampleWorkspace(StoreInADS=False)
        masked_ws = ReflectometryISISSumBanks().mask_detectors(test_ws, str(roi_detector_id))
        self.assertIsInstance(masked_ws, MatrixWorkspace)
        detector_info = masked_ws.detectorInfo()
        for i in range(100, 300):
            det_index = detector_info.indexOf(i)
            if i == roi_detector_id:
                self.assertFalse(detector_info.isMasked(det_index))
            else:
                self.assertTrue(detector_info.isMasked(det_index))

    def test_sum_banks(self):
        num_banks = 3
        test_ws = CreateSampleWorkspace(StoreInADS=False, NumBanks=1, BankPixelWidth=num_banks)
        summed_ws = ReflectometryISISSumBanks().sum_banks(test_ws)

        self.assertIsInstance(summed_ws, MatrixWorkspace)
        self.assertNotEqual(test_ws, summed_ws)
        self.assertTrue(numpy.allclose(num_banks * test_ws.readY(0), summed_ws.readY(0)))


if __name__ == '__main__':
    unittest.main()
