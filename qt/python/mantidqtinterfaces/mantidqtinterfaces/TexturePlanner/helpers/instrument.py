# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import os

from mantid.api import InstrumentFileFinder
from mantid.simpleapi import CreateSimulationWorkspace, GroupDetectors
from mantid.kernel import logger
from Engineering.EnggUtils import CALIB_DIR
from Engineering.common.instrument_config import get_instr_config, SUPPORTED_INSTRUMENTS
from typing import List, Sequence, Protocol
from abc import abstractmethod

# Group used when the user supplies their own grouping XML file rather than
# selecting one of an instrument's grouping presets. Also the label shown in the group combo.
CUSTOM_GROUP = "Custom"


class _WorkspaceManagerType(Protocol):
    """For the purpose of type hinting while this module is orphaned
    Will be removed and replaced with actual model before final PR"""

    @abstractmethod
    def update_ws(self) -> None:
        pass


class _BaseModelType(Protocol):
    """For the purpose of type hinting while this module is orphaned
    Will be removed and replaced with actual model before final PR"""

    workspaces: _WorkspaceManagerType


class InstrumentHelper:
    """Handles the instrument specific logic for the TexturePlanner interface"""

    # Per-instrument detector grouping presets. Add a new instrument by adding a row here.
    _SUPPORTED_GROUPS_BY_INSTRUMENT = {
        "ENGINX": ("Texture20", "Texture30", "banks"),
        "IMAT": ("Module1", "Module4", "Row1", "Row4", "banks"),
    }
    _DEFAULT_SUPPORTED_GROUPS = ("banks",)

    def __init__(self, model: _BaseModelType, instrument: str = "ENGINX"):
        self._model = model
        # instrument config
        self.instr = instrument
        self.config = None
        self.group = None
        self.supported_groups = self._DEFAULT_SUPPORTED_GROUPS
        # path to a user-supplied grouping XML file, used when self.group is CUSTOM_GROUP
        self.custom_grouping_file = None

    # instrument config -------------------------------------------------
    def update_instrument(self, instrument: str) -> None:
        if not self.is_valid_instrument(instrument):
            logger.error(f"Instrument Definition File for: '{instrument}' could not be found")
        self.instr = instrument
        # custom instruments are an arbitrary (validated) IDF name with no registered
        # config, so only look one up for the supported instruments.
        self.config = get_instr_config(self.instr) if self.instr in SUPPORTED_INSTRUMENTS else None
        self._model.workspaces.update_ws()
        self.update_supported_groups()

    def get_instrument(self) -> str:
        return self.instr

    @staticmethod
    def get_supported_instruments() -> List[str]:
        return SUPPORTED_INSTRUMENTS

    @staticmethod
    def is_valid_instrument(name: str) -> bool:
        """True if name resolves to a known instrument definition file (IDF)."""
        return bool(InstrumentFileFinder.getInstrumentFilename(name)) if name else False

    @staticmethod
    def is_grouping_file_applicable(instrument: str, grouping_path: str) -> bool:
        """Whether grouping_path can be applied to instrument.

        Checked on throwaway workspaces (StoreInADS=False, so the live state and the ADS are
        untouched) rather than with a bare try/except around the real recompute. A leading null
        group is tolerated to match DetectorGeometry.recompute, but any other detector-less group
        means the file references detectors this instrument does not have - i.e. it does not fit.
        """
        if not instrument or not grouping_path:
            return False
        try:
            ws = CreateSimulationWorkspace(Instrument=instrument, BinParams="0,1,2", StoreInADS=False)
            grouped = GroupDetectors(InputWorkspace=ws, MapFile=grouping_path, StoreInADS=False)
        except (RuntimeError, ValueError):
            return False
        spec_info = grouped.spectrumInfo()
        n_hist = grouped.getNumberHistograms()
        first = 0
        while first < n_hist and not spec_info.hasDetectors(first):
            first += 1
        return first < n_hist and all(spec_info.hasDetectors(i) for i in range(first, n_hist))

    def set_group(self, group_str: str) -> None:
        if group_str == CUSTOM_GROUP:
            self.group = CUSTOM_GROUP
        else:
            self.group = self.config.group(group_str)

    def set_custom_grouping_file(self, path: str) -> None:
        self.custom_grouping_file = path

    def update_supported_groups(self) -> None:
        self.supported_groups = self.groups_for_instrument(self.instr)

    def groups_for_instrument(self, instrument: str) -> Sequence[str]:
        """Grouping options offered for an instrument, without applying it to the model.

        Lets the view repopulate the group combo when the user merely selects a different
        instrument (the actual switch happens later, on the Update Instrument button).
        """
        if instrument in SUPPORTED_INSTRUMENTS:
            base = self._SUPPORTED_GROUPS_BY_INSTRUMENT.get(instrument, self._DEFAULT_SUPPORTED_GROUPS)
            return base + (CUSTOM_GROUP,)
        # custom instruments have no presets; their only grouping option is a user file
        return (CUSTOM_GROUP,)

    def get_grouping_file(self) -> str:
        return self.config.grouping_files[self.group]

    def get_grouping_path(self) -> str:
        if self.group == CUSTOM_GROUP:
            # user-supplied file is already an absolute path
            return self.custom_grouping_file
        return os.path.join(CALIB_DIR, self.get_grouping_file())
