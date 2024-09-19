# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from isis_sans_system_test import ISISSansSystemTest
from sans.command_interface.ISISCommandInterface import ZOOM, MaskFile, TransmissionSample, AssignSample, WavRangeReduction
from SANS.sans.common.enums import SANSInstrument
from systemtesting import MantidSystemTest


@ISISSansSystemTest(SANSInstrument.ZOOM)
class ZOOMTransmissionEventDataTest(MantidSystemTest):
    def runTest(self):
        """
        Loads data using raw event transmission data
        """
        ZOOM()
        MaskFile("USER_ZOOM_SANSteam_4m_SampleChanger_202A_12mm_Large_BEAMSTOP.txt")
        AssignSample("ZOOM9898")
        TransmissionSample("ZOOM9946", "ZOOM9946")
        WavRangeReduction()

    def validate(self):
        self.disableChecking.append("Instrument")
        return "9898_rear_1D_1.75_16.5", "ZOOM_trans_raw_event_ref.nxs"


@ISISSansSystemTest(SANSInstrument.ZOOM)
class ZOOMTransmissionAddedEventDataTest(MantidSystemTest):
    """
    Loads data using added event mode transmission data
    """

    def runTest(self):
        ZOOM()
        MaskFile("USER_ZOOM_SANSteam_4m_SampleChanger_202A_12mm_Large_BEAMSTOP.txt")
        AssignSample("ZOOM9898")
        TransmissionSample("ZOOM9947_event", "ZOOM9947_event")
        WavRangeReduction()

    def validate(self):
        self.disableChecking.append("Instrument")
        # Added data in both histogram and event mode should produce identical results
        return "9898_rear_1D_1.75_16.5", "ZOOM_trans_added_ref.nxs"


@ISISSansSystemTest(SANSInstrument.ZOOM)
class ZOOMTransmissionAddedHistoDataTest(MantidSystemTest):
    """
    Loads data using added histo mode transmission data
    """

    def runTest(self):
        ZOOM()
        MaskFile("USER_ZOOM_SANSteam_4m_SampleChanger_202A_12mm_Large_BEAMSTOP.txt")
        AssignSample("ZOOM9898")
        TransmissionSample("ZOOM9947_histo", "ZOOM9947_histo")
        WavRangeReduction()

    def validate(self):
        self.disableChecking.append("Instrument")
        # Added data in both histogram and event mode should produce identical results
        return "9898_rear_1D_1.75_16.5", "ZOOM_trans_added_ref.nxs"
