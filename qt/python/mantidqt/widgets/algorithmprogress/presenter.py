from __future__ import absolute_import, print_function

from .model import AlgorithmProgressModel


class AlgorithmProgressPresenter(object):
    """
    Presents progress reports on algorithms.
    """
    def __init__(self, view):
        self.view = view
        self.model = AlgorithmProgressModel(self)

    def update_progress_bar(self, progress, message):
        """
        Update the progress bar in the view.
        :param progress: Progress value to update the progress bar with.
        :param message: A message that may come from the algorithm.
        """
        pass

    def close_progress_bar(self):
        """
        Close (remove) the progress bar when algorithm finishes.
        """
        pass
