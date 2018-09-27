from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.muon_workspace import MuonWorkspace


class MuonPair(object):
    """Simple struct to store information on a detector group pair.

    The name is set at initialization and after that cannot be changed.
    The detector list can be modified by passing a list of ints (type checks for this)
    The number of detetors is stored
    """

    def __init__(self, pair_name, group1_name="", group2_name="", alpha=1.0):
        self._pair_name = pair_name
        self._group1_name = group1_name
        self._group2_name = group2_name
        self._alpha = float(alpha)

        self._workspace = None

    @property
    def workspace(self):
        return self._workspace

    @workspace.setter
    def workspace(self, new_workspace):
        if isinstance(new_workspace, MuonWorkspace):
            self._workspace = new_workspace
        else:
            raise AttributeError("Attempting to set workspace to type " + str(type(new_workspace)) +
                                 " but should be MuonWorkspace")

    @property
    def name(self):
        return self._pair_name

    @property
    def group1(self):
        return self._group1_name

    @property
    def group2(self):
        return self._group2_name

    @group1.setter
    def group1(self, new_name):
        self._group1_name = new_name

    @group2.setter
    def group2(self, new_name):
        self._group2_name = new_name

    @property
    def alpha(self):
        return float("{0:.3f}".format(round(self._alpha, 3)))

    @alpha.setter
    def alpha(self, new_alpha):
        if float(new_alpha) >= 0.0:
            self._alpha = float(new_alpha)
        else:
            raise AttributeError("Alpha must be > 0.0.")
