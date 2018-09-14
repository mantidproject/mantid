from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.load_utils import MuonWorkspace


class MuonPair:
    """Simple struct to store information on a detector group pair.

    The name is set at initialization and after that cannot be changed.
    The detector list can be modified by passing a list of ints (type checks for this)
    The number of detetors is stored
    """

    def __init__(self, pair_name="", group1_name="", group2_name="", alpha=1.0):
        self._pair_name = pair_name
        self._group1_name = group1_name
        self._group2_name = group2_name
        self._alpha = alpha

        self._workspace = None

    @property
    def workspace(self):
        return self._workspace

    @workspace.setter
    def workspace(self, new_workspace):
        if isinstance(new_workspace, MuonWorkspace):
            self._workspace = new_workspace

    @property
    def name(self):
        return self._pair_name

    @property
    def group1(self):
        return self._group1_name

    @property
    def group2(self):
        return self._group2_name

    @property
    def alpha(self):
        return self._alpha

    @alpha.setter
    def alpha(self, new_alpha):
        if new_alpha >= 0.0:
            self._alpha = new_alpha
        else:
            raise ValueError("Alpha must be > 0.0.")
