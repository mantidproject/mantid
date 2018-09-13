from __future__ import (absolute_import, division, print_function)

class MuonGroup:
    """Simple struct to store information on a detector group.

    The name is set at initialization and after that cannot be changed.
    The detector list can be modified by passing a list of ints (type checks for this)
    The number of detetors is stored
    """

    def __init__(self, group_name="", detector_IDs=[]):
        self._group_name = group_name

        self._detector_IDs = None
        self.detectors = detector_IDs

    @property
    def name(self):
        return self._group_name

    @property
    def detectors(self):
        return self._detector_IDs

    @property
    def n_detectors(self):
        return len(self.detectors)

    @detectors.setter
    def detectors(self, detector_IDs):
        if isinstance(detector_IDs, six.string_types):
            raise ValueError("detectors must be a list of ints.")
        elif isinstance(detector_IDs, list):
            if sum([not isinstance(item, int) for item in detector_IDs]) == 0:
                self._detector_IDs = detector_IDs
            else:
                raise ValueError("detectors must be a list of ints.")
        else:
            raise ValueError("detectors must be a list of ints.")