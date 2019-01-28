from Muon.Gui.Common.muon_data_context import MuonDataContext


class MuonContext(object):
    def __init__(self):
        self._data_context = MuonDataContext()

    @property
    def data_context(self):
        return self._data_context

    @data_context.setter
    def data_context(self, value):
        self._data_context = value
