# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest, os
from mantid import AnalysisDataServiceImpl, config, simpleapi


class SavePlot1DTest(unittest.TestCase):
    def makeWs(self):
        simpleapi.CreateWorkspace(OutputWorkspace='test1', DataX='1,2,3,4,5,1,2,3,4,5', DataY='1,2,3,4,2,3,4,5',
                                  DataE='1,2,3,4,2,3,4,5', NSpec='2', UnitX='TOF', Distribution='1', YUnitlabel="S(q)")
        simpleapi.CreateWorkspace(OutputWorkspace='test2', DataX='1,2,3,4,5,1,2,3,4,5', DataY='1,2,3,4,2,3,4,5',
                                  DataE='1,2,3,4,2,3,4,5', NSpec='2',
                                  UnitX='Momentum', VerticalAxisUnit='TOF', VerticalAxisValues='1,2', Distribution='1',
                                  YUnitLabel='E', WorkspaceTitle='x')
        simpleapi.GroupWorkspaces("test1,test2", OutputWorkspace="group")
        self.plotfile = os.path.join(config.getString('defaultsave.directory'), 'plot.png')

    def cleanup(self):
        ads = AnalysisDataServiceImpl.Instance()
        ads.remove("group")
        ads.remove("test1")
        ads.remove("test2")
        if os.path.exists(self.plotfile):
            os.remove(self.plotfile)

    def testPlot(self):
        self.makeWs()
        ok2run = ''
        try:
            import matplotlib
            from distutils.version import LooseVersion
            if LooseVersion(matplotlib.__version__) < LooseVersion("1.2.0"):
                ok2run = 'Wrong version of matplotlib. Required >= 1.2.0'
            matplotlib.use("agg")
            import matplotlib.pyplot as plt
        except:
            ok2run = 'Problem importing matplotlib'
        if ok2run == '':
            simpleapi.SavePlot1D("group", self.plotfile)
            self.assertGreater(os.path.getsize(self.plotfile), 1e4)
        self.cleanup()


if __name__ == "__main__":
    unittest.main()
