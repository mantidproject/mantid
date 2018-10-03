# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,attribute-defined-outside-init
import stresstesting
from mantid.api import FileFinder
from mantid.simpleapi import *
from reduction_workflow.instruments.sans.hfir_command_interface import *
from reduction_workflow.command_interface import AppendDataFile, Reduce1D

import os


def do_cleanup():
    Files = ["BioSANS_test_data_reduction.log",
             "BioSANS_test_data_Iq.xml",
             "BioSANS_test_data_Iq.txt",
             "BioSANS_test_data_Iqxy.dat"]
    for filename in Files:
        absfile = FileFinder.getFullPath(filename)
        if os.path.exists(absfile):
            os.remove(absfile)
    return True


class HFIRTrans1(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"]='HFIR'
        GPSANS()
        SetSampleDetectorDistance(6000)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        TimeNormalization()
        DirectBeamTransmission(sample_file="BioSANS_sample_trans.xml",
                               empty_file="BioSANS_empty_trans.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11")
        AppendDataFile("BioSANS_test_data.xml")
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRTrans.nxs'


class HFIRTrans2(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"]='HFIR'
        GPSANS()
        SetSampleDetectorDistance(6000)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        TimeNormalization()
        SetTransmission(0.522296, 0.009134)
        AzimuthalAverage(binning="0.01,0.001,0.11")
        AppendDataFile("BioSANS_test_data.xml")
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRTrans.nxs'


class HFIRTransmissionDarkCurrent(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"]='HFIR'
        GPSANS()
        SetSampleDetectorDistance(6000)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        TimeNormalization()
        DirectBeamTransmission(sample_file="BioSANS_sample_trans.xml",
                               empty_file="BioSANS_empty_trans.xml")
        TransmissionDarkCurrent("BioSANS_dark_current.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11")
        AppendDataFile("BioSANS_test_data.xml")
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRTransmissionDarkCurrent.nxs'


class HFIRTransmissionDirectBeamCenter(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"]='HFIR'
        GPSANS()
        SetSampleDetectorDistance(6000)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        TimeNormalization()
        DirectBeamTransmission(sample_file="BioSANS_sample_trans.xml",
                               empty_file="BioSANS_empty_trans.xml")
        TransmissionDirectBeamCenter("BioSANS_empty_trans.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11")
        AppendDataFile("BioSANS_test_data.xml")
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRTransmissionDirectBeamCenter.nxs'


class HFIRTransmissionBeamCenter(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"]='HFIR'
        GPSANS()
        SetSampleDetectorDistance(6000)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        TimeNormalization()
        DirectBeamTransmission(sample_file="BioSANS_sample_trans.xml",
                               empty_file="BioSANS_empty_trans.xml")
        SetTransmissionBeamCenter(16.389123399465063,
                                  95.530251864359087)
        AzimuthalAverage(binning="0.01,0.001,0.11")
        AppendDataFile("BioSANS_test_data.xml")
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRTransmissionDirectBeamCenter.nxs'


class HFIRTransmissionBeamSpreader(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"]='HFIR'
        GPSANS()
        SetSampleDetectorDistance(6000)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        TimeNormalization()
        BeamSpreaderTransmission(sample_spreader="BioSANS_test_data.xml",
                                 direct_spreader="BioSANS_empty_cell.xml",
                                 sample_scattering="BioSANS_test_data.xml",
                                 direct_scattering="BioSANS_empty_cell.xml",
                                 spreader_transmission=0.5,
                                 spreader_transmission_err=0.1)
        AzimuthalAverage(binning="0.01,0.001,0.11")
        AppendDataFile("BioSANS_test_data.xml")
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRTransmissionBeamSpreader.nxs'


class HFIRTransmissionBeamSpreaderDC(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"]='HFIR'
        GPSANS()
        SetSampleDetectorDistance(6000)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        TimeNormalization()
        BeamSpreaderTransmission(sample_spreader="BioSANS_test_data.xml",
                                 direct_spreader="BioSANS_empty_cell.xml",
                                 sample_scattering="BioSANS_test_data.xml",
                                 direct_scattering="BioSANS_empty_cell.xml",
                                 spreader_transmission=0.5,
                                 spreader_transmission_err=0.1)
        TransmissionDarkCurrent("BioSANS_dark_current.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11")
        AppendDataFile("BioSANS_test_data.xml")
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRTransmissionBeamSpreaderDC.nxs'


class HFIRTransmissionBeamSpreaderDBC(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"]='HFIR'
        GPSANS()
        SetSampleDetectorDistance(6000)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        TimeNormalization()
        BeamSpreaderTransmission(sample_spreader="BioSANS_test_data.xml",
                                 direct_spreader="BioSANS_empty_cell.xml",
                                 sample_scattering="BioSANS_test_data.xml",
                                 direct_scattering="BioSANS_empty_cell.xml",
                                 spreader_transmission=0.5,
                                 spreader_transmission_err=0.1)
        TransmissionDirectBeamCenter("BioSANS_empty_trans.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11")
        AppendDataFile("BioSANS_test_data.xml")
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRTransmissionBeamSpreaderDBC.nxs'


class HFIRTransmissionBeamSpreaderBC(stresstesting.MantidStressTest):

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"]='HFIR'
        GPSANS()
        SetSampleDetectorDistance(6000)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        TimeNormalization()
        BeamSpreaderTransmission(sample_spreader="BioSANS_test_data.xml",
                                 direct_spreader="BioSANS_empty_cell.xml",
                                 sample_scattering="BioSANS_test_data.xml",
                                 direct_scattering="BioSANS_empty_cell.xml",
                                 spreader_transmission=0.5,
                                 spreader_transmission_err=0.1)
        SetTransmissionBeamCenter(16.389123399465063,
                                  95.530251864359087)
        AzimuthalAverage(binning="0.01,0.001,0.11")
        AppendDataFile("BioSANS_test_data.xml")
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", 'HFIRTransmissionBeamSpreaderDBC.nxs'
