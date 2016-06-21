from PyQt4 import QtGui, QtCore
from reduction_gui.reduction.output_script import Output
from base_widget import BaseWidget
import ui.ui_hfir_output

class OutputWidget(BaseWidget):
    """
        Widget that presents the transmission options to the user
    """
    _plot = None

    ## Widget name
    name = "Output"

    def __init__(self, parent=None, state=None, settings=None):
        BaseWidget.__init__(self, parent=parent, state=state, settings=settings)

        class OutputFrame(QtGui.QFrame, ui.ui_hfir_output.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._content = OutputFrame(self)
        self._layout.addWidget(self._content)
        self.initialize_content()

    def initialize_content(self):
        """
            Declare the validators and event connections for the
            widgets loaded through the .ui file.
        """

        # Clear data list
        self._content.output_text_edit.clear()

        # Clear rebin
        self._content.rebin_groupbox.deleteLater()
        self._content.n_q_bins_label.hide()
        self._content.n_q_bins_edit.hide()
        self._content.rebin_button.hide()
        self._content.lin_binning_radio.hide()
        self._content.log_binning_radio.hide()

    def set_state(self, state):
        self._content.output_text_edit.setText(state.log_text)

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        return Output()
