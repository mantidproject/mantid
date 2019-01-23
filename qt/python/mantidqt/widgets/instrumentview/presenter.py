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

# local imports
from mantidqt.widgets.common.observing_presenter import ObservingPresenter
from mantidqt.widgets.common.workspacedisplay_ads_observer import WorkspaceDisplayADSObserver
from .view import InstrumentView


class InstrumentViewPresenter(ObservingPresenter):
    """
    Presenter holding the view widget for the InstrumentView.
    It has no model as its an old widget written in C++ with out MVP
    """

    view = None

    def __init__(self, ws, parent=None, ads_observer=None):
        self.ws_name = ws.name()
        self.view = InstrumentView(self, self.ws_name, parent)

        if ads_observer:
            self.ads_observer = ads_observer
        else:
            self.ads_observer = WorkspaceDisplayADSObserver(self, observe_replace=False)

    def close(self, workspace_name):
        """
        This closes the external window of the Instrument view.

        The C++ InstrumentWidget handles all events to the workspace itself,
        if the workspace is deleted then the widget closes itself.

        The InstrumentWidget is also wrapped in a QWidget made from Python,
        and that needs to be closed from here, otherwise we are left with an empty window,
        when the InstrumentWidget closes itself on workspace deletion.

        :param workspace_name: Used to check if it is the current workspace of the instrument view.
                               If it is - then close the instrument view,
                               but if it isn't - it does nothing
        """
        if self.ws_name == workspace_name:
            # if the observer is not cleared here then the C++ object is never freed,
            # and observers keep getting created, and triggering on ADS events
            self.ads_observer = None
            self.view.close_later()

    def replace_workspace(self, workspace_name, workspace):
        # replace is handled by the InstrumentWidget inside C++
        # this method is also unused, but is added to conform to the interface
        pass
