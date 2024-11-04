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
    SensitivityCorrection,
    SensitivityDirectBeamCenter,
    SensitivityScatteringBeamCenter,
    SetSampleDetectorDistance,
    SetTransmission,
)
from reduction_workflow.command_interface import AppendDataFile, DataPath, Reduce1D

from mantid.kernel import ConfigService


class HFIREffAPIv2(systemtesting.MantidSystemTest):
    def setUp(self):
        self._work_dir = self.temporary_directory()

    def runTest(self):
        """
        System test for sensitivity correction
        """
        configI = ConfigService.Instance()
        configI["facilityName"] = "HFIR"
        GPSANS()
        DataPath(self._work_dir)
        SetSampleDetectorDistance(6000)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SetTransmission(0.51944, 0.011078)
        SensitivityCorrection("BioSANS_flood_data.xml", dark_current="BioSANS_dark_current.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "BioSANS_test_data_Iq", "HFIREff.nxs"


class HFIRSensitivityDirectBeamCenter(systemtesting.MantidSystemTest):
    def setUp(self):
        self._work_dir = self.temporary_directory()

    def runTest(self):
        """
        System test for sensitivity correction
        """
        configI = ConfigService.Instance()
        configI["facilityName"] = "HFIR"
        GPSANS()
        DataPath(self._work_dir)
        SetSampleDetectorDistance(6000)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SetTransmission(0.51944, 0.011078)
        SensitivityCorrection("BioSANS_flood_data.xml", dark_current="BioSANS_dark_current.xml")
        SensitivityDirectBeamCenter("BioSANS_empty_trans.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "BioSANS_test_data_Iq", "HFIRSensitivityDirectBeamCenter.nxs"


class HFIRSensitivityScatteringBeamCenter(systemtesting.MantidSystemTest):
    def setUp(self):
        self._work_dir = self.temporary_directory()

    def runTest(self):
        """
        System test for sensitivity correction
        """
        configI = ConfigService.Instance()
        configI["facilityName"] = "HFIR"
        GPSANS()
        DataPath(self._work_dir)
        SetSampleDetectorDistance(6000)
        DirectBeamCenter("BioSANS_empty_cell.xml")
        AppendDataFile("BioSANS_test_data.xml")
        SetTransmission(0.51944, 0.011078)
        SensitivityCorrection("BioSANS_flood_data.xml", dark_current="BioSANS_dark_current.xml")
        SensitivityScatteringBeamCenter("BioSANS_test_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "BioSANS_test_data_Iq", "HFIRSensitivityScatteringBeamCenter.nxs"
