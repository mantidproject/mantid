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
        self.assertEquals(len(properties), 6)
        self.assertEquals(properties[0].name(), "OutputWorkspace")
        self.assertEquals(properties[0].value(), "GEM38370")
        self.assertEquals(properties[0].isDefault(), False)
        self.assertEquals(properties[0].direction(), "Output")

        self.assertEquals(properties[1].name(), "SpectrumMin")
        self.assertEquals(properties[1].value(), 1)
        self.assertEquals(properties[1].isDefault(), True)
        self.assertEquals(properties[1].direction(), "Input")

        self.assertEquals(properties[2].name(), "SpectrumMax")
        self.assertEquals(properties[2].value(), 2147483632)
        self.assertEquals(properties[2].isDefault(), True)
        self.assertEquals(properties[2].direction(), "Input")

        self.assertEquals(properties[3].name(), "SpectrumList")
        self.assertEquals(properties[3].value(), "")
        self.assertEquals(properties[3].isDefault(), True)
        self.assertEquals(properties[3].direction(), "Input")

        self.assertEquals(properties[4].name(), "Cache")
        self.assertEquals(properties[4].value(), "If Slow")
        self.assertEquals(properties[4].isDefault(), True)
        self.assertEquals(properties[4].direction(), "Input")

        self.assertEquals(properties[5].name(), "LoadLogFiles")
        self.assertEquals(properties[5].value(), True)
        self.assertEquals(properties[5].isDefault(), True)
        self.assertEquals(properties[5].direction(), "Input")

if __name__ == '__main__':
    unittest.main()
