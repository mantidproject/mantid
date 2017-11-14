from enum import Enum
from mantid import FileFinder
import os

class BinningType(Enum):
    SaveAsEventData = 0
    Custom = 1
    FromMonitors = 2

class SummableRunModel(object):
    def __init__(self, path):
        self._path = path
        self._display_name = SummableRunModel._path_to_display_name(path)

    def file_path(self):
        return self._path

    def display_name(self):
        return self._display_name

    @staticmethod
    def _path_to_display_name(path):
        return os.path.splitext(os.path.basename(path))[0]


class AddRunsModel(object):
    def __init__(self, runs = []):
        self.runs = runs

    def create_run_from_path(self, path):
        return SummableRunModel(path)

    def find_all_from_query(self, query_string):
        try:
            results = FileFinder.findRuns(query_string.encode('utf-8'))
            return ('', [SummableRunModel(file_path) for file_path in results])
        except RuntimeError:
            return ('', [])
        except ValueError as ex:
            return (str(ex), [])

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
