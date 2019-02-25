# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
"""
Contains the presenter for displaying the InstrumentWidget
"""
from __future__ import (absolute_import, unicode_literals)

from mantidqt.widgets.observers.ads_observer import WorkspaceDisplayADSObserver
from mantidqt.widgets.observers.observing_presenter import ObservingPresenter
from .view import InstrumentView


class InstrumentViewPresenter(ObservingPresenter):
    """
    Presenter holding the view widget for the InstrumentView.
    It has no model as its an old widget written in C++ with out MVP
    """

    def __init__(self, ws, parent=None, ads_observer=None):
        super(InstrumentViewPresenter, self).__init__()
        self.ws_name = ws.name()
        self.container = InstrumentView(parent, self, self.ws_name)

        if ads_observer:
            self.ads_observer = ads_observer
        else:
            self.ads_observer = WorkspaceDisplayADSObserver(self, observe_replace=False)

    def current_workspace_equals(self, name):
        return self.ws_name == name

    def replace_workspace(self, workspace_name, workspace):
        # replace is handled by the InstrumentWidget inside C++
        # this method is also unused, but is added to conform to the interface
        pass

    def rename_workspace(self, old_name, new_name):
        # rename is handled by the InstrumentWidget inside C++
        pass

    def show_view(self):
        self.container.show()
