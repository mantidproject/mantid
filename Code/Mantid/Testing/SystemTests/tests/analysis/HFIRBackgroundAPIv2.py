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

class HFIRBackground(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        config = ConfigService.Instance()
        config["facilityName"]='HFIR'
        GPSANS()
        SetBeamCenter(16, 95)
        AppendDataFile("BioSANS_test_data.xml")
        Background("BioSANS_test_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()
                
    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRBackground.nxs'

class HFIRBackgroundTransmission(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True
        
    def runTest(self):
        config = ConfigService.Instance()
        config["facilityName"]='HFIR'
        GPSANS()
        AppendDataFile("BioSANS_test_data.xml")
        Background("BioSANS_test_data.xml")
        SetBckTransmission(0.55, 0.1)
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()
                
    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRBackgroundTransmission.nxs'

class HFIRBackgroundDirectBeamTrans(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True
        
    def runTest(self):
        config = ConfigService.Instance()
        config["facilityName"]='HFIR'
        GPSANS()
        AppendDataFile("BioSANS_test_data.xml")
        Background("BioSANS_test_data.xml")
        BckDirectBeamTransmission(sample_file="BioSANS_sample_trans.xml",
                                  empty_file="BioSANS_empty_trans.xml",
                                  beam_radius=10.0)
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()
                
    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRBackgroundDirectBeamTrans.nxs'

class HFIRBackgroundBeamSpreaderTrans(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True
        
    def runTest(self):
        config = ConfigService.Instance()
        config["facilityName"]='HFIR'
        GPSANS()
        AppendDataFile("BioSANS_test_data.xml")
        Background("BioSANS_test_data.xml")
        BckBeamSpreaderTransmission(sample_spreader="BioSANS_test_data.xml", 
                                 direct_spreader="BioSANS_empty_cell.xml",
                                 sample_scattering="BioSANS_test_data.xml", 
                                 direct_scattering="BioSANS_empty_cell.xml",
                                 spreader_transmission=0.5, 
                                 spreader_transmission_err=0.1)
        AzimuthalAverage(binning="0.01,0.001,0.11")
        Reduce1D()
                
    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRBackgroundBeamSpreaderTrans.nxs'

class HFIRBackgroundTransDarkCurrent(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True
        
    def runTest(self):
        config = ConfigService.Instance()
        config["facilityName"]='HFIR'
        GPSANS()
        AppendDataFile("BioSANS_test_data.xml")
        Background("BioSANS_test_data.xml")
        BckDirectBeamTransmission(sample_file="BioSANS_sample_trans.xml",
                                  empty_file="BioSANS_empty_trans.xml",
                                  beam_radius=10.0)
        BckTransmissionDarkCurrent("BioSANS_dark_current.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRBackgroundTransDarkCurrent.nxs'
    
class HFIRBackgroundDirectBeamTransDC(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True
        
    def runTest(self):
        config = ConfigService.Instance()
        config["facilityName"]='HFIR'
        GPSANS()
        AppendDataFile("BioSANS_test_data.xml")
        Background("BioSANS_test_data.xml")
        BckDirectBeamTransmission(sample_file="BioSANS_sample_trans.xml",
                                  empty_file="BioSANS_empty_trans.xml",
                                  beam_radius=10.0)
        BckTransmissionDarkCurrent("BioSANS_dark_current.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()
                
    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRBackgroundDirectBeamTransDC.nxs'
    
