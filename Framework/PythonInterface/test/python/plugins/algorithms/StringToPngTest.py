# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import os
from mantid import config, simpleapi


class StringToPngTest(unittest.TestCase):
    plotfile = os.path.join(config.getString("defaultsave.directory"), "StringToPngTest.png")

    def tearDown(self):
        if os.path.exists(self.plotfile):
            os.remove(self.plotfile)

    def testPlot(self):
        to_plot = "This is a string\nAnd this is a second line"
        simpleapi.StringToPng(String=to_plot, OutputFilename=self.plotfile)
        self.assertGreater(os.path.getsize(self.plotfile), 1e3)


if __name__ == "__main__":
    unittest.main()
