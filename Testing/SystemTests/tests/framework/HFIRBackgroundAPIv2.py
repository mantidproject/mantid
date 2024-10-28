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
    Background,
    BckBeamSpreaderTransmission,
    BckDirectBeamTransmission,
    BckTransmissionDarkCurrent,
    GPSANS,
    SetBeamCenter,
    SetBckTransmission,
    SetSampleDetectorDistance,
)
from reduction_workflow.command_interface import AppendDataFile, DataPath, Reduce1D
from mantid.kernel import ConfigService


class HFIRBackground(systemtesting.MantidSystemTest):
    def setUp(self):
        self._work_dir = self.temporary_directory()

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"] = "HFIR"
        GPSANS()
        DataPath(self._work_dir)
        SetSampleDetectorDistance(6000)
        SetBeamCenter(16, 95)
        AppendDataFile("BioSANS_test_data.xml")
        Background("BioSANS_test_data.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "BioSANS_test_data_Iq", "HFIRBackground.nxs"


class HFIRBackgroundTransmission(systemtesting.MantidSystemTest):
    def setUp(self):
        self._work_dir = self.temporary_directory()

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"] = "HFIR"
        GPSANS()
        DataPath(self._work_dir)
        SetSampleDetectorDistance(6000)
        AppendDataFile("BioSANS_test_data.xml")
        Background("BioSANS_test_data.xml")
        SetBckTransmission(0.55, 0.1)
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "BioSANS_test_data_Iq", "HFIRBackgroundTransmission.nxs"


class HFIRBackgroundDirectBeamTrans(systemtesting.MantidSystemTest):
    def setUp(self):
        self._work_dir = self.temporary_directory()

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"] = "HFIR"
        GPSANS()
        DataPath(self._work_dir)
        SetSampleDetectorDistance(6000)
        AppendDataFile("BioSANS_test_data.xml")
        Background("BioSANS_test_data.xml")
        BckDirectBeamTransmission(sample_file="BioSANS_sample_trans.xml", empty_file="BioSANS_empty_trans.xml", beam_radius=10.0)
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "BioSANS_test_data_Iq", "HFIRBackgroundDirectBeamTrans.nxs"


class HFIRBackgroundBeamSpreaderTrans(systemtesting.MantidSystemTest):
    def setUp(self):
        self._work_dir = self.temporary_directory()

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"] = "HFIR"
        GPSANS()
        DataPath(self._work_dir)
        SetSampleDetectorDistance(6000)
        AppendDataFile("BioSANS_test_data.xml")
        Background("BioSANS_test_data.xml")
        BckBeamSpreaderTransmission(
            sample_spreader="BioSANS_test_data.xml",
            direct_spreader="BioSANS_empty_cell.xml",
            sample_scattering="BioSANS_test_data.xml",
            direct_scattering="BioSANS_empty_cell.xml",
            spreader_transmission=0.5,
            spreader_transmission_err=0.1,
        )
        AzimuthalAverage(binning="0.01,0.001,0.11")
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "BioSANS_test_data_Iq", "HFIRBackgroundBeamSpreaderTrans.nxs"


class HFIRBackgroundTransDarkCurrent(systemtesting.MantidSystemTest):
    def setUp(self):
        self._work_dir = self.temporary_directory()

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"] = "HFIR"
        GPSANS()
        DataPath(self._work_dir)
        SetSampleDetectorDistance(6000)
        AppendDataFile("BioSANS_test_data.xml")
        Background("BioSANS_test_data.xml")
        BckDirectBeamTransmission(sample_file="BioSANS_sample_trans.xml", empty_file="BioSANS_empty_trans.xml", beam_radius=10.0)
        BckTransmissionDarkCurrent("BioSANS_dark_current.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "BioSANS_test_data_Iq", "HFIRBackgroundTransDarkCurrent.nxs"


class HFIRBackgroundDirectBeamTransDC(systemtesting.MantidSystemTest):
    def setUp(self):
        self._work_dir = self.temporary_directory()

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"] = "HFIR"
        GPSANS()
        DataPath(self._work_dir)
        SetSampleDetectorDistance(6000)
        AppendDataFile("BioSANS_test_data.xml")
        Background("BioSANS_test_data.xml")
        BckDirectBeamTransmission(sample_file="BioSANS_sample_trans.xml", empty_file="BioSANS_empty_trans.xml", beam_radius=10.0)
        BckTransmissionDarkCurrent("BioSANS_dark_current.xml")
        AzimuthalAverage(binning="0.01,0.001,0.11", error_weighting=True)
        Reduce1D()

    def validate(self):
        self.tolerance = 0.00001
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "BioSANS_test_data_Iq", "HFIRBackgroundDirectBeamTransDC.nxs"
