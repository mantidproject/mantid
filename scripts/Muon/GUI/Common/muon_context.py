# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.muon_data_context import MuonDataContext
from Muon.GUI.Common.muon_gui_context import MuonGuiContext


class MuonContext(object):
    def __init__(self):
        self._data_context = MuonDataContext()
        self._gui_context = MuonGuiContext()

    @property
    def data_context(self):
        return self._data_context

    @data_context.setter
    def data_context(self, value):
        self._data_context = value

    @property
    def gui_context(self):
        return self._gui_context

    @gui_context.setter
    def gui_context(self, value):
        self._gui_context = value
