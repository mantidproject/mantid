# Dialog for message
from PyQt4 import QtGui, QtCore

import ui_messagebox


class MessageDialog(QtGui.QDialog):
    """
    extension of QDialog
    """
    def __init__(self, parent):
        """
        blabla
        :param parent:
        :return:
        """
        super(MessageDialog, self).__init__(parent)

        # set up UI
        self.ui = ui_messagebox.Ui_Dialog()
        self.ui.setupUi(self)

        # define operation
        self.connect(self.ui.pushButton_close, QtCore.SIGNAL('clicked()'),
                     self.do_quit)

        return

    def do_quit(self):
        """

        :return:
        """
        self.close()

    def set_text(self, text):
        """
        blabla
        :param text:
        :return:
        """
        self.ui.plainTextEdit_message.setPlainText(text)

        return

    def set_peak_integration_details(self, motor_pos_vec, pt_intensity_vec):
        """
        blabla
        :param motor_pos_vec:
        :param pt_intensity_vec:
        :return:
        """
        # check TODO/ISSUE/TODAY/NOW

        text = '# Pt. \tIntensity \tMotor Position\n'
        num_loops = max(len(motor_pos_vec), len(pt_intensity_vec))

        for pt in range(num_loops):
            text += '{0} \t'.format(pt+1)
            if pt < len(pt_intensity_vec):
                text += '{0} \t'.format(pt_intensity_vec[pt])
            else:
                text += '     \t'
            if pt < len(motor_pos_vec):
                text += '{0}\n'.format(motor_pos_vec[pt])
            else:
                text += '   \n'
        # END-FOR

        self.set_text(text)

        return
