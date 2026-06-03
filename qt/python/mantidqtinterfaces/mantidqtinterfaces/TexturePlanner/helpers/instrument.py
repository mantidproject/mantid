# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import os

from Engineering.EnggUtils import CALIB_DIR
from Engineering.common.instrument_config import get_instr_config, SUPPORTED_INSTRUMENTS


class InstrumentHelper:
    """Handles the instrument specific logic for the TexturePlanner interface"""

    # Per-instrument detector grouping presets. Add a new instrument by adding a row here.
    _SUPPORTED_GROUPS_BY_INSTRUMENT = {
        "ENGINX": ("Texture20", "Texture30", "banks"),
        "IMAT": ("Module1", "Module4", "Row1", "Row4", "banks"),
    }
    _DEFAULT_SUPPORTED_GROUPS = ("banks",)

    def __init__(self, model, instrument="ENGINX"):
        self._model = model
        # instrument config
        self.instr = instrument
        self.config = None
        self.group = None
        self.supported_groups = self._DEFAULT_SUPPORTED_GROUPS

    # instrument config -------------------------------------------------
    def update_instrument(self, instrument):
        self.instr = instrument
        self.config = get_instr_config(self.instr)
        self._model.workspaces.update_ws()
        self.update_supported_groups()

    def get_instrument(self):
        return self.instr

    @staticmethod
    def get_supported_instruments():
        return SUPPORTED_INSTRUMENTS

    def set_group(self, group_str):
        self.group = self.config.group(group_str)

    def update_supported_groups(self):
        self.supported_groups = self._SUPPORTED_GROUPS_BY_INSTRUMENT.get(self.instr, self._DEFAULT_SUPPORTED_GROUPS)

    def get_grouping_file(self):
        return self.config.grouping_files[self.group]

    def get_grouping_path(self):
        return os.path.join(CALIB_DIR, self.get_grouping_file())
