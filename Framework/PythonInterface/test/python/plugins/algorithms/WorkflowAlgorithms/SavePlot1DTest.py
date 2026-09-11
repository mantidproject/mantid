# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import os
from mantid import AnalysisDataServiceImpl, config, simpleapi

try:
    import plotly  # noqa: F401

    havePlotly = True
except ImportError:
    havePlotly = False
# check if matplotlib is available and a new enough version
matplotlibissue = None  # indicates there are no issues
try:
    import matplotlib

    matplotlib.use("agg")
except:
    matplotlibissue = "Problem importing matplotlib"


class SavePlot1DTest(unittest.TestCase):
    def makeWs(self):
        simpleapi.CreateWorkspace(
            OutputWorkspace="test1",
            DataX="1,2,3,4,5,1,2,3,4,5",
            DataY="1,2,3,4,2,3,4,5",
            DataE="1,2,3,4,2,3,4,5",
            NSpec="2",
            UnitX="dSpacing",
            Distribution="1",
            YUnitlabel="S(q)",
        )
        simpleapi.CreateWorkspace(
            OutputWorkspace="test2",
            DataX="1,2,3,4,5,1,2,3,4,5",
            DataY="1,2,3,4,2,3,4,5",
            DataE="1,2,3,4,2,3,4,5",
            NSpec="2",
            UnitX="Momentum",
            VerticalAxisUnit="TOF",
            VerticalAxisValues="1,2",
            Distribution="1",
            YUnitLabel="E",
            WorkspaceTitle="x",
        )
        simpleapi.GroupWorkspaces("test1,test2", OutputWorkspace="group")
        self.plotfile = os.path.join(config.getString("defaultsave.directory"), "plot.png")
        self.plotlyfile = os.path.join(config.getString("defaultsave.directory"), "plot.html")

    def cleanup(self):
        ads = AnalysisDataServiceImpl.Instance()
        ads.remove("group")
        ads.remove("test1")
        ads.remove("test2")
        if os.path.exists(self.plotfile):
            os.remove(self.plotfile)
        if os.path.exists(self.plotlyfile):
            os.remove(self.plotlyfile)

    @unittest.skipIf(matplotlibissue is not None, matplotlibissue)
    def testPlotSingle(self):
        self.makeWs()
        simpleapi.SavePlot1D("test1", self.plotfile)
        self.assertGreater(os.path.getsize(self.plotfile), 1e4)
        self.cleanup()

    @unittest.skipIf(matplotlibissue is not None, matplotlibissue)
    def testPlotGroup(self):
        self.makeWs()
        simpleapi.SavePlot1D("group", self.plotfile)
        self.assertGreater(os.path.getsize(self.plotfile), 1e4)
        self.cleanup()

    def assertIsPlotlyDiv(self, div):
        """A bare plot div, with plotly.js left out so the page can supply it"""
        self.assertGreater(len(div), 0)  # confirm result is non-empty
        self.assertIn("plotly-graph-div", div)
        self.assertIn("Plotly.newPlot", div)
        self.assertNotIn("<html", div)  # a div, not a whole page

    @unittest.skipIf(not havePlotly, "Do not have plotly installed")
    def testPlotlySingle(self):
        self.makeWs()
        div = simpleapi.SavePlot1D(InputWorkspace="test1", OutputType="plotly")
        self.cleanup()
        self.assertIsPlotlyDiv(div)

    @unittest.skipIf(not havePlotly, "Do not have plotly installed")
    def testPlotlyGroup(self):
        self.makeWs()
        div = simpleapi.SavePlot1D(InputWorkspace="group", OutputType="plotly")
        self.cleanup()
        self.assertIsPlotlyDiv(div)

    @unittest.skipIf(not havePlotly, "Do not have plotly installed")
    def testPlotlyFullSingle(self):
        self.makeWs()
        result = simpleapi.SavePlot1D(InputWorkspace="test1", OutputFilename=self.plotlyfile, OutputType="plotly-full")
        self.assertTrue(os.path.exists(self.plotlyfile))
        with open(self.plotlyfile) as handle:
            contents = handle.read()
        self.cleanup()
        # Result reports the file that was written, however the platform spells the path
        self.assertEqual(os.path.normcase(os.path.abspath(result)), os.path.normcase(os.path.abspath(self.plotlyfile)))
        self.assertIn("plotly-graph-div", contents)
        self.assertIn("Plotly.newPlot", contents)
        self.assertIn("</html>", contents)  # a whole page, not just the div
        self.assertGreater(len(contents), 1e5)  # plotly.js is bundled into the page

    @unittest.skipIf(not havePlotly, "Do not have plotly installed")
    def testPlotlyFullGroup(self):
        self.makeWs()
        simpleapi.SavePlot1D(InputWorkspace="group", OutputFilename=self.plotlyfile, OutputType="plotly-full")
        self.assertTrue(os.path.exists(self.plotlyfile))
        size = os.path.getsize(self.plotlyfile)
        self.cleanup()
        self.assertGreater(size, 1e5)


if __name__ == "__main__":
    unittest.main()
