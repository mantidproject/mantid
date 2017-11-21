class RunSelection(object):
    def __init__(self, runs = None):
        self._runs = runs if runs is not None else []

    def add_run(self, run):
        self._runs.append(run)

    def remove_run(self, index):
        self._runs.pop(index)

    def clear_all_runs(self):
        del self._runs[:]

    def __iter__(self):
        return self._runs.__iter__()
