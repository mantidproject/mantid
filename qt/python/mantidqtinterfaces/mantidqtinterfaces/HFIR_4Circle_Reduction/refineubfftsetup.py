# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtWidgets import QDialog
from mantid.kernel import Logger

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("HFIR_4Circle_Reduction").information("Using legacy ui importer")
    from mantidplot import load_ui


class RefineUBFFTSetupDialog(QDialog):
    """A dialog window to get the setup for refining UB matrix by FFT."""

    def __init__(self, parent):
        """Initialization

        :param parent:
        """
        super(RefineUBFFTSetupDialog, self).__init__(parent)

        # create UI
        ui_path = "RefineUbFftDialog.ui"
        self.ui = load_ui(__file__, ui_path, baseinstance=self)

        # init widget value
        self.ui.lineEdit_minD.setText("1.0")
        self.ui.lineEdit_maxD.setText("40.0")
        self.ui.lineEdit_tolerance.setText("0.15")

        # connected to event handler
        self.ui.buttonBox.accepted.connect(self.do_ok)
        self.ui.buttonBox.rejected.connect(self.do_cancel)

        # class variables
        self._minD = 1.0
        self._maxD = 40.0
        self._tolerance = 0.15

        return

    def do_ok(self):
        """accept the current set up and return

        :return:
        """
        try:
            min_d = float(str(self.ui.lineEdit_minD.text()))
            max_d = float(str(self.ui.lineEdit_maxD.text()))
            tolerance = float(str(self.ui.lineEdit_tolerance.text()))

        except ValueError:
            # unable to parse the value right
            self.ui.label_message.setText(
                'Unable to set up MinD, MaxD or Tolerance due to value error.\nEither enter correct value or press "Cancel".'
            )

        else:
            # good to go?
            if min_d >= max_d:
                self.ui.label_message.setText("MinD cannot be equal or larger than MaxD.")
            elif tolerance <= 0.0 or tolerance >= 1.0:
                self.ui.label_message.setText("Tolerance cannot be negative or larger than 1.")
            else:
                # finally good value
                self._minD = min_d
                self._maxD = max_d
                self._tolerance = tolerance
                # and close the window
                self.close()
            # END-IF-ELSE
        # END-TRY-EXCEPTION

        return

    def do_cancel(self):
        """cancel window

        :return:
        """
        self.close()

        return

    def get_values(self):
        """get the setup: min D, max D and tolerance

        :return: 3-tuple
        """
        return self._minD, self._maxD, self._tolerance
