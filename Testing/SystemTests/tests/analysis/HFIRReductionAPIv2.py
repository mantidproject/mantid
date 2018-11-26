# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from mantid.api import FileFinder
from mantid.simpleapi import *
from reduction_workflow.instruments.sans.hfir_command_interface import *
from reduction_workflow.command_interface import AppendDataFile, Reduce

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


class HFIRReductionAPIv2(systemtesting.MantidSystemTest):
    """
        Simple reduction example
    """

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):

        configI = ConfigService.Instance()
        configI["facilityName"]='HFIR'
        GPSANS()
        SetSampleDetectorDistance(6000)
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


class HFIRAbsoluteScalingReference(systemtesting.MantidSystemTest):
    """
        Test absolute scaling using a reference data set
    """

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"]='HFIR'
        GPSANS()
        SetSampleDetectorDistance(6000)
        SolidAngle(detector_tubes=True)
        MonitorNormalization()
        AzimuthalAverage(binning="0.01,0.001,0.2")
        SetBeamCenter(16.39, 95.53)
        SetDirectBeamAbsoluteScale('BioSANS_empty_trans.xml')
        AppendDataFile(["BioSANS_test_data.xml"])
        Reduce()

    def validate(self):
        self.tolerance = 0.2
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", "HFIRAbsoluteScalingReference.nxs"


class HFIRAbsoluteScalingValue(systemtesting.MantidSystemTest):
    """
        Test absolute scaling using a reference data set
    """

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"]='HFIR'
        GPSANS()
        SetSampleDetectorDistance(6000)
        SolidAngle(detector_tubes=True)
        MonitorNormalization()
        AzimuthalAverage(binning="0.01,0.001,0.2")
        SetBeamCenter(16.39, 95.53)
        SetAbsoluteScale(1.680537663117948)
        AppendDataFile(["BioSANS_test_data.xml"])
        Reduce()

    def validate(self):
        self.tolerance = 0.2
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "BioSANS_test_data_Iq", "HFIRAbsoluteScalingReference.nxs"
