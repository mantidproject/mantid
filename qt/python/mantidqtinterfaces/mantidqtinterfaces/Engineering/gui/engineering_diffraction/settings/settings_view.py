# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore
from qtpy.QtGui import QIntValidator, QDoubleValidator

from mantidqt.utils.qt import load_ui

Ui_settings, _ = load_ui(__file__, "settings_widget.ui")


class SettingsView(QtWidgets.QDialog, Ui_settings):
    def __init__(self, parent=None):
        super(SettingsView, self).__init__(parent)
        self.setWindowFlags(self.windowFlags() & ~QtCore.Qt.WindowContextHelpButtonHint)
        self.setupUi(self)
        self.setModal(True)
        self.init_tool_tips()

        self.finder_save.setLabelText("Save Location")
        self.finder_save.isForRunFiles(False)
        self.finder_save.isForDirectory(True)

        self.finder_fullCalib.setLabelText("Full Calibration")
        self.finder_fullCalib.isForRunFiles(False)

        self.finder_path_to_gsas2.setLabelText("Path to GSASII")
        self.finder_path_to_gsas2.isForRunFiles(False)
        self.finder_path_to_gsas2.isForDirectory(True)
        self.finder_path_to_gsas2.isOptional(True)

        self.timeout_lineedit.setValidator(QIntValidator(0, 200))
        self.dSpacing_min_lineedit.setValidator(QDoubleValidator(0.0, 200.0, 3))

        # set text of labels
        self.log_list_label.setText("Check logs to average when loading focused data")
        self.primary_log_label.setText(
            "Sort workspaces by selected log average in sequential fitting (default is ascending order)\n"
            "If the box below is empty the workspaces will be fitted in the order they appear in the table."
        )
        self.peak_list_label.setText("Default Peak Function")

        # Setup Tool Tips

    def init_tool_tips(self):
        self.lineedit_RD.setToolTip("Label for the first (PF: in-plane) intrinsic sample direction")
        self.lineedit_RD0.setToolTip("X component of the first intrinsic sample direction")
        self.lineedit_RD1.setToolTip("Y component of the first intrinsic sample direction")
        self.lineedit_RD2.setToolTip("Z component of the first intrinsic sample direction")
        self.lineedit_ND.setToolTip("Label for the second (PF: normal) intrinsic sample direction")
        self.lineedit_ND0.setToolTip("X component of the second intrinsic sample direction")
        self.lineedit_ND1.setToolTip("Y component of the second intrinsic sample direction")
        self.lineedit_ND2.setToolTip("Z component of the second intrinsic sample direction")
        self.lineedit_TD.setToolTip("Label for the third (PF: in-plane) intrinsic sample direction")
        self.lineedit_TD0.setToolTip("X component of the third intrinsic sample direction")
        self.lineedit_TD1.setToolTip("Y component of the third intrinsic sample direction")
        self.lineedit_TD2.setToolTip("Z component of the third intrinsic sample direction")

        self.monte_carlo_lineedit.setToolTip(
            "Python dictionary-style string for input parameters to the MonteCarloAbsorption algorithm, "
            "where keywords and values are given as 'keyword1:value1, keyword2, value2'"
        )
        self.abs_corr_checkBox.setToolTip(
            "Flag for whether the calculated corrected workspaces need to be kept in the ADS "
            "(flagging them for removal frees up system memory)"
        )
        self.eulerAngles_checkBox.setToolTip(
            "Flag for notifying whether the orientation file which will be provided in the correction tab"
            "is a text file with euler angles (checked) "
            "or whether each line is a flattened matrix (unchecked)"
        )
        self.eulerAngles_lineedit.setToolTip(
            "Lab-frame axes that the euler angles are defined along, when in neutral position"
            "- expect a string of axis letters, one per goniometer axis eg. XYX"
        )
        self.eulerAnglesSense_lineedit.setToolTip(
            "The sense of the rotation around each euler axis "
            "where 1 is counter clockwise and -1 is clockwise "
            "- expect string of comma separated values, one value per axis, eg."
            "'1,-1,-1' "
        )

        self.cost_func_thresh_lineedit.setToolTip(
            "The maximum cost function value for a given spectra's fit to be plotted in the pole figure"
        )
        self.peak_pos_thresh_lineedit.setToolTip(
            "The maximum deviation in peak position from the expected position "
            "for a given spectra's fit to be plotted in the pole figure."
            " Expected position is either mean of all peaks or, if provided, peak position of given HKL"
        )
        self.expPF_checkBox.setToolTip(
            "Flag for whether the experimental pole figure should have individual detector groups plotted "
            "as scatter points (checked), or should be interpolated and given as a contour plot (unchecked)"
        )
        self.contourKernel_lineedit.setToolTip("Sigma value of the gaussian smoothing kernel applied before the interpolation")

    # ===============
    # Slot Connectors
    # ===============

    def set_on_apply_clicked(self, slot):
        self.btn_apply.clicked.connect(slot)

    def set_on_ok_clicked(self, slot):
        self.btn_ok.clicked.connect(slot)

    def set_on_cancel_clicked(self, slot):
        self.btn_cancel.clicked.connect(slot)

    def set_on_log_changed(self, slot):
        self.log_list.itemChanged.connect(slot)

    def set_on_check_ascending_changed(self, slot):
        self.check_ascending.stateChanged.connect(slot)

    def set_on_check_descending_changed(self, slot):
        self.check_descending.stateChanged.connect(slot)

    def set_on_gsas2_path_edited(self, slot):
        self.finder_path_to_gsas2.fileEditingFinished.connect(slot)

    # =================
    # Component Getters
    # =================

    def get_save_location(self):
        return self.finder_save.getFirstFilename()

    def get_rd_name(self):
        return self.lineedit_RD.text()

    def get_nd_name(self):
        return self.lineedit_ND.text()

    def get_td_name(self):
        return self.lineedit_TD.text()

    def get_rd_dir(self):
        return ",".join([self.lineedit_RD0.text(), self.lineedit_RD1.text(), self.lineedit_RD2.text()])

    def get_td_dir(self):
        return ",".join([self.lineedit_TD0.text(), self.lineedit_TD1.text(), self.lineedit_TD2.text()])

    def get_nd_dir(self):
        return ",".join([self.lineedit_ND0.text(), self.lineedit_ND1.text(), self.lineedit_ND2.text()])

    def get_full_calibration(self):
        return self.finder_fullCalib.getFirstFilename()

    def get_checked_logs(self):
        return ",".join(
            [
                self.log_list.item(ilog).text()
                for ilog in range(self.log_list.count())
                if self.log_list.item(ilog).checkState() == QtCore.Qt.Checked
            ]
        )

    def get_primary_log(self):
        return self.primary_log.currentText()

    def get_ascending_checked(self):
        return self.check_ascending.isChecked()

    def get_peak_function(self):
        return self.peak_list.currentText()

    def get_path_to_gsas2(self):
        return self.finder_path_to_gsas2.getFirstFilename()

    def get_timeout(self):
        return self.timeout_lineedit.text()

    def get_dSpacing_min(self):
        return self.dSpacing_min_lineedit.text()

    def get_monte_carlo_params(self):
        return self.monte_carlo_lineedit.text()

    def get_remove_corr_ws_after_processing(self):
        return self.abs_corr_checkBox.isChecked()

    def get_cost_func_thresh(self):
        return self.cost_func_thresh_lineedit.text()

    def get_peak_pos_thresh(self):
        return self.peak_pos_thresh_lineedit.text()

    def get_use_euler_angles(self):
        return self.eulerAngles_checkBox.isChecked()

    def get_euler_angles_scheme(self):
        return self.eulerAngles_lineedit.text()

    def get_euler_angles_sense(self):
        return self.eulerAnglesSense_lineedit.text()

    def get_plot_exp_pf(self):
        return self.expPF_checkBox.isChecked()

    def get_contour_kernel(self):
        return self.contourKernel_lineedit.text()

    def get_auto_populate_texture(self):
        return self.textureAutoPopulate.isChecked()

    # =================
    # Component Setters
    # =================

    def set_save_location(self, text):
        self.finder_save.setText(text)

    def set_rd_name(self, text):
        self.lineedit_RD.setText(text)

    def set_nd_name(self, text):
        self.lineedit_ND.setText(text)

    def set_td_name(self, text):
        self.lineedit_TD.setText(text)

    def set_rd_dir(self, text):
        vec = text.split(",")
        self.lineedit_RD0.setText(vec[0])
        self.lineedit_RD1.setText(vec[1])
        self.lineedit_RD2.setText(vec[2])

    def set_td_dir(self, text):
        vec = text.split(",")
        self.lineedit_TD0.setText(vec[0])
        self.lineedit_TD1.setText(vec[1])
        self.lineedit_TD2.setText(vec[2])

    def set_nd_dir(self, text):
        vec = text.split(",")
        self.lineedit_ND0.setText(vec[0])
        self.lineedit_ND1.setText(vec[1])
        self.lineedit_ND2.setText(vec[2])

    def set_full_calibration(self, text):
        self.finder_fullCalib.setText(text)

    def set_van_recalc(self, checked):
        self.check_vanRecalc.setChecked(checked)

    def add_log_checkboxs(self, logs):
        for log in logs.split(","):
            item = QtWidgets.QListWidgetItem(self.log_list)
            item.setText(log)
            item.setCheckState(QtCore.Qt.Unchecked)
            self.log_list.addItem(item)

    def set_checked_logs(self, logs):
        # block signal so as not to reset primary log
        self.log_list.blockSignals(True)
        for log in logs.split(","):
            items = self.log_list.findItems(log, QtCore.Qt.MatchExactly)
            items[0].setCheckState(QtCore.Qt.Checked)
        self.log_list.blockSignals(False)

    def set_primary_log_combobox(self, primary_log):
        checked_logs = self.get_checked_logs().split(",") + [""]
        self.primary_log.clear()
        self.primary_log.addItems(checked_logs)
        if primary_log in checked_logs:
            self.primary_log.setCurrentText(primary_log)
        else:
            self.primary_log.setCurrentText("")

    def set_ascending_checked(self, checked):
        self.check_ascending.setChecked(checked)

    def set_descending_checked(self, checked):
        self.check_descending.setChecked(checked)

    def set_peak_function(self, peak_name):
        self.peak_list.setCurrentText(peak_name)

    def populate_peak_function_list(self, peak_names):
        self.peak_list.addItems(peak_names.split(","))

    def set_path_to_gsas2(self, text):
        self.finder_path_to_gsas2.setText(text)

    def set_timeout(self, text):
        self.timeout_lineedit.setText(text)

    def set_dSpacing_min(self, text):
        self.dSpacing_min_lineedit.setText(text)

    def set_monte_carlo_params(self, text):
        self.monte_carlo_lineedit.setText(text)

    def set_remove_corr_ws_after_processing(self, val):
        self.abs_corr_checkBox.setChecked(val)

    def set_cost_func_thresh(self, text):
        self.cost_func_thresh_lineedit.setText(text)

    def set_peak_pos_thresh(self, text):
        self.peak_pos_thresh_lineedit.setText(text)

    def set_use_euler_angles(self, val):
        self.eulerAngles_checkBox.setChecked(val)

    def set_euler_angles_scheme(self, text):
        self.eulerAngles_lineedit.setText(text)

    def set_euler_angles_sense(self, text):
        self.eulerAnglesSense_lineedit.setText(text)

    def set_plot_exp_pf(self, val):
        self.expPF_checkBox.setChecked(val)

    def set_contour_kernel(self, text):
        self.contourKernel_lineedit.setText(text)

    def set_auto_populate_texture(self, val):
        self.textureAutoPopulate.setChecked(val)

    # =================
    # Force Actions
    # =================

    def find_full_calibration(self):
        self.finder_fullCalib.findFiles(True)

    def find_save(self):
        self.finder_save.findFiles(True)

    def find_path_to_gsas2(self):
        self.finder_path_to_gsas2.findFiles(True)

    # ======================
    # Toggle Active Options
    # ======================

    def on_orientation_type_toggled(self, slot):
        self.eulerAngles_checkBox.toggled.connect(slot)

    def on_scatter_pf_toggled(self, slot):
        self.expPF_checkBox.toggled.connect(slot)
