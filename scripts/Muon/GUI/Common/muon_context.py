from __future__ import (absolute_import, division, print_function)

import Muon.GUI.Common.load_utils as load_utils

from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair

from collections import OrderedDict

class MuonContext(object):

    def __init__(self):
        self._groups = OrderedDict()
        self._pairs = OrderedDict()

        self._loaded_data = self.get_result()

        print(self._loaded_data)

    def get_result(self):
        #filename = "C:\Users\JUBT\Dropbox\Mantid-RAL\Testing\TrainingCourseData\multi_period_data\EMU00083015.nxs"
        filename = "C:\Users\JUBT\Dropbox\Mantid-RAL\Testing\TrainingCourseData\muon_cupper\EMU00020883.nxs"
        result, _run = load_utils.load_workspace_from_filename(filename)

        self._groups["fwd"] = MuonGroup()
        self._groups["bwd"] = MuonGroup()
        self._pairs["long"] = MuonPair()

        return result

    def get_result_2(self):
        #filename = "C:\Users\JUBT\Dropbox\Mantid-RAL\Testing\TrainingCourseData\multi_period_data\EMU00083015.nxs"
        filename = "C:\Users\JUBT\Dropbox\Mantid-RAL\Testing\TrainingCourseData\muon_cupper\EMU00020884.nxs"
        result, _run = load_utils.load_workspace_from_filename(filename)

        self._groups["fwd2"] = MuonGroup()
        self._groups["bwd2"] = MuonGroup()
        self._pairs["long2"] = MuonPair()

        return result

    @property
    def loaded_data(self):
        return self._loaded_data

    def clear(self):
        self._groups = OrderedDict()
        self._pairs = OrderedDict()
        self._loaded_data = self.get_result_2()