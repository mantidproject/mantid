# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import platform

import numpy as np
import systemtesting
from mantid.simpleapi import ConvertHFIRSCDtoMDE, DeleteWorkspaces, LoadMD, MDNorm, SetGoniometer, SetUB, mtd


def _skip_test():
    """Helper function to determine if we run the test"""
    return "Linux" not in platform.platform()


class MDNormWANDTest(systemtesting.MantidSystemTest):
    """End-to-end test of MDNorm's monochromatic-SCD path for WAND (HB2C): convert the same
    data to a Q-sample MDEventWorkspace twice, once each for "data" and "normalization", then
    feed both into MDNorm. Since data and normalization are identical, every non-empty output
    bin must be exactly 1.0."""

    def requiredFiles(self):
        return ["HB2C_WANDSCD_data.nxs"]

    def skipTests(self):
        return _skip_test()

    def requiredMemoryMB(self):
        return 4000

    def runTest(self):
        ws_name = "MDNormWANDTest"
        LoadMD("HB2C_WANDSCD_data.nxs", OutputWorkspace=ws_name + "_raw")
        SetGoniometer(ws_name + "_raw", Axis0="s1,0,1,0,1", Average=False)

        ConvertHFIRSCDtoMDE(InputWorkspace=ws_name + "_raw", Wavelength=1.488, OutputWorkspace=ws_name + "_data")
        ConvertHFIRSCDtoMDE(InputWorkspace=ws_name + "_raw", Wavelength=1.488, OutputWorkspace=ws_name + "_norm")

        MDNorm(
            InputWorkspace=ws_name + "_data",
            MonoSCDNormalizationWorkspace=ws_name + "_norm",
            RLU=False,
            Dimension0Binning="-10,1,10",
            Dimension1Binning="-10,1,10",
            Dimension2Binning="-10,1,10",
            SymmetryOperations="x,y,z;-x,-y,-z",
            OutputWorkspace=ws_name + "_out",
            OutputDataWorkspace=ws_name + "_outdata",
            OutputNormalizationWorkspace=ws_name + "_outnorm",
        )

        out = mtd[ws_name + "_out"]
        self.assertEqual(out.getNumDims(), 3)
        signal = out.getSignalArray()
        self.assertGreater(signal.size, 1)
        norm = mtd[ws_name + "_outnorm"].getSignalArray()
        non_empty = norm != 0
        self.assertTrue(non_empty.any())
        self.assertTrue(np.all(np.isfinite(signal[non_empty])))
        np.testing.assert_allclose(signal[non_empty], 1.0, rtol=1e-6)

        DeleteWorkspaces(
            [
                ws_name + "_raw",
                ws_name + "_data",
                ws_name + "_norm",
                ws_name + "_out",
                ws_name + "_outdata",
                ws_name + "_outnorm",
            ]
        )


class MDNormWANDRLUTest(systemtesting.MantidSystemTest):
    """Same as MDNormWANDTest, but with RLU=True and the real WAND UB, binned onto the same
    known-good, peak-containing HKL extents as ConvertWANDSCDtoQTest. Data and normalization
    are still identical, so every non-empty output bin must still be exactly 1.0."""

    def requiredFiles(self):
        return ["HB2C_WANDSCD_data.nxs"]

    def skipTests(self):
        return _skip_test()

    def requiredMemoryMB(self):
        return 4000

    def runTest(self):
        ws_name = "MDNormWANDRLUTest"
        LoadMD("HB2C_WANDSCD_data.nxs", OutputWorkspace=ws_name + "_raw")
        SetGoniometer(ws_name + "_raw", Axis0="s1,0,1,0,1", Average=False)

        ConvertHFIRSCDtoMDE(InputWorkspace=ws_name + "_raw", Wavelength=1.488, OutputWorkspace=ws_name + "_data")
        ConvertHFIRSCDtoMDE(InputWorkspace=ws_name + "_raw", Wavelength=1.488, OutputWorkspace=ws_name + "_norm")
        SetUB(
            ws_name + "_data",
            UB="-2.7315243158024499e-17,1.7706197424726486e-01,-9.2794248657701375e-03,"
            "1.773049645390071e-01,0.,0.,1.2303228382369809e-17,-9.2794248657701254e-03,-1.7706197424726489e-01",
        )

        MDNorm(
            InputWorkspace=ws_name + "_data",
            MonoSCDNormalizationWorkspace=ws_name + "_norm",
            RLU=True,
            Dimension0Binning="-0.62,0.062,0.62",
            Dimension1Binning="-2.02,0.452,7.02",
            Dimension2Binning="-6.52,0.452,2.52",
            SymmetryOperations="x,y,z;-x,-y,-z",
            OutputWorkspace=ws_name + "_out",
            OutputDataWorkspace=ws_name + "_outdata",
            OutputNormalizationWorkspace=ws_name + "_outnorm",
        )

        out = mtd[ws_name + "_out"]
        self.assertEqual(out.getNumDims(), 3)
        signal = out.getSignalArray()
        self.assertGreater(signal.size, 1)
        norm = mtd[ws_name + "_outnorm"].getSignalArray()
        non_empty = norm != 0
        self.assertTrue(non_empty.any())
        self.assertTrue(np.all(np.isfinite(signal[non_empty])))
        np.testing.assert_allclose(signal[non_empty], 1.0, rtol=1e-6)

        DeleteWorkspaces(
            [
                ws_name + "_raw",
                ws_name + "_data",
                ws_name + "_norm",
                ws_name + "_out",
                ws_name + "_outdata",
                ws_name + "_outnorm",
            ]
        )
