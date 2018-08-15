class MuonLoadData:
    """Lightweight struct to store the results of loading from any of the load widgets.


    - Clients responsibility to prevent duplicates
    - No validation is performed so it's left to the client code to ensure the entries are
    correct.
    """

    def __init__(self):
        self._filenames = []
        self._workspaces = []
        self._runs = []

        self.params = {"run": self._runs, "workspace": self._workspaces, "filename": self._filenames}
        self.defaults = {"run": 0, "workspace": [], "filename": ""}

    def __iter__(self):
        self._n = -1
        self._max = len(self.params["run"])
        return self

    def next(self):
        if self._n < self._max - 1:
            self._n += 1
            return {key: val[self._n] for key, val in self.params.items()}
        else:
            raise StopIteration

    def remove_last_added_data(self):
        indices = [i for i in range(self.num_items()) if i != self.num_items() -2]
        for key, vals in self.params.items():
            self.params[key] = [vals[i] for i in indices]

    def num_items(self):
        return len(next(iter(self.params.values())))

    def get_parameter(self, param_name):
        return self.params.get(param_name, None)

    def add_data(self, **kwargs):
        for key, value_list in self.params.items():
            value_list += [kwargs.get(key, self.defaults[key])]

    def remove_data(self, **kwargs):
        indices = [i for i, j in enumerate(self._matches(**kwargs)) if not j]
        for key, vals in self.params.items():
            self.params[key] = [vals[i] for i in indices]

    def _matches(self, **kwargs):
        checks = [kwargs.get(key, True) for key in self.params.keys()]
        return [True if sum([x == y for (x, y) in zip(list(args), checks)]) > 0 else False for args in
                zip(*self.params.values())]

    def contains_n(self, **kwargs):
        return sum(self._matches(**kwargs))

    # TODO : unit test
    def contains(self, **kwargs):
        n_matches = self.contains_n(kwargs)
        if n_matches > 0:
            return True
        else:
            return False

    def clear(self):
        self.params = {key: [] for key, _ in self.params.items()}


if __name__ == "__main__":
    data = MuonLoadData()

    data.add_data(filename="file1.nxs", run=1234, workspace=[1])
    data.add_data(filename="file2.nxs", run=1235, workspace=[2])
    data.add_data(filename="file3.nxs", run=1236, workspace=[3])
    data.add_data(filename="file4.nxs", run=1237, workspace=[4])

    print(data.contains_n(run=1235))

    print(data.get_parameter("run"), data.get_parameter("workspace"), data.get_parameter("filename"))

    print(data.remove_data(run=1236))

    print(data.get_parameter("run"), data.get_parameter("workspace"), data.get_parameter("filename"))

    for data_item in iter(data):
        print(data_item)