# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import systemtesting
from mantid.simpleapi import DeleteWorkspaces, HB3AAdjustSampleNorm, MDNorm, SetUB, mtd


class MDNormDEMANDTest(systemtesting.MantidSystemTest):
    """End-to-end test of MDNorm's monochromatic-SCD path for DEMAND (HB3A): produce a data
    workspace and a normalization workspace via HB3AAdjustSampleNorm (using the same scan as
    both data and vanadium, so the two are identical, as in HB3AAdjustSampleNormTest's
    SingleScanDataAsVanadiumOutputNormalizationWorkspace), then feed both into MDNorm via
    MonoSCDNormalizationWorkspace. Since data and normalization are identical, every non-empty
    output bin must be exactly 1.0 regardless of the exact binning/symmetry operations applied,
    which makes this a strong, easily-assertable correctness check for the new binNormalizationWS
    path (per-symmetry-op binning applied identically to both workspaces)."""

    def requiredFiles(self):
        return ["HB3A_exp0722_scan0220.nxs"]

    def runTest(self):
        ws_name = "MDNormDEMANDTest"
        HB3AAdjustSampleNorm(
            Filename="HB3A_exp0722_scan0220.nxs",
            VanadiumFile="HB3A_exp0722_scan0220.nxs",
            NormaliseBy="None",
            NormalizeData=False,
            OutputType="Q-sample events",
            MergeInputs=False,
            OutputWorkspace=ws_name + "_data",
            OutputNormalizationWorkspace=ws_name + "_norm",
        )

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
                ws_name + "_data",
                ws_name + "_norm",
                ws_name + "_out",
                ws_name + "_outdata",
                ws_name + "_outnorm",
            ]
        )


class MDNormDEMANDRLUTest(systemtesting.MantidSystemTest):
    """Same as MDNormDEMANDTest, but with RLU=True: exercises the reciprocal-lattice-unit
    binning path for monochromatic-SCD input, including the box-tree-derived maxQ estimate
    used to set default Q-dimension extents (since there is no time-of-flight trajectory,
    and hence no MDNorm_low/MDNorm_high, to derive it from). Data and normalization are
    still identical, so every non-empty output bin must still be exactly 1.0, regardless
    of RLU."""

    def requiredFiles(self):
        return ["HB3A_exp0722_scan0220.nxs"]

    def runTest(self):
        ws_name = "MDNormDEMANDRLUTest"
        HB3AAdjustSampleNorm(
            Filename="HB3A_exp0722_scan0220.nxs",
            VanadiumFile="HB3A_exp0722_scan0220.nxs",
            NormaliseBy="None",
            NormalizeData=False,
            OutputType="Q-sample events",
            MergeInputs=False,
            OutputWorkspace=ws_name + "_data",
            OutputNormalizationWorkspace=ws_name + "_norm",
        )
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
                ws_name + "_data",
                ws_name + "_norm",
                ws_name + "_out",
                ws_name + "_outdata",
                ws_name + "_outnorm",
            ]
        )
