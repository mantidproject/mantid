class RunSelection(object):
    def __init__(self, runs = []):
        self._runs = runs

    def add_run(self, run):
        self._runs.append(run)

    def remove_run(self, index):
        self._runs.pop(index)

    def clear_all_runs(self):
        self._runs = []

    def __iter__(self):
        return self._runs.__iter__()
