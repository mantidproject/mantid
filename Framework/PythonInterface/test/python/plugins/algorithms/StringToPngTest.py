# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest, os
from mantid import AnalysisDataServiceImpl, config, simpleapi


class StringToPngTest(unittest.TestCase):
    plotfile = os.path.join(config.getString('defaultsave.directory'), "StringToPngTest.png")

    def cleanup(self):
        if os.path.exists(self.plotfile):
            os.remove(self.plotfile)

    def testPlot(self):
        to_plot = 'This is a string\nAnd this is a second line'
        ok2run = ''
        try:
            import matplotlib
            from distutils.version import LooseVersion
            if LooseVersion(matplotlib.__version__) < LooseVersion("1.2.0"):
                ok2run = 'Wrong version of matplotlib. Required >= 1.2.0'
            else:
                matplotlib.use("agg")
                import matplotlib.pyplot as plt
        except:
            ok2run = 'Problem importing matplotlib'
        if ok2run == '':
            simpleapi.StringToPng(String=to_plot, OutputFilename=self.plotfile)
            self.assertGreater(os.path.getsize(self.plotfile), 1e3)
        self.cleanup()


if __name__ == "__main__":
    unittest.main()
