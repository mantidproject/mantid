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

    def cleanup(self):
        ads = AnalysisDataServiceImpl.Instance()
        ads.remove("group")
        ads.remove("test1")
        ads.remove("test2")
        if os.path.exists(self.plotfile):
            os.remove(self.plotfile)

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

    @unittest.skipIf(not havePlotly, "Do not have plotly installed")
    def testPlotlySingle(self):
        self.makeWs()
        div = simpleapi.SavePlot1D(InputWorkspace="test1", OutputType="plotly")
        self.cleanup()
        self.assertGreater(len(div), 0)  # confirm result is non-empty

    @unittest.skipIf(not havePlotly, "Do not have plotly installed")
    def testPlotlyGroup(self):
        self.makeWs()
        div = simpleapi.SavePlot1D(InputWorkspace="group", OutputType="plotly")
        self.cleanup()
        self.assertGreater(len(div), 0)  # confirm result is non-empty


if __name__ == "__main__":
    unittest.main()
