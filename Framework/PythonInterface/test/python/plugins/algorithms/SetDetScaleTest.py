# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import os
from mantid.simpleapi import LoadNexusProcessed, SetDetScale


class SetDetScaleTest(unittest.TestCase):
    @staticmethod
    def _detScales(workspace):
        component_info = workspace.componentInfo()
        return (
            component_info.getNumberParameter("detScale17")[0],
            component_info.getNumberParameter("detScale49")[0],
        )

    def testScaleDetectors(self):
        w = LoadNexusProcessed("TOPAZ_3007.peaks.nxs")
        x, y = self._detScales(w)
        self.assertEqual(x, 1.18898)
        self.assertEqual(y, 0.79420)
        SetDetScale(Workspace=w, DetScaleList="17:1.0,49:2.0")
        x, y = self._detScales(w)
        self.assertEqual(x, 1.0)
        self.assertEqual(y, 2.0)

        # create file for test
        filename = "testDetScale.txt"
        hklfile = open(filename, "w")
        hklfile.write("  17  0.5\n")
        hklfile.write("  49  1.5\n")
        hklfile.close()

        SetDetScale(Workspace=w, DetScaleFile=filename)
        x, y = self._detScales(w)
        self.assertEqual(x, 0.5)
        self.assertEqual(y, 1.5)

        # test both input
        SetDetScale(Workspace=w, DetScaleList="17:1.0,49:2.0", DetScaleFile=filename)
        x, y = self._detScales(w)
        self.assertEqual(x, 1.0)
        self.assertEqual(y, 2.0)
        os.remove(filename)


if __name__ == "__main__":
    unittest.main()
