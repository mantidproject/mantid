import unittest
from mantid.api import AlgorithmManager, MatrixWorkspace
import numpy as np


class SANSStitch1DTest(unittest.TestCase):
    def test_initalize(self):
        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        self.assertTrue(alg.isInitialized())

    def test_permissable_modes(self):
        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        self.assertRaises(ValueError, alg.setProperty, 'Mode', 'InvalidMode')

    def test_default_mode(self):
        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        self.assertEquals('None', alg.getProperty('Mode').value)

    def test_q_forms_valid_range(self):
        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'Both')

        # Same as
        alg.setProperty('QMin', 1.0)
        alg.setProperty('QMax', 1.0)
        errors = alg.validateInputs()
        self.assertTrue('QMin' in errors)
        self.assertTrue('QMax' in errors)

        # Less than
        alg.setProperty('QMin', 1.0)
        alg.setProperty('QMax', 0.0)
        errors = alg.validateInputs()
        self.assertTrue('QMin' in errors)
        self.assertTrue('QMax' in errors)

        # Fine
        alg.setProperty('QMin', 0.0)
        alg.setProperty('QMax', 1.0)
        errors = alg.validateInputs()
        self.assertEquals(0, len(errors))

    def test_none_mode_requires_scale_and_shift_factors(self):
        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'None')
        errors = alg.validateInputs()
        self.assertTrue('ScaleFactor' in errors)
        self.assertTrue('ShiftFactor' in errors)

    def test_fit_scale_requires_shift_factor(self):
        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'ScaleOnly')
        errors = alg.validateInputs()
        self.assertTrue('ShiftFactor' in errors)

    def test_fit_shift_requires_scale_factor(self):
        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'ShiftOnly')
        errors = alg.validateInputs()
        self.assertTrue('ScaleFactor' in errors)

    def test_workspace_entries_must_be_q1d(self):
        # create an input workspace that has multiple spectra
        create_alg = AlgorithmManager.create('CreateWorkspace')
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty('DataX', range(0, 1))
        create_alg.setProperty('DataY', [1, 2])
        create_alg.setProperty('NSpec', 2)  # Wrong number of spectra
        create_alg.setProperty('UnitX', 'MomentumTransfer')
        create_alg.setPropertyValue('OutputWorkspace', 'out_ws')
        create_alg.execute()
        multi_spectra_input = create_alg.getProperty('OutputWorkspace').value

        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'Both')
        alg.setProperty('HABCountsSample', multi_spectra_input)
        alg.setProperty('LABCountsSample', multi_spectra_input)
        alg.setProperty('HABNormSample', multi_spectra_input)
        alg.setProperty('LABNormSample', multi_spectra_input)

        errors = alg.validateInputs()
        self.assertTrue('HABCountsSample' in errors)
        self.assertTrue('LABCountsSample' in errors)
        self.assertTrue('HABNormSample' in errors)
        self.assertTrue('LABNormSample' in errors)

    def test_can_workspaces_required_if_process_can(self):
        # create an input workspace that has multiple spectra
        create_alg = AlgorithmManager.create('CreateWorkspace')
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty('DataX', range(0, 1))
        create_alg.setProperty('DataY', [1])
        create_alg.setProperty('NSpec', 1)
        create_alg.setProperty('UnitX', 'MomentumTransfer')
        create_alg.setPropertyValue('OutputWorkspace', 'out_ws')
        create_alg.execute()
        single_spectra_input = create_alg.getProperty('OutputWorkspace').value

        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'Both')
        alg.setProperty('HABCountsSample', single_spectra_input)
        alg.setProperty('LABCountsSample', single_spectra_input)
        alg.setProperty('HABNormSample', single_spectra_input)
        alg.setProperty('LABNormSample', single_spectra_input)
        alg.setProperty('ProcessCan', True)  # Now can workspaces should be provided

        errors = alg.validateInputs()
        self.assertTrue('HABCountsCan' in errors)
        self.assertTrue('LABCountsCan' in errors)
        self.assertTrue('HABNormCan' in errors)
        self.assertTrue('LABNormCan' in errors)


    def test_stitch_2d_restricted_to_none(self):
        # create an input workspace that has multiple spectra
        create_alg = AlgorithmManager.create('CreateWorkspace')
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty('DataX', range(0, 1))
        create_alg.setProperty('DataY', [1,1])
        create_alg.setProperty('NSpec', 2)
        create_alg.setProperty('UnitX', 'MomentumTransfer')
        create_alg.setPropertyValue('OutputWorkspace', 'out_ws')
        create_alg.execute()
        double_spectra_input = create_alg.getProperty('OutputWorkspace').value

        # Basic algorithm setup
        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('HABCountsSample', double_spectra_input)
        alg.setProperty('LABCountsSample', double_spectra_input)
        alg.setProperty('HABNormSample', double_spectra_input)
        alg.setProperty('LABNormSample', double_spectra_input)
        alg.setProperty('ProcessCan', False)
        alg.setProperty('ShiftFactor', 1.0)
        alg.setProperty('ScaleFactor', 0.0)

        # 2D inputs Should not be allowed for mode Both
        alg.setProperty('Mode', 'Both')
        errors = alg.validateInputs()
        self.assertTrue('HABCountsSample' in errors)
        self.assertTrue('LABCountsSample' in errors)
        self.assertTrue('HABNormSample' in errors)
        self.assertTrue('LABNormSample' in errors)

        # 2D inputs Should not be allowed for mode ScaleOnly
        alg.setProperty('Mode', 'ScaleOnly')
        errors = alg.validateInputs()
        self.assertTrue('HABCountsSample' in errors)
        self.assertTrue('LABCountsSample' in errors)
        self.assertTrue('HABNormSample' in errors)
        self.assertTrue('LABNormSample' in errors)

        # 2D inputs Should not be allowed for mode ShiftOnly
        alg.setProperty('Mode', 'ShiftOnly')
        errors = alg.validateInputs()
        self.assertTrue('HABCountsSample' in errors)
        self.assertTrue('LABCountsSample' in errors)
        self.assertTrue('HABNormSample' in errors)
        self.assertTrue('LABNormSample' in errors)

        # With no fitting 2D inputs are allowed
        alg.setProperty('Mode', 'None')
        errors = alg.validateInputs()
        self.assertEqual(0, len(errors))


    def test_scale_none(self):
        create_alg = AlgorithmManager.create('CreateWorkspace')
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty('DataX', range(0, 10))
        create_alg.setProperty('DataY', [1] * 9)
        create_alg.setProperty('NSpec', 1)
        create_alg.setProperty('UnitX', 'MomentumTransfer')
        create_alg.setPropertyValue('OutputWorkspace', 'out_ws')
        create_alg.execute()
        single_spectra_input = create_alg.getProperty('OutputWorkspace').value

        in_scale_factor = 1.0
        in_shift_factor = 1.0
        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'None')
        alg.setProperty('HABCountsSample', single_spectra_input)
        alg.setProperty('LABCountsSample', single_spectra_input)
        alg.setProperty('HABNormSample', single_spectra_input)
        alg.setProperty('LABNormSample', single_spectra_input)
        alg.setProperty('OutputWorkspace', 'dummy_name')
        alg.setProperty('ShiftFactor', in_shift_factor)
        alg.setProperty('ScaleFactor', in_scale_factor)
        alg.execute()
        out_ws = alg.getProperty('OutputWorkspace').value
        out_shift_factor = alg.getProperty('OutShiftFactor').value
        out_scale_factor = alg.getProperty('OutScaleFactor').value

        self.assertTrue(isinstance(out_ws, MatrixWorkspace))

        self.assertEquals(out_scale_factor, in_scale_factor)
        self.assertEquals(out_shift_factor, in_shift_factor)
        y_array = out_ws.readY(0)

        expected_y_array = [1.5] * 9
        self.assertTrue(all(map(lambda element: element in y_array, expected_y_array)))
        x_array = out_ws.readX(0)

    def test_strip_special_values(self):
        create_alg = AlgorithmManager.create('CreateWorkspace')
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty('DataX', range(0, 10))
        y_data = np.array([1] * 7)
        y_data = np.append(y_data, [np.nan])
        y_data = np.append(y_data, [np.inf])
        create_alg.setProperty('DataY', y_data)
        create_alg.setProperty('NSpec', 1)
        create_alg.setProperty('UnitX', 'MomentumTransfer')
        create_alg.setPropertyValue('OutputWorkspace', 'out_ws')
        create_alg.execute()
        single_spectra_input = create_alg.getProperty('OutputWorkspace').value

        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'Both')
        alg.setProperty('HABCountsSample', single_spectra_input)
        alg.setProperty('LABCountsSample', single_spectra_input)
        alg.setProperty('HABNormSample', single_spectra_input)
        alg.setProperty('LABNormSample', single_spectra_input)
        alg.setProperty('OutputWorkspace', 'dummy_name')
        # This would throw at the point of fitting in NaNs or infs where present
        alg.execute()



    def test_scale_none_with_can(self):
        create_alg = AlgorithmManager.create('CreateWorkspace')
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty('DataX', range(0, 10))
        create_alg.setProperty('DataY', [1] * 9)
        create_alg.setProperty('NSpec', 1)
        create_alg.setProperty('UnitX', 'MomentumTransfer')
        create_alg.setPropertyValue('OutputWorkspace', 'out_ws')
        create_alg.execute()
        single_spectra_input = create_alg.getProperty('OutputWorkspace').value

        create_alg.setProperty('DataY', [0.5] * 9)
        create_alg.execute()
        smaller_single_spectra_input = create_alg.getProperty('OutputWorkspace').value

        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'None')
        alg.setProperty('HABCountsSample', single_spectra_input)
        alg.setProperty('LABCountsSample', single_spectra_input)
        alg.setProperty('HABNormSample', single_spectra_input)
        alg.setProperty('LABNormSample', single_spectra_input)
        alg.setProperty('ProcessCan', True)
        alg.setProperty('HABCountsCan', smaller_single_spectra_input)
        alg.setProperty('LABCountsCan', smaller_single_spectra_input)
        alg.setProperty('HABNormCan', single_spectra_input)
        alg.setProperty('LABNormCan', single_spectra_input)
        alg.setProperty('OutputWorkspace', 'dummy_name')
        alg.setProperty('ShiftFactor', 0.0)
        alg.setProperty('ScaleFactor', 1.0)
        alg.execute()
        out_ws = alg.getProperty('OutputWorkspace').value

        self.assertTrue(isinstance(out_ws, MatrixWorkspace))

        y_array = out_ws.readY(0)

        expected_y_array = [0.5] * 9

        self.assertTrue(all(map(lambda element: element in y_array, expected_y_array)),
                        msg='can gets subtracted so expect 1 - 0.5 as output signal. Proves the can workspace gets used correctly.')

    def test_scale_both_without_can(self):
        create_alg = AlgorithmManager.create('CreateWorkspace')
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty('NSpec', 1)
        create_alg.setProperty('UnitX', 'MomentumTransfer')
        create_alg.setPropertyValue('OutputWorkspace', 'out_ws')
        create_alg.setProperty('DataX', range(0, 10))

        # HAB as linear function y=x+5
        create_alg.setProperty('DataY', range(5, 14))
        create_alg.execute()
        hab_workspace = create_alg.getProperty('OutputWorkspace').value

        # LAB as linear function y=x+0
        create_alg.setProperty('DataY', range(0, 9))
        create_alg.execute()
        lab_workspace= create_alg.getProperty('OutputWorkspace').value

        # FLAT NORM
        create_alg.setProperty('DataY', [1] * 9)
        create_alg.execute()
        flat_norm = create_alg.getProperty('OutputWorkspace').value

        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'Both')
        alg.setProperty('HABCountsSample', hab_workspace)
        alg.setProperty('LABCountsSample', lab_workspace)
        alg.setProperty('HABNormSample', flat_norm)
        alg.setProperty('LABNormSample', flat_norm)
        alg.setProperty('OutputWorkspace', 'dummy_name')
        alg.execute()
        out_ws = alg.getProperty('OutputWorkspace').value
        out_shift_factor = alg.getProperty('OutShiftFactor').value
        out_scale_factor = alg.getProperty('OutScaleFactor').value


        self.assertEquals(out_scale_factor, 1.0)
        self.assertEquals(out_shift_factor, -5.0)

        y_array = out_ws.readY(0)

        expected_y_array = lab_workspace.readY(0) # We scale and shift to the back (lab) detectors

        self.assertTrue(all(map(lambda element: element in y_array, expected_y_array)),
                        msg='All data should be scaled and shifted to the LAB scale=1 shift=-5')

    def test_shift_only_without_can(self):
        create_alg = AlgorithmManager.create('CreateWorkspace')
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty('NSpec', 1)
        create_alg.setProperty('UnitX', 'MomentumTransfer')
        create_alg.setPropertyValue('OutputWorkspace', 'out_ws')
        create_alg.setProperty('DataX', range(0, 10))

        # HAB as linear function y=x+5
        create_alg.setProperty('DataY', range(5, 14))
        create_alg.execute()
        hab_workspace = create_alg.getProperty('OutputWorkspace').value

        # LAB as linear function y=x+0
        create_alg.setProperty('DataY', range(0, 9))
        create_alg.execute()
        lab_workspace= create_alg.getProperty('OutputWorkspace').value

        # FLAT NORM
        create_alg.setProperty('DataY', [1] * 9)
        create_alg.execute()
        flat_norm = create_alg.getProperty('OutputWorkspace').value

        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'ShiftOnly')
        alg.setProperty('HABCountsSample', hab_workspace)
        alg.setProperty('LABCountsSample', lab_workspace)
        alg.setProperty('HABNormSample', flat_norm)
        alg.setProperty('LABNormSample', flat_norm)
        alg.setProperty('ScaleFactor', 1.0)
        alg.setProperty('OutputWorkspace', 'dummy_name')

        alg.execute()
        out_ws = alg.getProperty('OutputWorkspace').value

        y_array = out_ws.readY(0)

        expected_y_array = lab_workspace.readY(0) # We scale and shift to the back (lab) detectors

        self.assertTrue(all(map(lambda element: element in y_array, expected_y_array)),
                        msg='All data should be scaled and shifted to the LAB scale=1 shift=-5')


    def test_scale_only_without_can(self):
        create_alg = AlgorithmManager.create('CreateWorkspace')
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty('NSpec', 1)
        create_alg.setProperty('UnitX', 'MomentumTransfer')
        create_alg.setPropertyValue('OutputWorkspace', 'out_ws')
        create_alg.setProperty('DataX', range(0, 10))

        # HAB as linear function y=x+5
        create_alg.setProperty('DataY', range(5, 14))
        create_alg.execute()
        hab_workspace = create_alg.getProperty('OutputWorkspace').value

        # LAB as linear function y=x+0
        create_alg.setProperty('DataY', range(0, 9))
        create_alg.execute()
        lab_workspace= create_alg.getProperty('OutputWorkspace').value

        # FLAT NORM
        create_alg.setProperty('DataY', [1] * 9)
        create_alg.execute()
        flat_norm = create_alg.getProperty('OutputWorkspace').value

        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'ScaleOnly')
        alg.setProperty('HABCountsSample', hab_workspace)
        alg.setProperty('LABCountsSample', lab_workspace)
        alg.setProperty('HABNormSample', flat_norm)
        alg.setProperty('LABNormSample', flat_norm)
        alg.setProperty('ShiftFactor', -5.0)
        alg.setProperty('OutputWorkspace', 'dummy_name')

        alg.execute()
        out_ws = alg.getProperty('OutputWorkspace').value

        y_array = out_ws.readY(0)

        expected_y_array = lab_workspace.readY(0) # We scale and shift to the back (lab) detectors

        self.assertTrue(all(map(lambda element: element in y_array, expected_y_array)),
                        msg='All data should be scaled and shifted to the LAB scale=1 shift=-5')






if __name__ == '__main__':
    unittest.main()
