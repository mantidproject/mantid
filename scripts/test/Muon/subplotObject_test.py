import unittest


from Muon.GUI.ElementalAnalysis.Plotting.subPlot_object import subPlot
from mantid.simpleapi import *
from mantid import plots

from matplotlib.figure import Figure


class subplotTest(unittest.TestCase):

    def setUp(self):
        self.subplot = subPlot("unit test")

    def test_basicAddLine(self):
        # can use dummy line here
        line = ["a", "test"]
        ws = CreateWorkspace(DataX=[1, 2, 3, 4], DataY=[4, 5, 6, 7], NSpec=2)
        label = "label"
        # add line
        self.subplot.addLine(label, line, ws)
        # get output
        out = self.subplot.lines
        self.assertEqual(len(out), 1)
        self.assertEqual(out[label], line)
        self.assertEqual(self.subplot.specNum[label], 1)
        stored = self.subplot.ws
        self.assertEqual(stored[ws], [label])

        DeleteWorkspace(ws)

    def test_addTwoLines(self):
        # can use dummy lines here
        line1 = ["a", "unit"]
        line2 = ["b", "test"]
        ws = CreateWorkspace(DataX=[1, 2, 3, 4], DataY=[4, 5, 6, 7], NSpec=2)
        label1 = "label1"
        label2 = "label2"
        # add lines - share a ws
        self.subplot.addLine(label1, line1, ws, 1)
        self.subplot.addLine(label2, line2, ws, 2)
        # get output
        out = self.subplot.lines

        self.assertEqual(len(out), 2)
        self.assertEqual(out[label1], line1)
        self.assertEqual(self.subplot.specNum[label1], 1)

        self.assertEqual(out[label2], line2)
        self.assertEqual(self.subplot.specNum[label2], 2)
        # since both use same ws, should have 2 labels
        stored = self.subplot.ws
        self.assertEqual(stored[ws], [label1, label2])
        DeleteWorkspace(ws)

    def test_addTwoLinesDiff(self):
        # can use dummy lines here
        line1 = ["a", "unit"]
        line2 = ["b", "test"]
        ws1 = CreateWorkspace(DataX=[1, 2, 3, 4], DataY=[4, 5, 6, 7], NSpec=2)
        ws2 = CreateWorkspace(DataX=[1, 2, 3, 4], DataY=[4, 5, 6, 7], NSpec=1)
        label1 = "label1"
        label2 = "label2"
        # add lines
        self.subplot.addLine(label1, line1, ws1, 2)
        self.subplot.addLine(label2, line2, ws2, 1)
        # get output
        out = self.subplot.lines
        # should have both lines
        self.assertEqual(len(out), 2)
        self.assertEqual(out[label1], line1)
        self.assertEqual(self.subplot.specNum[label1], 2)

        self.assertEqual(out[label2], line2)
        self.assertEqual(self.subplot.specNum[label2], 1)
        # each ws should have 1 label
        stored = self.subplot.ws
        self.assertEqual(stored[ws1], [label1])
        self.assertEqual(stored[ws2], [label2])

        DeleteWorkspace(ws1)
        DeleteWorkspace(ws2)

    def test_removeLine(self):
        ws1 = CreateWorkspace(DataX=[1, 2, 3, 4], DataY=[4, 5, 6, 7], NSpec=2)
        ws2 = CreateWorkspace(DataX=[1, 2, 3, 4], DataY=[4, 5, 6, 7], NSpec=1)
        label1 = "label1"
        label2 = "label2"
        # create real lines
        fig = Figure()
        sub = fig.add_subplot(1, 1, 1)
        line1 = plots.plotfunctions.plot(sub, ws1, specNum=1)
        line2 = plots.plotfunctions.plot(sub, ws2, specNum=1)
        # add them both
        self.subplot.addLine(label1, line1, ws1, 2)
        self.subplot.addLine(label2, line2, ws2, 1)
        # remove one line
        self.subplot.removeLine(label2)
        # check output
        out = self.subplot.lines
        # should only have line 1
        self.assertEqual(len(out), 1)
        self.assertEqual(out[label1], line1)
        self.assertEqual(self.subplot.specNum[label1], 2)

        stored = self.subplot.ws
        self.assertEqual(stored[ws1], [label1])

        DeleteWorkspace(ws1)
        DeleteWorkspace(ws2)

    def test_removeLineShare(self):
        ws = CreateWorkspace(DataX=[1, 2, 3, 4], DataY=[4, 5, 6, 7], NSpec=2)
        label1 = "label1"
        label2 = "label2"
        # create real lines
        fig = Figure()
        sub = fig.add_subplot(1, 1, 1)
        line1 = plots.plotfunctions.plot(sub, ws, specNum=1)
        line2 = plots.plotfunctions.plot(sub, ws, specNum=2)
        # add them both
        self.subplot.addLine(label1, line1, ws, 1)
        self.subplot.addLine(label2, line2, ws, 2)
        # remove one line
        self.subplot.removeLine(label2)
        # check output
        out = self.subplot.lines
        # should only have 1
        self.assertEqual(len(out), 1)
        self.assertEqual(out[label1], line1)
        self.assertEqual(self.subplot.specNum[label1], 1)
        # only have 1 label for ws
        stored = self.subplot.ws
        self.assertEqual(stored[ws], [label1])

        DeleteWorkspace(ws)


if __name__ == "__main__":
    unittest.main()
