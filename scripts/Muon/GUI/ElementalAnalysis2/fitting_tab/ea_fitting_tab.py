# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.ElementalAnalysis2.fitting_tab.ea_fitting_tab_presenter import EAFittingTabPresenter
from Muon.GUI.ElementalAnalysis2.fitting_tab.ea_fitting_tab_view import EAFittingTabView
from Muon.GUI.ElementalAnalysis2.fitting_tab.ea_fitting_tab_model import EAFittingTabModel


class EAFittingTabWidget(object):
    """
    The FittingTabWidget creates the tab used for fitting in Elemental analysis widget.
    """

    def __init__(self, context, parent):
        self.fitting_tab_view = EAFittingTabView(parent)
        self.fitting_tab_view.set_start_and_end_x_labels("Energy (Kev) Start", "Energy (KeV) End")
        self.fitting_tab_model = EAFittingTabModel(context, context.fitting_context)
        self.fitting_tab_presenter = EAFittingTabPresenter(self.fitting_tab_view, self.fitting_tab_model)
