# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.py36compat import dataclass
from Muon.GUI.Common.muon_load_data import MuonLoadData

DEAD_TIME_FROM_FILE = "FromFile"
DEAD_TIME_FROM_ADS = "FromADS"


@dataclass
class CorrectionsContext:
    # We require this to be a string instead of a run number because of co-add mode. In co-add mode we correct the
    # summed data of several runs. Therefore, the 'current_run_string' can have a format similar to "84447-84449,84451"
    # when in co-add mode. When in normal mode, the 'current_run_string' is a single run like "84447".
    current_run_string: str = None

    # The 'dead_time_source' can be "FromFile", "FromADS" or None.
    dead_time_source: str = None
    dead_time_table_name_from_ads: str = None

    def __init__(self, load_data: MuonLoadData):
        """Initialize the CorrectionsContext and pass in MuonLoadData so that we can access dead time tables."""
        self._loaded_data = load_data

    def current_dead_time_table_name_for_run(self, instrument: str, run: list) -> str:
        """Returns the name of the dead time table for the provided run and the active dead time mode."""
        if self.dead_time_source == DEAD_TIME_FROM_FILE:
            return self.get_default_dead_time_table_name_for_run(instrument, run)
        elif self.dead_time_source == DEAD_TIME_FROM_ADS:
            return self.dead_time_table_name_from_ads
        else:
            return None

    def get_default_dead_time_table_name_for_run(self, instrument: str, run: list) -> str:
        """Returns the default loaded dead time table for a specific instrument run."""
        loaded_dict = self._loaded_data.get_data(instrument=instrument, run=run)
        if loaded_dict is not None and "workspace" in loaded_dict:
            return loaded_dict["workspace"]["DataDeadTimeTable"]
        else:
            return None

    def set_default_dead_time_table_name_for_run(self, instrument: str, run: list, dead_time_name: str) -> None:
        """Sets the default loaded dead time table for a specific instrument run."""
        self._loaded_data.get_data(instrument=instrument, run=run)['workspace']['DataDeadTimeTable'] = dead_time_name
