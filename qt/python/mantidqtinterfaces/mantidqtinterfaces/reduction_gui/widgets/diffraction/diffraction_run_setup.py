# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
################################################################################
# This is my first attempt to make a tab from quasi-scratch
################################################################################
from qtpy.QtWidgets import QDialog, QFrame
from qtpy.QtCore import QRegExp
from qtpy.QtGui import QDoubleValidator, QIntValidator, QRegExpValidator
from mantidqtinterfaces.reduction_gui.widgets.base_widget import BaseWidget
from mantid.kernel import Logger
from reduction_gui.reduction.diffraction.diffraction_run_setup_script import RunSetupScript

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("SampleSetupWidget").information("Using legacy ui importer")
    from mantidplot import load_ui


def generateRegExpValidator(widget, expression):
    rx = QRegExp(expression)
    return QRegExpValidator(rx, widget)


class RunSetupWidget(BaseWidget):
    """Widget that presents run setup including sample run, optional vanadium run and etc."""

    # Widge name
    name = "Experiment Run Setup"

    def __init__(self, parent=None, state=None, settings=None, data_type=None):
        """Initialization"""
        super(RunSetupWidget, self).__init__(parent, state, settings, data_type=data_type)

        class RunSetFrame(QFrame):
            """Define class linked to UI Frame"""

            def __init__(self, parent=None):
                QFrame.__init__(self, parent)
                self.ui = load_ui(__file__, "../../../ui/diffraction/diffraction_run_setup.ui", baseinstance=self)

        # END-DEF RunSetFrame

        # Instrument and facility information
        self._instrument_name = settings.instrument_name
        self._facility_name = settings.facility_name

        msg = "run_setup: facility = %s instrument = % s" % (self._facility_name, self._instrument_name)
        Logger("RunSetupWidget").debug(str(msg))

        self._content = RunSetFrame(self)
        self._layout.addWidget(self._content)

        self.initialize_content()

        if state is not None:
            self.set_state(state)
        else:
            self.set_state(RunSetupScript(self._instrument_name))

        return

    def initialize_content(self):
        """Initialize content/UI"""
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
        self._content.interpolateenable_chkbox.setChecked(False)

        # Label
        if self._instrument_name is not False:
            self._content.label.setText("Instrument: %s" % self._instrument_name.upper())
        else:
            self._content.label.setText("Instrument has not been set up.  You may not launch correctly.")

        # Enable disable
        if self._instrument_name.lower().startswith("nom") is False:
            self._content.lineEdit_expIniFile.setEnabled(False)
            self._content.pushButton_browseExpIniFile.setEnabled(False)
        else:
            self._content.lineEdit_expIniFile.setEnabled(True)
            self._content.pushButton_browseExpIniFile.setEnabled(True)

        # Line edit
        self._content.emptyrun_edit.setEnabled(True)
        self._content.vanrun_edit.setEnabled(True)
        self._content.vanbkgdrun_edit.setEnabled(True)
        self._content.resamplex_edit.setEnabled(False)
        self._content.interpolatetemp_edit.setEnabled(False)

        # Constraints/Validator
        expression = r"[\d,-]*"
        iv0 = generateRegExpValidator(self._content.emptyrun_edit, expression)
        self._content.emptyrun_edit.setValidator(iv0)

        iv1 = generateRegExpValidator(self._content.vanrun_edit, expression)
        self._content.vanrun_edit.setValidator(iv1)

        iv3 = generateRegExpValidator(self._content.vanbkgdrun_edit, expression)
        self._content.vanbkgdrun_edit.setValidator(iv3)

        siv = QIntValidator(self._content.resamplex_edit)
        siv.setBottom(0)
        self._content.resamplex_edit.setValidator(siv)

        iv4 = QDoubleValidator(self._content.interpolatetemp_edit)
        iv4.setBottom(0)
        self._content.interpolatetemp_edit.setValidator(iv4)

        # Float/Double
        fiv = QDoubleValidator(self._content.binning_edit)
        self._content.binning_edit.setValidator(fiv)

        # Default states
        # self._handle_tzero_guess(self._content.use_ei_guess_chkbox.isChecked())

        # Connections from action/event to function to handle
        self._content.calfile_browse.clicked.connect(self._calfile_browse)
        self._content.charfile_browse.clicked.connect(self._charfile_browse)
        self._content.groupfile_browse.clicked.connect(self._groupfile_browse)
        self._content.pushButton_browseExpIniFile.clicked.connect(self.do_browse_ini_file)
        self._content.outputdir_browse.clicked.connect(self._outputdir_browse)
        self._content.binning_edit.textChanged.connect(self._binvalue_edit)
        self._content.bintype_combo.currentIndexChanged.connect(self._bintype_process)

        self._content.disablebkgdcorr_chkbox.clicked.connect(self._disablebkgdcorr_clicked)
        self._content.disablevancorr_chkbox.clicked.connect(self._disablevancorr_clicked)
        self._content.disablevanbkgdcorr_chkbox.clicked.connect(self._disablevanbkgdcorr_clicked)

        self._content.usebin_button.clicked.connect(self._usebin_clicked)
        self._content.resamplex_button.clicked.connect(self._resamplex_clicked)
        self._content.interpolateenable_chkbox.clicked.connect(self._enableinterpo_clicked)
        self._content.help_button.clicked.connect(self._show_help)

        # mutex for whether to update the linear/log drop-down
        self._content._binning_edit_mutex = False

    def set_state(self, state):
        """Populate the UI elements with the data from the given state.
        @param state: RunSetupScript object
        """
        self._content.runnumbers_edit.setText(state.runnumbers)
        self._content.runnumbers_edit.setValidator(generateRegExpValidator(self._content.runnumbers_edit, r"[\d,-:]*"))

        self._content.calfile_edit.setText(state.calibfilename)
        self._content.groupfile_edit.setText(state.groupfilename)
        self._content.lineEdit_expIniFile.setText(state.exp_ini_file_name)
        self._content.charfile_edit.setText(state.charfilename)
        self._content.sum_checkbox.setChecked(state.dosum)

        # Set binning parameter
        try:
            binning_float = float(state.binning)
            binning_str = "%.6f" % abs(binning_float)
        except ValueError:
            binning_str = str(state.binning)
        self._content._binning_edit_mutex = True
        self._content.binning_edit.setText(binning_str)
        # Set ResampleX
        try:
            resamplex_i = int(state.resamplex)
            resamplex_str = "%d" % abs(resamplex_i)
        except ValueError:
            resamplex_str = str(state.resamplex)
        self._content.resamplex_edit.setText(resamplex_str)
        self._content._binning_edit_mutex = False

        # Set binning type (logarithm=1 or linear=0) - must be done after the
        # binning/resamplex boxes are set or it will be lost
        bintype_index = 1
        if state.doresamplex is True:
            # resample x
            self._content.resamplex_button.setChecked(True)
            self._content.binning_edit.setEnabled(False)
            self._content.resamplex_edit.setEnabled(True)
            if state.resamplex > 0:
                bintype_index = 0
        else:
            # binning
            self._content.usebin_button.setChecked(True)
            self._content.binning_edit.setEnabled(True)
            self._content.resamplex_edit.setEnabled(False)
            if state.binning > 0.0:
                bintype_index = 0
        # END-IF-ELSE
        self._content.bintype_combo.setCurrentIndex(bintype_index)

        # Others
        self._content.binind_checkbox.setChecked(state.binindspace)
        self._content.outputdir_edit.setText(state.outputdir)
        self._content.saveas_combo.setCurrentIndex(self._content.saveas_combo.findText(state.saveas))
        self._content.unit_combo.setCurrentIndex(self._content.unit_combo.findText(state.finalunits))

        # Background correction
        if state.bkgdrunnumber is not None and state.bkgdrunnumber != "":
            self._content.emptyrun_edit.setText(state.bkgdrunnumber)
        self._content.disablebkgdcorr_chkbox.setChecked(state.disablebkgdcorrection)
        # Vanadium correction
        if state.vanrunnumber is not None and state.vanrunnumber != "":
            self._content.vanrun_edit.setText(state.vanrunnumber)
        self._content.disablevancorr_chkbox.setChecked(state.disablevancorrection)
        # Vanadium background correction
        if state.vanbkgdrunnumber is not None and state.vanbkgdrunnumber != "":
            self._content.vanbkgdrun_edit.setText(state.vanbkgdrunnumber)
        self._content.disablevanbkgdcorr_chkbox.setChecked(state.disablevanbkgdcorrection)
        # Interpolate background
        self._content.interpolateenable_chkbox.setChecked(state.enableinterpolate)
        if state.interpolatetemp is not None and self._content.interpolateenable_chkbox.isChecked() is True:
            self._content.interpolatetemp_edit.setEnabled(True)
            self._content.interpolatetemp_edit.setText(str(state.interpolatetemp))
        # self._content.vannoiserun_edit.setText(str(state.vannoiserunnumber))
        # if state.vanrunnumber < 0:
        #     self._content.disablevancorr_chkbox.setChecked(True)
        # else:
        #     self._content.disablevancorr_chkbox.setChecked(False)

        return

    def get_state(self):
        """Returns a RunSetupScript with the state of Run_Setup_Interface
        Set up all the class parameters in RunSetupScrpt with values in the content
        i.e., get the all parameters from GUI
        """
        s = RunSetupScript(self._instrument_name)

        s.runnumbers = str(self._content.runnumbers_edit.text())
        rtup = self.validateIntegerList(s.runnumbers)
        isvalid = rtup[0]
        if isvalid is False:
            raise NotImplementedError("Run number error @ %s" % (rtup[1]))
        else:
            # s.runnumbers = rtup[2]
            pass

        s.calibfilename = self._content.calfile_edit.text()
        s.groupfilename = self._content.groupfile_edit.text()
        s.exp_ini_file_name = str(self._content.lineEdit_expIniFile.text())
        s.charfilename = self._content.charfile_edit.text()
        s.dosum = self._content.sum_checkbox.isChecked()

        bintypestr = str(self._content.bintype_combo.currentText())
        s.binindspace = self._content.binind_checkbox.isChecked()

        if self._content.resamplex_button.isChecked() is True:
            # use ResampleX: do not touch pre-saved s.binning
            s.doresamplex = True
            try:
                s.resamplex = int(self._content.resamplex_edit.text())
            except ValueError:
                raise RuntimeError("ResampleX parameter is not given!")

            if s.resamplex < 0 and bintypestr.startswith("Linear"):
                self._content.bintype_combo.setCurrentIndex(1)
            elif s.resamplex > 0 and bintypestr.startswith("Logarithmic"):
                s.resamplex = -1 * s.resamplex
            elif s.resamplex == 0:
                raise RuntimeError("ResampleX's parameter cannot be equal to 0!")
        else:
            # use binning: do not touch pre-saved s.resamplex
            s.doresamplex = False
            try:
                s.binning = float(self._content.binning_edit.text())
            except ValueError:
                raise RuntimeError("Binning parameter is not given!")

            if s.binning < 0.0 and bintypestr.startswith("Linear"):
                self._content.bintype_combo.setCurrentIndex(1)
            elif s.binning > 0.0 and bintypestr.startswith("Logarithmic"):
                s.binning = -1 * s.binning
            elif abs(s.binning) < 1.0e-20:
                raise RuntimeError("Binning's parameter cannot be equal to 0!")
        # END-IF-ELSE (binning/resampleX)

        s.outputdir = self._content.outputdir_edit.text()
        s.saveas = str(self._content.saveas_combo.currentText())
        s.finalunits = str(self._content.unit_combo.currentText())

        s.bkgdrunnumber = self._content.emptyrun_edit.text()
        s.disablebkgdcorrection = self._content.disablebkgdcorr_chkbox.isChecked()

        s.vanrunnumber = self._content.vanrun_edit.text()
        s.disablevancorrection = self._content.disablevancorr_chkbox.isChecked()

        s.vanbkgdrunnumber = self._content.vanbkgdrun_edit.text()
        s.disablevanbkgdcorrection = self._content.disablevanbkgdcorr_chkbox.isChecked()

        s.interpolatetemp = self._content.interpolatetemp_edit.text()
        s.enableinterpolate = self._content.interpolateenable_chkbox.isChecked()

        return s

    def _calfile_browse(self):
        """Event handing for browsing calibration file"""
        fname = self.data_browse_dialog(data_type="*.h5;;*.cal;;*.hd5;;*.hdf;;*")
        if fname:
            self._content.calfile_edit.setText(fname)

    def _charfile_browse(self):
        """Event handing for browsing calibration file"""
        fname = self.data_browse_dialog("*.txt;;*", multi=True)
        if fname:
            self._content.charfile_edit.setText(",".join(fname))

    def _groupfile_browse(self):
        """Event handling for browsing for a grouping file"""
        fname = self.data_browse_dialog(data_type="*.xml;;*.h5;;*")
        if fname:
            self._content.groupfile_edit.setText(fname)

    def do_browse_ini_file(self):
        """Event handling for browsing Exp Ini file
        :return:
        """
        exp_ini_file_name = self.data_browse_dialog(data_type="*.ini;;*")
        if exp_ini_file_name:
            self._content.lineEdit_expIniFile.setText(exp_ini_file_name)

    def _outputdir_browse(self):
        """Event handling for browing output directory"""
        dirname = self.dir_browse_dialog()
        if dirname:
            self._content.outputdir_edit.setText(dirname)

    def _binvalue_edit(self):
        """Handling event for binning value changed"""
        if self._content.resamplex_button.isChecked():
            fvalue = int(self._content.resamplex_edit.text())
        else:
            fvalue = float(self._content.binning_edit.text())
        if fvalue < 0:
            self._content.bintype_combo.setCurrentIndex(1)
        else:
            self._content.bintype_combo.setCurrentIndex(0)

    def _bintype_process(self):
        """Handling bin type changed"""
        if self._content._binning_edit_mutex:
            return
        currindex = self._content.bintype_combo.currentIndex()
        curbinning = self._content.binning_edit.text()
        if curbinning != "" and curbinning is not None:
            curbinning = float(curbinning)
            if currindex == 0:
                self._content.binning_edit.setText(str(abs(curbinning)))
            else:
                self._content.binning_edit.setText(str(-1.0 * abs(curbinning)))
            # ENDIFELSE
        # ENDIF

    def validateIntegerList(self, intliststring):
        """Validate whether the string can be divided into integer strings.
        Allowed: a, b, c-d, e, f, g:h
        and replace ':' by ':'
        :return: 3-tuple: state/error message/new integer list string
        """
        intliststring = str(intliststring)
        if intliststring == "":
            return True, "", intliststring

        # replace ':' by '-'
        intliststring = intliststring.replace(":", "-")

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
                        err_msg = "String {0} cannot be converted to an integer properly.".format(valuestr)
                        return False, err_msg, ""
                except ValueError:
                    err_msg = "String {0} cannot be converted to an integer.".format(valuestr)
                    return False, err_msg, ""

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
                return False, level0term, intliststring
        # ENDFOR

        return True, "", intliststring

    def _overrideemptyrun_clicked(self):
        """Handling event if overriding empty run"""
        if self._content.override_emptyrun_checkBox.isChecked() is True:
            self._content.emptyrun_edit.setEnabled(True)
            self._content.disablebkgdcorr_chkbox.setChecked(False)
        else:
            self._content.emptyrun_edit.setEnabled(False)
            self._content.emptyrun_edit.setText("")

        return

    def _overridevanrun_clicked(self):
        """Handling event if overriding empty run"""
        if self._content.override_vanrun_checkBox.isChecked() is True:
            self._content.vanrun_edit.setEnabled(True)
            self._content.disablebkgdcorr_chkbox.setChecked(False)
        else:
            self._content.emptyrun_edit.setEnabled(False)
            self._content.vanrun_edit.setText("")

        return

    def _overridevanbkgdrun_clicked(self):
        """Handling event if overriding empty run"""
        if self._content.override_vanbkgdrun_checkBox.isChecked() is True:
            self._content.vanbkgdrun_edit.setEnabled(True)
        else:
            self._content.vanbkgdrun_edit.setEnabled(False)
            self._content.vanbkgdrun_edit.setText("")

        return

    def _disablebkgdcorr_clicked(self):
        """Handling event if disable empty run check box is clicked"""
        if self._content.disablebkgdcorr_chkbox.isChecked() is True:
            self._content.emptyrun_edit.setEnabled(False)
            self._content.emptyrun_edit.setText("")
            # self._content.override_emptyrun_checkBox.setChecked(False)
        else:
            self._content.emptyrun_edit.setEnabled(True)

        return

    def _disablevancorr_clicked(self):
        """Handling event if disable empty run check box is clicked"""
        if self._content.disablevancorr_chkbox.isChecked() is True:
            self._content.vanrun_edit.setEnabled(False)
            self._content.vanrun_edit.setText("")
            # self._content.override_vanrun_checkBox.setChecked(False)
        else:
            self._content.vanrun_edit.setEnabled(True)

        return

    def _disablevanbkgdcorr_clicked(self):
        """Handling event if disable empty run check box is clicked"""
        if self._content.disablevanbkgdcorr_chkbox.isChecked() is True:
            self._content.vanbkgdrun_edit.setEnabled(False)
            self._content.vanbkgdrun_edit.setText("")
            # self._content.override_vanbkgdrun_checkBox.setChecked(False)
        else:
            self._content.vanbkgdrun_edit.setEnabled(True)

        return

    def _usebin_clicked(self):
        """Handling event if 'Binning' button is clicked"""
        if self._content.usebin_button.isChecked() is True:
            self._content.binning_edit.setEnabled(True)
            self._content.resamplex_edit.setEnabled(False)
        else:
            self._content.binning_edit.setEnabled(False)
            self._content.resamplex_edit.setEnabled(True)

        return

    def _resamplex_clicked(self):
        """Handling event if 'ResampleX' is clicked"""
        if self._content.resamplex_button.isChecked() is True:
            self._content.binning_edit.setEnabled(False)
            self._content.resamplex_edit.setEnabled(True)
        else:
            self._content.binning_edit.setEnabled(True)
            self._content.resamplex_edit.setEnabled(False)

        return

    def _enableinterpo_clicked(self):
        """Handling event if 'Interpo Empty' is clicked"""
        if self._content.interpolateenable_chkbox.isChecked() is True:
            self._content.interpolatetemp_edit.setEnabled(True)
        else:
            self._content.interpolatetemp_edit.setEnabled(False)
            self._content.interpolatetemp_edit.setText("")
        return

    def _show_help(self):
        class HelpDialog(QDialog):
            def __init__(self, parent=None):
                QDialog.__init__(self, parent)
                self.ui = load_ui(__file__, "../../../ui/diffraction/diffraction_info.ui", baseinstance=self)

        dialog = HelpDialog(self)
        dialog.exec_()
