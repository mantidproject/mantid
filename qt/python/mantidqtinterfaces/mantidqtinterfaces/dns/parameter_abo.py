# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Reduction GUI parameter abo
"""
from collections import OrderedDict


class ParameterAbo:
    """
    Sharing of common parameter dictionary between DNSObservers

    observers are the presenters of the gui widgets
    they are instances of DNSObserver and have a argument name which is a
    unique string and a method update which can get information from the gui
    parameters
    they react on .get_option_dict() calls
    """

    def __init__(self):
        self.observers = []  # we keep it as list since, order is important
        self.gui_parameter = OrderedDict()  # Ordered Dictionary
        self.observer_dict = {}

    def get_gui_param(self):
        self.update_from_all_observers()
        return self.gui_parameter

    def project_save_load(self, gui_param):
        """loads the gui status from mantid workbench project save"""
        # this should replace xml loud at some point
        self.gui_parameter = gui_param
        self._notify_observers()
        for observer in self.observers:
            observer.set_view_from_param()

    def clear_gui_parameter_dict(self):
        self.gui_parameter.clear()

    def register(self, observer):
        """register a specific observer
        """
        if observer not in self.observers:  # do not allow multiple
            # registrations, should not happen anyhow
            self.observers.append(observer)
            self.observer_dict[observer.name] = observer
            # for automatic reduction observers can request data processing
            # from parameter abo, only common script generator presenter
            # uses this
            observer.request_from_abo = self.process_request
        self.update_from_observer(observer)

    def unregister(self, observer):
        """unregisters a specific observer
        """
        self.observers.remove(observer)
        self.observer_dict.pop(observer.name, False)

    def clear(self):
        """clear observers
        """
        self.observers = []
        self.observer_dict = {}

    def _notify_observers(self):
        """general notifcation of the observers, that parameters of
        other observers were changed
        """
        for observer in self.observers:
            observer.update(self.gui_parameter)

    def notify_modus_change(self):
        """some observers, are used in multiple reduction modes, but have
        different behaviour in differnt modes,
        this function tells them that the mode has changed
        """
        for observer in self.observers:
            observer.on_modus_change()

    @staticmethod
    def notify_focused_tab(observer):
        """notifies the tab which was selected by the user, that it got focus
        """
        observer.tab_got_focus()

    def xml_load(self):
        """loads the gui status from an xml file"""
        gui_param = self.observer_dict['xml_dump'].load_xml()
        if gui_param is not None:
            self.gui_parameter = gui_param
            self._notify_observers()
            for observer in self.observers:
                observer.set_view_from_param()

    def xml_save(self):
        """saves xml to previous saved filename or if not there asks for name
        """
        self.update_from_all_observers()
        self.observer_dict['xml_dump'].save_xml()

    def xml_save_as(self):
        """saves to an xml file by choosing a name"""
        self.update_from_all_observers()
        self.observer_dict['xml_dump'].save_as_xml()

    def update_from_observer(self, observer):
        """updates the gui_parameter dictionary from one specific observer"""
        self.gui_parameter[observer.name] = observer.get_option_dict()
        self._notify_observers()

    def update_from_all_observers(self):
        """
        gets data from all observers
        """
        for observer in self.observers:
            self.gui_parameter[observer.name] = observer.get_option_dict()

        self._notify_observers()

    def process_request(self):
        """observers can process requests from other observers, this is used
        for automatic data reduction
        """
        for observer in self.observers:
            observer.process_request()
        self.update_from_all_observers()

    def process_commandline_request(self, command_dict):
        """observers have a special function to process command line requests.
        a commandline command will be run through the observers, as they are
        ordered in the list in dns_modus
        """
        for observer in self.observers:
            observer.process_commandline_request(command_dict)
            self.update_from_observer(observer)
            self._notify_observers()
