#pylint: disable=no-init,attribute-defined-outside-init
import stresstesting
from mantid.api import FileFinder
from mantid.simpleapi import *
from reduction_workflow.instruments.sans.hfir_command_interface import *

import os

def do_cleanup():
    Files = ["BioSANS_test_data_reduction.log",
    "BioSANS_test_data_Iq.xml",
    "BioSANS_test_data_Iq.txt",
    "BioSANS_test_data_Iqxy.dat"]
    for file in Files:
        absfile = FileFinder.getFullPath(file)
        if os.path.exists(absfile):
            os.remove(absfile)
    return True

class HFIREffAPIv2(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        """
            System test for sensitivity correction
        """
        config = ConfigService.Instance()
        config["facilityName"]='HFIR'
        GPSANS()
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SetTransmission(0.51944, 0.011078)
        SensitivityCorrection("BioSANS_flood_data.xml", dark_current="BioSANS_dark_current.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIREff.nxs'

class HFIRSensitivityDirectBeamCenter(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        """
            System test for sensitivity correction
        """
        config = ConfigService.Instance()
        config["facilityName"]='HFIR'
        GPSANS()
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SetTransmission(0.51944, 0.011078)
        SensitivityCorrection("BioSANS_flood_data.xml",
                              dark_current="BioSANS_dark_current.xml")
        SensitivityDirectBeamCenter("BioSANS_empty_trans.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRSensitivityDirectBeamCenter.nxs'

class HFIRSensitivityScatteringBeamCenter(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        """
            System test for sensitivity correction
        """
        config = ConfigService.Instance()
        config["facilityName"]='HFIR'
        GPSANS()
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SetTransmission(0.51944, 0.011078)
        SensitivityCorrection("BioSANS_flood_data.xml",
                              dark_current="BioSANS_dark_current.xml")
        SensitivityScatteringBeamCenter("BioSANS_test_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRSensitivityScatteringBeamCenter.nxs'



