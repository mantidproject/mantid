from enum import Enum

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
        self.runs.append(run)

    def remove_run(self, index):
        self.runs.pop(index)

    def clear_all_runs(self):
        self.runs = []

    def __iter__(self):
        return self.runs.__iter__()

    def sumOf():
        pass
