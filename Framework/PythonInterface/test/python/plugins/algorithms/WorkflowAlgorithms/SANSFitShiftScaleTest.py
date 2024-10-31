# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import AlgorithmManager, FrameworkManager
import numpy as np
from SANSFitShiftScale import ErrorTransferFromModelToData


class SANSFitShiftScaleTest(unittest.TestCase):
    def test_initalize(self):
        alg = AlgorithmManager.create("SANSFitShiftScale")
        alg.setChild(True)
        alg.initialize()
        self.assertTrue(alg.isInitialized())

    def test_permissable_modes(self):
        alg = AlgorithmManager.create("SANSFitShiftScale")
        alg.setChild(True)
        alg.initialize()
        self.assertRaises(ValueError, alg.setProperty, "Mode", "InvalidMode")

    def test_default_mode(self):
        alg = AlgorithmManager.create("SANSFitShiftScale")
        alg.setChild(True)
        alg.initialize()
        self.assertEqual("None", alg.getProperty("Mode").value)

    def test_none_mode_requires_scale_and_shift_factors(self):
        alg = AlgorithmManager.create("SANSFitShiftScale")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "None")
        errors = alg.validateInputs()
        self.assertTrue("ScaleFactor" in errors)
        self.assertTrue("ShiftFactor" in errors)

    def test_fit_scale_requires_shift_factor(self):
        alg = AlgorithmManager.create("SANSFitShiftScale")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "ScaleOnly")
        errors = alg.validateInputs()
        self.assertTrue("ShiftFactor" in errors)

    def test_fit_shift_requires_scale_factor(self):
        alg = AlgorithmManager.create("SANSFitShiftScale")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "ShiftOnly")
        errors = alg.validateInputs()
        self.assertTrue("ScaleFactor" in errors)

    def test_workspace_entries_must_be_q1d(self):
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

        alg = AlgorithmManager.create("SANSFitShiftScale")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "Both")
        alg.setProperty("HABWorkspace", multi_spectra_input)
        alg.setProperty("LABWorkspace", multi_spectra_input)

        errors = alg.validateInputs()
        self.assertTrue("HABWorkspace" in errors)
        self.assertTrue("LABWorkspace" in errors)

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
        alg = AlgorithmManager.create("SANSFitShiftScale")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "None")
        alg.setProperty("HABWorkspace", single_spectra_input)
        alg.setProperty("LABWorkspace", single_spectra_input)
        alg.setProperty("ShiftFactor", in_shift_factor)
        alg.setProperty("ScaleFactor", in_scale_factor)
        alg.execute()
        out_shift_factor = alg.getProperty("OutShiftFactor").value
        out_scale_factor = alg.getProperty("OutScaleFactor").value

        self.assertEqual(out_scale_factor, in_scale_factor)
        self.assertEqual(out_shift_factor, in_shift_factor)

    def test_shift_only(self):
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

        alg = AlgorithmManager.create("SANSFitShiftScale")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "ShiftOnly")
        alg.setProperty("HABWorkspace", hab_workspace)
        alg.setProperty("LABWorkspace", lab_workspace)
        alg.setProperty("ScaleFactor", 1.0)
        alg.setProperty("ShiftFactor", 5.2)
        alg.execute()

        out_shift_factor = alg.getProperty("OutShiftFactor").value
        out_scale_factor = alg.getProperty("OutScaleFactor").value

        self.assertEqual(out_scale_factor, 1.0)
        self.assertEqual(out_shift_factor, -5.0)

    def test_scale_only(self):
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

        alg = AlgorithmManager.create("SANSFitShiftScale")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "ScaleOnly")
        alg.setProperty("HABWorkspace", hab_workspace)
        alg.setProperty("LABWorkspace", lab_workspace)
        alg.setProperty("ShiftFactor", -5.0)
        alg.setProperty("ScaleFactor", 7.2)

        alg.execute()

        out_shift_factor = alg.getProperty("OutShiftFactor").value
        out_scale_factor = alg.getProperty("OutScaleFactor").value

        self.assertEqual(out_scale_factor, 1.0)
        self.assertEqual(out_shift_factor, -5.0)

    def do_test_scale_both(self, hab_range, min_x=0.0, max_x=1000.0):
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.setProperty("DataX", range(0, 10))

        # HAB as linear function y=x+5
        create_alg.setProperty("DataY", hab_range)
        create_alg.execute()
        hab_workspace = create_alg.getProperty("OutputWorkspace").value

        # LAB as linear function y=x+0
        create_alg.setProperty("DataY", range(0, 9))
        create_alg.execute()
        lab_workspace = create_alg.getProperty("OutputWorkspace").value

        alg = AlgorithmManager.create("SANSFitShiftScale")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Mode", "Both")
        alg.setProperty("HABWorkspace", hab_workspace)
        alg.setProperty("LABWorkspace", lab_workspace)
        alg.setProperty("ShiftFactor", -7.6)
        alg.setProperty("ScaleFactor", 2.4)
        alg.setProperty("FitMin", min_x)
        alg.setProperty("FitMax", max_x)
        alg.execute()
        out_shift_factor = alg.getProperty("OutShiftFactor").value
        out_scale_factor = alg.getProperty("OutScaleFactor").value

        self.assertAlmostEqual(out_scale_factor, 1.0, 5)
        self.assertAlmostEqual(out_shift_factor, -5.0, 5)

    def test_scale_both(self):
        hab_range = range(5, 14)
        self.do_test_scale_both(hab_range)

    def test_scale_both_with_nan(self):
        hab_range = list(range(5, 14))
        hab_range[6] = np.nan
        hab_range[7] = np.nan
        self.assertRaisesRegex(
            RuntimeError, "Trying to merge the two reduced data sets for HAB and LAB failed.", self.do_test_scale_both, hab_range
        )

    def test_scale_both_with_q_range(self):
        hab_range = list(range(5, 14))
        hab_range[0] = 15000
        self.do_test_scale_both(hab_range, min_x=1, max_x=10)
        hab_range = list(range(5, 14))
        hab_range[8] = 15000
        self.do_test_scale_both(hab_range, min_x=0, max_x=6)


