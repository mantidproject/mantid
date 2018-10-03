# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import mantid
from mantid.simpleapi import *

class SANSBeamFluxCorrectionTest(unittest.TestCase):

    def setUp(self):

        self.test_ws_name = "EQSANS_test_ws"
        x = [1.,2.,3.,4.,5.,6.,7.,8.,9.,10.,11.]
        y = 491520*[0.1]
        CreateWorkspace(OutputWorkspace=self.test_ws_name,DataX=x,DataY=y,DataE=y,NSpec='49152',UnitX='Wavelength')
        LoadInstrument(self.test_ws_name, InstrumentName="EQSANS", RewriteSpectraMap=True)

        self.monitor = "EQSANS_test_monitor_ws"
        SumSpectra(InputWorkspace=self.test_ws_name, OutputWorkspace=self.monitor)

    def tearDown(self):
        if AnalysisDataService.doesExist(self.test_ws_name):
            AnalysisDataService.remove(self.test_ws_name)
        if AnalysisDataService.doesExist(self.monitor):
            AnalysisDataService.remove(self.monitor)

    def test_simple(self):
        output = SANSBeamFluxCorrection(InputWorkspace=self.test_ws_name,
                                        InputMonitorWorkspace=self.monitor,
                                        ReferenceFluxFilename="SANSBeamFluxCorrectionMonitor.nxs")

        ref_value = 0.1/(49152*0.1)/(49152*0.1)
        output_y = output[0].readY(0)
        self.assertAlmostEqual(ref_value, output_y[0], 6)

    def test_in_place(self):
        output = SANSBeamFluxCorrection(InputWorkspace=self.test_ws_name,
                                        InputMonitorWorkspace=self.monitor,
                                        ReferenceFluxFilename="SANSBeamFluxCorrectionMonitor.nxs",
                                        OutputWorkspace=self.test_ws_name)

        ref_value = 0.1/(49152*0.1)/(49152*0.1)
        output_y = output[0].readY(0)
        self.assertAlmostEqual(ref_value, output_y[0], 6)

if __name__ == '__main__':
    unittest.main()