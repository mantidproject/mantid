# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import Load
from mantid import mtd, FileFinder


class WorkspaceHistoryTest(unittest.TestCase):

    def test_history(self):
        ws_name = "GEM38370_Focussed_Legacy"
        file_name = FileFinder.getFullPath(ws_name + ".nxs")
        Load(file_name, OutputWorkspace=ws_name)

        ws = mtd[ws_name]
        history = ws.getHistory()

        self.assertEqual(history.empty(), False)
        self.assertEqual(history.size(), 4)

        alg_hists = history.getAlgorithmHistories()
        self.assertEqual(len(alg_hists), 4)
        self.assertEqual(alg_hists[0].name(), "LoadRaw")
        self.assertEqual(alg_hists[1].name(), "AlignDetectors")
        self.assertEqual(alg_hists[2].name(), "DiffractionFocussing")
        self.assertEqual(alg_hists[3].name(), "Load")

        self.assertEqual(history.getAlgorithmHistory(0).name(), "LoadRaw")
        self.assertEqual(history.getAlgorithmHistory(1).name(), "AlignDetectors")
        self.assertEqual(history.getAlgorithmHistory(2).name(), "DiffractionFocussing")
        self.assertEqual(history.getAlgorithmHistory(3).name(), "Load")

        alg = history.lastAlgorithm()
        self.assertEqual(alg.name(), "Load")

        alg = history.getAlgorithm(history.size()-1)
        self.assertEqual(alg.name(), "Load")

if __name__ == '__main__':
    unittest.main()
