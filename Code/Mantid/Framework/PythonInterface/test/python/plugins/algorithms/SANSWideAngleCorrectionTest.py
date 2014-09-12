import unittest
import numpy
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import (CreateWorkspace, LoadInstrument,
                              MoveInstrumentComponent, CropWorkspace,
                              SANSWideAngleCorrection, Min, Max, Transpose,
                              Multiply)
class SANSWideAngleCorrectionTest(unittest.TestCase):
    _sample = None
    _trans = None
    def setUp(self):
        # Simulation of the creation of the Sample data
        nb = 102
        xd = 10
        yd = 10
        data = numpy.random.normal(size=xd*yd*nb)/2.0 + 5.0
        xvalues = numpy.linspace(3500,43500,nb+1)
        tv = numpy.linspace(7e-2,6e-2,nb)
        Sample = CreateWorkspace(xvalues,data,NSpec=xd*yd)
        LoadInstrument(Sample, InstrumentName='SANS2D')
        Sample = CropWorkspace(Sample,StartWorkspaceIndex=8)#remove the monitors
        # create a transmission workspace
        Trans = CropWorkspace(Sample,StartWorkspaceIndex=10,EndWorkspaceIndex=10)
        x_v = Trans.dataX(0)[:-1]
        y_v = numpy.linspace(0.743139, 0.6,nb)
        e_v = y_v/58.0
        Trans.setY(0,y_v)
        Trans.setE(0,e_v)
        self._sample = Sample
        self._trans = Trans
        self.n_det = xd*yd

    def test_calculate_correction(self):
        correction = SANSWideAngleCorrection(self._sample, self._trans)
        self.assertTrue(correction.getNumberHistograms()==self._sample.getNumberHistograms())
        self.assertTrue(len(correction.readX(0))== len(self._sample.readX(0)))
        self.assertTrue(len(correction.readY(0))==len(self._sample.readY(0)))
        lRange = Min(correction)
        hRange = Max(correction)
        lRange = Transpose(lRange)
        hRange = Transpose(hRange)
        self.assertTrue(97 > hRange.dataY(0).all())
        self.assertTrue(1 >= hRange.dataY(0).all())


    def test_negative_trans_data(self):
        trans_invalid = self._trans * -1
        self.assertRaises(RuntimeError, SANSWideAngleCorrection, self._sample, trans_invalid, OutputWorkspace='out')

    def test_shorter_trans_data(self):
        Trans = CreateWorkspace([0.3,1,2],[1,2])
        self.assertRaises(RuntimeError, SANSWideAngleCorrection, self._sample, self._trans, OutputWorkspace='out')
    def test_no_instrument_associated(self):
        Sample = CreateWorkspace([1,2,3],[1,2])
        Trans = CreateWorkspace([1,2,3],[1,2])
        self.assertRaises(RuntimeError, SANSWideAngleCorrection, Sample, Trans, OutputWorkspace='out')

if __name__ == "__main__":
    unittest.main()

