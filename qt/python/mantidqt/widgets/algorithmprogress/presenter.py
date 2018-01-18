from __future__ import absolute_import, print_function

from .model import AlgorithmProgressModel
from .presenter_base import AlgorithmProgressPresenterBase


class AlgorithmProgressPresenter(AlgorithmProgressPresenterBase):
    """
    Presents progress reports on algorithms.
    """
    def __init__(self, view):
        super(AlgorithmProgressPresenter, self).__init__()
        self.view = view
        self.model = AlgorithmProgressModel()
        self.model.add_presenter(self)
        self.algorithm = None

    def update_progress_bar(self, algorithm, progress, message):
        """
        Update the progress bar in the view.
        :param algorithm: The progressed algorithm.
        :param progress: Progress value to update the progress bar with.
        :param message: A message that may come from the algorithm.
        """
        if algorithm is self.algorithm:
            self.set_progress_bar(self.view.progress_bar, progress, message)

    def update_gui(self):
        """
        Close (remove) the progress bar when algorithm finishes.
        :param algorithms: A list of running algorithms
        """
        if self.algorithm is None:
            self.view.hide_progress_bar()
        else:
            self.view.show_progress_bar()

    def update(self, algorithms):
        """
        Close (remove) the progress bar when algorithm finishes.
        :param algorithms: A list of running algorithms
        """
        if len(algorithms) == 0:
            self.algorithm = None
        else:
            self.algorithm = algorithms[-1]
        self.need_update_gui.emit()
