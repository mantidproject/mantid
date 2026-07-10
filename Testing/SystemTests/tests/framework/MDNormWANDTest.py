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
    detector-space MDHisto data (as produced by LoadWANDSCD) to a Q-sample MDEventWorkspace via
    ConvertHFIRSCDtoMDE twice, once each for "data" and "normalization", so the two are
    identical, then feed both into MDNorm via MonoSCDNormalizationWorkspace. Since data and
    normalization are identical, every non-empty output bin must be exactly 1.0."""

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
            SymmetryOperations="x,y,z;-x,-y,-z",
            OutputWorkspace=ws_name + "_out",
            OutputDataWorkspace=ws_name + "_outdata",
            OutputNormalizationWorkspace=ws_name + "_outnorm",
        )

        out = mtd[ws_name + "_out"]
        self.assertEqual(out.getNumDims(), 3)
        signal = out.getSignalArray()
        self.assertTrue(np.all(np.isfinite(signal)))
        np.testing.assert_allclose(signal, 1.0, rtol=1e-6)

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
    """Same as MDNormWANDTest, but with RLU=True: exercises the reciprocal-lattice-unit
    binning path for monochromatic-SCD input, including the box-tree-derived maxQ estimate
    used to set default Q-dimension extents. Data and normalization are still identical, so
    every non-empty output bin must still be exactly 1.0, regardless of RLU."""

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
        SetUB(ws_name + "_data", 5, 5, 5, 90, 90, 90)

        MDNorm(
            InputWorkspace=ws_name + "_data",
            MonoSCDNormalizationWorkspace=ws_name + "_norm",
            RLU=True,
            SymmetryOperations="x,y,z;-x,-y,-z",
            OutputWorkspace=ws_name + "_out",
            OutputDataWorkspace=ws_name + "_outdata",
            OutputNormalizationWorkspace=ws_name + "_outnorm",
        )

        out = mtd[ws_name + "_out"]
        self.assertEqual(out.getNumDims(), 3)
        signal = out.getSignalArray()
        self.assertTrue(np.all(np.isfinite(signal)))
        np.testing.assert_allclose(signal, 1.0, rtol=1e-6)

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
