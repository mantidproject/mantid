# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from systemtesting import MantidSystemTest
from abc import abstractmethod, ABCMeta
from mantid.simpleapi import ReflectometryISISCreateTransmission, Load


class ReflectometryISISCreateTransmissionTestBase(MantidSystemTest, metaclass=ABCMeta):
    _OUTPUT_WS_NAME = "result"
    _NON_MAG_RUNS = ["POLREF00032130.nxs", "POLREF00032132.nxs"]
    _BACKGROUND_PROCESSING_INSTRUCTIONS = "520-640"
    _FLOOD_FILE = "POLREF_Flood_TOF_single_bin.nx5"

    def __init__(self):
        super(ReflectometryISISCreateTransmissionTestBase, self).__init__()

    @property
    @abstractmethod
    def perform_flood(self) -> bool:
        """
        Whether to perform a flood correction as part of creating the transmission workspace
        """
        pass

    @property
    @abstractmethod
    def perform_background_sub(self) -> bool:
        """
        Whether to perform a background subtraction as part of creating the transmission workspace
        """
        pass

    @property
    @abstractmethod
    def reference_file(self) -> str:
        """
        The name of the reference file for validating the test result
        """
        pass

    def requiredFiles(self):
        required_files = self._NON_MAG_RUNS.copy()

        if self.perform_flood:
            required_files.append(self._FLOOD_FILE)

        required_files.append(self.reference_file)

        return required_files

    def runTest(self):
        if self.perform_flood:
            flood_ws = Load(self._FLOOD_FILE)
        else:
            flood_ws = None

        background = self._BACKGROUND_PROCESSING_INSTRUCTIONS if self.perform_background_sub else None

        ReflectometryISISCreateTransmission(
            InputRuns=self._NON_MAG_RUNS,
            ProcessingInstructions="270-292",
            I0MonitorIndex="2",
            MonitorIntegrationWavelengthMin=2.5,
            MonitorIntegrationWavelengthMax=10.0,
            FloodWorkspace=flood_ws,
            BackgroundProcessingInstructions=background,
            OutputWorkspace=self._OUTPUT_WS_NAME,
        )

    def validate(self):
        self.tolerance = 1e-10

        return self._OUTPUT_WS_NAME, self.reference_file


class ReflectometryISISCreateTransmissionNoCorrectionsTest(ReflectometryISISCreateTransmissionTestBase):
    reference_file = "ReflectometryISISCreateTransmissionNoCorrections.nxs"
    perform_flood = False
    perform_background_sub = False

    def __init__(self):
        super(ReflectometryISISCreateTransmissionNoCorrectionsTest, self).__init__()


class ReflectometryISISCreateTransmissionBackgroundSubTest(ReflectometryISISCreateTransmissionTestBase):
    reference_file = "ReflectometryISISCreateTransmissionBackgroundSub.nxs"
    perform_flood = False
    perform_background_sub = True

    def __init__(self):
        super(ReflectometryISISCreateTransmissionBackgroundSubTest, self).__init__()


class ReflectometryISISCreateTransmissionFloodTest(ReflectometryISISCreateTransmissionTestBase):
    reference_file = "ReflectometryISISCreateTransmissionFloodCor.nxs"
    perform_flood = True
    perform_background_sub = False

    def __init__(self):
        super(ReflectometryISISCreateTransmissionFloodTest, self).__init__()


class ReflectometryISISCreateTransmissionAllCorrectionsTest(ReflectometryISISCreateTransmissionTestBase):
    reference_file = "ReflectometryISISCreateTransmissionAllCorrections.nxs"
    perform_flood = True
    perform_background_sub = True

    def __init__(self):
        super(ReflectometryISISCreateTransmissionAllCorrectionsTest, self).__init__()
