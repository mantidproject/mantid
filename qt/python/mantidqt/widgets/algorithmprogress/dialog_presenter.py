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
        self.need_update.connect(self.update_gui, Qt.QueuedConnection)
        self.progress_bars = {}

    def update_gui(self):
        """
        Update the gui elements.
        """
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
            progress_bar.setValue(progress * 100)
            if message is None:
                progress_bar.setFormat('%p%')
            else:
                progress_bar.setFormat(message + ' %p%')

    def update(self, algorithms):
        """
        Close (remove) the progress bar when algorithm finishes.
        :param algorithms: A list of running algorithms
        """
        stored_algorithms = self.progress_bars.keys()
        for alg in stored_algorithms:
            if alg not in algorithms:
                del self.progress_bars[alg]
        self.need_update.emit()

    def close(self):
        self.model.remove_presenter(self)
        self.progress_bars.clear()
        self.view.close()

    def cancel_algorithm(self, algorithm_id):
        """
        Cancel an algorithm.
        :param algorithm_id: An id of an algorithm
        """
        # algorithm_id is actually an instance of an algorithm
        algorithm_id.cancel()
