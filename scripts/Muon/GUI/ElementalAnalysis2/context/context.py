# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.ElementalAnalysis2.context.data_context import DataContext
from mantidqt.utils.observer_pattern import Observable


class ElementalAnalysisContext(object):

    def __init__(self, muon_group_context=None, muon_gui_context=None):
        self._window_title = "Elemental Analysis 2"
        self.data_context = DataContext()
        self._gui_context = muon_gui_context
        self._group_pair_context = muon_group_context

        self.update_view_from_model_notifier = Observable()

    @property
    def name(self):
        return self._window_title

    @property
    def gui_context(self):
        return self._gui_context

    @property
    def group_pair_context(self):
        return self._group_pair_context
