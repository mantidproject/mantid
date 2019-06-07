# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, division, print_function
from mantid.simpleapi import ConvertWANDSCDtoQ, CreateMDHistoWorkspace, CreateSingleValuedWorkspace, SetUB, mtd
import unittest
import numpy as np


class ConvertWANDSCDtoQTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        def gaussian(x,y,z,x0,y0,z0,ox,oy,oz,A):
            return A*np.exp(-(x-x0)**2/(2*ox**2)-(y-y0)**2/(2*oy**2)-(z-z0)**2/(2*oz**2))

        def peaks(i,j,k):
            return gaussian(i,j,k,16,100,50,2,2,2,20)+gaussian(i,j,k,16,150,50,1,1,1,10)

        S=np.fromfunction(peaks,(32,240,100))

        ConvertWANDSCDtoQTest_data=CreateMDHistoWorkspace(Dimensionality=3,Extents='0.5,32.5,0.5,240.5,0.5,100.5',
                                                          SignalInput=S.ravel('F'),ErrorInput=np.sqrt(S.ravel('F')),
                                                          NumberOfBins='32,240,100',Names='y,x,scanIndex',Units='bin,bin,number')

        ConvertWANDSCDtoQTest_dummy = CreateSingleValuedWorkspace()

        ConvertWANDSCDtoQTest_data.addExperimentInfo(ConvertWANDSCDtoQTest_dummy)

        ConvertWANDSCDtoQTest_data.getExperimentInfo(0).run().addProperty('s1', list(np.arange(0,50,0.5)), True)
        ConvertWANDSCDtoQTest_data.getExperimentInfo(0).run().addProperty('duration', [60.]*100, True)
        ConvertWANDSCDtoQTest_data.getExperimentInfo(0).run().addProperty('monitor_count', [120000.]*100, True)
        ConvertWANDSCDtoQTest_data.getExperimentInfo(0).run().addProperty('twotheta', list(np.linspace(np.pi*2/3,0,240).repeat(32)), True)
        ConvertWANDSCDtoQTest_data.getExperimentInfo(0).run().addProperty('azimuthal', list(np.tile(np.linspace(-0.15,0.15,32),240)), True)

        SetUB(ConvertWANDSCDtoQTest_data, 5,5,7,90,90,120,u=[-1,0,1],v=[1,0,1])

        # Create Normalisation workspace
        S=np.ones((32,240,1))
        ConvertWANDSCDtoQTest_norm=CreateMDHistoWorkspace(Dimensionality=3,Extents='0.5,32.5,0.5,240.5,0.5,1.5',SignalInput=S,ErrorInput=S,
                                                          NumberOfBins='32,240,1',Names='y,x,scanIndex',Units='bin,bin,number')

        ConvertWANDSCDtoQTest_dummy2 = CreateSingleValuedWorkspace()
        ConvertWANDSCDtoQTest_norm.addExperimentInfo(ConvertWANDSCDtoQTest_dummy2)
        ConvertWANDSCDtoQTest_norm.getExperimentInfo(0).run().addProperty('monitor_count', [100000.], True)

    @classmethod
    def tearDownClass(cls):
        [mtd.remove(ws) for ws in ['ConvertWANDSCDtoQTest_data',
                                   'ConvertWANDSCDtoQTest_dummy'
                                   'ConvertWANDSCDtoQTest_norm',
                                   'ConvertWANDSCDtoQTest_dummy2']]

    def test_Q(self):
        ConvertWANDSCDtoQTest_out=ConvertWANDSCDtoQ('ConvertWANDSCDtoQTest_data',BinningDim0='-8.08,8.08,101',
                                                    BinningDim1='-0.88,0.88,11',BinningDim2='-8.08,8.08,101',NormaliseBy='None')

        self.assertTrue(ConvertWANDSCDtoQTest_out)

        s = ConvertWANDSCDtoQTest_out.getSignalArray()
        self.assertAlmostEqual(np.nanmax(s), 8.97233331213612)
        self.assertAlmostEqual(np.nanargmax(s), 22780)

        self.assertEqual(ConvertWANDSCDtoQTest_out.getNumDims(), 3)
        self.assertEqual(ConvertWANDSCDtoQTest_out.getNPoints(), 112211)

        d0 = ConvertWANDSCDtoQTest_out.getDimension(0)
        self.assertEqual(d0.name, 'Q_sample_x')
        self.assertEqual(d0.getNBins(), 101)
        self.assertAlmostEquals(d0.getMinimum(), -8.08, 5)
        self.assertAlmostEquals(d0.getMaximum(), 8.08, 5)

        d1 = ConvertWANDSCDtoQTest_out.getDimension(1)
        self.assertEqual(d1.name, 'Q_sample_y')
        self.assertEqual(d1.getNBins(), 11)
        self.assertAlmostEquals(d1.getMinimum(), -0.88, 5)
        self.assertAlmostEquals(d1.getMaximum(), 0.88, 5)

        d2 = ConvertWANDSCDtoQTest_out.getDimension(2)
        self.assertEqual(d2.name, 'Q_sample_z')
        self.assertEqual(d2.getNBins(), 101)
        self.assertAlmostEquals(d2.getMinimum(), -8.08, 5)
        self.assertAlmostEquals(d2.getMaximum(), 8.08, 5)

        self.assertEqual(ConvertWANDSCDtoQTest_out.getNumExperimentInfo(), 1)

        ConvertWANDSCDtoQTest_out.delete()

    def test_Q_norm(self):
        ConvertWANDSCDtoQTest_out = ConvertWANDSCDtoQ('ConvertWANDSCDtoQTest_data',NormalisationWorkspace='ConvertWANDSCDtoQTest_norm',
                                                      BinningDim0='-8.08,8.08,101',BinningDim1='-0.88,0.88,11',BinningDim2='-8.08,8.08,101')

        s = ConvertWANDSCDtoQTest_out.getSignalArray()
        self.assertAlmostEqual(np.nanmax(s), 7.476944426780101)
        self.assertAlmostEqual(np.nanargmax(s), 22780)

        ConvertWANDSCDtoQTest_out.delete()

    def test_HKL_norm_and_KeepTemporary(self):
        ConvertWANDSCDtoQTest_out = ConvertWANDSCDtoQ('ConvertWANDSCDtoQTest_data',NormalisationWorkspace='ConvertWANDSCDtoQTest_norm',
                                                      Frame='HKL',KeepTemporaryWorkspaces=True,BinningDim0='-8.08,8.08,101',
                                                      BinningDim1='-8.08,8.08,101',BinningDim2='-8.08,8.08,101',
                                                      Uproj='1,1,0',Vproj='1,-1,0',Wproj='0,0,1')

        self.assertTrue(ConvertWANDSCDtoQTest_out)
        self.assertTrue(mtd.doesExist('ConvertWANDSCDtoQTest_out'))
        self.assertTrue(mtd.doesExist('ConvertWANDSCDtoQTest_out_data'))
        self.assertTrue(mtd.doesExist('ConvertWANDSCDtoQTest_out_normalization'))

        s = ConvertWANDSCDtoQTest_out.getSignalArray()
        self.assertAlmostEqual(np.nanmax(s), 4.646855396509936)
        self.assertAlmostEqual(np.nanargmax(s), 443011)

        self.assertEqual(ConvertWANDSCDtoQTest_out.getNumDims(), 3)
        self.assertEqual(ConvertWANDSCDtoQTest_out.getNPoints(), 101**3)

        d0 = ConvertWANDSCDtoQTest_out.getDimension(0)
        self.assertEqual(d0.name, '[H,H,0]')
        self.assertEqual(d0.getNBins(), 101)
        self.assertAlmostEquals(d0.getMinimum(), -8.08, 5)
        self.assertAlmostEquals(d0.getMaximum(), 8.08, 5)

        d1 = ConvertWANDSCDtoQTest_out.getDimension(1)
        self.assertEqual(d1.name, '[H,-H,0]')
        self.assertEqual(d1.getNBins(), 101)
        self.assertAlmostEquals(d1.getMinimum(), -8.08, 5)
        self.assertAlmostEquals(d1.getMaximum(), 8.08, 5)

        d2 = ConvertWANDSCDtoQTest_out.getDimension(2)
        self.assertEqual(d2.name, '[0,0,L]')
        self.assertEqual(d2.getNBins(), 101)
        self.assertAlmostEquals(d2.getMinimum(), -8.08, 5)
        self.assertAlmostEquals(d2.getMaximum(), 8.08, 5)

        self.assertEqual(ConvertWANDSCDtoQTest_out.getNumExperimentInfo(), 1)

        ConvertWANDSCDtoQTest_out.delete()


if __name__ == '__main__':
    unittest.main()
