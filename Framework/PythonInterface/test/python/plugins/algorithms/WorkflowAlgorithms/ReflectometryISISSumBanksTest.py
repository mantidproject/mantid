# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import numpy

from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace, CreateWorkspace
from plugins.algorithms.WorkflowAlgorithms.ReflectometryISISSumBanks import ReflectometryISISSumBanks
from testhelpers import WorkspaceCreationHelper


class ReflectometryISISSumBanksTest(unittest.TestCase):

    def test_validate_inputs(self):
        test_ws = WorkspaceCreationHelper.create2DWorkspaceWithRectangularInstrument(1, 5, 5)

        alg = ReflectometryISISSumBanks()
        alg.initialize()
        alg.setProperty('InputWorkspace', test_ws)

        issues = alg.validateInputs()
        self.assertEqual(len(issues), 0)

    def test_validate_inputs_fails_if_no_instrument(self):
        test_ws = CreateWorkspace(StoreInADS=False, DataX=[1, 2, 3], DataY=[10,20,30])

        alg = ReflectometryISISSumBanks()
        alg.initialize()
        alg.setProperty('InputWorkspace', test_ws)

        issues = alg.validateInputs()
        self.assertEqual(len(issues), 1)
        self.assertEqual(issues['InputWorkspace'], 'The input workspace must have an instrument')

    def test_validate_inputs_fails_if_multiple_rectangular_detectors(self):
        test_ws = WorkspaceCreationHelper.create2DWorkspaceWithRectangularInstrument(2, 5, 5)

        alg = ReflectometryISISSumBanks()
        alg.initialize()
        alg.setProperty('InputWorkspace', test_ws)

        issues = alg.validateInputs()
        self.assertEqual(len(issues), 1)
        self.assertEqual(issues['InputWorkspace'], 'The input workspace must only contain one rectangular detector: multiple were found')

    def test_validate_inputs_fails_if_not_a_rectangular_detector(self):
        test_ws = WorkspaceCreationHelper.createEventWorkspaceWithNonUniformInstrument(1, True)

        alg = ReflectometryISISSumBanks()
        alg.initialize()
        alg.setProperty('InputWorkspace', test_ws)

        issues = alg.validateInputs()
        self.assertEqual(len(issues), 1)
        self.assertEqual(issues['InputWorkspace'], 'The input workspace must contain a rectangular detector')

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

    def test_mask_and_sum_banks(self):
        num_banks = 3
        test_ws = CreateSampleWorkspace(StoreInADS=False, NumBanks=1, BankPixelWidth=num_banks)

        # We have 3x3 pixels, so 9 detectors. Include the first 6, i.e. first 2 banks.
        num_banks_included = 2
        roi_detector_ids = '9-14'
        masked_ws = ReflectometryISISSumBanks().mask_detectors(test_ws, roi_detector_ids)
        summed_ws = ReflectometryISISSumBanks().sum_banks(masked_ws)

        self.assertIsInstance(summed_ws, MatrixWorkspace)
        self.assertNotEqual(test_ws, summed_ws)
        self.assertTrue(numpy.allclose(num_banks_included * test_ws.readY(0), summed_ws.readY(0)))

if __name__ == '__main__':
    unittest.main()
