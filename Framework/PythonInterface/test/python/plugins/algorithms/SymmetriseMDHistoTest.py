# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.simpleapi import SymmetriseMDHisto, CreateMDHistoWorkspace, AnalysisDataService as ADS
import numpy as np


class SymmeteriseMDHistoTest(unittest.TestCase):
    def tearDown(self):
        ADS.clear()

    def test_even_nbins_unweighted_avg_full_signal(self):
        signal = np.ones(8)
        ws = self._make_md_ws(np.ones(8), "ws1")

        ws_sym = SymmetriseMDHisto(ws, PointGroup="-1", WeightedAverage=False, OutputWorkspace=ws.name() + "_sym")

        self.assertTrue(np.allclose(signal, ws_sym.getSignalArray().flatten()))
        self.assertTrue(np.allclose(signal, ws_sym.getErrorSquaredArray().flatten()))

    def test_even_nbins_weighted_avg_full_signal(self):
        signal = np.ones(8)
        ws = self._make_md_ws(np.ones(8), "ws2")

        ws_sym = SymmetriseMDHisto(ws, PointGroup="-1", WeightedAverage=True, OutputWorkspace=ws.name() + "_sym")

        self.assertTrue(np.allclose(signal, ws_sym.getSignalArray().flatten()))
        self.assertTrue(np.allclose(0.5 * signal, ws_sym.getErrorSquaredArray().flatten()))  # 2 contributing bins

    def test_even_nbins_unweighted_avg_partial_signal(self):
        signal = np.zeros(8)
        signal[0] = 1.0
        ws = self._make_md_ws(signal, "ws4")

        ws_sym = SymmetriseMDHisto(ws, PointGroup="-1", WeightedAverage=False, OutputWorkspace=ws.name() + "_sym")

        expected_signal = signal.copy()
        expected_signal[-1] = 1.0
        self.assertTrue(np.allclose(expected_signal, ws_sym.getSignalArray().flatten()))
        self.assertTrue(np.allclose(expected_signal, ws_sym.getErrorSquaredArray().flatten()))  # 1 contributing bin

    def test_even_nbins_weighted_avg_partial_signal(self):
        signal = np.zeros(8)
        signal[0] = 1.0
        ws = self._make_md_ws(signal, "ws4")

        ws_sym = SymmetriseMDHisto(ws, PointGroup="-1", WeightedAverage=True, OutputWorkspace=ws.name() + "_sym")

        expected_signal = signal.copy()
        expected_signal[-1] = 1.0
        self.assertTrue(np.allclose(expected_signal, ws_sym.getSignalArray().flatten()))
        self.assertTrue(np.allclose(expected_signal, ws_sym.getErrorSquaredArray().flatten()))  # 1 contributing bin

    @staticmethod
    def _make_md_ws(signal, wsname):
        nbins = int(len(signal) ** (1 / 3))
        return CreateMDHistoWorkspace(
            SignalInput=signal,
            ErrorInput=signal,
            Dimensionality=3,
            Extents="-1,1,-1,1,-1,1",
            NumberOfBins="2,2,2",
            Names="H,K,L",
            Units="rlu,rlu,rlu",
            OutputWorkspace=wsname,
        )


if __name__ == "__main__":
    unittest.main()
