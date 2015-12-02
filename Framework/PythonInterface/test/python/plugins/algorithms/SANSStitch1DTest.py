

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
        self.assertEquals(2, len(errors))
        self.assertTrue('QMin' in errors)
        self.assertTrue('QMax' in errors)

        # Less than
        alg.setProperty('QMin', 1.0)
        alg.setProperty('QMax', 0.0)
        errors = alg.validateInputs()
        self.assertEquals(2, len(errors))
        self.assertTrue('QMin' in errors)
        self.assertTrue('QMax' in errors)

        # Fine
        alg.setProperty('QMin', 0.0)
        alg.setProperty('QMax', 1.0)
        errors = alg.validateInputs()
        self.assertEquals(0, len(errors))


    def test_none_mode_requires_scale_and_stitch_factors (self):
        alg = AlgorithmManager.create('SANSStitch1D')
        alg.setChild(True)
        alg.initialize()
        alg.setProperty('Mode', 'None')
        errors = alg.validateInputs()
        self.assertEquals(2, len(errors))
        self.assertTrue('ScaleFactor' in errors)
        self.assertTrue('ShiftFactor' in errors)







if __name__ == '__main__':
    unittest.main()
