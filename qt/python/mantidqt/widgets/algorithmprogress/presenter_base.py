from __future__ import absolute_import, print_function

from qtpy.QtCore import QObject, Qt, Signal


class AlgorithmProgressPresenterBase(QObject):
    """
    Base class for progress presenters.
    """
    need_update_gui = Signal()

    def __init__(self):
        super(AlgorithmProgressPresenterBase, self).__init__()
        self.need_update_gui.connect(self.update_gui, Qt.QueuedConnection)

    @staticmethod
    def set_progress_bar(progress_bar, progress, message):
        """
        Update the progress bar in a view.
        :param progress_bar: The progress bar to be updated.
        :param progress: Progress value to update the progress bar with.
        :param message: A message that may come from the algorithm.
        """
        progress_bar.setValue(progress * 100)
        if message is None:
            progress_bar.setFormat('%p%')
        else:
            progress_bar.setFormat(message + ' %p%')
