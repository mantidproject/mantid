# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# ruff: noqa: E741  # Ambiguous variable name
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
        DeltaPDF3DTest_MDE = CreateMDWorkspace(
            Dimensions="3",
            Extents="-3.1,3.1,-3.1,3.1,-3.1,3.1",
            Names="[H,0,0],[0,K,0],[0,0,L]",
            Units="rlu,rlu,rlu",
            SplitInto="4",
            Frames="HKL,HKL,HKL",
        )

        # Add Bragg peaks
        for h in range(-3, 4):
            for k in range(-3, 4):
                for l in range(-3, 4):
                    FakeMDEventData(
                        DeltaPDF3DTest_MDE, PeakParams="100," + str(h) + "," + str(k) + "," + str(l) + ",0.1", RandomSeed="1337"
                    )

        # Add addiontal peaks on [0.5,0.5,0.5] type positions
        # This would correspond to negative substitutional correlations
        for h in np.arange(-2.5, 3):
            for k in np.arange(-2.5, 3):
                for l in np.arange(-2.5, 3):
                    FakeMDEventData(
                        DeltaPDF3DTest_MDE, PeakParams="20," + str(h) + "," + str(k) + "," + str(l) + ",0.1", RandomSeed="13337"
                    )

        BinMD(
            InputWorkspace="DeltaPDF3DTest_MDE",
            AlignedDim0="[H,0,0],-3.05,3.05,61",
            AlignedDim1="[0,K,0],-3.05,3.05,61",
            AlignedDim2="[0,0,L],-3.05,3.05,61",
            OutputWorkspace="DeltaPDF3DTest_MDH",
        )

        # 2D
        DeltaPDF3DTest_MDE_2 = CreateMDWorkspace(
            Dimensions="3",
            Extents="-3.1,3.1,-3.1,3.1,-0.1,0.1",
            Names="[H,0,0],[0,K,0],[0,0,L]",
            Units="rlu,rlu,rlu",
            SplitInto="4",
            Frames="HKL,HKL,HKL",
        )

        # Add Bragg peaks
        for h in range(-3, 4):
            for k in range(-3, 4):
                FakeMDEventData(DeltaPDF3DTest_MDE_2, PeakParams="100," + str(h) + "," + str(k) + ",0,0.01", RandomSeed="1337")

        # Add addiontal peaks on [0.5,0.5,0.5] type positions
        # This would correspond to negative substitutional correlations
        for h in np.arange(-2.5, 3):
            for k in np.arange(-2.5, 3):
                FakeMDEventData(DeltaPDF3DTest_MDE_2, PeakParams="20," + str(h) + "," + str(k) + ",0,0.1", RandomSeed="13337")

        BinMD(
            InputWorkspace="DeltaPDF3DTest_MDE_2",
            AlignedDim0="[H,0,0],-3.05,3.05,61",
            AlignedDim1="[0,K,0],-3.05,3.05,61",
            AlignedDim2="[0,0,L],-0.1,0.1,1",
            OutputWorkspace="DeltaPDF3DTest_MDH_2",
        )

    def test_3D(self):
        DeltaPDF3D(
            InputWorkspace="DeltaPDF3DTest_MDH",
            OutputWorkspace="fft",
            Method="None",
            CropSphere=False,
            Convolution=False,
            WindowFunction="None",
        )
        fft = mtd["fft"]
        self.assertAlmostEqual(fft.signalAt(113490), 34209.0, delta=80.0)  # [0,0,0] - discrepancy windows, OSX and RHEL, Ubuntu
        self.assertAlmostEqual(fft.signalAt(113496), 23202.32114192)  # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(113862), 29097.728403971)  # [1,1,0]
        dimX = fft.getXDimension()
        self.assertAlmostEqual(dimX.getMinimum(), -4.9180326)
        self.assertAlmostEqual(dimX.getMaximum(), 4.9180326)

    def test_3D_RemoveReflections(self):
        DeltaPDF3D(
            InputWorkspace="DeltaPDF3DTest_MDH",
            OutputWorkspace="fft",
            IntermediateWorkspace="int",
            Method="Punch and fill",
            Size=0.4,
            CropSphere=False,
            Convolution=False,
            WindowFunction="None",
        )
        fft = mtd["fft"]
        self.assertAlmostEqual(fft.signalAt(113490), 4320.0)  # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(113496), -4057.0, delta=350.0)  # [1,0,0] - discrepancy windows, OSX and RHEL, Ubuntu
        self.assertAlmostEqual(fft.signalAt(113862), 3839.3468074482)  # [1,1,0]

    def test_3D_CropSphere(self):
        DeltaPDF3D(
            InputWorkspace="DeltaPDF3DTest_MDH",
            OutputWorkspace="fft",
            IntermediateWorkspace="int",
            Method="Punch and fill",
            Size=0.4,
            CropSphere=True,
            SphereMax=3,
            Convolution=False,
            WindowFunction="None",
        )
        fft = mtd["fft"]
        self.assertAlmostEqual(fft.signalAt(113490), 2464.0)  # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(113496), -2320.0, delta=300.0)  # [1,0,0] - discrepancy windows, OSX and RHEL, Ubuntu
        self.assertAlmostEqual(fft.signalAt(113862), 2202.6748248247)  # [1,1,0]

    def test_2D(self):
        DeltaPDF3D(
            InputWorkspace="DeltaPDF3DTest_MDH_2",
            OutputWorkspace="fft",
            Method="None",
            CropSphere=False,
            Convolution=False,
            WindowFunction="None",
        )
        fft = mtd["fft"]
        self.assertAlmostEqual(fft.signalAt(1860), 5620.0)  # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), 4120.5, delta=60)  # [1,0,0] - discrepancy between windows, OSX and RHEL, Ubuntu
        self.assertAlmostEqual(fft.signalAt(2232), 5335.4048817)  # [1,1,0]
        dimX = fft.getXDimension()
        self.assertAlmostEqual(dimX.getMinimum(), -4.9180326)
        self.assertAlmostEqual(dimX.getMaximum(), 4.9180326)
        dimZ = fft.getZDimension()
        self.assertAlmostEqual(dimZ.getMinimum(), -2.5)
        self.assertAlmostEqual(dimZ.getMaximum(), 2.5)

    def test_2D_RemoveReflections(self):
        DeltaPDF3D(
            InputWorkspace="DeltaPDF3DTest_MDH_2",
            OutputWorkspace="fft",
            IntermediateWorkspace="int",
            Method="Punch and fill",
            Size=0.4,
            CropSphere=False,
            Convolution=False,
            WindowFunction="None",
        )
        fft = mtd["fft"]
        self.assertAlmostEqual(fft.signalAt(1860), 720.0)  # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -676, delta=60.0)  # [0,0,0] - discrepancy between windows, OSX and RHEL, Ubuntu
        self.assertAlmostEqual(fft.signalAt(2232), 639.89113457)  # [1,1,0]

    def test_2D_CropSphere(self):
        DeltaPDF3D(
            InputWorkspace="DeltaPDF3DTest_MDH_2",
            OutputWorkspace="fft",
            IntermediateWorkspace="int",
            Method="Punch and fill",
            Size=0.4,
            CropSphere=True,
            SphereMax=3,
            Convolution=False,
            WindowFunction="None",
        )
        fft = mtd["fft"]
        self.assertAlmostEqual(fft.signalAt(1860), 620.0, delta=20.0)  # [0,0,0] - discrepancy between windows, OSX and RHEL, Ubuntu
        self.assertAlmostEqual(fft.signalAt(1866), -583.145906100)  # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 552.675152713)  # [1,1,0]

    def test_2D_RemoveReflections_sphere(self):
        DeltaPDF3D(
            InputWorkspace="DeltaPDF3DTest_MDH_2",
            OutputWorkspace="fft",
            IntermediateWorkspace="int",
            Method="Punch and fill",
            Shape="sphere",
            Size=0.3,
            CropSphere=False,
            Convolution=False,
            WindowFunction="None",
        )
        fft = mtd["fft"]
        self.assertAlmostEqual(fft.signalAt(1860), 720.0)  # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -676, delta=60.0)  # [0,0,0] - discrepancy between windows, OSX and RHEL, Ubuntu
        self.assertAlmostEqual(fft.signalAt(2232), 639.89113457)  # [1,1,0]

    def test_2D_RemoveReflections_sphere_CropSphere(self):
        DeltaPDF3D(
            InputWorkspace="DeltaPDF3DTest_MDH_2",
            OutputWorkspace="fft",
            IntermediateWorkspace="int",
            Method="Punch and fill",
            Shape="sphere",
            Size=0.3,
            CropSphere=True,
            SphereMax=3,
            Convolution=False,
            WindowFunction="None",
        )
        fft = mtd["fft"]
        self.assertAlmostEqual(fft.signalAt(1860), 620, delta=20.0)  # [0,0,0] - discrepancy between windows, OSX and RHEL, Ubuntu
        self.assertAlmostEqual(fft.signalAt(1866), -583.14590610)  # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 552.67515271)  # [1,1,0]

    def test_2D_KAREN(self):
        DeltaPDF3D(InputWorkspace="DeltaPDF3DTest_MDH_2", OutputWorkspace="fft", Method="KAREN", WindowFunction="None", KARENWidth=3)
        fft = mtd["fft"]
        self.assertAlmostEqual(fft.signalAt(1860), 187.2, delta=50.0)  # [0,0,0] - discrepancy between windows, OSX and RHEL, Ubuntu
        self.assertAlmostEqual(fft.signalAt(1866), -177.76006923)  # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 168.56653695)  # [1,1,0]

    def test_2D_KAREN_blackman(self):
        DeltaPDF3D(InputWorkspace="DeltaPDF3DTest_MDH_2", OutputWorkspace="fft", Method="KAREN", KARENWidth=3)
        fft = mtd["fft"]
        self.assertAlmostEqual(fft.signalAt(1860), 32.7, delta=10.0)  # [0,0,0] - discrepancy between windows, OSX and RHEL, Ubuntu
        self.assertAlmostEqual(fft.signalAt(1866), -31.3504178404)  # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 30.0367428916)  # [1,1,0]

    def test_2D_KAREN_gaussian(self):
        DeltaPDF3D(InputWorkspace="DeltaPDF3DTest_MDH_2", OutputWorkspace="fft", Method="KAREN", WindowFunction="Gaussian", KARENWidth=3)
        fft = mtd["fft"]
        self.assertAlmostEqual(fft.signalAt(1860), 138.93657974)  # [0,0,0]
        self.assertAlmostEqual(fft.signalAt(1866), -132.11218854)  # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 125.51404909)  # [1,1,0]

    @unittest.skipIf(not hasattr(signal, "tukey"), "Window function not available with this version of scipy")
    def test_2D_KAREN_tukey(self):
        DeltaPDF3D(InputWorkspace="DeltaPDF3DTest_MDH_2", OutputWorkspace="fft", Method="KAREN", WindowFunction="Tukey", KARENWidth=3)
        fft = mtd["fft"]
        self.assertAlmostEqual(fft.signalAt(1860), 104, delta=25.0)  # [0,0,0] - discrepancy between windows, OSX and RHEL, Ubuntu
        self.assertAlmostEqual(fft.signalAt(1866), -99.5461877381)  # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 95.0065971292)  # [1,1,0]

    def test_2D_KAREN_kaiser(self):
        DeltaPDF3D(
            InputWorkspace="DeltaPDF3DTest_MDH_2",
            OutputWorkspace="fft",
            Method="KAREN",
            WindowFunction="Kaiser",
            WindowParameter=2,
            KARENWidth=3,
        )
        fft = mtd["fft"]
        self.assertAlmostEqual(fft.signalAt(1860), 119.3, delta=30.0)  # [0,0,0] - discrepancy between windows, OSX and RHEL, Ubuntu
        self.assertAlmostEqual(fft.signalAt(1866), -113.4886083267)  # [1,0,0]
        self.assertAlmostEqual(fft.signalAt(2232), 107.9361501707)  # [1,1,0]


if __name__ == "__main__":
    unittest.main()
