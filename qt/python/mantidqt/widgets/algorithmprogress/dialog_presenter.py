from __future__ import absolute_import, print_function

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QProgressBar, QPushButton, QTreeWidgetItem

from .model import AlgorithmProgressModel


class AlgorithmProgressDialogPresenter(object):
    """
    Presents progress reports on algorithms.
    """
    def __init__(self, view, model):
        self.view = view
        self.model = model

    def update(self):
        """
        Update the gui elements.
        """
        tree = self.view.tree
        tree.clear()
        for observer in self.model.progress_observers:
            item = QTreeWidgetItem([observer.name()])
            tree.addTopLevelItem(item)
            progress_bar = QProgressBar()
            progress_bar.setAlignment(Qt.AlignHCenter)
            cancel_button = QPushButton("Cancel")
            tree.setItemWidget(item, 1, progress_bar)
            tree.setItemWidget(item, 2, cancel_button)
            for prop in observer.properties():
                lstr = [prop.name, str(prop.value)]
                if prop.isDefault:
                    lstr.append('Default')
                item.addChild(QTreeWidgetItem(lstr))

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
