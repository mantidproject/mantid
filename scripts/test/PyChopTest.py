"""Test suite for the PyChop package
"""
import unittest
import numpy as np

# Import mantid to setup the python paths to the bundled scripts
import mantid
from PyChop import PyChop2

class PyChop2Tests(unittest.TestCase):

    def test_pychop2(self):
        instnames = ['let', 'maps', 'mari', 'merlin']
        modulenames = ['ISISDisk', 'ISISFermi', 'ISISFermi', 'ISISFermi']
        res = []
        flux = []
        for inc, instname in enumerate(instnames):
            chopobj = PyChop2(instname)
            # Checks that initialisations instanciates the correct submodule
            # which does the actual calculations. PyChop2 is just a wrapper.
            self.assertTrue(modulenames[inc] in chopobj.getObject().__module__)
            # Code should give an error if the chopper settings and Ei have
            # not been set.
            self.assertRaises(ValueError, chopobj.getResolution)
            if 'Fermi' in modulenames[inc]:
                chopobj.setChopper('s', 200)
            else:
                chopobj.setFrequency(200)
            chopobj.setEi(18)
            rr, ff = chopobj.getResFlux(np.linspace(0,17,10))
            res.append(rr)
            flux.append(ff)
        # Checks that the flux should be highest for LET, then MERLIN, MARI
        # and MAPS in that order
        self.assertTrue(flux[0] > flux[3])
        self.assertTrue(flux[3] > flux[2])
        self.assertTrue(flux[2] > flux[1])
        # Checks that the resolution should be best for LET, then MARI, MAPS
        # and MERLIN in that order
        self.assertTrue(res[0][0] < res[2][0])
        self.assertTrue(res[2][0] < res[1][0])
        self.assertTrue(res[1][0] < res[3][0]) 
        # Now tests the standalone function
        for inc, instname in enumerate(instnames):
            if 'Fermi' in modulenames[inc]:
                rr, ff = PyChop2.calculate(instname, 's', 200, 18, 0)
            else:
                rr, ff = PyChop2.calculate(instname, 200, 18, 0)
            self.assertAlmostEqual(rr[0], res[inc][0], places=7)
            self.assertAlmostEqual(ff, flux[inc], places=7)

if __name__ == "__main__":
    unittest.main()
