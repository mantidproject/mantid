# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Observer Class, they share a common data model DNSReductionGui_model
and are updated by DNSReductionGui_presenter
"""
from __future__ import (absolute_import, division, print_function)
from collections import OrderedDict

from qtpy.QtCore import Signal, QObject

from mantidqt.gui_helper import get_qapplication


class DNSObserver(QObject):
    """
    common class definition for DNS Observers,
    they have their own parameter dictionary and can have a view
    """
    def __init__(self, parent, name):
        super(DNSObserver, self).__init__()
        self.name = name
        self.parent = parent
        self.view = None
        self.param_dict = OrderedDict()
        self.own_dict = OrderedDict()
        self.modus = ''
        self.app = get_qapplication()[0]

    sig_modus_changed = Signal()
    sig_request_from_abo = Signal()

    def update(self, param_dict):
        """Updating the own dictionary from ParameterAbo"""
        if self.modus != self.parent.modus.name:
            self.modus = self.parent.modus.name
            self.sig_modus_changed.emit()
        self.param_dict.update(param_dict)
        if self.param_dict[self.name] is not None:
            self.own_dict.update(self.param_dict[self.name])

    def set_view_from_param(self):
        self.view.set_state(self.param_dict[self.name])

    def get_option_dict(self):
        """Return own options from view"""
        if self.view is not None:
            self.own_dict.update(self.view.get_state())
        return self.own_dict

    def raise_error(self, message, critical=False, info=False):
        self.view.raise_error(message, critical, info)

    def process_request(self):
        """Main presenter can request data from DNSObservers"""
        pass

    def tab_got_focus(self):
        pass
