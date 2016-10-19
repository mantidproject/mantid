import ui_RefineUbFftDialog

from PyQt4 import QtCore, QtGui
try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s


class RefineUBFFTSetupDialog(QtGui.QDialog):
    """

    """
    def __init__(self, parent):
        """

        :param parent:
        """
        super(RefineUBFFTSetupDialog, self).__init__(parent)

        self.ui = ui_RefineUbFftDialog.Ui_Dialog()
        self.ui.setupUi(self)

        self.connect(self.ui.buttonBox, QtCore.SIGNAL('accepted()'),
                     self.do_ok)

        self.connect(self.ui.buttonBox, QtCore.SIGNAL('rejected()'),
                     self.do_cancel)

        return

    def do_ok(self):
        """

        :return:
        """
        print 'ok'

        self.close()

        return

    def do_cancel(self):
        """

        :return:
        """
        print 'cancel'

        self.close()

        return

    def get_values(self):
        """

        :return:
        """
        return 'blablalba'