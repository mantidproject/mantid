from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import Load
from mantid import mtd, FileFinder


class PropertyHistoryTest(unittest.TestCase):

    def test_history(self):
        ws_name = "GEM38370_Focussed_Legacy"
        file_name = FileFinder.getFullPath(ws_name + ".nxs")
        Load(file_name, OutputWorkspace=ws_name)

        ws = mtd[ws_name]
        history = ws.getHistory()
        alg_hist = history.getAlgorithmHistory(0)

        self.assertEquals(alg_hist.name(), "LoadRaw")

        properties = alg_hist.getProperties()
        self.assertEquals(len(properties), 7)
        self.assertEquals(properties[0].name(), "Filename")
        self.assertEquals(properties[0].value(), "/home/dmn58364/Mantid/trunk/Test/Data/GEM38370.raw")
        self.assertEquals(properties[0].isDefault(), False)
        self.assertEquals(properties[0].direction(), 0)

        self.assertEquals(properties[1].name(), "OutputWorkspace")
        self.assertEquals(properties[1].value(), "GEM38370")
        self.assertEquals(properties[1].isDefault(), False)
        self.assertEquals(properties[1].direction(), 1)

        self.assertEquals(properties[2].name(), "SpectrumMin")
        self.assertEquals(properties[2].value(), '1')
        self.assertEquals(properties[2].isDefault(), True)
        self.assertEquals(properties[2].direction(), 0)

        self.assertEquals(properties[3].name(), "SpectrumMax")
        self.assertEquals(properties[3].value(), '2147483632')
        self.assertEquals(properties[3].isDefault(), True)
        self.assertEquals(properties[3].direction(), 0)

        self.assertEquals(properties[4].name(), "SpectrumList")
        self.assertEquals(properties[4].value(), "")
        self.assertEquals(properties[4].isDefault(), True)
        self.assertEquals(properties[4].direction(), 0)

        self.assertEquals(properties[5].name(), "Cache")
        self.assertEquals(properties[5].value(), "If Slow")
        self.assertEquals(properties[5].isDefault(), True)
        self.assertEquals(properties[5].direction(), 0)

        self.assertEquals(properties[6].name(), "LoadLogFiles")
        self.assertEquals(properties[6].value(), '1')
        self.assertEquals(properties[6].isDefault(), True)
        self.assertEquals(properties[6].direction(), 0)

if __name__ == '__main__':
    unittest.main()
