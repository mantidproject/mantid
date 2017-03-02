from PyQt4 import QtGui, QtCore
import ui_PeakIntegrationSpreadSheet


class PeaksIntegrationReportDialog(QtGui.QDialog):
    """
    Dialog to report the details of peaks integration
    """
    def __init__(self, parent):
        """
        initialization
        :param parent:
        """
        super(PeaksIntegrationReportDialog, self).__init__(parent)

        # set up UI
        self.ui = ui_PeakIntegrationSpreadSheet.Ui_Dialog()
        self.ui.setupUi(self)

        return