# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import DeltaPDF3D, CreateMDWorkspace, FakeMDEventData, BinMD, mtd
import numpy as np
from scipy import signal


class DeltaPDF3DTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        """
        Create fake workspaces, 3D and 2D
        """

        # 3D
        DeltaPDF3DTest_MDE = CreateMDWorkspace(Dimensions='3', Extents='-3.1,3.1,-3.1,3.1,-3.1,3.1', Names='[H,0,0],[0,K,0],[0,0,L]',
                                               Units='rlu,rlu,rlu', SplitInto='4',Frames='HKL,HKL,HKL')

        # Add Bragg peaks
        for h in range(-3,4):
            for k in range(-3,4):
                for l in range(-3,4):
                    FakeMDEventData(DeltaPDF3DTest_MDE, PeakParams='100,'+str(h)+','+str(k)+','+str(l)+',0.1', RandomSeed='1337')

        # Add addiontal peaks on [0.5,0.5,0.5] type positions
        # This would correspond to negative substitutional correlations
        for h in np.arange(-2.5,3):
            for k in np.arange(-2.5,3):
                for l in np.arange(-2.5,3):
                    FakeMDEventData(DeltaPDF3DTest_MDE, PeakParams='20,'+str(h)+','+str(k)+','+str(l)+',0.1', RandomSeed='13337')

        BinMD(InputWorkspace='DeltaPDF3DTest_MDE', AlignedDim0='[H,0,0],-3.05,3.05,61', AlignedDim1='[0,K,0],-3.05,3.05,61',
              AlignedDim2='[0,0,L],-3.05,3.05,61', OutputWorkspace='DeltaPDF3DTest_MDH')

        # 2D
        DeltaPDF3DTest_MDE_2 = CreateMDWorkspace(Dimensions='3', Extents='-3.1,3.1,-3.1,3.1,-0.1,0.1', Names='[H,0,0],[0,K,0],[0,0,L]',
                                                 Units='rlu,rlu,rlu', SplitInto='4',Frames='HKL,HKL,HKL')

        # Add Bragg peaks
        for h in range(-3,4):
            for k in range(-3,4):
                FakeMDEventData(DeltaPDF3DTest_MDE_2, PeakParams='100,'+str(h)+','+str(k)+',0,0.01', RandomSeed='1337')

        # Add addiontal peaks on [0.5,0.5,0.5] type positions
        # This would correspond to negative substitutional correlations
        for h in np.arange(-2.5,3):
            for k in np.arange(-2.5,3):
                FakeMDEventData(DeltaPDF3DTest_MDE_2, PeakParams='20,'+str(h)+','+str(k)+',0,0.1', RandomSeed='13337')

        BinMD(InputWorkspace='DeltaPDF3DTest_MDE_2', AlignedDim0='[H,0,0],-3.05,3.05,61', AlignedDim1='[0,K,0],-3.05,3.05,61',
              AlignedDim2='[0,0,L],-0.1,0.1,1', OutputWorkspace='DeltaPDF3DTest_MDH_2')

    def test_3D(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH',OutputWorkspace='fft',
                   Method='None',CropSphere=False,Convolution=False,WindowFunction='None')
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(113490), 33958.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(113496), 23671.27596343) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(113862), 28947.50521336) # [1,1,0]
        dimX=fft.getXDimension()
        self.assertAlmostEqual(dimX.getMinimum(), -4.9180326)
        self.assertAlmostEqual(dimX.getMaximum(), 4.9180326)

    def test_3D_RemoveReflections(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH',OutputWorkspace='fft',IntermediateWorkspace='int',
                   Method='Punch and fill',Size=0.4,CropSphere=False,Convolution=False,WindowFunction='None')
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(113490), 4320.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(113496), -3899.411112565) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(113862), 3994.2768284083) # [1,1,0]

    def test_3D_CropSphere(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH',OutputWorkspace='fft',IntermediateWorkspace='int',
                   Method='Punch and fill',Size=0.4,CropSphere=True,SphereMax=3,Convolution=False,WindowFunction='None')
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(113490), 2510.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(113496), -2274.141590160) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(113862), 2341.378453873) # [1,1,0]

    def test_2D(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',
                   Method='None',CropSphere=False,Convolution=False,WindowFunction='None')
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 5620.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), 4146.7654660) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 5361.2265518) # [1,1,0]
        dimX=fft.getXDimension()
        self.assertAlmostEqual(dimX.getMinimum(), -4.9180326)
        self.assertAlmostEqual(dimX.getMaximum(), 4.9180326)
        dimZ=fft.getZDimension()
        self.assertAlmostEqual(dimZ.getMinimum(), -2.5)
        self.assertAlmostEqual(dimZ.getMaximum(), 2.5)

    def test_2D_RemoveReflections(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',IntermediateWorkspace='int',
                   Method='Punch and fill',Size=0.4,CropSphere=False,Convolution=False,WindowFunction='None')
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 720.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -649.90185209) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 665.71280473) # [1,1,0]

    def test_2D_CropSphere(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',IntermediateWorkspace='int',
                   Method='Punch and fill',Size=0.4,CropSphere=True,SphereMax=3,Convolution=False,WindowFunction='None')
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 622.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -562.30106845) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 577.15758916) # [1,1,0]

    def test_2D_RemoveReflections_sphere(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',IntermediateWorkspace='int',
                   Method='Punch and fill',Shape='sphere',Size=0.3,CropSphere=False,Convolution=False,WindowFunction='None')
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 720.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -649.90185209) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 665.71280473) # [1,1,0]

    def test_2D_RemoveReflections_sphere_CropSphere(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',IntermediateWorkspace='int',
                   Method='Punch and fill',Shape='sphere',Size=0.3,CropSphere=True,SphereMax=3,Convolution=False,WindowFunction='None')
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 622.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -562.30106845) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 577.15758916) # [1,1,0]

    def test_2D_KAREN(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',
                   Method='KAREN',WindowFunction='None',KARENWidth=3)
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 115.2) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -113.42552489) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 111.67838279) # [1,1,0]

    def test_2D_KAREN_blackman(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',
                   Method='KAREN',KARENWidth=3)
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 18.4552085327) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -18.3728358886) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 18.2797248611) # [1,1,0]

    def test_2D_KAREN_gaussian(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',
                   Method='KAREN',WindowFunction='Gaussian',KARENWidth=3)
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 84.255827882) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -83.109989976) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 81.957445912) # [1,1,0]

    @unittest.skipIf(not hasattr(signal,'tukey'), 'Window function not available with this version of scipy')
    def test_2D_KAREN_tukey(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',
                   Method='KAREN',WindowFunction='Tukey',KARENWidth=3)
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 60.8833379907) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -60.3561604256) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 59.7969775224) # [1,1,0]

    def test_2D_KAREN_kaiser(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',
                   Method='KAREN',WindowFunction='Kaiser',WindowParameter=2,KARENWidth=3)
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 71.67514587555) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -70.77683306878) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 69.86001401877) # [1,1,0]


if __name__ == '__main__':
    unittest.main()
