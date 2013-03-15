################################################################################ 
# Advanced Setup Widget
################################################################################
from PyQt4 import QtGui, uic, QtCore
from functools import partial
from reduction_gui.widgets.base_widget import BaseWidget
import reduction_gui.widgets.util as util

from reduction_gui.reduction.diffraction.diffraction_adv_setup_script import AdvancedSetupScript 
import ui.diffraction.ui_diffraction_adv_setup

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

        dv5 = QtGui.QDoubleValidator(self._content.vanpeakfwhm_edit)
        dv5.setBottom(0.0)
        self._content.vanpeakfwhm_edit.setValidator(dv5)

        dv6 = QtGui.QDoubleValidator(self._content.vanpeaktol_edit)
        dv6.setBottom(0.0)
        self._content.vanpeaktol_edit.setValidator(dv6)

        # Default states
        # self._handle_preserveevents(self._content.preserveevents_checkbox.isChecked())

        # self._handle_tzero_guess(self._content.use_ei_guess_chkbox.isChecked())
        
        # Connections from action/event to function to handle 
        # self.connect(self._content.calfile_browse, QtCore.SIGNAL("clicked()"), 
        #         self._calfile_browse)
        # self.connect(self._content.charfile_browse, QtCore.SIGNAL("clicked()"), 
        #         self._charfile_browse)
        # self.connect(self._content.outputdir_browse, QtCore.SIGNAL("clicked()"),
        #         self._outputdir_browse)

        # Hanlder for events
        # self.connect(self._content.detcal_browse, QtCore.SIGNAL("clicked()"), 
        #              self._detcal_browse)
        # self.connect(self._content.hardmask_browse, QtCore.SIGNAL("clicked()"), 
        #              self._hardmask_browse)
        # self.connect(self._content.grouping_browse, QtCore.SIGNAL("clicked()"), 
        #              self._grouping_browse)
        # self.connect(self._content.use_ei_guess_chkbox, QtCore.SIGNAL("stateChanged(int)"),
        #              self._handle_tzero_guess)
        
        # Validated widgets
        # self._connect_validated_lineedit(self._content.sample_edit)
        # self._connect_validated_lineedit(self._content.ei_guess_edit)

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
        self._content.filterbadpulses_chkbox.setChecked(state.filterbadpulses)
        
        self._content.stripvanpeaks_chkbox.setChecked(state.stripvanadiumpeaks)
        self._content.vanpeakfwhm_edit.setText(str(state.vanadiumfwhm))
        self._content.vanpeaktol_edit.setText(str(state.vanadiumpeaktol))
        self._content.vansmoothpar_edit.setText(str(state.vanadiumsmoothparams))

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
        s.filterbadpulses = self._content.filterbadpulses_chkbox.isChecked()
        
        s.stripvanadiumpeaks = self._content.stripvanpeaks_chkbox.isChecked()
        s.vanadiumfwhm = self._content.vanpeakfwhm_edit.text()
        s.vanadiumpeaktol = self._content.vanpeaktol_edit.text()
        s.vanadiumsmoothparams = self._content.vansmoothpar_edit.text()

        return s


    def _detinstrumentchange(self):
        """ 
        """
        self._instrument_name = str(self._content.instrument_combo.currentText())

        return
#ENDCLASSDEF
