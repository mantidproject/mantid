from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import DeltaPDF3D, CreateMDWorkspace, FakeMDEventData, BinMD, mtd
import numpy as np


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
                FakeMDEventData(DeltaPDF3DTest_MDE_2, PeakParams='100,'+str(h)+','+str(k)+',0,0.1', RandomSeed='1337')

        # Add addiontal peaks on [0.5,0.5,0.5] type positions
        # This would correspond to negative substitutional correlations
        for h in np.arange(-2.5,3):
            for k in np.arange(-2.5,3):
                FakeMDEventData(DeltaPDF3DTest_MDE_2, PeakParams='20,'+str(h)+','+str(k)+',0,0.1', RandomSeed='13337')

        BinMD(InputWorkspace='DeltaPDF3DTest_MDE_2', AlignedDim0='[H,0,0],-3.05,3.05,61', AlignedDim1='[0,K,0],-3.05,3.05,61',
              AlignedDim2='[0,0,L],-0.1,0.1,1', OutputWorkspace='DeltaPDF3DTest_MDH_2')

    def test_3D(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH',OutputWorkspace='fft',
                   RemoveReflections=False,CropSphere=False,Convolution=False)
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(113490), 33958.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(113496), 22728.32924037) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(113862), 27438.59481082) # [1,1,0]
        dimX=fft.getXDimension()
        self.assertAlmostEqual(dimX.getMinimum(), -4.9180326)
        self.assertAlmostEqual(dimX.getMaximum(), 4.9180326)

    def test_3D_RemoveReflections(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH',OutputWorkspace='fft',IntermediateWorkspace='int',
                   RemoveReflections=True,Width=0.2,CropSphere=False,Convolution=False)
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(113490), 4320.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(113496), -3938.749564891) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(113862), 3660.013113021) # [1,1,0]

    def test_3D_CropSphere(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH',OutputWorkspace='fft',IntermediateWorkspace='int',
                   RemoveReflections=True,Width=0.2,CropSphere=True,SphereMax=3,Convolution=False)
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(113490), 2510.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(113496), -2296.470592168) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(113862), 2144.644073379) # [1,1,0]

    def test_2D(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',
                   RemoveReflections=False,CropSphere=False,Convolution=False)
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 5224.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), 3452.0247366) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 4468.5363433) # [1,1,0]
        dimX=fft.getXDimension()
        self.assertAlmostEqual(dimX.getMinimum(), -4.9180326)
        self.assertAlmostEqual(dimX.getMaximum(), 4.9180326)
        dimZ=fft.getZDimension()
        self.assertAlmostEqual(dimZ.getMinimum(), -2.5)
        self.assertAlmostEqual(dimZ.getMaximum(), 2.5)

    def test_2D_RemoveReflections(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',IntermediateWorkspace='int',
                   RemoveReflections=True,Width=0.2,CropSphere=False,Convolution=False)
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 720.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -682.68389570) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 665.71280473) # [1,1,0]

    def test_2D_CropSphere(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',IntermediateWorkspace='int',
                   RemoveReflections=True,Width=0.2,CropSphere=True,SphereMax=3,Convolution=False)
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 622.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -590.98194677) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 577.15758916) # [1,1,0]

    def test_2D_RemoveReflections_sphere(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',IntermediateWorkspace='int',
                   RemoveReflections=True,Shape='sphere',Width=0.15,CropSphere=False,Convolution=False)
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 720.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -682.68389570) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 665.71280473) # [1,1,0]

    def test_2D_RemoveReflections_sphere_CropSphere(self):
        DeltaPDF3D(InputWorkspace='DeltaPDF3DTest_MDH_2',OutputWorkspace='fft',IntermediateWorkspace='int',
                   RemoveReflections=True,Shape='sphere',Width=0.15,CropSphere=True,SphereMax=3,Convolution=False)
        fft=mtd['fft']
        self.assertAlmostEqual(fft.signalAt(1860), 622.0) # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -590.98194677) # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 577.15758916) # [1,1,0]


if __name__ == '__main__':
    unittest.main()
