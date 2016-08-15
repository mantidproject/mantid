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

        self.assertEquals(history.empty(), False)
        self.assertEquals(history.size(), 4)

        alg_hists = history.getAlgorithmHistories()
        self.assertEquals(len(alg_hists), 4)
        self.assertEquals(alg_hists[0].name(), "LoadRaw")
        self.assertEquals(alg_hists[1].name(), "AlignDetectors")
        self.assertEquals(alg_hists[2].name(), "DiffractionFocussing")
        self.assertEquals(alg_hists[3].name(), "Load")

        self.assertEquals(history.getAlgorithmHistory(0).name(), "LoadRaw")
        self.assertEquals(history.getAlgorithmHistory(1).name(), "AlignDetectors")
        self.assertEquals(history.getAlgorithmHistory(2).name(), "DiffractionFocussing")
        self.assertEquals(history.getAlgorithmHistory(3).name(), "Load")

        alg = history.lastAlgorithm()
        self.assertEquals(alg.name(), "Load")

        alg = history.getAlgorithm(history.size()-1)
        self.assertEquals(alg.name(), "Load")

if __name__ == '__main__':
    unittest.main()
