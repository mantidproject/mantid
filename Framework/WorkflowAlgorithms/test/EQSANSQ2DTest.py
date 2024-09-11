# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import *


class EQSANSQ2DTest(unittest.TestCase):
    def setUp(self):
        self.test_ws_name = "EQSANS_test_ws"
        x = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0]
        y = 491520 * [0.1]
        CreateWorkspace(OutputWorkspace=self.test_ws_name, DataX=x, DataY=y, DataE=y, NSpec="49152", UnitX="Wavelength")
        LoadInstrument(Workspace=self.test_ws_name, InstrumentName="EQSANS", RewriteSpectraMap=True)

        run = mtd[self.test_ws_name].mutableRun()

        run.addProperty("sample_detector_distance", 4000.0, "mm", True)
        run.addProperty("beam_center_x", 96.0, "pixel", True)
        run.addProperty("beam_center_y", 128.0, "pixel", True)
        run.addProperty("wavelength_min", 1.0, "Angstrom", True)
        run.addProperty("wavelength_max", 11.0, "Angstrom", True)
        run.addProperty("is_frame_skipping", 0, True)
        run.addProperty("wavelength_min_frame2", 5.0, "Angstrom", True)
        run.addProperty("wavelength_max_frame2", 10.0, "Angstrom", True)

    def tearDown(self):
        if self.test_ws_name in mtd:
            DeleteWorkspace(Workspace=self.test_ws_name)

    def test_q2d(self):
        EQSANSQ2D(InputWorkspace=self.test_ws_name)
        ReplaceSpecialValues(
            InputWorkspace=self.test_ws_name + "_Iqxy", OutputWorkspace=self.test_ws_name + "_Iqxy", NaNValue=0, NaNError=0
        )
        Integration(InputWorkspace=self.test_ws_name + "_Iqxy", OutputWorkspace="__tmp")
        SumSpectra(InputWorkspace="__tmp", OutputWorkspace="summed")
        self.assertAlmostEqual(mtd["summed"].readY(0)[0], 7.24077, 6)
        for ws in ["__tmp", "summed"]:
            if mtd.doesExist(ws):
                DeleteWorkspace(Workspace=ws)


if __name__ == "__main__":
    unittest.main()
