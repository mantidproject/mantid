# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantidqtinterfaces.Muon.GUI.Common.utilities.load_utils as load_utils


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
        self.params = []
        self.defaults = {"run": [0], "workspace": [], "filename": "", "instrument": ""}

    def __iter__(self):
        self._n = -1
        self._max = len(self.params)
        return self

    def __next__(self):
        if self._n < self._max - 1:
            self._n += 1
            return self.params[self._n]
        else:
            raise StopIteration

    # Getters

    def num_items(self):
        """Total number of entries"""
        return len(self.params)

    def get_parameter(self, param_name):
        """Get list of entries for a given parameter"""
        return [x.get(param_name) for x in self.params]

    # Adding/removing data

    def add_data(self, **kwargs):
        new_entry = {}
        for key, value in self.defaults.items():
            new_entry[key] = kwargs.get(key, self.defaults[key])
        self.params.append(new_entry)

    def remove_data(self, **kwargs):
        indices = [i for i, j in enumerate(self._matches(**kwargs)) if not j]
        self.params = [self.params[i] for i in indices]

    def clear(self):
        self.params = []

    def remove_nth_last_entry(self, n):
        """Remove the nth last entry given to the instance by add_data, n=1 refers to the most
        recently added."""
        keep_indices = [i for i in range(self.num_items()) if i != self.num_items() - n]
        self.params = [self.params[i] for i in keep_indices]

    def remove_current_data(self):
        """Remove the most recently added data item"""
        self.remove_nth_last_entry(1)

    def remove_last_added_data(self):
        """Remove the data item before the current one"""
        self.remove_nth_last_entry(2)

    # Searching

    def _matches(self, **kwargs):
        matches = []

        for entries in self.params:
            matching_parameters = [entries.get(key) == kwargs.get(key) for key in entries]
            if sum(matching_parameters) >= len(kwargs):
                matches.append(True)
            else:
                matches.append(False)

        return matches

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

    def get_data(self, **kwargs):
        if self.contains_n(**kwargs) == 1:
            indices = [i for i, val in enumerate(self._matches(**kwargs)) if val is True]
            index = indices[0]
            return self.params[index]

    def get_latest_data(self):
        if self.num_items() > 0:
            return self.params[-1]
        else:
            ret = self.defaults
            ret["workspace"] = load_utils.empty_loaded_data()
            return ret

    def get_main_field_direction(self, **kwargs):
        if self.get_data(**kwargs):
            return self.get_data(**kwargs)["workspace"]["MainFieldDirection"]
        else:
            return None

    def remove_workspace_by_name(self, workspace_name, instrument=""):
        list_of_workspace_names_to_remove = []
        for entry in self.params:
            if (
                any([workspace.workspace_name == workspace_name for workspace in entry["workspace"]["OutputWorkspace"]])
                or entry["workspace"]["DataDeadTimeTable"] == workspace_name
            ):
                list_of_workspace_names_to_remove.append(entry)

        runs_removed = []
        for entry in list_of_workspace_names_to_remove:
            if instrument == entry["instrument"]:
                runs_removed.append(entry["run"])
            self.remove_data(**entry)

        return runs_removed
