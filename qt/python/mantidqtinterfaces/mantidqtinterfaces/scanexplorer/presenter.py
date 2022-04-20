# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from enum import Enum

from mantid.api import MatrixWorkspace, MultipleExperimentInfos
from mantidqt.widgets.sliceviewer.views.dataview import SliceViewerDataView
from mantidqt.widgets.sliceviewer.presenters.presenter import SliceViewer

from .model import ScanExplorerModel
from .view import ScanExplorerView


class WS_TYPE(Enum):
    MDE = 0
    MDH = 1
    MATRIX = 2


class ScanExplorerPresenter:

    def __init__(self):
        self._ws = None
        self.view = ScanExplorerView(presenter=self)
        self.model = ScanExplorerModel(presenter=self)

        self.view.sig_files_selected.connect(self.on_files_selected)
        self.view.file_line_edit.returnPressed.connect(self.on_line_edited)

    def get_dim_info(self, n: int) -> dict:
        """
        @param n: the dimension to consider
        @return: dict of (minimum :float,
                          maximum :float,
                          number_of_bins :int,
                          width :float,
                          name :str,
                          units :str,
                          type :str,
                          can_rebin: bool,
                          qdim: bool) for this nth dimension
        """
        workspace = self._ws
        dim = workspace.getDimension(n)
        return {
            'minimum': dim.getMinimum(),
            'maximum': dim.getMaximum(),
            'number_of_bins': dim.getNBins(),
            'width': dim.getBinWidth(),
            'name': dim.name,
            'units': dim.getUnits(),
            'type': self.get_ws_type().name,
            'can_rebin': False,
            'qdim': dim.getMDFrame().isQ()
        }

    def get_dimensions_info(self) -> list:
        """
        @return: a list of dict for each dimension containing dim_info
        """
        return [self.get_dim_info(n) for n in range(self._ws.getNumDims())]

    def get_ws_type(self) -> WS_TYPE:
        """
        @return the type of the workspace being shown, from the enum defined above.
        """
        # TODO should we limit ourselves to matrix workspaces, since they are the most relevant to our purpose ?
        if isinstance(self._ws, MatrixWorkspace):
            return WS_TYPE.MATRIX
        elif isinstance(self._ws, MultipleExperimentInfos):
            if self._ws.isMDHistoWorkspace():
                return WS_TYPE.MDH
            else:
                return WS_TYPE.MDE
        else:
            raise ValueError("Unsupported workspace type")

    def create_slice_viewer(self, workspace):
        """
        Create the slice viewer widget to be shown.
        @param workspace: the workspace to show
        """
        self._ws = workspace
        presenter = SliceViewer(ws=workspace)
        self.view._data_view = SliceViewerDataView(presenter=presenter,
                                                   dims_info=self.get_dimensions_info(),
                                                   can_normalise=False)
        self.view.show_slice_viewer(workspace)

    def on_files_selected(self, files: list):
        """
        Slot triggered by the user selecting files through the browser
        @param files: the list of the path to each file selected
        """
        self.view.file_line_edit.setText(', '.join(files))

    def on_line_edited(self):
        """
        Slot triggered by the line edit being validated.
        """
        files = self.view.file_line_edit.text()
        self.model.process_files(files)

    def get_ws(self):
        return self._ws

    def show(self):
        self.view.show()
