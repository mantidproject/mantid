################################################################################
# Advanced Setup Widget
################################################################################
from PyQt4 import QtGui, uic, QtCore
from functools import partial
from reduction_gui.widgets.base_widget import BaseWidget
import reduction_gui.widgets.util as util

from reduction_gui.reduction.diffraction.diffraction_adv_setup_script import AdvancedSetupScript
import ui.diffraction.ui_diffraction_adv_setup
import ui.diffraction.ui_diffraction_info

class AdvancedSetupWidget(BaseWidget):
    """ Widget that presents run setup including sample run, optional vanadium run and etc.
    """
    # Widge name
    name = "Advanced Setup"

    def __init__(self, parent=None, state=None, settings=None, data_type=None):
        """ Initialization
        """
        super(AdvancedSetupWidget, self).__init__(parent, state, settings, data_type=data_type)

        class AdvancedSetFrame(QtGui.QFrame, ui.diffraction.ui_diffraction_adv_setup.Ui_Frame):
            """ Define class linked to UI Frame
            """
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._content = AdvancedSetFrame(self)
        self._layout.addWidget(self._content)
        self._instrument_name = settings.instrument_name
        self._facility_name = settings.facility_name
        self.initialize_content()

        if state is not None:
            self.set_state(state)
        else:
            self.set_state(AdvancedSetupScript(self._instrument_name))

        return

    def initialize_content(self):
        """ Initialize content/UI
        """
        # FIXME : Fill in the contraints, states and connections to this method ASAP

        # Constraints/Validator
        iv4 = QtGui.QIntValidator(self._content.maxchunksize_edit)
        iv4.setBottom(0)
        self._content.maxchunksize_edit.setValidator(iv4)

        dv0 = QtGui.QDoubleValidator(self._content.unwrap_edit)
        self._content.unwrap_edit.setValidator(dv0)

        dv2 = QtGui.QDoubleValidator(self._content.lowres_edit)
        self._content.lowres_edit.setValidator(dv2)

        dv3 = QtGui.QDoubleValidator(self._content.cropwavelengthmin_edit)
        dv3.setBottom(0.0)
        self._content.cropwavelengthmin_edit.setValidator(dv3)

        dv4 = QtGui.QDoubleValidator(self._content.removepromptwidth_edit)
        dv4.setBottom(0.0)
        self._content.removepromptwidth_edit.setValidator(dv4)
        self._content.removepromptwidth_edit.setText("50.0")

        dv5 = QtGui.QDoubleValidator(self._content.vanpeakfwhm_edit)
        dv5.setBottom(0.0)
        self._content.vanpeakfwhm_edit.setValidator(dv5)

        dv6 = QtGui.QDoubleValidator(self._content.vanpeaktol_edit)
        dv6.setBottom(0.0)
        self._content.vanpeaktol_edit.setValidator(dv6)

        dv7 = QtGui.QDoubleValidator(self._content.scaledata_edit)
        dv7.setBottom(0.0)
        self._content.scaledata_edit.setValidator(dv7)

        # Default states
        self._content.extension_combo.setCurrentIndex(1)

        self._content.stripvanpeaks_chkbox.setChecked(True)
        self._syncStripVanPeakWidgets(True)

        self._content.preserveevents_checkbox.setChecked(True)

        dv8 = QtGui.QDoubleValidator(self._content.filterbadpulses_edit)
        dv8.setBottom(0.0)
        self._content.filterbadpulses_edit.setValidator(dv8)
        self._content.filterbadpulses_edit.setText("95.")

        # Connections from action/event to function to handle
        self.connect(self._content.stripvanpeaks_chkbox, QtCore.SIGNAL("clicked()"),
                self._stripvanpeaks_clicked)

        self.connect(self._content.help_button, QtCore.SIGNAL("clicked()"),
                self._show_help)
        # Hanlder for events
        # FIXME - Need to add an event hanlder for the change of instrument and facility

        # Validated widgets

        return

    def set_state(self, state):
        """ Populate the UI elements with the data from the given state.
            @param state: RunSetupScript object
        """

        self._content.pushdatapos_combo.setCurrentIndex(self._content.pushdatapos_combo.findText(state.pushdatapositive))
        self._content.unwrap_edit.setText(str(state.unwrapref))
        self._content.lowres_edit.setText(str(state.lowresref))
        self._content.removepromptwidth_edit.setText(str(state.removepropmppulsewidth))
        self._content.maxchunksize_edit.setText(str(state.maxchunksize))
        self._content.scaledata_edit.setText(str(state.scaledata))
        self._content.filterbadpulses_edit.setText(str(state.filterbadpulses))
        self._content.bkgdsmoothpar_edit.setText(str(state.bkgdsmoothpars))

        self._content.stripvanpeaks_chkbox.setChecked(state.stripvanadiumpeaks)
        self._syncStripVanPeakWidgets(state.stripvanadiumpeaks)
        self._content.vanpeakfwhm_edit.setText(str(state.vanadiumfwhm))
        self._content.vanpeaktol_edit.setText(str(state.vanadiumpeaktol))
        self._content.vansmoothpar_edit.setText(str(state.vanadiumsmoothparams))

        self._content.preserveevents_checkbox.setChecked(state.preserveevents)
        self._content.extension_combo.setCurrentIndex(self._content.extension_combo.findText(state.extension))
        self._content.outputfileprefix_edit.setText(state.outputfileprefix)

        return


    def get_state(self):
        """ Returns a RunSetupScript with the state of Run_Setup_Interface
        Set up all the class parameters in RunSetupScrpt with values in the content
        """
        s = AdvancedSetupScript(self._instrument_name)

        s.pushdatapositive = str(self._content.pushdatapos_combo.currentText())
        s.unwrapref = self._content.unwrap_edit.text()
        s.lowresref = self._content.lowres_edit.text()
        s.cropwavelengthmin = self._content.cropwavelengthmin_edit.text()
        s.removepropmppulsewidth = self._content.removepromptwidth_edit.text()
        s.maxchunksize = self._content.maxchunksize_edit.text()
        s.scaledata = self._content.scaledata_edit.text()
        s.filterbadpulses = self._content.filterbadpulses_edit.text()
        s.bkgdsmoothpars = self._content.bkgdsmoothpar_edit.text()

        s.stripvanadiumpeaks = self._content.stripvanpeaks_chkbox.isChecked()
        s.vanadiumfwhm = self._content.vanpeakfwhm_edit.text()
        s.vanadiumpeaktol = self._content.vanpeaktol_edit.text()
        s.vanadiumsmoothparams = self._content.vansmoothpar_edit.text()

        s.preserveevents = self._content.preserveevents_checkbox.isChecked()
        s.extension = str(self._content.extension_combo.currentText())

        s.outputfileprefix = self._content.outputfileprefix_edit.text()

        return s


    def _detinstrumentchange(self):
        """
        """
        self._instrument_name = str(self._content.instrument_combo.currentText())

        return

    def _stripvanpeaks_clicked(self):
        """ Handling if strip-vanadium-peak check box is clicked
        """
        self._syncStripVanPeakWidgets(self._content.stripvanpeaks_chkbox.isChecked())

        return

    def _show_help(self):
        class HelpDialog(QtGui.QDialog, ui.diffraction.ui_diffraction_info.Ui_Dialog):
            def __init__(self, parent=None):
                QtGui.QDialog.__init__(self, parent)
                self.setupUi(self)
        dialog = HelpDialog(self)
        dialog.exec_()

        return

    def _syncStripVanPeakWidgets(self, stripvanpeak):
        """ Synchronize the other widgets with vanadium peak
        """
        self._content.vanpeakfwhm_edit.setEnabled(stripvanpeak)
        self._content.vansmoothpar_edit.setEnabled(stripvanpeak)
        self._content.vanpeaktol_edit.setEnabled(stripvanpeak)

        return

#ENDCLASSDEF