class TestErrorTransferFromModelToData(unittest.TestCase):
    def _createWorkspace(self, x1, y1, err1, x2, y2, err2):
        create_alg = AlgorithmManager.create("CreateWorkspace")
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty("DataX", x1)
        create_alg.setProperty("DataY", y1)
        create_alg.setProperty("DataE", err1)
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        front = create_alg.getProperty("OutputWorkspace").value

        create_alg.setProperty("DataX", x2)
        create_alg.setProperty("DataY", y2)
        create_alg.setProperty("DataE", err2)
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("UnitX", "MomentumTransfer")
        create_alg.setPropertyValue("OutputWorkspace", "out_ws")
        create_alg.execute()
        rear = create_alg.getProperty("OutputWorkspace").value

        return front, rear

    def test_that_error_is_transferred(self):
        # Arrange
        x1 = [1, 2, 3, 4, 5, 6, 7, 8, 9]
        x2 = [1, 2, 3, 4, 5, 6, 7, 8, 9]
        y1 = [2, 2, 2, 2, 2, 2, 2, 2]
        y2 = [2, 2, 2, 2, 2, 2, 2, 2]
        e1 = [1, 1, 1, 1, 1, 1, 1, 1]
        e2 = [2, 2, 2, 2, 2, 2, 2, 2]
        front, rear = self._createWorkspace(x1, y1, e1, x2, y2, e2)

        x_min = 3
        x_max = 7
        # Act
        error_correction = ErrorTransferFromModelToData()
        f_return, r_return = error_correction.get_error_corrected(front, rear, x_min, x_max)

        # Assert
        self.assertEqual(5, len(f_return.dataX(0)))
        self.assertEqual(5, len(r_return.dataX(0)))

        expected_errors_in_rear = [np.sqrt(5), np.sqrt(5), np.sqrt(5), np.sqrt(5)]
        self.assertEqual(expected_errors_in_rear[0], r_return.dataE(0)[0])
        self.assertEqual(expected_errors_in_rear[1], r_return.dataE(0)[1])
        self.assertEqual(expected_errors_in_rear[2], r_return.dataE(0)[2])
        self.assertEqual(expected_errors_in_rear[3], r_return.dataE(0)[3])


if __name__ == "__main__":
    FrameworkManager.Instance()
    unittest.main()
