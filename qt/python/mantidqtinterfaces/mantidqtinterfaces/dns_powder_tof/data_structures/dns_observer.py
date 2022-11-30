# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS observer class. Observers share a common data model and are updated
by presenter objects.
"""

from collections import OrderedDict


class DNSObserver:
    # pylint: disable=too-many-instance-attributes
    """
    Common class definition for DNS Observers. All presenter classes are
    children of the DNSObserver class. Each presenter class has its own
    parameter dictionary and can have a view and a model. Presenters are
    the only ones who communicate with their view and model through their
    parent DNSWidget class. The latter provides the dependency injection
    of view and model to the presenter.
    """

    def __init__(self, parent=None, name=None, view=None, model=None):
        super().__init__()
        self.name = name
        self.parent = parent
        self.view = view
        self.model = model
        # the keys of the parameter dictionary will be filled with
        # strings, each representing a unique presenter class.
        # The values of the parameter dictionary will be filled with
        # parameters (and their values) of the respective presenter.
        # As a result param_dict will contain all info on the GUI's
        # state.
        self.param_dict = OrderedDict()
        # the keys of the own dictionary will be filled with
        # strings, each representing a unique parameter inside a
        # presenter. The values will represent the respective
        # values corresponding to these parameters.
        self.own_dict = OrderedDict()
        self.modus = ''
        self.request_from_abo = None

    def update(self, param_dict):
        """
        Updating the own dictionary from ParameterAbo.
        """
        if self.modus != self.parent.parent.modus.name:
            self.modus = self.parent.parent.modus.name
            self.on_modus_change()
        self.param_dict.update(param_dict)
        if self.param_dict[self.name] is not None:
            self.own_dict.update(self.param_dict[self.name])

    def set_view_from_param(self):
        """
        Sets the view from the own parameter dictionary.
        """
        self.view.set_state(self.own_dict) # own dict not from abo

    def get_option_dict(self):
        """
        Returns own options from view.
        """
        if self.view is not None:
            self.own_dict.update(self.view.get_state())
        return self.own_dict

    def raise_error(self, message, critical=False, info=False, do_raise=True):
        if do_raise:
            self.view.raise_error(message, critical, info)

    def process_request(self):
        """
        Main presenter can request data from DNSObservers.
        """

    def tab_got_focus(self):
        """
        Run if the tab of the associated view got the focus.
        """

    def on_modus_change(self):
        """
        Run when the modus of the gui changes.
        """
