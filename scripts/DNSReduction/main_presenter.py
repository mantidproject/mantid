# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Reduction GUI for DNS Instrument at MLZ
"""
from __future__ import (absolute_import, division, print_function)

from DNSReduction.dns_modus import DNSModus
from DNSReduction.parameter_abo import ParameterAbo


class DNSReductionGUI_presenter(object):
    """
    main gui presenter for dns, presenter is onwed by mainview
    """
    def __init__(self, view):
        self.view = view
        self.view.sig_tab_changed.connect(self.tab_changed)
        self.view.sig_save_as_triggered.connect(self.save_as)
        self.view.sig_open_triggered.connect(self.load_xml)
        self.view.sig_modus_change.connect(self.switch_mode)
        self.view.clear_subviews()
        self.modus = DNSModus('powder_elastic', parent=self)
        self.parameter_abo = ParameterAbo()
        for presenter in self.modus.presenters.values():
            self.view.add_subview(presenter.view)
            self.parameter_abo.register(presenter)
        self.switch_mode('powder_elastic')
        return

    def load_xml(self):
        """Loading of GUI status from XML file"""
        gui_param = self.modus.presenters['xml_dump'].load_xml()
        if gui_param is not None:
            self.parameter_abo.xml_load(gui_param)

    def save_as(self):
        """Saving of GUI status as XML file"""
        self.parameter_abo.update_from_all_observers()
        self.modus.presenters['xml_dump'].save_xml()

    def switch_mode(self, modus):
        """
        Switching between differnt data reduction modes
        elastic/TOF, powder/single crystal  and simulation
        """
        self.view.clear_subviews()
        self.parameter_abo.clear()
        self.modus.change(modus)
        for name, presenter in self.modus.presenters.items():
            self.parameter_abo.register(presenter)
            if presenter.view.has_tab:
                self.view.add_subview(presenter.view)

    def tab_changed(self, oldtabindex, tabindex):
        oldview = self.view.get_view_for_tabindex(oldtabindex)
        actualview = self.view.get_view_for_tabindex(tabindex)
        for observer in self.parameter_abo.observers:
            if observer.view == oldview:
                self.parameter_abo.update_from_observer(observer)
        for observer in self.parameter_abo.observers:
            if observer.view == actualview:
                self.parameter_abo.notify_focused_tab(observer)
