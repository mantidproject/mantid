# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from mantid.api import AnalysisDataService
from mantid.simpleapi import LoadElementalAnalysisData
import numpy as np


class LoadElementalAnalysisTest(systemtesting.MantidSystemTest):

    def __init__(self):
        super(LoadElementalAnalysisTest, self).__init__()

    def requiredFiles(self):
        return ['ral09999.rooth2010.dat', 'ral09999.rooth2020.dat', 'ral09999.rooth2099.dat',
                'ral09999.rooth3010.dat', 'ral09999.rooth3020.dat', 'ral09999.rooth3099.dat',
                'ral09999.rooth4010.dat', 'ral09999.rooth4020.dat', 'ral09999.rooth4099.dat',
                'ral09999.rooth5010.dat', 'ral09999.rooth5020.dat', 'ral09999.rooth5099.dat']

    def cleanup(self):
        AnalysisDataService.clear()

    def runTest(self):
        ws = LoadElementalAnalysisData(Run='9999',
                                       GroupWorkspace='9999')

        # Check the Groupworkspace contains 4 Workspaces
        self.assertEqual(AnalysisDataService.retrieve('9999').size(), 4)
        # Check the first workspace contains 3 spectra
        detector1_ws = AnalysisDataService.retrieve('9999; Detector 1')
        detectorSpectrumAxis = detector1_ws.getAxis(1)
        self.assertEqual(detectorSpectrumAxis.length(), 3)
        # Check that Bin 3318 of the Prompt spectra of the workspace of Detector 1 is 0.206791
        self.assertAlmostEqual(detector1_ws.readY(1)[3318], 0.206791, places=4)
        # Check that error in Bin 3318 of the Prompt spectra of the workspace of Detector 1 is 0.454742
        self.assertAlmostEqual(detector1_ws.readE(1)[3318], 0.454742, places=4)
        # Check that Bin 230 of the Total spectra of the workspace of Detector 2 is 243.729
        detector2_ws = AnalysisDataService.retrieve('9999; Detector 2')
        self.assertAlmostEqual(detector2_ws.readY(2)[230], 243.729, places=4)
        # Check that error in Bin 230 of the Total spectra of the workspace of Detector 2 is 15.6118
        self.assertAlmostEqual(detector2_ws.readE(2)[230], 15.6118, places=4)
        # Check that Bin 2915 of the Delayed spectra of the workspace of Detector 3 is 99.0456
        detector3_ws = AnalysisDataService.retrieve('9999; Detector 3')
        self.assertAlmostEqual(detector3_ws.readY(0)[2915], 99.0456, places=4)
        # Check that error in Bin 2915 of the Delayed spectra of the workspace of Detector 3 is 9.95216
        self.assertAlmostEqual(detector3_ws.readE(0)[2915], 9.95216, places=4)
        # Check that the Bin 4083 of the Total spectra of the workspace of Detecror 4 is 120.457
        detector4_ws = AnalysisDataService.retrieve('9999; Detector 4')
        self.assertAlmostEqual(detector4_ws.readY(2)[4083], 120.457, places=4)
        # Check that the error in Bin 4083 of the Total spectra of the workspace of Detecror 4 is 10.9753
        self.assertAlmostEqual(detector4_ws.readE(2)[4083], 10.9753, places=4)
        # Check that the directory output is not empty
        self.assertIsNotNone(ws[1])


class LoadPartialElementalAnalysisTest(systemtesting.MantidSystemTest):
    def __init__(self):
        super(LoadPartialElementalAnalysisTest, self).__init__()

    def requiredFiles(self):
        return ['ral02683.rooth2099.dat', 'ral02683.rooth4099.dat']

    def cleanup(self):
        AnalysisDataService.clear()

    def runTest(self):
        ws = LoadElementalAnalysisData(Run='2683',
                                       GroupWorkspace='2683')

        # Check the Groupworkspace contains 2 Workspaces
        self.assertEqual(AnalysisDataService.retrieve('2683').size(), 2)
        # Check the first workspace contains 3 spectra as it populates missing spectra with
        # value 0.0 in each bin
        detector1_ws = AnalysisDataService.retrieve('2683; Detector 1')
        detectorSpectrumAxis = detector1_ws.getAxis(1)
        self.assertEqual(detectorSpectrumAxis.length(), 3)
        # Check that the Prompt and Delay Spectra in Dectector 1 workspace do not contain any values
        # and therefore max and min should be 0
        detector1_prompt = detector1_ws.readY(0)
        detector1_delay = detector1_ws.readY(1)
        self.assertEqual(np.amax(detector1_prompt), 0.0)
        self.assertEqual(np.amax(detector1_delay), 0.0)
        self.assertEqual(np.amin(detector1_prompt), 0.0)
        self.assertEqual(np.amin(detector1_delay), 0.0)
        # Check that Bin 5237 of the Total spectra of the workspace of Detector 1 is 4.0
        self.assertAlmostEqual(detector1_ws.readY(2)[5237], 4.0, places=4)
        # Check that error in Bin 5237 of the Total spectra of the workspace of Detector 1 is 2.0
        self.assertAlmostEqual(detector1_ws.readE(2)[5237], 2.0, places=4)
        # Check that Bin 4697 of the Total spectra of the workspace of Detector 3 is 10.0
        detector3_ws = AnalysisDataService.retrieve('2683; Detector 3')
        self.assertAlmostEqual(detector3_ws.readY(2)[4697], 10.0, places=4)
        # Check that error in Bin 4697 of the Total spectra of the workspace of Detector 3 is 3.16227
        self.assertAlmostEqual(detector3_ws.readE(2)[4697], 3.16227, places=4)
        # Check that the directory output is not empty
        self.assertIsNotNone(ws[1])
