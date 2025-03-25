# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
################################################################################
# Advanced Setup Widget
################################################################################
from qtpy.QtWidgets import QDialog, QFrame
from qtpy.QtGui import QDoubleValidator, QIntValidator
from mantidqtinterfaces.reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.diffraction.diffraction_adv_setup_script import AdvancedSetupScript
from mantid.kernel import MaterialBuilder
from mantidqt.interfacemanager import InterfaceManager
import functools

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    from mantid.kernel import Logger

    Logger("AdvancedSetupWidget").information("Using legacy ui importer")
    from mantidplot import load_ui


class AdvancedSetupWidget(BaseWidget):
    """Widget that presents run setup including sample run, optional vanadium run and etc."""

    # Widge name
    name = "Advanced Setup"

    def __init__(self, parent=None, state=None, settings=None, data_type=None):
        """Initialization"""
        super(AdvancedSetupWidget, self).__init__(parent, state, settings, data_type=data_type)

        class AdvancedSetFrame(QFrame):
            """Define class linked to UI Frame"""

            def __init__(self, parent=None):
                QFrame.__init__(self, parent)
                self.ui = load_ui(__file__, "../../../ui/diffraction/diffraction_adv_setup.ui", baseinstance=self)

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
        """Initialize content/UI"""
        # Constraints/Validator
        iv4 = QIntValidator(self._content.maxchunksize_edit)
        iv4.setBottom(0)
        self._content.maxchunksize_edit.setValidator(iv4)

        dv2 = QDoubleValidator(self._content.lowres_edit)
        self._content.lowres_edit.setValidator(dv2)

        dv3 = QDoubleValidator(self._content.cropwavelengthmin_edit)
        dv3.setBottom(0.0)
        self._content.cropwavelengthmin_edit.setValidator(dv3)

        dv3b = QDoubleValidator(self._content.lineEdit_croppedWavelengthMax)
        dv3b.setBottom(0.1)
        self._content.lineEdit_croppedWavelengthMax.setValidator(dv3b)

        dv4 = QDoubleValidator(self._content.removepromptwidth_edit)
        dv4.setBottom(0.0)
        self._content.removepromptwidth_edit.setValidator(dv4)
        self._content.removepromptwidth_edit.setText("50.0")

        dv5 = QDoubleValidator(self._content.vanpeakfwhm_edit)
        dv5.setBottom(0.0)
        self._content.vanpeakfwhm_edit.setValidator(dv5)

        dv6 = QDoubleValidator(self._content.vanpeaktol_edit)
        dv6.setBottom(0.0)
        self._content.vanpeaktol_edit.setValidator(dv6)

        dv7 = QDoubleValidator(self._content.scaledata_edit)
        dv7.setBottom(0.0)
        self._content.scaledata_edit.setValidator(dv7)

        self._content.pushdatapos_combo.activated.connect(self._onlyOneAdditive)

        dv_offsetdata = QDoubleValidator(self._content.offsetdata_edit)
        dv_offsetdata.setBottom(0.0)
        self._content.offsetdata_edit.setValidator(dv_offsetdata)
        self._content.offsetdata_edit.editingFinished.connect(self._onlyOneAdditive)

        dv8 = QDoubleValidator(self._content.numberdensity_edit)
        dv8.setBottom(0.0)
        self._content.numberdensity_edit.setValidator(dv8)
        self._content.numberdensity_edit.editingFinished.connect(self._calculate_packing_fraction)

        dv9 = QDoubleValidator(self._content.massdensity_edit)
        dv9.setBottom(0.0)
        self._content.massdensity_edit.setValidator(dv9)
        self._content.massdensity_edit.editingFinished.connect(self._calculate_packing_fraction)

        self._content.sampleformula_edit.textEdited.connect(self._validate_formula)
        self._content.sampleformula_edit.editingFinished.connect(self._calculate_packing_fraction)

        dv10 = QDoubleValidator(self._content.vanadiumradius_edit)
        dv10.setBottom(0.0)
        self._content.vanadiumradius_edit.setValidator(dv10)

        # Default states
        self._content.stripvanpeaks_chkbox.setChecked(True)
        self._syncStripVanPeakWidgets(True)

        self._content.preserveevents_checkbox.setChecked(True)

        dv8 = QDoubleValidator(self._content.filterbadpulses_edit)
        dv8.setBottom(0.0)
        self._content.filterbadpulses_edit.setValidator(dv8)
        self._content.filterbadpulses_edit.setText("95.")

        # Connections from action/event to function to handle
        self._content.stripvanpeaks_chkbox.clicked.connect(self._stripvanpeaks_clicked)

        self._content.help_button.clicked.connect(self._show_help)
        # Handler for events
        # TODO - Need to add an event handler for the change of instrument and facility

        self._content.material_help_button.clicked.connect(functools.partial(self._show_concept_help, "Materials"))
        self._content.absorption_help_button.clicked.connect(functools.partial(self._show_concept_help, "AbsorptionAndMultipleScattering"))

        # Initialization for Caching options
        self._content.cache_dir_browse_1.clicked.connect(self._cache_dir_browse_1)
        self._content.cache_dir_browse_2.clicked.connect(self._cache_dir_browse_2)
        self._content.cache_dir_browse_3.clicked.connect(self._cache_dir_browse_3)

        # Validated widgets

        return

    def set_state(self, state):
        """Populate the UI elements with the data from the given state.
        @param state: RunSetupScript object
        """

        self._content.pushdatapos_combo.setCurrentIndex(self._content.pushdatapos_combo.findText(state.pushdatapositive))
        if state.offsetdata:
            self._content.offsetdata_edit.setText(str(state.offsetdata).strip())
        else:
            self._content.offsetdata_edit.setText("0")
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
        self._content.vanadiumradius_edit.setText(str(state.vanadiumradius))

        self._content.sampleformula_edit.setText(str(state.sampleformula))
        self._content.numberdensity_edit.setText(str(state.samplenumberdensity))
        self._content.massdensity_edit.setText(str(state.measuredmassdensity))
        if state.samplegeometry:
            self._content.sampleheight_edit.setText(str(state.samplegeometry["Height"]))
        else:
            self._content.sampleheight_edit.setText("")
        self._content.containertype_combo.setCurrentIndex(self._content.containertype_combo.findText(state.containershape))
        self._content.correctiontype_combo.setCurrentIndex(self._content.correctiontype_combo.findText(state.typeofcorrection))

        self._content.preserveevents_checkbox.setChecked(state.preserveevents)
        self._content.outputfileprefix_edit.setText(state.outputfileprefix)

        # range of wavelength
        self._content.cropwavelengthmin_edit.setText(str(state.cropwavelengthmin))
        self._content.lineEdit_croppedWavelengthMax.setText(str(state.cropwavelengthmax))

        # populate Caching options
        self._content.cache_dir_edit_1.setText(state.cache_dir_scan_save)
        self._content.cache_dir_edit_2.setText(state.cache_dir_scan_1)
        self._content.cache_dir_edit_3.setText(state.cache_dir_scan_2)
        self._content.clean_cache_box.setChecked(state.clean_cache)

        return

    def get_state(self):
        """Returns a RunSetupScript with the state of Run_Setup_Interface
        Set up all the class parameters in RunSetupScrpt with values in the content
        """
        s = AdvancedSetupScript(self._instrument_name)

        #
        # Initialize AdvancedSetupScript static variables with widget's content
        #
        s.pushdatapositive = str(self._content.pushdatapos_combo.currentText())
        s.offsetdata = self._content.offsetdata_edit.text().strip()
        s.lowresref = self._content.lowres_edit.text()
        s.cropwavelengthmin = str(self._content.cropwavelengthmin_edit.text())
        s.cropwavelengthmax = str(self._content.lineEdit_croppedWavelengthMax.text())
        s.removepropmppulsewidth = self._content.removepromptwidth_edit.text()
        s.maxchunksize = self._content.maxchunksize_edit.text()
        s.scaledata = self._content.scaledata_edit.text()
        s.filterbadpulses = self._content.filterbadpulses_edit.text()
        s.bkgdsmoothpars = self._content.bkgdsmoothpar_edit.text()

        s.stripvanadiumpeaks = self._content.stripvanpeaks_chkbox.isChecked()
        s.vanadiumfwhm = self._content.vanpeakfwhm_edit.text()
        s.vanadiumpeaktol = self._content.vanpeaktol_edit.text()
        s.vanadiumsmoothparams = self._content.vansmoothpar_edit.text()
        s.vanadiumradius = self._content.vanadiumradius_edit.text()

        s.sampleformula = self._content.sampleformula_edit.text()
        s.samplenumberdensity = self._content.numberdensity_edit.text()
        s.measuredmassdensity = self._content.massdensity_edit.text()
        try:
            s.samplegeometry = {"Height": float(self._content.sampleheight_edit.text())}
        except ValueError:
            s.samplegeometry = {"Height": self._content.sampleheight_edit.text()}
        s.containershape = self._content.containertype_combo.currentText()
        s.typeofcorrection = self._content.correctiontype_combo.currentText()

        s.preserveevents = self._content.preserveevents_checkbox.isChecked()

        s.outputfileprefix = self._content.outputfileprefix_edit.text()
        # Caching options
        s.cache_dir_scan_save = self._content.cache_dir_edit_1.text()
        s.cache_dir_scan_1 = self._content.cache_dir_edit_2.text()
        s.cache_dir_scan_2 = self._content.cache_dir_edit_3.text()
        s.clean_cache = self._content.clean_cache_box.isChecked()

        return s

    def _detinstrumentchange(self):
        """ """
        self._instrument_name = str(self._content.instrument_combo.currentText())

        return

    def _stripvanpeaks_clicked(self):
        """Handling if strip-vanadium-peak check box is clicked"""
        self._syncStripVanPeakWidgets(self._content.stripvanpeaks_chkbox.isChecked())

        return

    def _show_help(self):
        class HelpDialog(QDialog):
            def __init__(self, parent=None):
                QDialog.__init__(self, parent)
                self.ui = load_ui(__file__, "../../../ui/diffraction/diffraction_info.ui", baseinstance=self)

        dialog = HelpDialog(self)
        dialog.exec_()

    def _show_concept_help(self, concept):
        InterfaceManager().showHelpPage(f"qthelp://org.sphinx.mantidproject/doc/concepts/{concept}.html")

    def _syncStripVanPeakWidgets(self, stripvanpeak):
        """Synchronize the other widgets with vanadium peak"""
        self._content.vanpeakfwhm_edit.setEnabled(stripvanpeak)
        self._content.vansmoothpar_edit.setEnabled(stripvanpeak)
        self._content.vanpeaktol_edit.setEnabled(stripvanpeak)

    def _validate_formula(self):
        try:
            if self._content.sampleformula_edit.text().strip() != "":
                MaterialBuilder().setFormula(self._content.sampleformula_edit.text().strip())
            self._content.sampleformula_edit.setToolTip("")
            self._content.sampleformula_edit.setStyleSheet("QLineEdit{}")
        except ValueError as e:
            self._content.sampleformula_edit.setToolTip(str(e).replace("MaterialBuilder::setFormula() - ", ""))
            self._content.sampleformula_edit.setStyleSheet("QLineEdit{background:salmon;}")

    def _onlyOneAdditive(self):
        # enable offsetdata only when pushdatapositive is None
        pushdatapositive = str(self._content.pushdatapos_combo.currentText())
        if pushdatapositive == "None":
            # can edit the offsetdata
            self._content.offsetdata_edit.setEnabled(True)
        else:
            self._content.offsetdata_edit.setEnabled(False)
            self._content.offsetdata_edit.setText("0")

        # enable pushdatapositive only when offsetdata isn't set
        offsetdata_is_set = False
        offsetdata = self._content.offsetdata_edit.text().strip()
        if len(offsetdata) > 0:
            offsetdata_is_set = bool(float(offsetdata) != 0.0)
        self._content.pushdatapos_combo.setEnabled(not offsetdata_is_set)

    def _calculate_packing_fraction(self):
        try:
            self._content.packingfraction_edit.setText(
                "{:.5f}".format(
                    MaterialBuilder()
                    .setFormula(self._content.sampleformula_edit.text())
                    .setNumberDensity(float(self._content.numberdensity_edit.text()))
                    .setMassDensity(float(self._content.massdensity_edit.text()))
                    .build()
                    .packingFraction
                )
            )
        except ValueError:  # boost.python.ArgumentError are not catchable
            self._content.packingfraction_edit.setText(str(1))

    def _cache_dir_browse_1(self):
        r"""Event handling for browsing the cache directory"""
        dir_path = self.dir_browse_dialog(title="Select the Cache Directory (scan&save)")
        if dir_path:
            self._content.cache_dir_edit_1.setText(dir_path)

    def _cache_dir_browse_2(self):
        r"""Event handling for browsing the cache directory"""
        dir_path = self.dir_browse_dialog(title="Select the Cache Directory (scan only)")
        if dir_path:
            self._content.cache_dir_edit_2.setText(dir_path)

    def _cache_dir_browse_3(self):
        r"""Event handling for browsing the cache directory"""
        dir_path = self.dir_browse_dialog(title="Select the Cache Directory (scan only)")
        if dir_path:
            self._content.cache_dir_edit_3.setText(dir_path)


# ENDCLASSDEF
