from __future__ import absolute_import, print_function

from qtpy.QtCore import QObject, Qt, Signal


class AlgorithmProgressDialogPresenter(QObject):
    """
    Presents progress reports on algorithms.
    """
    need_update = Signal()

    def __init__(self, view, model):
        super(AlgorithmProgressDialogPresenter, self).__init__()
        view.close_button.clicked.connect(self.close)
        self.view = view
        self.model = model
        self.model.add_presenter(self)
        self.need_update.connect(self.update, Qt.QueuedConnection)

    def update(self):
        """
        Update the gui elements.
        """
        algorithm_data = self.model.get_running_algorithm_data()
        self.view.update(algorithm_data)

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
        self.need_update.emit()

    def close(self):
        self.model.remove_presenter(self)
        self.view.close()
