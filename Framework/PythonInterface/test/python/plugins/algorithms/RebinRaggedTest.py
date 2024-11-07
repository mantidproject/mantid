# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import mantid.simpleapi as api
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
import numpy as np
from numpy import nan as np_nan
from numpy import inf as np_inf

try:
    from math import nan as math_nan
except ImportError:
    math_nan = np_nan  # rhel7 python is too old


class RebinRaggedTest(unittest.TestCase):
    def test_nomad_inplace(self):
        api.LoadNexusProcessed(Filename="NOM_91796_banks.nxs", OutputWorkspace="NOM_91796_banks")
        alg_test = run_algorithm(
            "RebinRagged",
            InputWorkspace="NOM_91796_banks",
            OutputWorkspace="NOM_91796_banks",
            XMin=[0.67, 1.20, 2.42, 3.70, 4.12, 0.39],
            Delta=0.02,  # original data bin size
            XMax=[10.20, 20.8, np_nan, math_nan, np_nan, 9.35],
            Version=1,
        )

        self.assertTrue(alg_test.isExecuted())

        # Verify ....
        outputws = AnalysisDataService.retrieve("NOM_91796_banks")
        for i, Xlen in enumerate([478, 981, 1880, 1816, 1795, 449]):
            self.assertEqual(len(outputws.readX(i)), Xlen)

        AnalysisDataService.remove("NOM_91796_banks")

    def test_nomad_no_mins(self):
        api.LoadNexusProcessed(Filename="NOM_91796_banks.nxs", OutputWorkspace="NOM_91796_banks")
        alg_test = run_algorithm(
            "RebinRagged",
            InputWorkspace="NOM_91796_banks",
            OutputWorkspace="NOM_91796_banks",
            Delta=0.04,  # double original data bin size
            XMax=[10.20, 20.8, np_inf, math_nan, np_nan, 9.35],
            Version=1,
        )

        self.assertTrue(alg_test.isExecuted())

        # Verify ....
        outputws = AnalysisDataService.retrieve("NOM_91796_banks")
        for i, Xlen in enumerate([256, 521, 1001, 1001, 1001, 235]):  # larger than in test_nomad_inplace
            self.assertEqual(len(outputws.readX(i)), Xlen)

        AnalysisDataService.remove("NOM_91796_banks")

    def test_hist_workspace(self):
        # numpy 1.7 (on rhel7) doesn't have np.full
        xmins = np.full((200,), 2600.0)
        xmins[11] = 3000.0
        xmaxs = np.full((200,), 6200.0)
        xmaxs[12] = 5000.0
        deltas = np.full(200, 400.0)
        deltas[13] = 600.0

        ws = api.CreateSampleWorkspace(OutputWorkspace="RebinRagged_hist", WorkspaceType="Histogram")

        rebinned = api.RebinRagged(ws, XMin=xmins, XMax=xmaxs, Delta=deltas, Version=1)

        self.assertEqual(rebinned.getNumberHistograms(), 200)
        for i in range(rebinned.getNumberHistograms()):
            label = "index={}".format(i)
            if i == 11:
                self.assertEqual(rebinned.readX(i).size, 9, label)
            elif i == 12 or i == 13:
                self.assertEqual(rebinned.readX(i).size, 7, label)
            else:
                self.assertEqual(
                    rebinned.readX(i).size,
                    10,
                )

            y = rebinned.readY(i)
            if i == 13:
                np.testing.assert_almost_equal(0.9, y, err_msg=label)
            else:
                # parameters are set so all y-values are 0.6
                np.testing.assert_almost_equal(0.6, y, err_msg=label)

    def test_event_workspace(self):
        # numpy 1.7 (on rhel7) doesn't have np.full
        xmins = np.full((200,), 2600.0)
        xmins[11] = 3000.0
        xmaxs = np.full((200,), 6200.0)
        xmaxs[12] = 5000.0
        deltas = np.full(200, 400.0)
        deltas[13] = 600.0

        ws = api.CreateSampleWorkspace(OutputWorkspace="RebinRagged_events", WorkspaceType="Event")

        rebinned = api.RebinRagged(ws, XMin=xmins, XMax=xmaxs, Delta=deltas, Version=1)

        self.assertEqual(rebinned.getNumberHistograms(), 200)
        for i in range(rebinned.getNumberHistograms()):
            label = "index={}".format(i)
            if i == 11:
                self.assertEqual(rebinned.readX(i).size, 9, label)
            elif i == 12 or i == 13:
                self.assertEqual(rebinned.readX(i).size, 7, label)
            else:
                self.assertEqual(
                    rebinned.readX(i).size,
                    10,
                )

            y = rebinned.readY(i)
            if i == 13:
                np.testing.assert_almost_equal(21, y, err_msg=label)
            else:
                # parameters are set so all y-values are 0.6
                np.testing.assert_almost_equal(14, y, err_msg=label)


if __name__ == "__main__":
    unittest.main()
