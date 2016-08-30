from PyQt4.QtCore import *
from PyQt4.QtGui import *


class SortOrderDialog(QDialog):
    """

    """
    # TODO/FIXME/NOW - Make this dialog class work!
    def __init__(self, parent=None):
        """

        Args:
            parent:
        """
        super(SortOrderDialog, self).__init__(parent)

        # set up layout
        layout = QVBoxLayout(self)

        # define widget for order
        self.col_item0 = QComboBox(self)
        self.col_item1 = QComboBox(self)

        # add to layout
        layout.addWidget(self.col_item0)
        layout.addWidget(self.col_item1)

        # OK and cancel buttons
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel,
                                   Qt.Horizontal, self)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

        # ...
        self._order = None

        return

    def get_result(self):
        """

        Returns:

        """
        return self._order

    # static method to create the dialog and return (date, time, accepted)
    @staticmethod
    def get_sort_order(parent=None):
        """

        Args:
            parent:

        Returns: Order True/False (for accept or reject)

        """
        dialog = SortOrderDialog(parent)
        result = dialog.exec_()
        order = dialog.get_result()
        return order, result == QDialog.Accepted
