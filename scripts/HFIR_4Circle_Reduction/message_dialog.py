# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# Dialog for message
from __future__ import (absolute_import, division, print_function)
from six.moves import range
from qtpy.QtWidgets import (QDialog)  # noqa
from mantid.kernel import Logger
try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("HFIR_4Circle_Reduction").information('Using legacy ui importer')
    from mantidplot import load_ui


class MessageDialog(QDialog):
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
        ui_path = "messagebox.ui"
        self.ui = load_ui(__file__, ui_path, baseinstance=self)

        # define operation
        self.ui.pushButton_close.clicked.connect(self.do_quit)

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
