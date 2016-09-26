import stresstesting
#------------------------------------------------------------------------------------

class PVPythonTest(stresstesting.MantidStressTest):

    def runTest(self):
        from paraview.simple import *
        self.assertEquals(str(GetParaViewVersion()),'5.1')

