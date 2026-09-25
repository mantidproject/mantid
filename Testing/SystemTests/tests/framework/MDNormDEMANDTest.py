# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import systemtesting
from mantid.simpleapi import DeleteWorkspaces, HB3AAdjustSampleNorm, HFIRGoniometerIndependentBackground, MDNorm, SetUB, mtd


def _delete_existing(workspaces):
    existing = [workspace for workspace in workspaces if mtd.doesExist(workspace)]
    if existing:
        DeleteWorkspaces(existing)


class MDNormDEMANDTest(systemtesting.MantidSystemTest):
    """End-to-end test of MDNorm's monochromatic-SCD path for DEMAND (HB3A): produce a data
    workspace and an identical normalization workspace via HB3AAdjustSampleNorm, then feed both
    into MDNorm. Since data and normalization are identical, every non-empty output bin must be
    exactly 1.0."""

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
                ws_name + "_data",
                ws_name + "_norm",
                ws_name + "_out",
                ws_name + "_outdata",
                ws_name + "_outnorm",
            ]
        )


class MDNormDEMANDRLUTest(systemtesting.MantidSystemTest):
    """Same as MDNormDEMANDTest, but with RLU=True, a cubic placeholder UB, and the "23" point
    group as SymmetryOperations (valid here since the UB is defined by the test itself). Data
    and normalization are still identical, so every non-empty output bin must still be
    exactly 1.0."""

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
            Dimension0Binning="0.45",
            Dimension1Binning="0.45",
            Dimension2Binning="0.45",
            SymmetryOperations="23",
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
                ws_name + "_data",
                ws_name + "_norm",
                ws_name + "_out",
                ws_name + "_outdata",
                ws_name + "_outnorm",
            ]
        )


class MDNormDEMANDBackgroundTest(systemtesting.MantidSystemTest):
    """End-to-end mono-SCD background subtraction for DEMAND. The detector-space background is
    estimated by HFIRGoniometerIndependentBackground, converted to Q_sample events with
    HB3AAdjustSampleNorm, then passed to MDNorm with the same mono-SCD normalization as the sample."""

    def requiredFiles(self):
        return ["HB3A_exp0722_scan0220.nxs"]

    def runTest(self):
        ws_name = "MDNormDEMANDBackgroundTest"
        workspaces = [
            ws_name + "_detector",
            ws_name + "_detector_background",
            ws_name + "_data",
            ws_name + "_norm",
            ws_name + "_background",
            ws_name + "_out",
            ws_name + "_outdata",
            ws_name + "_outnorm",
            ws_name + "_outbkgdata",
            ws_name + "_outbkgnorm",
        ]
        try:
            HB3AAdjustSampleNorm(
                Filename="HB3A_exp0722_scan0220.nxs",
                NormaliseBy="None",
                NormalizeData=False,
                OutputType="Detector",
                MergeInputs=False,
                OutputWorkspace=ws_name + "_detector",
            )

            HFIRGoniometerIndependentBackground(
                InputWorkspace=ws_name + "_detector",
                BackgroundLevel=100,
                OutputWorkspace=ws_name + "_detector_background",
            )

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

            HB3AAdjustSampleNorm(
                InputWorkspaces=ws_name + "_detector_background",
                NormaliseBy="None",
                NormalizeData=False,
                OutputType="Q-sample events",
                MergeInputs=False,
                OutputWorkspace=ws_name + "_background",
            )

            MDNorm(
                InputWorkspace=ws_name + "_data",
                MonoSCDNormalizationWorkspace=ws_name + "_norm",
                BackgroundWorkspace=ws_name + "_background",
                RLU=False,
                Dimension0Binning="-10,1,10",
                Dimension1Binning="-10,1,10",
                Dimension2Binning="-10,1,10",
                SymmetryOperations="x,y,z;-x,-y,-z",
                OutputWorkspace=ws_name + "_out",
                OutputDataWorkspace=ws_name + "_outdata",
                OutputNormalizationWorkspace=ws_name + "_outnorm",
                OutputBackgroundDataWorkspace=ws_name + "_outbkgdata",
                OutputBackgroundNormalizationWorkspace=ws_name + "_outbkgnorm",
            )

            out = mtd[ws_name + "_out"].getSignalArray()
            data = mtd[ws_name + "_outdata"].getSignalArray()
            norm = mtd[ws_name + "_outnorm"].getSignalArray()
            background = mtd[ws_name + "_outbkgdata"].getSignalArray()
            background_norm = mtd[ws_name + "_outbkgnorm"].getSignalArray()

            np.testing.assert_allclose(background_norm, norm)
            valid = (norm != 0) & (background_norm != 0)
            self.assertTrue(valid.any())
            self.assertTrue(np.any(background[valid] != 0))
            np.testing.assert_allclose(out[valid], data[valid] / norm[valid] - background[valid] / background_norm[valid], rtol=1e-6)
        finally:
            _delete_existing(workspaces)
