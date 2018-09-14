from __future__ import (absolute_import, division, print_function)

# import mantid.simpleapi as mantid
from Muon.GUI.Common.muon_context import MuonContext


class HomeTabModel(object):

    def __init__(self, muon_data=MuonContext()):
        self._data = muon_data
