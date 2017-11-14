from enum import Enum
from mantid.api import FileProperty, FileAction
from mantid.kernel import Direction

class BinningType(Enum):
    SaveAsEventData = 0
    Custom = 1
    FromMonitors = 2

class AddRunsModel(object):
    def __init__(self, runs = []):
        self.runs = runs

    def find_run_with_name(self, run_name):
        return run_name

    def add_run(self, run):
        error = self.error_message_if_invalid_run(run)
        if error:
            return error
        else:
            self.runs.append(run)

    def error_message_if_invalid_run(self, run):
        validator_property = \
            FileProperty("validator", run, FileAction.Load, [], Direction.Input)
        return validator_property.isValid

    def remove_run(self, index):
        self.runs.pop(index)

    def clear_all_runs(self):
        self.runs = []

    def __iter__(self):
        return self.runs.__iter__()

    def sumOf():
        pass
