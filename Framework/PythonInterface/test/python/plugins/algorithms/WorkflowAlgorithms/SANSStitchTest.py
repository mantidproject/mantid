# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import AlgorithmManager, FrameworkManager, MatrixWorkspace
import numpy as np

from SANSStitch import QErrorCorrectionForMergedWorkspaces


class SANSStitchTest(unittest.TestCase):
    def test_initalize(self):
        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        self.assertTrue(alg.isInitialized())

    def test_permissable_modes(self):
        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        self.assertRaises(ValueError, alg.setProperty, "Mode", "InvalidMode")

    def test_default_mode(self):
        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        self.assertEqual("None", alg.getProperty("Mode").value)

    def test_none_mode_requires_scale_and_shift_factors(self):
        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "None")
        errors = alg.validateInputs()
        self.assertTrue("ScaleFactor" in errors)
        self.assertTrue("ShiftFactor" in errors)

    def test_fit_scale_requires_shift_factor(self):
        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "ScaleOnly")
        errors = alg.validateInputs()
        self.assertTrue("ShiftFactor" in errors)

    def test_fit_shift_requires_scale_factor(self):
        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "ShiftOnly")
        errors = alg.validateInputs()
        self.assertTrue("ScaleFactor" in errors)

    def test_workspace_entries_must_be_q1d_if_fitting_is_enabled(self):
        # create an input workspace that has multiple spectra
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("DataX", range(0, 1))
        create_alg.setProperty("DataY", [1, 2])
        create_alg.setProperty("NSpec", 2)  # Wrong number of spectra
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        multi_spectra_input = create_alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "Both")
        alg.setProperty("HABCountsSample", multi_spectra_input)
        alg.setProperty("LABCountsSample", multi_spectra_input)
        alg.setProperty("HABNormSample", multi_spectra_input)
        alg.setProperty("LABNormSample", multi_spectra_input)

        errors = alg.validateInputs()
        self.assertTrue("HABCountsSample" in errors)
        self.assertTrue("LABCountsSample" in errors)
        self.assertTrue("HABNormSample" in errors)
        self.assertTrue("LABNormSample" in errors)

    def test_can_workspaces_required_if_process_can(self):
        # create an input workspace that has multiple spectra
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("DataX", range(0, 1))
        create_alg.setProperty("DataY", [1])
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        single_spectra_input = create_alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "Both")
        alg.setProperty("HABCountsSample", single_spectra_input)
        alg.setProperty("LABCountsSample", single_spectra_input)
        alg.setProperty("HABNormSample", single_spectra_input)
        alg.setProperty("LABNormSample", single_spectra_input)
        alg.setProperty("ProcessCan", True)  # Now can workspaces should be provided

        errors = alg.validateInputs()
        self.assertTrue("HABCountsCan" in errors)
        self.assertTrue("LABCountsCan" in errors)
        self.assertTrue("HABNormCan" in errors)
        self.assertTrue("LABNormCan" in errors)

    def test_stitch_2d_restricted_to_none(self):
        # create an input workspace that has multiple spectra
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("DataX", range(0, 1))
        create_alg.setProperty("DataY", [1, 1])
        create_alg.setProperty("NSpec", 2)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        double_spectra_input = create_alg.getProperty("OutputWorkspace").value

        # Basic algorithm setup
        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("HABCountsSample", double_spectra_input)
        alg.setProperty("LABCountsSample", double_spectra_input)
        alg.setProperty("HABNormSample", double_spectra_input)
        alg.setProperty("LABNormSample", double_spectra_input)
        alg.setProperty("ProcessCan", False)
        alg.setProperty("ShiftFactor", 1.0)
        alg.setProperty("ScaleFactor", 0.0)

        # 2D inputs Should not be allowed for mode Both
        alg.setProperty("Mode", "Both")
        errors = alg.validateInputs()
        self.assertTrue("HABCountsSample" in errors)
        self.assertTrue("LABCountsSample" in errors)
        self.assertTrue("HABNormSample" in errors)
        self.assertTrue("LABNormSample" in errors)

        # 2D inputs Should not be allowed for mode ScaleOnly
        alg.setProperty("Mode", "ScaleOnly")
        errors = alg.validateInputs()
        self.assertTrue("HABCountsSample" in errors)
        self.assertTrue("LABCountsSample" in errors)
        self.assertTrue("HABNormSample" in errors)
        self.assertTrue("LABNormSample" in errors)

        # 2D inputs Should not be allowed for mode ShiftOnly
        alg.setProperty("Mode", "ShiftOnly")
        errors = alg.validateInputs()
        self.assertTrue("HABCountsSample" in errors)
        self.assertTrue("LABCountsSample" in errors)
        self.assertTrue("HABNormSample" in errors)
        self.assertTrue("LABNormSample" in errors)

        # With no fitting 2D inputs are allowed
        alg.setProperty("Mode", "None")
        errors = alg.validateInputs()
        self.assertEqual(0, len(errors))

    def test_scale_none(self):
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("DataX", range(0, 10))
        create_alg.setProperty("DataY", [1] * 9)
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        single_spectra_input = create_alg.getProperty("OutputWorkspace").value

        in_scale_factor = 1.0
        in_shift_factor = 1.0
        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "None")
        alg.setProperty("HABCountsSample", single_spectra_input)
        alg.setProperty("LABCountsSample", single_spectra_input)
        alg.setProperty("HABNormSample", single_spectra_input)
        alg.setProperty("LABNormSample", single_spectra_input)
        alg.setProperty("OutputWorkspace", "dummy_name")
        alg.setProperty("ShiftFactor", in_shift_factor)
        alg.setProperty("ScaleFactor", in_scale_factor)
        alg.execute()
        out_ws = alg.getProperty("OutputWorkspace").value
        out_shift_factor = alg.getProperty("OutShiftFactor").value
        out_scale_factor = alg.getProperty("OutScaleFactor").value

        self.assertTrue(isinstance(out_ws, MatrixWorkspace))

        self.assertEqual(out_scale_factor, in_scale_factor)
        self.assertEqual(out_shift_factor, in_shift_factor)
        y_array = out_ws.readY(0)

        expected_y_array = [1.5] * 9
        np.testing.assert_equal(y_array, expected_y_array)

    def test_strip_special_values(self):
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("DataX", range(0, 10))
        y_data = np.array([1] * 7)
        y_data = np.append(y_data, [np.nan])
        y_data = np.append(y_data, [np.inf])
        create_alg.setProperty("DataY", y_data)
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        single_spectra_input = create_alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "Both")
        alg.setProperty("HABCountsSample", single_spectra_input)
        alg.setProperty("LABCountsSample", single_spectra_input)
        alg.setProperty("HABNormSample", single_spectra_input)
        alg.setProperty("LABNormSample", single_spectra_input)
        alg.setProperty("OutputWorkspace", "dummy_name")
        # This would throw at the point of fitting in NaNs or infs where present
        alg.execute()

    def test_scale_none_with_can(self):
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("DataX", range(0, 10))
        create_alg.setProperty("DataY", [1] * 9)
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        single_spectra_input = create_alg.getProperty("OutputWorkspace").value

        create_alg.setProperty("DataY", [0.5] * 9)
        create_alg.execute()
        smaller_single_spectra_input = create_alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "None")
        alg.setProperty("HABCountsSample", single_spectra_input)
        alg.setProperty("LABCountsSample", single_spectra_input)
        alg.setProperty("HABNormSample", single_spectra_input)
        alg.setProperty("LABNormSample", single_spectra_input)
        alg.setProperty("ProcessCan", True)
        alg.setProperty("HABCountsCan", smaller_single_spectra_input)
        alg.setProperty("LABCountsCan", smaller_single_spectra_input)
        alg.setProperty("HABNormCan", single_spectra_input)
        alg.setProperty("LABNormCan", single_spectra_input)
        alg.setProperty("OutputWorkspace", "dummy_name")
        alg.setProperty("ShiftFactor", 0.0)
        alg.setProperty("ScaleFactor", 1.0)
        alg.execute()
        out_ws = alg.getProperty("OutputWorkspace").value

        self.assertTrue(isinstance(out_ws, MatrixWorkspace))

        y_array = out_ws.readY(0)

        expected_y_array = [0.5] * 9

        np.testing.assert_equal(y_array, expected_y_array)

    def test_scale_both_without_can(self):
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.setProperty("DataX", range(0, 10))

        # HAB as linear function y=x+5
        create_alg.setProperty("DataY", range(5, 14))
        create_alg.execute()
        hab_workspace = create_alg.getProperty("OutputWorkspace").value

        # LAB as linear function y=x+0
        create_alg.setProperty("DataY", range(0, 9))
        create_alg.execute()
        lab_workspace = create_alg.getProperty("OutputWorkspace").value

        # FLAT NORM
        create_alg.setProperty("DataY", [1] * 9)
        create_alg.execute()
        flat_norm = create_alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "Both")
        alg.setProperty("HABCountsSample", hab_workspace)
        alg.setProperty("LABCountsSample", lab_workspace)
        alg.setProperty("HABNormSample", flat_norm)
        alg.setProperty("LABNormSample", flat_norm)
        alg.setProperty("OutputWorkspace", "dummy_name")
        alg.execute()
        out_ws = alg.getProperty("OutputWorkspace").value
        out_shift_factor = alg.getProperty("OutShiftFactor").value
        out_scale_factor = alg.getProperty("OutScaleFactor").value

        self.assertEqual(out_scale_factor, 1.0)
        self.assertEqual(out_shift_factor, -5.0)

        y_array = out_ws.readY(0)

        expected_y_array = lab_workspace.readY(0)  # We scale and shift to the back (lab) detectors

        np.testing.assert_equal(y_array, expected_y_array)

    def test_scale_both_without_can_with_q_fit_range(self):
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.setProperty("DataX", range(0, 12))

        # HAB as linear function y=x+5
        hab_range = list(range(5, 16))
        hab_range[0] = 15000
        hab_range[9] = 15000
        create_alg.setProperty("DataY", hab_range)
        create_alg.execute()
        hab_workspace = create_alg.getProperty("OutputWorkspace").value

        # LAB as linear function y=x+0
        create_alg.setProperty("DataY", range(0, 11))
        create_alg.execute()
        lab_workspace = create_alg.getProperty("OutputWorkspace").value

        # FLAT NORM
        create_alg.setProperty("DataY", [1] * 11)
        create_alg.execute()
        flat_norm = create_alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "Both")
        alg.setProperty("HABCountsSample", hab_workspace)
        alg.setProperty("LABCountsSample", lab_workspace)
        alg.setProperty("HABNormSample", flat_norm)
        alg.setProperty("LABNormSample", flat_norm)
        alg.setProperty("OutputWorkspace", "dummy_name")
        alg.setProperty("FitMin", 1)
        alg.setProperty("FitMax", 9)
        alg.execute()
        out_ws = alg.getProperty("OutputWorkspace").value
        out_shift_factor = alg.getProperty("OutShiftFactor").value
        out_scale_factor = alg.getProperty("OutScaleFactor").value
        self.assertEqual(out_scale_factor, 1.0)
        self.assertEqual(out_shift_factor, -5.0)

        out_ws = alg.getProperty("OutputWorkspace").value

        y_array = out_ws.readY(0)

        expected_y_array = [7497.5, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 7502, 10.0]  # We scale and shift to the back (lab) detectors

        np.testing.assert_equal(y_array, expected_y_array)

    def test_shift_only_without_can(self):
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.setProperty("DataX", range(0, 10))

        # HAB as linear function y=x+5
        create_alg.setProperty("DataY", range(5, 14))
        create_alg.execute()
        hab_workspace = create_alg.getProperty("OutputWorkspace").value

        # LAB as linear function y=x+0
        create_alg.setProperty("DataY", range(0, 9))
        create_alg.execute()
        lab_workspace = create_alg.getProperty("OutputWorkspace").value

        # FLAT NORM
        create_alg.setProperty("DataY", [1] * 9)
        create_alg.execute()
        flat_norm = create_alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "ShiftOnly")
        alg.setProperty("HABCountsSample", hab_workspace)
        alg.setProperty("LABCountsSample", lab_workspace)
        alg.setProperty("HABNormSample", flat_norm)
        alg.setProperty("LABNormSample", flat_norm)
        alg.setProperty("ScaleFactor", 1.0)
        alg.setProperty("OutputWorkspace", "dummy_name")

        alg.execute()
        out_ws = alg.getProperty("OutputWorkspace").value

        y_array = out_ws.readY(0)

        expected_y_array = lab_workspace.readY(0)  # We scale and shift to the back (lab) detectors
        np.testing.assert_equal(y_array, expected_y_array)

    def test_scale_only_without_can(self):
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.setProperty("DataX", range(0, 10))

        # HAB as linear function y=x+5
        create_alg.setProperty("DataY", range(5, 14))
        create_alg.execute()
        hab_workspace = create_alg.getProperty("OutputWorkspace").value

        # LAB as linear function y=x+0
        create_alg.setProperty("DataY", range(0, 9))
        create_alg.execute()
        lab_workspace = create_alg.getProperty("OutputWorkspace").value

        # FLAT NORM
        create_alg.setProperty("DataY", [1] * 9)
        create_alg.execute()
        flat_norm = create_alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "ScaleOnly")
        alg.setProperty("HABCountsSample", hab_workspace)
        alg.setProperty("LABCountsSample", lab_workspace)
        alg.setProperty("HABNormSample", flat_norm)
        alg.setProperty("LABNormSample", flat_norm)
        alg.setProperty("ShiftFactor", -5.0)
        alg.setProperty("OutputWorkspace", "dummy_name")

        alg.execute()
        out_ws = alg.getProperty("OutputWorkspace").value

        y_array = out_ws.readY(0)

        expected_y_array = lab_workspace.readY(0)  # We scale and shift to the back (lab) detectors

        np.testing.assert_equal(y_array, expected_y_array)

    def test_scale_none_with_can_and_q_merge_range_equal(self):
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("DataX", range(0, 10))
        create_alg.setProperty("DataY", [1] * 9)
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        single_spectra_input = create_alg.getProperty("OutputWorkspace").value
        create_alg.setProperty("DataY", [2] * 9)
        create_alg.execute()
        single_spectra_input_HAB = create_alg.getProperty("OutputWorkspace").value
        create_alg.setProperty("DataY", [0.5] * 9)
        create_alg.execute()
        smaller_single_spectra_input = create_alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "None")
        alg.setProperty("HABCountsSample", single_spectra_input_HAB)
        alg.setProperty("LABCountsSample", single_spectra_input)
        alg.setProperty("HABNormSample", single_spectra_input)
        alg.setProperty("LABNormSample", single_spectra_input)
        alg.setProperty("ProcessCan", True)
        alg.setProperty("HABCountsCan", smaller_single_spectra_input)
        alg.setProperty("LABCountsCan", smaller_single_spectra_input)
        alg.setProperty("HABNormCan", single_spectra_input)
        alg.setProperty("LABNormCan", single_spectra_input)
        alg.setProperty("OutputWorkspace", "dummy_name")
        alg.setProperty("ShiftFactor", 0.0)
        alg.setProperty("ScaleFactor", 1.0)
        alg.setProperty("MergeMask", True)
        alg.setProperty("MergeMin", 5)
        alg.setProperty("MergeMax", 5)
        alg.execute()
        out_ws = alg.getProperty("OutputWorkspace").value

        self.assertTrue(isinstance(out_ws, MatrixWorkspace))

        y_array = out_ws.readY(0)

        expected_y_array = [0.5] * 5 + [1.5] * 4

        np.testing.assert_equal(y_array, expected_y_array)

    def test_scale_none_with_can_and_q_merge_range(self):
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("DataX", range(0, 10))
        create_alg.setProperty("DataY", [1] * 9)
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        single_spectra_input = create_alg.getProperty("OutputWorkspace").value
        create_alg.setProperty("DataY", [2] * 9)
        create_alg.execute()
        single_spectra_input_HAB = create_alg.getProperty("OutputWorkspace").value
        create_alg.setProperty("DataY", [0.5] * 9)
        create_alg.execute()
        smaller_single_spectra_input = create_alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "None")
        alg.setProperty("HABCountsSample", single_spectra_input_HAB)
        alg.setProperty("LABCountsSample", single_spectra_input)
        alg.setProperty("HABNormSample", single_spectra_input)
        alg.setProperty("LABNormSample", single_spectra_input)
        alg.setProperty("ProcessCan", True)
        alg.setProperty("HABCountsCan", smaller_single_spectra_input)
        alg.setProperty("LABCountsCan", smaller_single_spectra_input)
        alg.setProperty("HABNormCan", single_spectra_input)
        alg.setProperty("LABNormCan", single_spectra_input)
        alg.setProperty("OutputWorkspace", "dummy_name")
        alg.setProperty("ShiftFactor", 0.0)
        alg.setProperty("ScaleFactor", 1.0)
        alg.setProperty("MergeMask", True)
        alg.setProperty("MergeMin", 2)
        alg.setProperty("MergeMax", 7)
        alg.execute()
        out_ws = alg.getProperty("OutputWorkspace").value

        self.assertTrue(isinstance(out_ws, MatrixWorkspace))

        y_array = out_ws.readY(0)

        expected_y_array = [0.5] * 2 + [1.0] * 5 + [1.5] * 2

        np.testing.assert_equal(y_array, expected_y_array)

    def test_that_merge_range_greater_than_overlap_bounds_set_to_upper_bound(self):
        # This tests that if a merge_max or merge_min is specified greater than the overlap region of
        # the HAB and LAB the relevant value is set to the maximum value.
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("DataX", range(0, 10))
        create_alg.setProperty("DataY", [1] * 9)
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        single_spectra_input = create_alg.getProperty("OutputWorkspace").value
        create_alg.setProperty("DataY", [2] * 9)
        create_alg.execute()
        single_spectra_input_HAB = create_alg.getProperty("OutputWorkspace").value
        create_alg.setProperty("DataY", [0.5] * 9)
        create_alg.execute()
        smaller_single_spectra_input = create_alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "None")
        alg.setProperty("HABCountsSample", single_spectra_input_HAB)
        alg.setProperty("LABCountsSample", single_spectra_input)
        alg.setProperty("HABNormSample", single_spectra_input)
        alg.setProperty("LABNormSample", single_spectra_input)
        alg.setProperty("ProcessCan", True)
        alg.setProperty("HABCountsCan", smaller_single_spectra_input)
        alg.setProperty("LABCountsCan", smaller_single_spectra_input)
        alg.setProperty("HABNormCan", single_spectra_input)
        alg.setProperty("LABNormCan", single_spectra_input)
        alg.setProperty("OutputWorkspace", "dummy_name")
        alg.setProperty("ShiftFactor", 0.0)
        alg.setProperty("ScaleFactor", 1.0)
        alg.setProperty("MergeMask", True)
        alg.setProperty("MergeMin", 50)
        alg.setProperty("MergeMax", 50)
        alg.execute()
        out_ws = alg.getProperty("OutputWorkspace").value

        self.assertTrue(isinstance(out_ws, MatrixWorkspace))

        y_array = out_ws.readY(0)

        expected_y_array = [0.5] * 9
        np.testing.assert_equal(y_array, expected_y_array)

    def test_that_merge_range_less_than_overlap_bounds_set_to_lower_bound(self):
        # This tests that if a merge_max or merge_min is specified greater than the overlap region of
        # the HAB and LAB the relevant value is set to the maximum value.
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("DataX", range(0, 10))
        create_alg.setProperty("DataY", [1] * 9)
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        single_spectra_input = create_alg.getProperty("OutputWorkspace").value
        create_alg.setProperty("DataY", [2] * 9)
        create_alg.execute()
        single_spectra_input_HAB = create_alg.getProperty("OutputWorkspace").value
        create_alg.setProperty("DataY", [0.5] * 9)
        create_alg.execute()
        smaller_single_spectra_input = create_alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "None")
        alg.setProperty("HABCountsSample", single_spectra_input_HAB)
        alg.setProperty("LABCountsSample", single_spectra_input)
        alg.setProperty("HABNormSample", single_spectra_input)
        alg.setProperty("LABNormSample", single_spectra_input)
        alg.setProperty("ProcessCan", True)
        alg.setProperty("HABCountsCan", smaller_single_spectra_input)
        alg.setProperty("LABCountsCan", smaller_single_spectra_input)
        alg.setProperty("HABNormCan", single_spectra_input)
        alg.setProperty("LABNormCan", single_spectra_input)
        alg.setProperty("OutputWorkspace", "dummy_name")
        alg.setProperty("ShiftFactor", 0.0)
        alg.setProperty("ScaleFactor", 1.0)
        alg.setProperty("MergeMask", True)
        alg.setProperty("MergeMin", 0)
        alg.setProperty("MergeMax", 0)
        alg.execute()
        out_ws = alg.getProperty("OutputWorkspace").value

        self.assertTrue(isinstance(out_ws, MatrixWorkspace))

        y_array = out_ws.readY(0)

        expected_y_array = [1.0] + [1.5] * 8

        np.testing.assert_equal(y_array, expected_y_array)

    def test_that_zero_merge_range_has_discrete_transition(self):
        # This tests that if a merge_max or merge_min is specified greater than the overlap region of
        # the HAB and LAB the relevant value is set to the maximum value.
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("DataX", range(0, 10))
        create_alg.setProperty("DataY", [1] * 9)
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        single_spectra_input = create_alg.getProperty("OutputWorkspace").value
        create_alg.setProperty("DataY", [2] * 9)
        create_alg.execute()
        single_spectra_input_HAB = create_alg.getProperty("OutputWorkspace").value
        create_alg.setProperty("DataY", [0.5] * 9)
        create_alg.execute()
        smaller_single_spectra_input = create_alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "None")
        alg.setProperty("HABCountsSample", single_spectra_input_HAB)
        alg.setProperty("LABCountsSample", single_spectra_input)
        alg.setProperty("HABNormSample", single_spectra_input)
        alg.setProperty("LABNormSample", single_spectra_input)
        alg.setProperty("ProcessCan", True)
        alg.setProperty("HABCountsCan", smaller_single_spectra_input)
        alg.setProperty("LABCountsCan", smaller_single_spectra_input)
        alg.setProperty("HABNormCan", single_spectra_input)
        alg.setProperty("LABNormCan", single_spectra_input)
        alg.setProperty("OutputWorkspace", "dummy_name")
        alg.setProperty("ShiftFactor", 0.0)
        alg.setProperty("ScaleFactor", 1.0)
        alg.setProperty("MergeMask", True)
        alg.setProperty("MergeMin", 5)
        alg.setProperty("MergeMax", 5)
        alg.execute()
        out_ws = alg.getProperty("OutputWorkspace").value

        self.assertTrue(isinstance(out_ws, MatrixWorkspace))

        y_array = out_ws.readY(0)

        expected_y_array = [0.5] * 5 + [1.5] * 4

        np.testing.assert_equal(y_array, expected_y_array)

    def test_that_can_merge_2D_reduction_when_fitting_set_to_none(self):
        # create an input workspace that has multiple spectra
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("DataX", range(0, 2))
        create_alg.setProperty("NSpec", 2)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setProperty("VerticalAxisUnit", "MomentumTransfer")
        create_alg.setProperty("VerticalAxisValues", range(0, 2))

        # hab counts
        create_alg.setProperty("DataY", [1, 1, 1, 1])
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        hab_counts = create_alg.getProperty("OutputWorkspace").value

        # hab norm
        create_alg.setProperty("DataY", [2, 2, 2, 2])
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        hab_norm = create_alg.getProperty("OutputWorkspace").value

        # lab counts
        create_alg.setProperty("DataY", [3, 3, 3, 3])
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        lab_counts = create_alg.getProperty("OutputWorkspace").value

        # lab norm
        create_alg.setProperty("DataY", [4, 4, 4, 4])
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        lab_norm = create_alg.getProperty("OutputWorkspace").value

        # Basic algorithm setup
        alg = AlgorithmManager.create("SANSStitch")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("HABCountsSample", hab_counts)
        alg.setProperty("LABCountsSample", lab_counts)
        alg.setProperty("HABNormSample", hab_norm)
        alg.setProperty("LABNormSample", lab_norm)
        alg.setProperty("ProcessCan", False)
        alg.setProperty("ShiftFactor", 0.0)
        alg.setProperty("ScaleFactor", 1.0)
        alg.setProperty("Mode", "None")
        alg.setProperty("OutputWorkspace", "dummy_name")

        errors = alg.validateInputs()
        self.assertEqual(0, len(errors))

        alg.execute()
        out_ws = alg.getProperty("OutputWorkspace").value
        self.assertTrue(isinstance(out_ws, MatrixWorkspace))
        self.assertEqual(out_ws.getNumberHistograms(), 2)
        expected_entries = (1.0 + 3.0) / (2.0 + 4.0)
        delta = 1e-5
        for index in range(0, 2):
            for element in out_ws.dataY(index):
                self.assertTrue(abs(expected_entries - element) < delta)


