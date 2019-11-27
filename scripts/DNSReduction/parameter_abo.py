# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Reduction GUI parameter abo
"""
from __future__ import (absolute_import, division, print_function)

from DNSReduction.main_model import DNSReductionGUI_model


class ParameterAbo(object):
    """
    Sharing of common parameter dictionary between DNSObservers


    observers are the presenters of the tabs of the gui or xml_dumper
    they are instances of DNSObserver and have a
    argument name which is a unique string
    and a method update which can get information from the gui parameters
    they are passive in relation to this presenter, just react on .get_
    option_dict() calls
    they can have their own view to which only they interact
    """
    def __init__(self):
        self.observers = []
        self.gui_parameter = DNSReductionGUI_model()  ## Ordered Dictionary

    def register(self, observer):
        if observer not in self.observers:  ## do not allow multiple
            ## registrations, should not happen anyhow
            self.observers.append(observer)
            observer.sig_request_from_abo.connect(self.process_request)
        self.update_from_observer(observer)

    def unregister(self, observer):
        observer.sig_request_from_abo.disconnect()
        self.observers.remove(observer)

    def clear(self):
        for observer in self.observers:
            observer.sig_request_from_abo.disconnect()
        self.observers = []

    def notify_observers(self):
        gui_param = self.gui_parameter.get()
        for observer in self.observers:
            observer.update(gui_param)

    def notify_focused_tab(self, observer):
        observer.tab_got_focus()

    def xml_load(self, gui_param):
        self.gui_parameter.set_whole_dict(gui_param)
        self.notify_observers()
        for observer in self.observers:
            observer.set_view_from_param()

    def update_from_observer(self, observer):
        self.gui_parameter.update(observer.name, observer.get_option_dict())
        self.notify_observers()

    def update_from_all_observers(self):
        """
        this functions gets data from all observers
        """
        for observer in self.observers:
            self.gui_parameter.update(observer.name,
                                      observer.get_option_dict())
        self.notify_observers()

    def process_request(self):
        for observer in self.observers:
            observer.process_request()
        self.update_from_all_observers()
