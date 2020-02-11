# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, unicode_literals)

# local imports
from mantidqt.widgets.workspacedisplay.table.model \
    import TableWorkspaceDisplayModel

# constants
DEFAULT_MARKER_COLOR = 'white'


class PeaksViewerModel(TableWorkspaceDisplayModel):
    """View model for PeaksViewer
    Extends PeaksWorkspace functionality to include color selection
    """
    def __init__(self, peaks_ws):
        """
        :param peaks_ws: A pointer to the PeaksWorkspace
        """
        if not hasattr(peaks_ws, 'getNumberPeaks'):
            raise ValueError("Expected a PeaksWorkspace type but found a {}".format(type(peaks_ws)))

        super(PeaksViewerModel, self).__init__(peaks_ws)
        self._marker_color = DEFAULT_MARKER_COLOR

    @property
    def marker_color(self):
        return self._marker_color

    @property
    def peaks_workspace(self):
        return self.ws
