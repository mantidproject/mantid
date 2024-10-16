# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from reduction_workflow.instruments.sans.hfir_command_interface import (
    AzimuthalAverage,
    DirectBeamCenter,
    GPSANS,
    MonitorNormalization,
    SensitivityCorrection,
    SetAbsoluteScale,
    SetBeamCenter,
    SetDirectBeamAbsoluteScale,
    SetSampleDetectorDistance,
    SetTransmission,
    SolidAngle,
)
from reduction_workflow.command_interface import AppendDataFile, DataPath, Reduce

from mantid.kernel import ConfigService


class HFIRReductionAPIv2(systemtesting.MantidSystemTest):
    """
    Simple reduction example
    """

    def setUp(self):
        self._work_dir = self.temporary_directory()

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"] = "HFIR"
        GPSANS()
        DataPath(self._work_dir)
        SetSampleDetectorDistance(6000)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SetTransmission(0.51944, 0.011078)
        SensitivityCorrection("BioSANS_flood_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce()

    def validate(self):
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "BioSANS_test_data_Iq", "HFIRReduction.nxs"


class HFIRAbsoluteScalingReference(systemtesting.MantidSystemTest):
    """
    Test absolute scaling using a reference data set
    """

    def setUp(self):
        self._work_dir = self.temporary_directory()

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"] = "HFIR"
        GPSANS()
        DataPath(self._work_dir)
        SetSampleDetectorDistance(6000)
        SolidAngle(detector_tubes=True)
        MonitorNormalization()
        AzimuthalAverage(binning="0.01,0.001,0.2")
        SetBeamCenter(16.39, 95.53)
        SetDirectBeamAbsoluteScale("BioSANS_empty_trans.xml")
        AppendDataFile(["BioSANS_test_data.xml"])
        Reduce()

    def validate(self):
        self.tolerance = 0.2
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "BioSANS_test_data_Iq", "HFIRAbsoluteScalingReference.nxs"


class HFIRAbsoluteScalingValue(systemtesting.MantidSystemTest):
    """
    Test absolute scaling using a reference data set
    """

    def setUp(self):
        self._work_dir = self.temporary_directory()

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"] = "HFIR"
        GPSANS()
        DataPath(self._work_dir)
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
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "BioSANS_test_data_Iq", "HFIRAbsoluteScalingReference.nxs"
