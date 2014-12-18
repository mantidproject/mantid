################################################################################
# This is my first attempt to make a tab from quasi-scratch
################################################################################
from PyQt4 import QtGui, uic, QtCore
from functools import partial
from reduction_gui.widgets.base_widget import BaseWidget
import reduction_gui.widgets.util as util

from reduction_gui.reduction.diffraction.diffraction_run_setup_script import RunSetupScript
import ui.diffraction.ui_diffraction_run_setup
import ui.diffraction.ui_diffraction_info

#import mantid.simpleapi as api
IS_IN_MANTIDPLOT = False
try:
    from mantid.api import *
    from mantid.kernel import *
    IS_IN_MANTIDPLOT = True
except:
    pass

class RunSetupWidget(BaseWidget):
    """ Widget that presents run setup including sample run, optional vanadium run and etc.
    """
    # Widge name
    name = "Experiment Run Setup"

    def __init__(self, parent=None, state=None, settings=None, data_type=None):
        """ Initialization
        """
        super(RunSetupWidget, self).__init__(parent, state, settings, data_type=data_type)

        class RunSetFrame(QtGui.QFrame, ui.diffraction.ui_diffraction_run_setup.Ui_Frame):
            """ Define class linked to UI Frame
            """
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
        #END-DEF RunSetFrame

        self._content = RunSetFrame(self)
        self._layout.addWidget(self._content)
        self._instrument_name = settings.instrument_name
        self._facility_name = settings.facility_name
        self.initialize_content()

        if state is not None:
            self.set_state(state)
        else:
            self.set_state(RunSetupScript(self._instrument_name))

        return

    def initialize_content(self):
        """ Initialize content/UI
        """
        # Initial values for combo boxes
        # Combo boxes
        self._content.saveas_combo.setCurrentIndex(1)
        self._content.unit_combo.setCurrentIndex(1)
        self._content.bintype_combo.setCurrentIndex(1)

        # Radio buttons
        self._content.disablevancorr_chkbox.setChecked(False)

        # Check boxes
        self._content.usebin_button.setChecked(True)
        self._content.resamplex_button.setChecked(False)
        self._content.disablebkgdcorr_chkbox.setChecked(False)
        self._content.disablevancorr_chkbox.setChecked(False)
        self._content.disablevanbkgdcorr_chkbox.setChecked(False)

        #self._content.override_emptyrun_checkBox.setChecked(False)
        #self._content.override_vanrun_checkBox.setChecked(False)
        #self._content.override_vanbkgdrun_checkBox.setChecked(False)

        # Line edit
        self._content.emptyrun_edit.setEnabled(True)
        self._content.vanrun_edit.setEnabled(True)
        self._content.vanbkgdrun_edit.setEnabled(True)
        self._content.resamplex_edit.setEnabled(False)

        # Constraints/Validator
        # Integers
        iv0 = QtGui.QIntValidator(self._content.emptyrun_edit)
        iv0.setBottom(0)
        self._content.emptyrun_edit.setValidator(iv0)

        iv1 = QtGui.QIntValidator(self._content.vanrun_edit)
        iv1.setBottom(0)
        self._content.vanrun_edit.setValidator(iv1)

        # iv2 = QtGui.QIntValidator(self._content.vannoiserun_edit)
        # iv2.setBottom(0)
        # self._content.vannoiserun_edit.setValidator(iv2)

        iv3 = QtGui.QIntValidator(self._content.vanbkgdrun_edit)
        iv3.setBottom(0)
        self._content.vanbkgdrun_edit.setValidator(iv3)

        siv = QtGui.QIntValidator(self._content.resamplex_edit)
        siv.setBottom(0)
        self._content.resamplex_edit.setValidator(siv)

        # Float/Double
        fiv = QtGui.QDoubleValidator(self._content.binning_edit)
        self._content.binning_edit.setValidator(fiv)

        # Default states

        # self._handle_tzero_guess(self._content.use_ei_guess_chkbox.isChecked())

        # Connections from action/event to function to handle
        self.connect(self._content.calfile_browse, QtCore.SIGNAL("clicked()"),
                self._calfile_browse)
        self.connect(self._content.charfile_browse, QtCore.SIGNAL("clicked()"),
                self._charfile_browse)
        self.connect(self._content.outputdir_browse, QtCore.SIGNAL("clicked()"),
                self._outputdir_browse)
        self.connect(self._content.binning_edit, QtCore.SIGNAL("valueChanged"),
                self._binvalue_edit)
        self.connect(self._content.bintype_combo, QtCore.SIGNAL("currentIndexChanged(QString)"),
                self._bintype_process)

        #self.connect(self._content.override_emptyrun_checkBox, QtCore.SIGNAL("clicked()"),
        #        self._overrideemptyrun_clicked)
        #self.connect(self._content.override_vanrun_checkBox, QtCore.SIGNAL("clicked()"),
        #        self._overridevanrun_clicked)
        #self.connect(self._content.override_vanbkgdrun_checkBox, QtCore.SIGNAL("clicked()"),
        #        self._overridevanbkgdrun_clicked)

        self.connect(self._content.disablebkgdcorr_chkbox, QtCore.SIGNAL("clicked()"),
                self._disablebkgdcorr_clicked)
        self.connect(self._content.disablevancorr_chkbox, QtCore.SIGNAL("clicked()"),
                self._disablevancorr_clicked)
        self.connect(self._content.disablevanbkgdcorr_chkbox, QtCore.SIGNAL("clicked()"),
                self._disablevanbkgdcorr_clicked)

        self.connect(self._content.usebin_button, QtCore.SIGNAL("clicked()"),
                self._usebin_clicked)
        self.connect(self._content.resamplex_button, QtCore.SIGNAL("clicked()"),
                self._resamplex_clicked)

        self.connect(self._content.help_button, QtCore.SIGNAL("clicked()"),
                self._show_help)

        # Validated widgets

        return

    def set_state(self, state):
        """ Populate the UI elements with the data from the given state.
            @param state: RunSetupScript object
        """
        self._content.runnumbers_edit.setText(state.runnumbers)
        self._content.calfile_edit.setText(state.calibfilename)
        self._content.charfile_edit.setText(state.charfilename)
        self._content.sum_checkbox.setChecked(state.dosum)
        self._content.binning_edit.setText(str(state.binning))
        state.binning = float(state.binning)
        if state.binning > 0.0:
            #print "[DBx304-1]: state.binning = %f, Set binning type to Linear" % (state.binning)
            self._content.bintype_combo.setCurrentIndex(0)
            #print "[DBx304-2]: Done... Set to self._content.bintype_combo.currentText()"
        else:
            #print "[DBx304-1]: state.binning = %f, Set binning type to Logarithmic" % (state.binning)
            self._content.bintype_combo.setCurrentIndex(1)
            #print "[DBx304-2]: Done... Set to self._content.bintype_combo.currentText()"

        self._content.binind_checkbox.setChecked(state.binindspace)
        self._content.resamplex_edit.setText(str(state.resamplex))
        self._content.outputdir_edit.setText(state.outputdir)
        self._content.saveas_combo.setCurrentIndex(self._content.saveas_combo.findText(state.saveas))
        self._content.unit_combo.setCurrentIndex(self._content.unit_combo.findText(state.finalunits))

        # Background correction
        if state.bkgdrunnumber is not None and state.bkgdrunnumber != "":
            self._content.emptyrun_edit.setText(str(int(state.bkgdrunnumber)))
        self._content.disablebkgdcorr_chkbox.setChecked(state.disablebkgdcorrection)
        # Vanadium correction
        if state.vanrunnumber is not None and state.vanrunnumber != "":
            self._content.vanrun_edit.setText(str(abs(int(state.vanrunnumber))))
        self._content.disablevancorr_chkbox.setChecked(state.disablevancorrection)
        # Vanadium background correction
        if state.vanbkgdrunnumber is not None and state.vanbkgdrunnumber != "":
            self._content.vanbkgdrun_edit.setText(str(int(state.vanbkgdrunnumber)))
        self._content.disablevanbkgdcorr_chkbox.setChecked(state.disablevanbkgdcorrection)

        # self._content.vannoiserun_edit.setText(str(state.vannoiserunnumber))
        # if state.vanrunnumber < 0:
        #     self._content.disablevancorr_chkbox.setChecked(True)
        # else:
        #     self._content.disablevancorr_chkbox.setChecked(False)

        return


    def get_state(self):
        """ Returns a RunSetupScript with the state of Run_Setup_Interface
        Set up all the class parameters in RunSetupScrpt with values in the content
        """
        s = RunSetupScript(self._instrument_name)

        s.runnumbers = self._content.runnumbers_edit.text()
        rtup = self.validateIntegerList(s.runnumbers)
        isvalid = rtup[0]
        if isvalid is False:
            raise NotImplementedError("Run number error @ %s" % (rtup[1]))

        s.calibfilename = self._content.calfile_edit.text()
        s.charfilename = self._content.charfile_edit.text()
        s.dosum = self._content.sum_checkbox.isChecked()
        s.binning = self._content.binning_edit.text()
        bintypestr = self._content.bintype_combo.currentText()
        if s.binning != "" and s.binning is not None:
            if bintypestr.count("Linear") == 1:
                s.binning = abs(float(s.binning))
            else:
                s.binning = -1.*abs(float(s.binning))
        s.binindspace = self._content.binind_checkbox.isChecked()
        s.resamplex = int(self._content.resamplex_edit.text())
        s.outputdir = self._content.outputdir_edit.text()
        s.saveas = str(self._content.saveas_combo.currentText())
        s.finalunits = str(self._content.unit_combo.currentText())

        s.bkgdrunnumber = self._content.emptyrun_edit.text()
        s.disablebkgdcorrection = self._content.disablebkgdcorr_chkbox.isChecked()

        s.vanrunnumber = self._content.vanrun_edit.text()
        s.disablevancorrection = self._content.disablevancorr_chkbox.isChecked()

        s.vanbkgdrunnumber = self._content.vanbkgdrun_edit.text()
        s.disablevanbkgdcorrection = self._content.disablevanbkgdcorr_chkbox.isChecked()

        s.dosamplex = self._content.resamplex_button.isChecked()

        #s.vannoiserunnumber = self._content.vannoiserun_edit.text()

        return s


    def _calfile_browse(self):
        """ Event handing for browsing calibrtion file
        """
        fname = self.data_browse_dialog(data_type="*.cal;;*.*")
        if fname:
            self._content.calfile_edit.setText(fname)

        return

    def _charfile_browse(self):
        """ Event handing for browsing calibrtion file
        """
        fname = self.data_browse_dialog("*.txt;;*.*")
        if fname:
            self._content.charfile_edit.setText(fname)

        return

    def _outputdir_browse(self):
        """ Event handling for browing output directory
        """
        dirname = self.dir_browse_dialog()
        if dirname:
            self._content.outputdir_edit.setText(dirname)

        return

    def _binvalue_edit(self):
        """ Handling event for binning value changed
        """
        print "New value...."
        fvalue = float(self._content.binning_edit.text())
        if fvalue < 0.0:
            self._content.bintype_combo.setCurrentIndex(2)
        else:
            self._content.bintype_combo.setCurrentIndex(1)

        return

    def _bintype_process(self):
        """ Handling bin type changed
        """
        currindex = self._content.bintype_combo.currentIndex()
        # print "New Index = %d" % (currindex)

        curbinning = self._content.binning_edit.text()
        if curbinning != "" and curbinning != None:
            curbinning = float(curbinning)
            if currindex == 0:
                self._content.binning_edit.setText(str(abs(curbinning)))
            else:
                self._content.binning_edit.setText(str(-1.0*abs(curbinning)))
            #ENDIFELSE
        #ENDIF

        return

    def validateIntegerList(self, intliststring):
        """ Validate whether the string can be divided into integer strings.
        Allowed: a, b, c-d, e, f
        """
        intliststring = str(intliststring)
        if intliststring == "":
            return (True, "")

        # 1. Split by ","
        termlevel0s = intliststring.split(",")

        # 2. For each term
        for level0term in termlevel0s:
            numdashes = level0term.count("-")
            if numdashes == 0:
                # One integer
                valuestr = level0term.strip()
                try:
                    intvalue = int(valuestr)
                    if str(intvalue) != valuestr:
                        return (False, valuestr)
                except ValueError:
                    return (False, valuestr)

            elif numdashes == 1:
                # Integer range
                twoterms = level0term.split("-")
                for valuestr in twoterms:
                    try:
                        intvalue = int(valuestr)
                        if str(intvalue) != valuestr:
                            return (False, level0term)
                    except ValueError:
                        return (False, level0term)
                # ENDFOR

            else:
                return (False, level0term)
        # ENDFOR

        return (True, "")

    def _overrideemptyrun_clicked(self):
        """ Handling event if overriding emptry run
        """
        if self._content.override_emptyrun_checkBox.isChecked() is True:
            self._content.emptyrun_edit.setEnabled(True)
            self._content.disablebkgdcorr_chkbox.setChecked(False)
        else:
            self._content.emptyrun_edit.setEnabled(False)
            self._content.emptyrun_edit.setText("")

        return

    def _overridevanrun_clicked(self):
        """ Handling event if overriding emptry run
        """
        if self._content.override_vanrun_checkBox.isChecked() is True:
            self._content.vanrun_edit.setEnabled(True)
            self._content.disablebkgdcorr_chkbox.setChecked(False)
        else:
            self._content.emptyrun_edit.setEnabled(False)
            self._content.vanrun_edit.setText("")

        return

    def _overridevanbkgdrun_clicked(self):
        """ Handling event if overriding emptry run
        """
        if self._content.override_vanbkgdrun_checkBox.isChecked() is True:
            self._content.vanbkgdrun_edit.setEnabled(True)
        else:
            self._content.vanbkgdrun_edit.setEnabled(False)
            self._content.vanbkgdrun_edit.setText("")

        return

    def _disablebkgdcorr_clicked(self):
        """ Handling event if disable empty run check box is clicked
        """
        if self._content.disablebkgdcorr_chkbox.isChecked() is True:
            self._content.emptyrun_edit.setEnabled(False)
            self._content.emptyrun_edit.setText("")
            #self._content.override_emptyrun_checkBox.setChecked(False)
        else:
            self._content.emptyrun_edit.setEnabled(True)

        return

    def _disablevancorr_clicked(self):
        """ Handling event if disable empty run check box is clicked
        """
        if self._content.disablevancorr_chkbox.isChecked() is True:
            self._content.vanrun_edit.setEnabled(False)
            self._content.vanrun_edit.setText("")
            #self._content.override_vanrun_checkBox.setChecked(False)
        else:
            self._content.vanrun_edit.setEnabled(True)

        return

    def _disablevanbkgdcorr_clicked(self):
        """ Handling event if disable empty run check box is clicked
        """
        if self._content.disablevanbkgdcorr_chkbox.isChecked() is True:
            self._content.vanbkgdrun_edit.setEnabled(False)
            self._content.vanbkgdrun_edit.setText("")
            #self._content.override_vanbkgdrun_checkBox.setChecked(False)
        else:
            self._content.vanbkgdrun_edit.setEnabled(True)

        return

    def _usebin_clicked(self):
        """ Handling event if 'Binning' button is clicked
        """
        if self._content.usebin_button.isChecked() is True:
            self._content.binning_edit.setEnabled(True)
            self._content.resamplex_edit.setEnabled(False)
        else:
            self._content.binning_edit.setEnabled(False)
            self._content.resamplex_edit.setEnabled(True)

        return

    def _resamplex_clicked(self):
        """ Handling event if 'ResampleX' is clicked
        """
        if self._content.resamplex_button.isChecked() is True:
            self._content.binning_edit.setEnabled(False)
            self._content.resamplex_edit.setEnabled(True)
        else:
            self._content.binning_edit.setEnabled(True)
            self._content.resamplex_edit.setEnabled(False)

        return

    def _show_help(self):
        class HelpDialog(QtGui.QDialog, ui.diffraction.ui_diffraction_info.Ui_Dialog):
            def __init__(self, parent=None):
                QtGui.QDialog.__init__(self, parent)
                self.setupUi(self)
        dialog = HelpDialog(self)
        dialog.exec_()
