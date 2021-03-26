# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class RunSelectionModel(object):
    def __init__(self, on_change, runs=None):
        self._on_change = on_change
        self._runs = runs if runs is not None else []
        self.on_change()

    def on_change(self):
        self._on_change(self)

    def has_any_runs(self):
        return len(self._runs) > 0

    def add_run(self, run):
        self._runs.append(run)
        self._on_change(self)

    def remove_run(self, index):
        self._runs.pop(index)
        self._on_change(self)

    def clear_all_runs(self):
        del self._runs[:]
        self._on_change(self)

    def __iter__(self):
        return self._runs.__iter__()


def has_any_event_data(selection):
    return any(run.is_event_data() for run in selection)
