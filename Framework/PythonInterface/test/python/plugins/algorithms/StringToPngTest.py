from __future__ import (absolute_import, division, print_function)

import unittest,os
import mantid

class StringToPngTest(unittest.TestCase):

    plotfile=os.path.join(mantid.config.getString('defaultsave.directory'),"StringToPngTest.png")

    def cleanup(self):
        if os.path.exists(self.plotfile):
            os.remove(self.plotfile)

    def testPlot(self):
        to_plot='This is a string\nAnd this is a second line'
        ok2run=''
        try:
            import matplotlib
            from distutils.version import LooseVersion
            if LooseVersion(matplotlib.__version__)<LooseVersion("1.2.0"):
                ok2run='Wrong version of matplotlib. Required >= 1.2.0'
            else:
                matplotlib.use("agg")
                import matplotlib.pyplot as plt
        except:
            ok2run='Problem importing matplotlib'
        if ok2run=='':
            mantid.simpleapi.StringToPng(String=to_plot,OutputFilename=self.plotfile)
            self.assertTrue(os.path.getsize(self.plotfile)>1e3)
        self.cleanup()

if __name__=="__main__":
    unittest.main()
