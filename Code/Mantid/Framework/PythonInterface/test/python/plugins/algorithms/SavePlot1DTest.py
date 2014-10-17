import unittest,os
import mantid

class SavePlot1DTest(unittest.TestCase):
    def makeWs(self):
        mantid.simpleapi.CreateWorkspace(OutputWorkspace='test1',DataX='1,2,3,4,5,1,2,3,4,5',DataY='1,2,3,4,2,3,4,5',DataE='1,2,3,4,2,3,4,5',NSpec='2',UnitX='TOF',Distribution='1',YUnitlabel="S(q)")
        mantid.simpleapi.CreateWorkspace(OutputWorkspace='test2',DataX='1,2,3,4,5,1,2,3,4,5',DataY='1,2,3,4,2,3,4,5',DataE='1,2,3,4,2,3,4,5',NSpec='2',
                                         UnitX='Momentum',VerticalAxisUnit='TOF',VerticalAxisValues='1,2',Distribution='1',YUnitLabel='E',WorkspaceTitle='x')
        mantid.simpleapi.GroupWorkspaces("test1,test2",OutputWorkspace="group")
        self.plotfile=os.path.join(mantid.config.getString('defaultsave.directory'),'plot.png')

    def cleanup(self):
        mantid.api.AnalysisDataService.remove("group")
        mantid.api.AnalysisDataService.remove("test1")
        mantid.api.AnalysisDataService.remove("test2")
        if os.path.exists(self.plotfile):
            os.remove(self.plotfile)

    def testPlot(self):
        self.makeWs()
        ok2run=''
        try:
            import matplotlib
            from distutils.version import LooseVersion
            if LooseVersion(matplotlib.__version__)<LooseVersion("1.2.0"):
                ok2run='Wrong version of matplotlib. Required >= 1.2.0'
            matplotlib.use("agg")
            import matplotlib.pyplot as plt
        except:
            ok2run='Problem importing matplotlib'
        if ok2run=='':
            mantid.simpleapi.SavePlot1D("group",self.plotfile)
            self.assertGreater(os.path.getsize(self.plotfile),1e4)
        self.cleanup()

if __name__=="__main__":
    unittest.main()
