from PyQt4 import QtGui, uic, QtCore
from functools import partial
from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.inelastic.dgs_pd_sc_conversion_script import PdAndScConversionScript
import ui.inelastic.ui_dgs_pd_sc_conversion
import reduction_gui.widgets.util as util

class PdAndScConversionWidget(BaseWidget):
    """
        Widget that presents powder and single crystal data conversion options
        to the user.
    """
    ## Widget name
    name = "Powder and SC"

    def __init__(self, parent=None, state=None, settings=None, data_type=None):
        super(PdAndScConversionWidget, self).__init__(parent, state, settings,
                                                      data_type=data_type)

        class PdAndScConversionFrame(QtGui.QFrame, ui.inelastic.ui_dgs_pd_sc_conversion.Ui_PdScConversionFrame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._content = PdAndScConversionFrame(self)
        self._layout.addWidget(self._content)
        self._instrument_name = settings.instrument_name
        self.initialize_content()

        if state is not None:
            self.set_state(state)
        else:
            self.set_state(PdAndScConversionScript(self._instrument_name))

    def initialize_content(self):
        # Constraints
        self._content.q_low_edit.setValidator(QtGui.QDoubleValidator(self._content.q_low_edit))
        self._content.q_width_edit.setValidator(QtGui.QDoubleValidator(self._content.q_width_edit))
        self._content.q_high_edit.setValidator(QtGui.QDoubleValidator(self._content.q_high_edit))

        # Default states
        self._save_powder_nxs_state(self._content.save_procnexus_cb.isChecked())

        # Connections
        self.connect(self._content.save_procnexus_save, QtCore.SIGNAL("clicked()"),
                     self._save_powder_nxs_save)
        self.connect(self._content.save_procnexus_cb, QtCore.SIGNAL("toggled(bool)"),
                     self._save_powder_nxs_state)

        # Validate widgets
        self._connect_validated_lineedit(self._content.q_low_edit)
        self._connect_validated_lineedit(self._content.q_width_edit)
        self._connect_validated_lineedit(self._content.q_high_edit)

    def _check_and_set_lineedit_content(self, lineedit, content):
        lineedit.setText(content)
        util.set_valid(lineedit, not lineedit.text() == '')

    def _connect_validated_lineedit(self, ui_ctrl):
        call_back = partial(self._validate_edit, ctrl=ui_ctrl)
        self.connect(ui_ctrl, QtCore.SIGNAL("editingFinished()"), call_back)
        self.connect(ui_ctrl, QtCore.SIGNAL("textEdited(QString)"), call_back)
        self.connect(ui_ctrl, QtCore.SIGNAL("textChanged(QString)"), call_back)

    def _validate_edit(self, ctrl=None):
        is_valid = True
        if not ctrl.text():
            is_valid = False
        util.set_valid(ctrl, is_valid)

    def _save_powder_nxs_state(self, state):
        self._content.save_procnexus_edit.setEnabled(state)
        self._content.save_procnexus_save.setEnabled(state)

    def _save_powder_nxs_save(self):
        fname = self.data_save_dialog("*.nxs")
        if fname:
            self._content.save_procnexus_edit.setText(fname)

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: PdAndScConversionScript object
        """
        self._content.powder_gb.setChecked(state.do_pd_convert)
        self._check_and_set_lineedit_content(self._content.q_low_edit,
                                             state.pd_q_range_low)
        self._check_and_set_lineedit_content(self._content.q_width_edit,
                                             state.pd_q_range_width)
        self._check_and_set_lineedit_content(self._content.q_high_edit,
                                             state.pd_q_range_high)
        self._content.save_procnexus_cb.setChecked(state.save_powder_nxs)
        self._content.save_procnexus_edit.setText(state.save_powder_nxs_file)

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        p = PdAndScConversionScript(self._instrument_name)
        p.do_pd_convert = self._content.powder_gb.isChecked()
        p.pd_q_range_low = self._content.q_low_edit.text()
        p.pd_q_range_width = self._content.q_width_edit.text()
        p.pd_q_range_high = self._content.q_high_edit.text()
        p.save_powder_nxs = self._content.save_procnexus_cb.isChecked()
        p.save_powder_nxs_file = self._content.save_procnexus_edit.text()
        return p
