import unittest
import numpy
from mantid.kernel import * 
from mantid.api import *
from mantid.simpleapi import (LoadRaw, MoveInstrumentComponent, CropWorkspace, 
                              SANSWideAngleCorrection, Min, Max, Transpose)
class SANSWideAngleCorrectionTest(unittest.TestCase):
    _sample = None
    _trans = None
    def setUp(self):
        # load and center the data
        Sample = LoadRaw('LOQ48098')
        Sample = MoveInstrumentComponent(Sample, ComponentName='main-detector-bank',X='-0.0066159999999999553',Y='-0.012459999999999971')
        Sample = CropWorkspace('Sample',StartWorkspaceIndex=2,EndWorkspaceIndex=16385)
        # create a transmission workspace
        Trans = CropWorkspace("Sample",StartWorkspaceIndex=10,EndWorkspaceIndex=10)
        Trans = mtd['Trans']
        x_v = Trans.dataX(0)[:-1]
        y_v = -1.73e-5 * x_v  + 0.743139
        e_v = 2.67e-6*x_v + 1.34e-4
        Trans.setY(0,y_v)
        Trans.setE(0,e_v)
        self._sample = Sample
        self._trans = Trans

    def test_calculate_correction(self):
        correction = SANSWideAngleCorrection(self._sample, self._trans)
        self.assertTrue(correction.getNumberHistograms(),self._sample.getNumberHistograms())
        self.assertTrue(len(correction.readX(0)), len(self._sample.readX(0)))
        self.assertTrue(len(correction.readY(0)), len(self._sample.readY(0)))
        lRange = Min(correction)
        hRange = Max(correction)
        lRange = Transpose(lRange)
        hRange = Transpose(hRange)
        self.assertTrue(97 > hRange.dataY(0).all())
        self.assertTrue(1 >= hRange.dataY(0).all())
        self.assertTrue(correction.dataY(6052)[50], 0.998576713345)
if __name__ == "__main__":
    unittest.main()
        
