# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from systemtesting import MantidSystemTest
from abc import abstractmethod, ABCMeta
from mantid.simpleapi import ReflectometryISISCalculatePolEff, Load


class ReflectometryISISCalculatePolEffTestBase(MantidSystemTest, metaclass=ABCMeta):
    _OUTPUT_WS_NAME = "calc_pol_eff_out"
    _NON_MAG_RUNS = ["POLREF00032130.nxs", "POLREF00032132.nxs"]
    _MAG_RUNS = ["POLREF00032131.nxs", "POLREF00032133.nxs"]
    _BACKGROUND_PROCESSING_INSTRUCTIONS = "520-640"
    _PROCESSING_INSTRUCTIONS = "270-292"
    _FLOOD_FILE = "POLREF_Flood_TOF_single_bin.nx5"
    _FLOOD_WS = "flood_ws"
    _DEFAULT_ARGS = {
        "InputRuns": _NON_MAG_RUNS,
        "ProcessingInstructions": _PROCESSING_INSTRUCTIONS,
        "I0MonitorIndex": 2,
        "MonitorIntegrationWavelengthMin": 2.5,
        "MonitorIntegrationWavelengthMax": 10.0,
        "OutputWorkspace": _OUTPUT_WS_NAME,
    }
    _MAG_ARGS = {
        "MagInputRuns": _MAG_RUNS,
        "MagProcessingInstructions": _PROCESSING_INSTRUCTIONS,
    }
    _CORRECT_ARGS = {"FloodWorkspace": _FLOOD_WS, "BackgroundProcessingInstructions": _BACKGROUND_PROCESSING_INSTRUCTIONS}
    _MAG_CORRECT_ARGS = {"MagBackgroundProcessingInstructions": _BACKGROUND_PROCESSING_INSTRUCTIONS}

    def __init__(self):
        super(ReflectometryISISCalculatePolEffTestBase, self).__init__()

    @property
    @abstractmethod
    def all_corrections(self) -> bool:
        """
        Whether to perform a flood correction and background subtractions as part of creating the transmission workspaces
        """
        pass

    @property
    @abstractmethod
    def include_mag_runs(self) -> bool:
        """
        Whether to include create a transmission workspace for magnetic runs
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
        if self.include_mag_runs:
            required_files.extend(self._MAG_RUNS)
        if self.all_corrections:
            required_files.append(self._FLOOD_FILE)
        required_files.append(self.reference_file)
        return required_files

    def runTest(self):
        args = self._DEFAULT_ARGS.copy()
        correction_args = {}
        if self.include_mag_runs:
            args.update(self._MAG_ARGS)
            correction_args.update(self._MAG_CORRECT_ARGS)
        if self.all_corrections:
            Load(self._FLOOD_FILE, OutputWorkspace=self._FLOOD_WS)
            correction_args.update(self._CORRECT_ARGS)
            args.update(correction_args)

        ReflectometryISISCalculatePolEff(**args)

    def validate(self):
        self.tolerance = 1e-10
        return self._OUTPUT_WS_NAME, self.reference_file


class ReflectometryISISCalculatePolEffNonMagTest(ReflectometryISISCalculatePolEffTestBase):
    reference_file = "ReflectometryISISCalculatePolEffNonMag.nxs"
    all_corrections = False
    include_mag_runs = False

    def __init__(self):
        super(ReflectometryISISCalculatePolEffNonMagTest, self).__init__()


class ReflectometryISISCalculatePolEffMagTest(ReflectometryISISCalculatePolEffTestBase):
    reference_file = "ReflectometryISISCalculatePolEffMag.nxs"
    all_corrections = False
    include_mag_runs = True

    def __init__(self):
        super(ReflectometryISISCalculatePolEffMagTest, self).__init__()


class ReflectometryISISCalculatePolEffAllCorrectionsTest(ReflectometryISISCalculatePolEffTestBase):
    reference_file = "ReflectometryISISCalculatePolEffAllCorrections.nxs"
    all_corrections = True
    include_mag_runs = True

    def __init__(self):
        super(ReflectometryISISCalculatePolEffAllCorrectionsTest, self).__init__()
