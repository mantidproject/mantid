# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from . import ui_SpiceViewerDialog
from PyQt4 import QtCore
from PyQt4 import QtGui


class ViewSpiceDialog(QtGui.QDialog):
    """Dialog to view SPICE """
    def __init__(self, parent):
        """Initialization

        :param parent: parent
        """
        super(ViewSpiceDialog, self).__init__()

        # define UI
        self.ui = ui_SpiceViewerDialog.Ui_Dialog()
        self.ui.setupUi(self)

        # define event handlers
        self.ui.pushButton_close.clicked.connect(self.do_quit)

        return

    def do_quit(self):
        """Quit from the dialog, i.e., close the window

        :return:
        """
        self.close()

        return

    def clear_text(self):
        """Clear the text of the edit

        :return: None
        """
        self.ui.textBrowser_spice.clear()

        return

    def write_text(self, plain_text):
        """Write the text to text browser

        :param plain_text:
        :return:
        """
        assert isinstance(plain_text, str) or isinstance(plain_text, QtCore.QString), \
            'Type of plain text is not supported.'
        self.ui.textBrowser_spice.setText(plain_text)

        return
