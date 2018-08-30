from __future__ import (absolute_import, division, print_function)


class MuonLoadData:
    """
    Lightweight 'struct' to store the results of loading from a load widget. Hard-code all required parameters for each
    entry in the __init__ method below (this way, extending for new parameters is easy).

    - Can be extended to add as many parameters as needed; all getting/setting is done through keyword arguments.
    - Can be used as an iterator, with elements being dictionaries of parameter:value pairs.
        see for example Iterator Types (https://docs.python.org/3/library/stdtypes.html)

    The keywords in all methods are "greedy" and will match any entries, with an OR like behaviour for
    multiple keywords. So for example "run=1234, filename ="file.nxs" would match to an entry with run=1234 OR
    filename = "file.nxs".

    - Clients responsibility to prevent duplicated entries.
    - Clients responsibility to ensure the entries are correct (i.e. no validation is performed).

    The instance of this class is intended to be shared between all models in the load widget (the parent, run and file
    widgets).
    """

    def __init__(self):
        """
        Hard code any parameters and their default values that are needed to be stored. The name given here
        can then be used as a keyword into any of the methods of the class. Use singular
        nouns.
        """
        self.params = {"run": [], "workspace": [], "filename": []}
        self.defaults = {"run": 0, "workspace": [], "filename": ""}
        if self.params.keys() != self.defaults.keys():
            raise AttributeError("params and defaults dictionaries do not have the same keys.")

    def __iter__(self):
        self._n = -1
        self._max = len(self.params["run"])
        return self

    def __next__(self):
        if self._n < self._max - 1:
            self._n += 1
            return {key: val[self._n] for key, val in self.params.items()}
        else:
            raise StopIteration

    # for Python 2/3 compatibility
    next = __next__

    # Getters

    def num_items(self):
        """Total number of entries"""
        return len(next(iter(self.params.values())))

    def get_parameter(self, param_name):
        """Get list of entries for a given parameter"""
        return self.params.get(param_name, None)

    # Adding/removing data

    def add_data(self, **kwargs):
        for key, value_list in self.params.items():
            # if keyword not supplied, use default defined in __init__
            value_list += [kwargs.get(key, self.defaults[key])]

    def remove_data(self, **kwargs):
        indices = [i for i, j in enumerate(self._matches(**kwargs)) if not j]
        for key, vals in self.params.items():
            self.params[key] = [vals[i] for i in indices]

    def clear(self):
        self.params = {key: [] for key, _ in self.params.items()}

    def remove_nth_last_entry(self, n):
        """Remove the nth last entry given to the instance by add_data, n=1 refers to the most
        recently added."""
        keep_indices = [i for i in range(self.num_items()) if i != self.num_items() - n]
        for key, vals in self.params.items():
            self.params[key] = [vals[i] for i in keep_indices]

    def remove_current_data(self):
        """Remove the most recently added data item"""
        self.remove_nth_last_entry(1)

    def remove_last_added_data(self):
        """Remove the data item before the current one"""
        self.remove_nth_last_entry(2)

    # Searching

    def _matches(self, **kwargs):
        # If kwarg given which is not in params default checks to True, as we want to
        # match to every data item in that case
        checks = [kwargs.get(key, True) for key in self.params.keys()]
        return [True if
                sum([data_value == search_value for (data_value, search_value) in zip(list(data_values), checks)]) > 0
                else False
                for data_values in
                zip(*self.params.values())]

    def contains_n(self, **kwargs):
        """Counts the number of matching entries where at least one of kwargs matches"""
        return sum(self._matches(**kwargs))

    def contains(self, **kwargs):
        """Does the data contain a match to at least one of the supplied keyword values"""
        n_matches = self.contains_n(**kwargs)
        if n_matches > 0:
            return True
        else:
            return False
