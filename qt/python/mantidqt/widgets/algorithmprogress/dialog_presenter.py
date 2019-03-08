# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

from .presenter_base import AlgorithmProgressPresenterBase


class AlgorithmProgressDialogPresenter(AlgorithmProgressPresenterBase):
    """
    Presents progress reports on algorithms.
    """

    def __init__(self, view, model):
        super(AlgorithmProgressDialogPresenter, self).__init__()
        # connect the close button to the closeEvent of the window
        # so that pressing the X button, and pressing `Close` go through
        # the same routine, and properly call the presenter's close()
        view.close_button.clicked.connect(view.close)
        self.view = view
        self.model = model
        self.model.add_presenter(self)
        self.progress_bars = {}

    def update_gui(self):
        """
        Update the gui elements.
        """
        self.progress_bars.clear()
        algorithm_data = self.model.get_running_algorithm_data()
        self.view.update(algorithm_data)

    def add_progress_bar(self, algorithm_id, progress_bar):
        """
        Store the mapping between the algorithm and its progress bar.
        :param algorithm_id: An id of an algorithm.
        :param progress_bar: QProgressBar widget.
        """
        self.progress_bars[algorithm_id] = progress_bar

    def update_progress_bar(self, algorithm_id, progress, message):
        """
        Update the progress bar in the view.
        :param algorithm_id: The progressed algorithm's id.
        :param progress: Progress value to update the progress bar with.
        :param message: A message that may come from the algorithm.
        """
        progress_bar = self.progress_bars.get(algorithm_id, None)
        if progress_bar is not None:
            self.need_update_progress_bar.emit(progress_bar, progress, message)

    def update(self):
        """
        Update the gui asynchronously.
        """
        self.update_watched_algorithm.emit()

    def close(self):
        """
        Close the dialog.
        """
        self.model.remove_presenter(self)
        self.progress_bars.clear()
        self.view.parent().clear_dialog()

    def cancel_algorithm(self, algorithm_id):
        """
        Cancel an algorithm.
        :param algorithm_id: An id of an algorithm
        """
        # algorithm_id is actually an instance of an algorithm
        algorithm_id.cancel()
