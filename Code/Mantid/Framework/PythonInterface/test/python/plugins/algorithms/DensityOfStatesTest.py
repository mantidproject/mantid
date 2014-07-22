import unittest
from mantid.simpleapi import DensityOfStates, CheckWorkspacesMatch, Scale

def scipy_supported():
    try:
        import scipy
        return True
    except:
        return False

@unittest.skipIf(scipy_supported, "DensityOfStates test skipped. Scipy is unavailable.")
class DensityOfStatesTest(unittest.TestCase):

    def setUp(self):
        self._file_name = 'squaricn.phonon'
    
    def test_phonon_load(self):
        ws = DensityOfStates(File=self._file_name)
        self.assertEquals(ws.getNumberHistograms(), 1)
    
    def test_castep_load(self):
        ws = DensityOfStates(File='squaricn.castep')
        self.assertEquals(ws.getNumberHistograms(), 1)
    
    def test_raman_active(self):
        spec_type = 'Raman_Active'
        ws = DensityOfStates(File=self._file_name, SpectrumType=spec_type)
        self.assertEquals(ws.getNumberHistograms(), 1)
        
    def test_ir_active(self):
        spec_type = 'IR_Active'
        ws = DensityOfStates(File=self._file_name, SpectrumType=spec_type)
        self.assertEquals(ws.getNumberHistograms(), 1)

    def test_lorentzian_function(self):
        ws = DensityOfStates(File=self._file_name, Function='Lorentzian')
        self.assertEquals(ws.getNumberHistograms(), 1)

    def test_peak_width(self):
        ws = DensityOfStates(File=self._file_name, PeakWidth=0.3)
        self.assertEquals(ws.getNumberHistograms(), 1)

    def test_temperature(self):
        ws = DensityOfStates(File=self._file_name, Temperature=50)
        self.assertEquals(ws.getNumberHistograms(), 1)

    def test_scale(self):
        ws = DensityOfStates(File=self._file_name, Scale=10)
        ref = DensityOfStates(File=self._file_name)
        ref = Scale(ref, Factor=10)

        CheckWorkspacesMatch(ws, ref)

    def test_bin_width(self):
        import math

        ref = DensityOfStates(File=self._file_name)
        ws = DensityOfStates(File=self._file_name, BinWidth=2)
        
        size = ws.blocksize()
        ref_size = ref.blocksize()
        
        self.assertEquals(size, math.ceil(ref_size/2.0))

    def test_zero_threshold(self):
        import numpy as np

        ws = DensityOfStates(File=self._file_name, ZeroThreshold=20)

        x = ws.readX(0)
        y = ws.readY(0)

        mask = np.where(x < 20)
        self.assertEquals(sum(y[mask]), 0)

    def test_partial(self):
        spec_type = 'DOS'
        ws = DensityOfStates(File=self._file_name, SpectrumType=spec_type, Ions="H,C,O")

        workspaces = ws.getNames()
        self.assertEquals(len(workspaces), 3)

    def test_sum_partial_contributions(self):
        spec_type = 'DOS'
        tolerance = 1e-10

        summed = DensityOfStates(File=self._file_name, SpectrumType=spec_type, Ions="H,C,O", SumContributions=True)
        total = DensityOfStates(File=self._file_name,  SpectrumType=spec_type)

        CheckWorkspacesMatch(summed, total, tolerance)

if __name__=="__main__":
    unittest.main()