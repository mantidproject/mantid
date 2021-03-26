# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from Interface.ui.drill.model.DrillSample import DrillSample


class DrillSampleTest(unittest.TestCase):
    def setUp(self):
        self.sample = DrillSample()

    def test_init(self):
        self.assertDictEqual(self.sample._parameters, {})

    def test_setParameters(self):
        params = {
            "SampleRuns": "7909+7925,7893,7877",
            "SampleTransmissionRuns": "7860",
            "AbsorberRuns": "7847+8209,7837+8187,7827+8165",
            "BeamRuns": "7846,7836,7826",
            "ContainerRuns": "8367,8349,8331",
            "TransmissionBeamRuns": "7815+7825+7872+7944-7953",
            "OutputWorkspace": "GM110",
            "MaskFiles": "maskBeamLowQ,maskBeamMiddleQ,maskBeamHighQ",
            "ContainerTransmissionRuns": "8313",
            "ReferenceFiles": "007845_Sample,007845_Sample,007835_Sample",
            "SampleThickness": "0.2"
        }
        self.sample.setParameters(params)
        self.assertDictEqual(self.sample._parameters, params)

    def test_getParameters(self):
        self.sample._parameters = {"p1": "v1", "p2": "v2"}
        self.assertDictEqual(self.sample.getParameters(), {"p1": "v1", "p2": "v2"})

    def test_changeParameter(self):
        self.sample._parameters = {"p1": "v1", "p2": "v2"}
        self.sample.changeParameter("p1", "v1'")
        self.assertDictEqual(self.sample._parameters, {"p1": "v1'", "p2": "v2"})
        self.sample.changeParameter("p2", "")
        self.assertDictEqual(self.sample._parameters, {"p1": "v1'"})


if __name__ == "__main__":
    unittest.main()
