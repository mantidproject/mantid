# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from enum import Enum
from os import path

from qtpy.QtCore import QObject, Signal

from mantid.api import MatrixWorkspace, MultipleExperimentInfos, AlgorithmObserver, Algorithm
from mantidqt.widgets.sliceviewer.presenters.presenter import SliceViewer
from mantidqt.widgets.sliceviewer.presenters.lineplots import PixelLinePlot  # , RectangleSelectionLinePlot
from mantid.kernel import logger
from mantid.api import mtd
# from mantidqt.plotting.mantid_navigation_toolbar import MantidNavigationTool


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

        self.view.sig_files_selected.connect(self.on_file_selected)
        self.view.file_line_edit.returnPressed.connect(self.on_line_edited)

        self.observer = ScanAlgorithmObserver()

        self.observer.signals.finished.connect(self.on_algorithm_finished)
        self.observer.signals.started.connect(self.set_algorithm_result_name)

        self.future_workspace: str = ""

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
        try:
            presenter = SliceViewer(ws=workspace)
        except ValueError as e:
            logger.error("Cannot open reduced workspace in Slice Viewer:\n{}".format(e))
            return

        self._ws = workspace
        self.view.data_view = presenter._data_view
        self.view.show_slice_viewer(workspace)
        self.view.data_view.add_line_plots(PixelLinePlot, self.view.data_view.presenter)

        self.view.multiple_action = self.view.data_view.mpl_toolbar.addAction("MULT", self.view.start_multiple_rect_mode)
        self.view.multiple_action.setCheckable(True)

        # TODO find a better way to activate the cursor tracking
        self.view.data_view.track_cursor.setChecked(False)
        self.view.data_view.track_cursor.setChecked(True)

    def on_file_selected(self, file: str):
        """
        Slot triggered by the user selecting a file through the browser
        @param file: the path to the file selected
        """
        self.view.file_line_edit.setText(file)
        self.on_line_edited()

    def on_line_edited(self):
        """
        Slot triggered by the line edit being validated.
        """
        file = self.view.file_line_edit.text()
        if path.isfile(file):
            self.model.process_file(file)
        else:
            logger.warning("Path {} does not point to a valid file.".format(file))

    def on_dialog_accepted(self):
        """
        Slot triggered when the scan dialog is accepted. That means the algorithm is started, and the observer needs to
        start watching for it.
        """
        self.observer.observeStarting()
        self.observer.observing = True

    def on_algorithm_finished(self, error: bool, error_message: str):
        """
        Slot called when the algorithm summoned by the dialog has run its course. Show the result in the slice viewer
        @param error: True if the algorithm threw an error, else False
        @param error_message: the error message given by the algorithm if there was any
        """
        if error:
            logger.error("Error running the algorithm :\n" + error_message)
            return

        if not mtd.doesExist(self.future_workspace):
            logger.warning("Output workspace not found.")
            return

        self._ws = mtd[self.future_workspace]
        self.future_workspace = ""

        self.create_slice_viewer(self._ws)

    def set_algorithm_result_name(self, new_algorithm):
        """
        Sets the name of the workspace that will be shown once the algorithm completes.
        @param new_algorithm: the algorithm that has been created
        """
        self.future_workspace = new_algorithm.getPropertyValue("OutputWorkspace")

    @property
    def ws(self):
        return self._ws

    def show(self):
        self.view.show()


class ScanAlgorithmObserverSignals(QObject):
    """
    Signals for the observer
    """
    finished = Signal(bool, str)  # return False for success, True for error, and in this case an error message
    started = Signal(Algorithm)


class ScanAlgorithmObserver(AlgorithmObserver):
    def __init__(self):
        super(ScanAlgorithmObserver, self).__init__()
        self.signals = ScanAlgorithmObserverSignals()
        self.error = False
        self.error_message = ""
        self.observing = False

    def startingHandle(self, alg):
        """
        Called when an algorithm is started.
        @param alg: the algorithm starting
        """
        # We are waiting for this specific algorithm, so in the off chance another is run just at the same time,
        # we check for its name
        if alg.name() != "SANSILLParameterScan":
            return

        self.signals.started.emit(alg)

        self.observeFinish(alg)

    def finishHandle(self):
        """
        Called when the observed algorithm is finished
        """
        self.signals.finished.emit(self.error, self.error_message)

        # the algorithm has run, so we can stop listening for now. IF IT WOULD WORK
        if self.observing:
            self.observing = False

            # this function hangs? it breaks nothing but eats the thread
            self.stopObservingManager()

    def errorHandle(self, msg):
        """
        Called when the observed algo encounter an error.
        """
        self.error = True
        self.error_message = msg
