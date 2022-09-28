# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.muon_ADS_observer import MuonADSObserver
from mantidqt.utils.observer_pattern import GenericObservable


class ResultsContext:

    def __init__(self):
        self._result_table_names: list = []

        self.ADS_observer = MuonADSObserver(self.remove,
                                            self.clear,
                                            self.replaced)
        self.remove_observable = GenericObservable()
        self.clear_observable = GenericObservable()
        self.replace_observable = GenericObservable()

    def remove(self, workspace):
        self.remove_observable.notify_subscribers(workspace)

    def clear(self):
        self.clear_observable.notify_subscribers()

    def replaced(self, workspace):
        self.replace_observable.notify_subscribers(workspace)

    @property
    def result_table_names(self) -> list:
        """Returns the names of the results tables loaded into the model fitting tab."""
        return self._result_table_names

    @result_table_names.setter
    def result_table_names(self, table_names: list) -> None:
        """Sets the names of the results tables loaded into the model fitting tab."""
        self._result_table_names = table_names

    def add_result_table(self, table_name: str) -> None:
        """Add a results table to the stored list of results tables."""
        if table_name not in self._result_table_names and check_if_workspace_exist(table_name):
            self._result_table_names.append(table_name)

    def remove_workspace_by_name(self, workspace_name: str) -> None:
        """Removes a results table after and ADS deletion event."""
        if workspace_name in self._result_table_names:
            self._result_table_names.remove(workspace_name)
