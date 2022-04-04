# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Observer Class, they share a common data model DNSReductionGui_model
and are updated by DNSReductionGui_presenter
"""

from collections import OrderedDict


class DNSObserver():
    # pylint: disable=too-many-instance-attributes
    """
    common class definition for DNS Observers,
    they have their own parameter dictionary and can have a view and a model,
    its parent is the widget,
    they are the only ones which communicate with their view and model
    which are DI to them by the widget
    """

    def __init__(self, parent=None, name=None, view=None, model=None):
        super().__init__()
        self.name = name
        self.parent = parent
        self.view = view
        self.model = model
        self.param_dict = OrderedDict()
        self.own_dict = OrderedDict()
        self.modus = ''
        self.request_from_abo = None

    def update(self, param_dict):
        """Updating the own dictionary from ParameterAbo"""
        if self.modus != self.parent.parent.modus.name:
            self.modus = self.parent.parent.modus.name
            self.on_modus_change()
        self.param_dict.update(param_dict)
        if self.param_dict[self.name] is not None:
            self.own_dict.update(self.param_dict[self.name])

    def set_view_from_param(self):
        """sets the view from the own parmeter dictionary"""
        self.view.set_state(self.param_dict.get(self.name, None))

    def get_option_dict(self):
        """Return own options from view"""
        if self.view is not None:
            self.own_dict.update(self.view.get_state())
        return self.own_dict

    def raise_error(self, message, critical=False, info=False, doraise=True):
        if doraise:
            self.view.raise_error(message, critical, info)

    def process_request(self):
        """Main presenter can request data from DNSObservers"""

    def tab_got_focus(self):
        """run if the tab of the associated view got the focus"""

    def on_modus_change(self):
        """run when the modus of the gui changes"""
