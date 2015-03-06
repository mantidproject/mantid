import stresstesting
import mantid
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

class HFIRReductionAPIv2(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    """
        Simple reduction example
    """
    
    def runTest(self):

        config = ConfigService.Instance()
        config["facilityName"]='HFIR'
        GPSANS()
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SetTransmission(0.51944, 0.011078)
        SensitivityCorrection("BioSANS_flood_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce()
        
    def validate(self):
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", "HFIRReduction.nxs"

class HFIRAbsoluteScalingReference(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    """
        Test absolute scaling using a reference data set
    """
    
    def runTest(self):
        config = ConfigService.Instance()
        config["facilityName"]='HFIR'
        GPSANS()
        SolidAngle(detector_tubes=True)
        MonitorNormalization()
        AzimuthalAverage(binning="0.01,0.001,0.2")
        SetBeamCenter(16.39, 95.53)
        SetDirectBeamAbsoluteScale('BioSANS_empty_trans.xml')
        AppendDataFile(["BioSANS_test_data.xml"])
        Reduce()
        
    def validate(self):
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", "HFIRAbsoluteScalingReference.nxs"

class HFIRAbsoluteScalingValue(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    """
        Test absolute scaling using a reference data set
    """
    
    def runTest(self):
        config = ConfigService.Instance()
        config["facilityName"]='HFIR'
        GPSANS()
        SolidAngle(detector_tubes=True)
        MonitorNormalization()
        AzimuthalAverage(binning="0.01,0.001,0.2")
        SetBeamCenter(16.39, 95.53)
        SetAbsoluteScale(1.680537663117948)
        AppendDataFile(["BioSANS_test_data.xml"])
        Reduce()
        
    def validate(self):
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", "HFIRAbsoluteScalingReference.nxs"

