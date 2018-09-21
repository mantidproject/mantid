from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.muon_workspace import MuonWorkspace
import six


class MuonGroup(object):
    """Simple structure to store information on a detector group.

    The name is set at initialization and after that cannot be changed.
    The detector list can be modified by passing a list of ints (type checks for this)
    The number of detectors is stored
    """

    def __init__(self, group_name, detector_ids=[]):
        self._group_name = group_name

        self._detector_IDs = None
        self.detectors = detector_ids

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
        return self._group_name

    @name.setter
    def name(self, _name):
        raise AttributeError("Cannot change name of MuonGroup object")

    @property
    def detectors(self):
        return self._detector_IDs

    @property
    def n_detectors(self):
        return len(self.detectors)

    @detectors.setter
    def detectors(self, detector_ids):
        if isinstance(detector_ids, six.string_types):
            raise AttributeError("MuonGroup : detectors must be a list of ints.")
        elif isinstance(detector_ids, list):
            if sum([not isinstance(item, int) for item in detector_ids]) == 0:
                self._detector_IDs = list(set(sorted(detector_ids)))
            else:
                raise AttributeError("MuonGroup : detectors must be a list of ints.")
        else:
            raise ValueError("detectors must be a list of ints.")
