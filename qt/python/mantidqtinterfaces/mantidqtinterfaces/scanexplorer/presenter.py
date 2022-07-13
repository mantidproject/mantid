# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from os import path, sep
from numpy import array

from qtpy.QtCore import QObject, Signal

from mantid.api import MatrixWorkspace, AlgorithmObserver, Algorithm
from mantidqt.widgets.sliceviewer.presenters.presenter import SliceViewer
from mantid.kernel import logger, config
from mantid.api import mtd


from .model import ScanExplorerModel
from .view import ScanExplorerView


class ScanExplorerPresenter:

    def __init__(self):
        self._ws: MatrixWorkspace = None  # the workspace containing the data shown
        self._bg_ws: MatrixWorkspace = None  # the workspace containing the background, if there is one.

        self.view = ScanExplorerView(presenter=self)
        self.model = ScanExplorerModel(presenter=self)

        self.view.sig_files_selected.connect(self.on_file_selected)
        self.view.file_line_edit.returnPressed.connect(self.on_line_edited)
        self.view.sig_background_selected.connect(self.model.process_background)

        self.observer = ScanAlgorithmObserver()

        self.observer.signals.finished.connect(self.on_algorithm_finished)
        self.observer.signals.started.connect(self.set_algorithm_result_name)

        # the name of the workspace in which the result of the ParameterScan algorithm will be loaded once it ran
        self.future_workspace: str = ""

    def create_slice_viewer(self, workspace: MatrixWorkspace):
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
        self.view.data_view = presenter.view.data_view
        self.view.show_slice_viewer(workspace)

        self.view.initialize_rectangle_manager()

        self.view.manage_buttons()

    def on_file_selected(self, file_path: str):
        """
        Slot triggered by the user selecting a file through the browser
        @param file_path: the path to the file selected
        """
        self.view.file_line_edit.setText(file_path)
        self.on_line_edited()

    def on_background_selected(self, bg_file_path: str):
        """
        Slot triggered when a background file is selected
        @param bg_file_path: the path to the file containing the background
        """
        self.model.process_background(bg_file_path)

    def on_line_edited(self):
        """
        Slot triggered by the line edit being validated.
        """
        file_path = self.view.file_line_edit.text()
        if path.isfile(file_path):
            self.model.process_file(file_path)
        else:
            logger.error("Path {} does not point to a valid file.".format(file_path))

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

    def get_axes(self) -> (array, array):
        """
        Get the axes from the workspace
        @return the x and y axes of the scan workspace
        """
        return self._ws.getAxis(0).extractValues(), self._ws.getAxis(1).extractValues()

    @staticmethod
    def get_base_directory() -> str:
        """
        Get the directory to use as default for browsing.
        Usually the first from the user defined data search directories list.
        @return the absolute path of this directory
        """
        data_search_dirs = config.getDataSearchDirs()

        for directory in data_search_dirs:
            if path.isdir(directory):
                return directory

        return path.abspath(sep)

    def set_bg_ws(self, ws_name):
        """
        Set the background workspace as the one with provided name.
        @param ws_name: the name of the workspace
        """
        self._bg_ws = mtd[ws_name]

    @property
    def ws(self):
        return self._ws

    def show(self):
        self.view.show()

    @property
    def rectangles_manager(self):
        return self.view.rectangles_manager


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
