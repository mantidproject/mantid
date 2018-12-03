# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from qtpy.QtWidgets import (QFrame)  # noqa
from reduction_gui.reduction.output_script import Output
from reduction_gui.widgets.base_widget import BaseWidget
try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    from mantid.kernel import Logger
    Logger("OutputWidget").information('Using legacy ui importer')
    from mantidplot import load_ui


class OutputWidget(BaseWidget):
    """
        Widget that presents the transmission options to the user
    """
    _plot = None

    ## Widget name
    name = "Output"

    def __init__(self, parent=None, state=None, settings=None):
        BaseWidget.__init__(self, parent=parent, state=state, settings=settings)

        class OutputFrame(QFrame):
            def __init__(self, parent=None):
                QFrame.__init__(self, parent)
                self.ui = load_ui(__file__, '../../ui/hfir_output.ui', baseinstance=self)

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