class TestQErrorCorrectionForMergedWorkspaces(unittest.TestCase):
    def _provide_workspace_with_x_errors(
        self,
        workspace_name,
        use_xerror=True,
        nspec=1,
        x_in=[1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
        y_in=[2, 2, 2, 2, 2, 2, 2, 2, 2],
        e_in=[1, 1, 1, 1, 1, 1, 1, 1, 1],
        x_error=[1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9],
    ):
        x = []
        y = []
        e = []
        for item in range(0, nspec):
            x = x + x_in
            y = y + y_in
            e = e + e_in
        ws_alg = AlgorithmManager.createUnmanaged("CreateWorkspace")
        ws_alg.initialize()
        ws_alg.setChild(True)
        ws_alg.setProperty("DataX", x)
        ws_alg.setProperty("DataY", y)
        ws_alg.setProperty("DataE", e)
        ws_alg.setProperty("NSpec", nspec)
        ws_alg.setProperty("UnitX", "MomentumTransfer")
        ws_alg.setProperty("OutputWorkspace", workspace_name)
        ws_alg.execute()

        ws = ws_alg.getProperty("OutputWorkspace").value
        if use_xerror:
            for hists in range(0, nspec):
                x_error_array = np.asarray(x_error)
                ws.setDx(hists, x_error_array)
        return ws

    def test_error_is_ignored_for_more_than_one_spectrum(self):
        # Arrange
        front_name = "front"
        rear_name = "rear"
        result_name = "result"
        front = self._provide_workspace_with_x_errors(front_name, True, 2)
        rear = self._provide_workspace_with_x_errors(rear_name, True, 2)
        result = self._provide_workspace_with_x_errors(result_name, False, 2)

        scale = 2.0
        # Act
        q_correction = QErrorCorrectionForMergedWorkspaces()
        q_correction.correct_q_resolution_for_merged(front, rear, result, scale)
        # Assert
        self.assertFalse(result.hasDx(0))

    def test_error_is_ignored_when_only_one_input_has_dx(self):
        # Arrange
        front_name = "front"
        rear_name = "rear"
        result_name = "result"
        front = self._provide_workspace_with_x_errors(front_name, True, 1)
        rear = self._provide_workspace_with_x_errors(rear_name, False, 1)
        result = self._provide_workspace_with_x_errors(result_name, False, 1)
        scale = 2.0
        # Act
        q_correction = QErrorCorrectionForMergedWorkspaces()
        q_correction.correct_q_resolution_for_merged(front, rear, result, scale)
        # Assert
        self.assertFalse(result.hasDx(0))

    def test_that_non_matching_workspaces_are_detected(self):
        # Arrange
        front_name = "front"
        rear_name = "rear"
        result_name = "result"
        x1 = [1, 2, 3]
        e1 = [1, 1]
        y1 = [2, 2]
        dx1 = [1.0, 2.0]
        x2 = [1, 2, 3, 4]
        e2 = [1, 1, 1]
        y2 = [2, 2, 2]
        dx2 = [1.0, 2.0, 3.0]
        front = self._provide_workspace_with_x_errors(front_name, True, 1, x1, y1, e1, dx1)
        rear = self._provide_workspace_with_x_errors(rear_name, True, 1, x2, y2, e2, dx2)
        result = self._provide_workspace_with_x_errors(result_name, False, 1)
        scale = 2.0
        # Act
        q_correction = QErrorCorrectionForMergedWorkspaces()
        q_correction.correct_q_resolution_for_merged(front, rear, result, scale)
        # Assert
        self.assertFalse(result.hasDx(0))

    def test_correct_x_error_is_produced(self):
        # Arrange
        x = [1, 2, 3]
        e = [1, 1]
        y_front = [2, 2]
        dx_front = [1.0, 2.0]
        y_rear = [1.5, 1.5]
        dx_rear = [3.0, 2.0]
        front_name = "front"
        rear_name = "rear"
        result_name = "result"
        front = self._provide_workspace_with_x_errors(front_name, True, 1, x, y_front, e, dx_front)
        rear = self._provide_workspace_with_x_errors(rear_name, True, 1, x, y_rear, e, dx_rear)
        result = self._provide_workspace_with_x_errors(result_name, False, 1, x, y_front, e)
        scale = 2.0
        # Act
        q_correction = QErrorCorrectionForMergedWorkspaces()
        q_correction.correct_q_resolution_for_merged(front, rear, result, scale)
        # Assert
        self.assertTrue(result.hasDx(0))

        dx_expected_0 = (dx_front[0] * y_front[0] * scale + dx_rear[0] * y_rear[0]) / (y_front[0] * scale + y_rear[0])
        dx_expected_1 = (dx_front[1] * y_front[1] * scale + dx_rear[1] * y_rear[1]) / (y_front[1] * scale + y_rear[1])
        dx_result = result.readDx(0)
        self.assertEqual(len(dx_result), 2)
        self.assertEqual(dx_result[0], dx_expected_0)
        self.assertEqual(dx_result[1], dx_expected_1)


if __name__ == "__main__":
    FrameworkManager.Instance()
    unittest.main()
