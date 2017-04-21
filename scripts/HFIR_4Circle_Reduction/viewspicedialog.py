import ui_SpiceViewerDialog
from PyQt4 import QtGui, QtCore


class ViewSpiceDialog(QtGui.QDialog):
    """
    Dialog ot view SPICE
    """
    def __init__(self, parent):
        """
        Initialization
        :param parent:
        """
        super(ViewSpiceDialog, self).__init__()

        # define UI
        self.ui = ui_SpiceViewerDialog.Ui_Dialog()
        self.ui.setupUi(self)

        # define event handlers
        self.connect(self.ui.pushButton_close, QtCore.SIGNAL('clicked()'),
                     self.do_quit)

        return

    def do_quit(self):
        """
        Quit from the dialog, i.e., close the window
        :return:
        """
        self.close()

        return

    def clear_text(self):
        """
        Clear the text of the edit
        :return:
        """
        self.ui.textBrowser_spice.clear()

        return

    def write_text(self, plain_text):
        """
        Write the text to text browser
        :param plain_text:
        :return:
        """
        assert isinstance(plain_text, str) or isinstance(plain_text, QtCore.QString), \
            'Type of plain text is not supported.'
        self.ui.textBrowser_spice.setText(plain_text)

        return
