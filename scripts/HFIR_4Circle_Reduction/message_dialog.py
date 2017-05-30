# Dialog for message
from PyQt4 import QtGui, QtCore

import ui_messagebox


class MessageDialog(QtGui.QDialog):
    """
    extension of QDialog
    """
    def __init__(self, parent):
        """
        initialization of customized dialog box
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
        set text to the text editor
        :param text:
        :return:
        """
        assert isinstance(text, str), 'Input text of type {0} must be a string.'.format(type(text))
        self.ui.plainTextEdit_message.setPlainText(text)

        return

    def set_peak_integration_details(self, motor_pos_vec, pt_intensity_vec):
        """
        set the  details information of integrated peak including the peak intensities of
        each Pt.
        :param motor_pos_vec:
        :param pt_intensity_vec:
        :return:
        """
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
