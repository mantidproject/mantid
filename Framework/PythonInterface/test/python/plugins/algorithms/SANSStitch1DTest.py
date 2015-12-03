

import unittest
from mantid.api import AlgorithmManager


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
        create_alg.setProperty('DataX', range(0,1))
        create_alg.setProperty('DataY', [1,2])
        create_alg.setProperty('NSpec', 2) # Wrong number of spectra
        create_alg.setProperty('UnitX', 'MomentumTransfer')
        create_alg.setPropertyValue('OutputWorkspace', 'out_ws')
        create_alg.execute()
        multi_spectra_input = create_alg.getProperty('OutputWorkspace').value

        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'Both')
        alg.setProperty('HABSample', multi_spectra_input)
        alg.setProperty('LABSample', multi_spectra_input)
        alg.setProperty('HABNorm', multi_spectra_input)
        alg.setProperty('LABNorm', multi_spectra_input)

        errors = alg.validateInputs()
        self.assertTrue('HABSample' in errors)
        self.assertTrue('LABSample' in errors)
        self.assertTrue('HABNorm' in errors)
        self.assertTrue('LABNorm' in errors)


    def test_workspace_entries_must_be_q1d(self):
        # create an input workspace that has multiple spectra
        create_alg = AlgorithmManager.create('CreateWorkspace')
        create_alg.setChild(True)
        create_alg.initialize()
        create_alg.setProperty('DataX', range(0,1))
        create_alg.setProperty('DataY', [1])
        create_alg.setProperty('NSpec', 1)
        create_alg.setProperty('UnitX', 'TOF') # Wrong units
        create_alg.setPropertyValue('OutputWorkspace', 'out_ws')
        create_alg.execute()
        multi_spectra_input = create_alg.getProperty('OutputWorkspace').value

        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'Both')
        alg.setProperty('HABSample', multi_spectra_input)
        alg.setProperty('LABSample', multi_spectra_input)
        alg.setProperty('HABNorm', multi_spectra_input)
        alg.setProperty('LABNorm', multi_spectra_input)

        errors = alg.validateInputs()
        self.assertTrue('HABSample' in errors)
        self.assertTrue('LABSample' in errors)
        self.assertTrue('HABNorm' in errors)
        self.assertTrue('LABNorm' in errors)












if __name__ == '__main__':
    unittest.main()
