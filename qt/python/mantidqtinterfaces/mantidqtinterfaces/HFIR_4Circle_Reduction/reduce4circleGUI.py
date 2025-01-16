# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,relative-import,W0611,R0921,R0902,R0904,R0921,C0302,R0912
# ruff: noqa: E741  # Ambiguous variable name
################################################################################
#
# MainWindow application for reducing HFIR 4-circle
#
################################################################################
import os
import sys
import csv
import time
import datetime
import random
import numpy
from mantidqtinterfaces.HFIR_4Circle_Reduction import reduce4circleControl as r4c
import mantidqtinterfaces.HFIR_4Circle_Reduction.guiutility as gutil
import mantidqtinterfaces.HFIR_4Circle_Reduction.peakprocesshelper as peak_util
import mantidqtinterfaces.HFIR_4Circle_Reduction.fourcircle_utility as hb3a_util
from mantidqtinterfaces.HFIR_4Circle_Reduction import plot3dwindow
import mantidqtinterfaces.HFIR_4Circle_Reduction.multi_threads_helpers as thread_pool
import mantidqtinterfaces.HFIR_4Circle_Reduction.optimizelatticewindow as ol_window
from mantidqtinterfaces.HFIR_4Circle_Reduction import viewspicedialog
from mantidqtinterfaces.HFIR_4Circle_Reduction import peak_integration_utility
from mantidqtinterfaces.HFIR_4Circle_Reduction import FindUBUtility
from mantidqtinterfaces.HFIR_4Circle_Reduction import message_dialog
from mantidqtinterfaces.HFIR_4Circle_Reduction import PreprocessWindow
from mantidqtinterfaces.HFIR_4Circle_Reduction.downloaddialog import DataDownloadDialog
import mantidqtinterfaces.HFIR_4Circle_Reduction.refineubfftsetup as refineubfftsetup
import mantidqtinterfaces.HFIR_4Circle_Reduction.PeaksIntegrationReport as PeaksIntegrationReport
import mantidqtinterfaces.HFIR_4Circle_Reduction.IntegrateSingePtSubWindow as IntegrateSingePtSubWindow
import mantidqtinterfaces.HFIR_4Circle_Reduction.generalplotview as generalplotview

# import line for the UI python class
from mantidqtinterfaces.HFIR_4Circle_Reduction.hfctables import UBMatrixPeakTable
from mantidqtinterfaces.HFIR_4Circle_Reduction.hfctables import UBMatrixTable
from mantidqtinterfaces.HFIR_4Circle_Reduction.hfctables import ProcessTableWidget
from mantidqtinterfaces.HFIR_4Circle_Reduction.hfctables import ScanSurveyTable
from mantidqtinterfaces.HFIR_4Circle_Reduction.integratedpeakview import IntegratedPeakView
from mantidqtinterfaces.HFIR_4Circle_Reduction.detector2dview import Detector2DView
from mantidqtinterfaces.HFIR_4Circle_Reduction.hfctables import KShiftTableWidget
from mantidqtinterfaces.HFIR_4Circle_Reduction.hfctables import MatrixTable
from mantid.kernel import Logger
from qtpy.QtWidgets import QButtonGroup, QFileDialog, QMessageBox, QMainWindow, QInputDialog
from qtpy.QtCore import QSettings

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("HFIR_4Circle_Reduction").information("Using legacy ui importer")
    from mantidplot import load_ui
from qtpy.QtWidgets import QVBoxLayout

unicode = str

# define constants
IndexFromSpice = "From Spice (pre-defined)"
IndexFromUB = "From Calculation By UB"
MAGNETIC_TOL = 0.2


class MainWindow(QMainWindow):
    """Class of Main Window (top)"""

    TabPage = {"View Raw Data": 2, "Calculate UB": 3, "UB Matrix": 4, "Peak Integration": 6, "Scans Processing": 5}

    def __init__(self, parent=None, window_flags=None):
        """Initialization and set up"""
        # Base class
        QMainWindow.__init__(self, parent)

        if window_flags:
            self.setWindowFlags(window_flags)

        # UI Window (from Qt Designer)
        ui_path = "MainWindow.ui"
        self.ui = load_ui(__file__, ui_path, baseinstance=self)
        self._promote_widgets()

        # children windows
        self._my3DWindow = None
        self._refineConfigWindow = None
        self._peakIntegrationInfoWindow = None
        self._addUBPeaksDialog = None
        self._spiceViewer = None
        self._mySinglePeakIntegrationDialog = None
        self._preProcessWindow = None
        self._singlePeakIntegrationDialogBuffer = ""
        self._dataDownloadDialog = None
        self._single_pt_peak_integration_window = None
        self._general_1d_plot_window = None

        self._init_widgets()

        # thread
        self._myIntegratePeaksThread = None
        self._addUBPeaksThread = None

        # Mantid configuration
        self._instrument = str(self.ui.comboBox_instrument.currentText())
        # config = ConfigService.Instance()
        # self._instrument = config["default.instrument"]

        # Event handling definitions
        # Top
        self.ui.pushButton_setExp.clicked.connect(self.do_set_experiment)

        # Tab 'Data Access'
        self.ui.pushButton_applySetup.clicked.connect(self.do_apply_setup)
        self.ui.pushButton_browseLocalDataDir.clicked.connect(self.do_browse_local_spice_data)
        self.ui.pushButton_applyCalibratedSampleDistance.clicked.connect(self.do_set_user_detector_distance)
        self.ui.pushButton_applyUserDetCenter.clicked.connect(self.do_set_user_detector_center)
        self.ui.pushButton_applyUserWavelength.clicked.connect(self.do_set_user_wave_length)
        self.ui.pushButton_applyDetectorSize.clicked.connect(self.do_set_detector_size)

        # Tab survey
        self.ui.pushButton_survey.clicked.connect(self.do_survey)
        self.ui.pushButton_saveSurvey.clicked.connect(self.do_save_survey)
        self.ui.pushButton_loadSurvey.clicked.connect(self.do_load_survey)
        self.ui.pushButton_viewSurveyPeak.clicked.connect(self.do_view_survey_peak)
        self.ui.pushButton_addPeaksToRefine.clicked.connect(self.do_add_peaks_for_ub)
        self.ui.pushButton_mergeScansSurvey.clicked.connect(self.do_merge_scans_survey)
        self.ui.pushButton_selectAllSurveyPeaks.clicked.connect(self.do_select_all_survey)
        self.ui.pushButton_sortInfoTable.clicked.connect(self.do_filter_sort_survey_table)
        self.ui.pushButton_clearSurvey.clicked.connect(self.do_clear_survey)
        self.ui.pushButton_viewRawSpice.clicked.connect(self.do_show_spice_file)

        # Tab 'View Raw Data'
        self.ui.pushButton_setScanInfo.clicked.connect(self.do_load_scan_info)
        self.ui.pushButton_plotRawPt.clicked.connect(self.do_plot_pt_raw)
        self.ui.pushButton_prevPtNumber.clicked.connect(self.do_plot_prev_pt_raw)
        self.ui.pushButton_nextPtNumber.clicked.connect(self.do_plot_next_pt_raw)
        self.ui.pushButton_showPtList.clicked.connect(self.show_scan_pt_list)
        self.ui.pushButton_showSPICEinRaw.clicked.connect(self.do_show_spice_file_raw)
        self.ui.pushButton_switchROIMode.clicked.connect(self.do_switch_roi_mode)
        self.ui.pushButton_removeROICanvas.clicked.connect(self.do_del_roi)
        self.ui.pushButton_nextScanNumber.clicked.connect(self.do_plot_next_scan)
        self.ui.pushButton_prevScanNumber.clicked.connect(self.do_plot_prev_scan)
        self.ui.pushButton_maskScanPt.clicked.connect(self.do_mask_pt_2d)
        self.ui.pushButton_integrateROI.clicked.connect(self.do_integrate_roi)
        self.ui.pushButton_exportMaskToFile.clicked.connect(self.do_export_mask)

        # Tab 'calculate ub matrix'
        self.ui.pushButton_addUBScans.clicked.connect(self.do_add_ub_peaks)
        self.ui.pushButton_calUB.clicked.connect(self.do_cal_ub_matrix)
        self.ui.pushButton_acceptUB.clicked.connect(self.do_accept_ub)
        self.ui.pushButton_indexUBPeaks.clicked.connect(self.do_index_ub_peaks)
        self.ui.pushButton_deleteUBPeak.clicked.connect(self.do_del_ub_peaks)
        self.ui.pushButton_clearUBPeakTable.clicked.connect(self.do_clear_ub_peaks)
        self.ui.pushButton_resetPeakHKLs.clicked.connect(self.do_reset_ub_peaks_hkl)
        self.ui.pushButton_viewScan3D.clicked.connect(self.do_view_data_3d)
        self.ui.pushButton_plotSelectedData.clicked.connect(self.do_view_data_set_3d)
        self.ui.pushButton_setHKL2Int.clicked.connect(self.do_set_ub_tab_hkl_to_integers)
        self.ui.pushButton_undoSetToInteger.clicked.connect(self.do_undo_ub_tab_hkl_to_integers)
        self.ui.pushButton_clearIndexing.clicked.connect(self.do_clear_all_peaks_index_ub)

        self.ui.pushButton_refineUB.clicked.connect(self.do_refine_ub_indexed_peaks)
        self.ui.pushButton_refineUBCalIndex.clicked.connect(self.do_refine_ub_cal_indexed_peaks)

        self.ui.pushButton_refineUBFFT.clicked.connect(self.do_refine_ub_fft)
        self.ui.pushButton_findUBLattice.clicked.connect(self.do_refine_ub_lattice)

        self.ui.radioButton_ubAdvancedSelection.toggled.connect(self.do_select_all_peaks)
        self.ui.radioButton_ubSelectAllScans.toggled.connect(self.do_select_all_peaks)
        self.ui.radioButton_ubSelectNoScan.toggled.connect(self.do_select_all_peaks)

        # Tab 'Setup'
        self.ui.pushButton_browseWorkDir.clicked.connect(self.do_browse_working_dir)
        self.ui.comboBox_instrument.currentIndexChanged.connect(self.do_change_instrument_name)
        self.ui.pushButton_browsePreprocessed.clicked.connect(self.do_browse_preprocessed_dir)

        # Tab 'UB Matrix'
        self.ui.pushButton_showUB2Edit.clicked.connect(self.do_show_ub_in_box)
        self.ui.pushButton_syncUB.clicked.connect(self.do_sync_ub)
        self.ui.pushButton_saveUB.clicked.connect(self.do_save_ub)

        # Tab 'Scans Processing'
        self.ui.pushButton_addScanSliceView.clicked.connect(self.do_add_scans_merge)
        self.ui.pushButton_mergeScans.clicked.connect(self.do_merge_scans)
        self.ui.pushButton_integratePeaks.clicked.connect(self.do_integrate_peaks)
        self.ui.pushButton_setupPeakIntegration.clicked.connect(self.do_switch_tab_peak_int)
        self.ui.pushButton_refreshMerged.clicked.connect(self.do_refresh_merged_scans_table)
        self.ui.pushButton_plotMergedScans.clicked.connect(self.do_view_merged_scans_3d)
        self.ui.pushButton_showUB.clicked.connect(self.do_view_ub)
        self.ui.pushButton_exportPeaks.clicked.connect(self.do_export_to_fp)
        self.ui.pushButton_selectAllScans2Merge.clicked.connect(self.do_select_merged_scans)
        self.ui.pushButton_indexMergedScans.clicked.connect(self.do_index_merged_scans_peaks)
        self.ui.pushButton_applyKShift.clicked.connect(self.do_apply_k_shift)
        self.ui.pushButton_clearMergeScanTable.clicked.connect(self.do_clear_merge_table)
        self.ui.pushButton_multipleScans.clicked.connect(self.do_merge_multi_scans)
        self.ui.pushButton_convertMerged2HKL.clicked.connect(self.do_convert_merged_to_hkl)
        self.ui.pushButton_showScanWSInfo.clicked.connect(self.do_show_workspaces)
        self.ui.pushButton_showIntegrateDetails.clicked.connect(self.do_show_integration_details)
        self.ui.pushButton_toggleIntegrateType.clicked.connect(self.do_toggle_table_integration)
        self.ui.pushButton_exportSelectedPeaks.clicked.connect(self.do_export_selected_peaks_to_integrate)

        # Tab 'Integrate (single) Peaks'
        self.ui.pushButton_integratePt.clicked.connect(self.do_integrate_single_scan)
        self.ui.comboBox_ptCountType.currentIndexChanged.connect(self.evt_change_normalization)

        # calculate the normalized data again
        self.ui.pushButton_showIntPeakDetails.clicked.connect(self.do_show_single_peak_integration)
        self.ui.pushButton_clearPeakIntFigure.clicked.connect(self.do_clear_peak_integration_canvas)

        self.ui.lineEdit_numSurveyOutput.editingFinished.connect(self.evt_show_survey)
        self.ui.lineEdit_numSurveyOutput.returnPressed.connect(self.evt_show_survey)
        self.ui.lineEdit_numSurveyOutput.textEdited.connect(self.evt_show_survey)
        self.ui.pushButton_exportToMovie.clicked.connect(self.do_export_detector_views_to_movie)

        self.ui.comboBox_viewRawDataMasks.currentIndexChanged.connect(self.evt_change_roi)
        self.ui.comboBox_mergePeakNormType.currentIndexChanged.connect(self.evt_change_norm_type)

        # Tab k-shift vector
        self.ui.pushButton_addKShift.clicked.connect(self.do_add_k_shift_vector)

        # Menu and advanced tab
        self.ui.actionExit.triggered.connect(self.menu_quit)
        self.ui.actionSave_Session.triggered.connect(self.save_current_session)
        self.ui.actionLoad_Session.triggered.connect(self.load_session)
        self.ui.actionLoad_Mask.triggered.connect(self.menu_load_mask)

        self.ui.actionSave_Project.triggered.connect(self.action_save_project)
        self.ui.actionOpen_Project.triggered.connect(self.action_load_project)
        self.ui.actionOpen_Last_Project.triggered.connect(self.action_load_last_project)

        self.ui.pushButton_loadLastNthProject.clicked.connect(self.do_load_nth_project)

        self.ui.actionPre_Processing.triggered.connect(self.menu_pre_process)
        self.ui.actionData_Downloading.triggered.connect(self.menu_download_data)

        self.ui.actionSingle_Pt_Integration.triggered.connect(self.menu_integrate_peak_single_pt)
        self.ui.actionSort_By_2Theta.triggered.connect(self.menu_sort_survey_2theta)
        self.ui.actionSort_By_Pt.triggered.connect(self.menu_sort_by_pt_number)

        # menu action: pop out a general figure plot window
        self.ui.action2theta_Sigma.triggered.connect(self.menu_pop_2theta_sigma_window)
        self.ui.actionSave_2theta_Sigma.triggered.connect(self.menu_save_2theta_sigma)

        # Validator ... (NEXT)
        # blabla... ...

        # Declaration of class variable
        # IPTS number
        self._iptsNumber = None
        self._current_exp_number = None

        # some configuration
        self._homeSrcDir = os.getcwd()
        self._homeDir = os.getcwd()

        # Control
        self._myControl = r4c.CWSCDReductionControl(self._instrument)
        self._allowDownload = True
        self._dataAccessMode = "Download"
        self._surveyTableFlag = True
        self._ubPeakTableFlag = True
        self._isROIApplied = False

        # set the detector geometry
        self.do_set_detector_size()

        # Sub window
        self._baseTitle = "Title is not initialized"

        # Timing and thread 'global'
        self._startMeringScans = time.process_time()
        self._errorMessageEnsemble = ""

        # QSettings
        self.load_settings()

        # mutex interlock
        self._roiComboBoxMutex = False

        return

    def _promote_widgets(self):
        tableWidget_surveyTable_layout = QVBoxLayout()
        self.ui.frame_tableWidget_surveyTable.setLayout(tableWidget_surveyTable_layout)
        self.ui.tableWidget_surveyTable = ScanSurveyTable(self)
        tableWidget_surveyTable_layout.addWidget(self.ui.tableWidget_surveyTable)

        graphicsView_detector2dPlot_layout = QVBoxLayout()
        self.ui.frame_graphicsView_detector2dPlot.setLayout(graphicsView_detector2dPlot_layout)
        self.ui.graphicsView_detector2dPlot = Detector2DView(self)
        graphicsView_detector2dPlot_layout.addWidget(self.ui.graphicsView_detector2dPlot)

        tableWidget_peaksCalUB_layout = QVBoxLayout()
        self.ui.frame_tableWidget_peaksCalUB.setLayout(tableWidget_peaksCalUB_layout)
        self.ui.tableWidget_peaksCalUB = UBMatrixPeakTable(self)
        tableWidget_peaksCalUB_layout.addWidget(self.ui.tableWidget_peaksCalUB)

        tableWidget_ubMatrix_layout = QVBoxLayout()
        self.ui.frame_tableWidget_ubMatrix.setLayout(tableWidget_ubMatrix_layout)
        self.ui.tableWidget_ubMatrix = UBMatrixTable(self)
        tableWidget_ubMatrix_layout.addWidget(self.ui.tableWidget_ubMatrix)

        tableWidget_ubInUse_layout = QVBoxLayout()
        self.ui.frame_tableWidget_ubInUse.setLayout(tableWidget_ubInUse_layout)
        self.ui.tableWidget_ubInUse = UBMatrixTable(self)
        tableWidget_ubInUse_layout.addWidget(self.ui.tableWidget_ubInUse)

        tableWidget_mergeScans_layout = QVBoxLayout()
        self.ui.frame_tableWidget_mergeScans.setLayout(tableWidget_mergeScans_layout)
        self.ui.tableWidget_mergeScans = ProcessTableWidget(self)
        tableWidget_mergeScans_layout.addWidget(self.ui.tableWidget_mergeScans)

        graphicsView_integratedPeakView_layout = QVBoxLayout()
        self.ui.frame_graphicsView_integratedPeakView.setLayout(graphicsView_integratedPeakView_layout)
        self.ui.graphicsView_integratedPeakView = IntegratedPeakView(self)
        graphicsView_integratedPeakView_layout.addWidget(self.ui.graphicsView_integratedPeakView)

        tableWidget_covariance_layout = QVBoxLayout()
        self.ui.frame_tableWidget_covariance.setLayout(tableWidget_covariance_layout)
        self.ui.tableWidget_covariance = MatrixTable(self)
        tableWidget_covariance_layout.addWidget(self.ui.tableWidget_covariance)

        tableWidget_kShift_layout = QVBoxLayout()
        self.ui.frame_tableWidget_kShift.setLayout(tableWidget_kShift_layout)
        self.ui.tableWidget_kShift = KShiftTableWidget(self)
        tableWidget_kShift_layout.addWidget(self.ui.tableWidget_kShift)

        return

    @property
    def controller(self):
        """Parameter controller"""
        assert self._myControl is not None, "Controller cannot be None."
        assert isinstance(self._myControl, r4c.CWSCDReductionControl), "My controller must be of type %s, but not %s." % (
            "CWSCDReductionControl",
            self._myControl.__class__.__name__,
        )

        return self._myControl

    def _init_widgets(self):
        """Initialize the table widgets
        :return:
        """
        self._baseTitle = str(self.windowTitle())
        self.setWindowTitle("No Experiment Is Set")

        # detector geometry (set to 256 x 256)
        self.ui.comboBox_detectorSize.setCurrentIndex(0)

        # Table widgets
        self.ui.tableWidget_peaksCalUB.setup()
        self.ui.tableWidget_ubMatrix.setup()
        self.ui.tableWidget_surveyTable.setup()
        self.ui.tableWidget_mergeScans.setup()
        self.ui.tableWidget_ubInUse.setup()
        self.ui.tableWidget_kShift.setup()

        # Radio buttons
        self.ui.radioButton_ubFromTab1.setChecked(True)
        # group for the source of UB matrix to import
        ub_source_group = QButtonGroup(self)
        ub_source_group.addButton(self.ui.radioButton_ubFromList)
        ub_source_group.addButton(self.ui.radioButton_ubFromTab1)
        # group for the UB matrix's style
        ub_style_group = QButtonGroup(self)
        ub_style_group.addButton(self.ui.radioButton_ubMantidStyle)
        ub_style_group.addButton(self.ui.radioButton_ubSpiceStyle)

        self.ui.radioButton_qsample.setChecked(True)

        # combo-box
        self.ui.comboBox_kVectors.clear()
        self.ui.comboBox_kVectors.addItem("0: (0, 0, 0)")

        self.ui.comboBox_indexFrom.clear()
        self.ui.comboBox_indexFrom.addItem("By calculation")
        self.ui.comboBox_indexFrom.addItem("From SPICE")

        self.ui.comboBox_hklType.clear()
        self.ui.comboBox_hklType.addItem("SPICE")
        self.ui.comboBox_hklType.addItem("Calculated")

        # normalization to peak
        self.ui.comboBox_ptCountType.clear()
        self.ui.comboBox_ptCountType.addItem("Time")
        self.ui.comboBox_ptCountType.addItem("Monitor")
        self.ui.comboBox_ptCountType.addItem("Absolute")

        self.ui.comboBox_viewRawDataMasks.addItem("")

        # tab
        self.ui.tabWidget.setCurrentIndex(0)

        self.ui.radioButton_ubMantidStyle.setChecked(True)
        self.ui.lineEdit_numSurveyOutput.setText("")
        self.ui.checkBox_sortDescending.setChecked(False)
        self.ui.radioButton_sortByCounts.setChecked(True)
        self.ui.radioButton_ubSelectNoScan.setChecked(True)

        # Tab 'Access'
        # self.ui.lineEdit_url.setText('http://neutron.ornl.gov/user_data/hb3a/')
        # self.ui.comboBox_mode.setCurrentIndex(0)
        self.ui.lineEdit_localSpiceDir.setEnabled(True)
        self.ui.pushButton_browseLocalDataDir.setEnabled(True)

        # progress bars
        self.ui.progressBar_mergeScans.setRange(0, 20)
        self.ui.progressBar_mergeScans.setValue(0)

        # check boxes
        self.ui.graphicsView_detector2dPlot.set_parent_window(self)
        self.ui.checkBox_fpHighPrecision.setChecked(True)

        # background points
        self.ui.lineEdit_backgroundPts.setText("1, 1")
        self.ui.lineEdit_scaleFactor.setText("1.")
        self.ui.lineEdit_numPt4BackgroundLeft.setText("1")
        self.ui.lineEdit_numPt4BackgroundRight.setText("1")

        # about pre-processed data
        # FIXME self.ui.checkBox_searchPreprocessedFirst.setChecked(True)

        # hide and disable some push buttons for future implementation
        self.ui.pushButton_viewScan3D.hide()
        self.ui.pushButton_plotSelectedData.hide()

        return

    def _build_peak_info_list(self, zero_hkl, is_spice=True):
        """Build a list of PeakInfo to build peak workspace
        peak HKL can be set to zero or from table
        :return: list of peak information, which is a PeakProcessRecord instance
        """
        # Collecting all peaks that will be used to refine UB matrix
        row_index_list = self.ui.tableWidget_peaksCalUB.get_selected_rows(True)
        if len(row_index_list) < 3:
            err_msg = "At least 3 peaks must be selected to refine UB matrix.Now it is only %d selected." % len(row_index_list)
            self.pop_one_button_dialog(err_msg)
            return None

        # loop over all peaks for peak information
        peak_info_list = list()
        status, exp_number = gutil.parse_integers_editors(self.ui.lineEdit_exp)
        assert status
        for i_row in row_index_list:
            scan_num, pt_num = self.ui.tableWidget_peaksCalUB.get_exp_info(i_row)
            if pt_num < 0:
                pt_num = None
            peak_info = self._myControl.get_peak_info(exp_number, scan_num, pt_num)
            assert isinstance(peak_info, r4c.PeakProcessRecord)

            if zero_hkl:
                # set HKL to zero
                peak_info.set_hkl(0.0, 0.0, 0.0)
            elif is_spice:
                # set from table
                spice_hkl = self.ui.tableWidget_peaksCalUB.get_hkl(i_row, True)
                peak_info.set_hkl_np_array(numpy.array(spice_hkl))
            else:
                try:
                    calculated_hkl = self.ui.tableWidget_peaksCalUB.get_hkl(i_row, False)
                except RuntimeError as run_err:
                    errmsg = "[ERROR] Failed to get calculated HKL from UB calculation table due to {0}".format(run_err)
                    self.pop_one_button_dialog(errmsg)
                    return None
                peak_info.set_hkl_np_array(numpy.array(calculated_hkl))
            # END-IF-ELSE

            peak_info_list.append(peak_info)
        # END-FOR

        return peak_info_list

    def _show_refined_ub_result(self):
        """
        Show the result from refined UB matrix
        :return:
        """
        # Deal with result
        ub_matrix, lattice, lattice_error = self._myControl.get_refined_ub_matrix()
        # ub matrix
        self.ui.tableWidget_ubMatrix.set_from_matrix(ub_matrix)

        # lattice parameter
        assert isinstance(lattice, list)
        assert len(lattice) == 6
        self.ui.lineEdit_aUnitCell.setText("%.5f" % lattice[0])
        self.ui.lineEdit_bUnitCell.setText("%.5f" % lattice[1])
        self.ui.lineEdit_cUnitCell.setText("%.5f" % lattice[2])
        self.ui.lineEdit_alphaUnitCell.setText("%.5f" % lattice[3])
        self.ui.lineEdit_betaUnitCell.setText("%.5f" % lattice[4])
        self.ui.lineEdit_gammaUnitCell.setText("%.5f" % lattice[5])

        assert isinstance(lattice_error, list)
        assert len(lattice_error) == 6
        self.ui.lineEdit_aError.setText("%.5f" % lattice_error[0])
        self.ui.lineEdit_bError.setText("%.5f" % lattice_error[1])
        self.ui.lineEdit_cError.setText("%.5f" % lattice_error[2])
        self.ui.lineEdit_alphaError.setText("%.5f" % lattice_error[3])
        self.ui.lineEdit_betaError.setText("%.5f" % lattice_error[4])
        self.ui.lineEdit_gammaError.setText("%.5f" % lattice_error[5])

        return

    def _show_message(self, message):
        """
        show message in the message bar while clearing previous information
        :param message:
        :return:
        """
        if message is not None:
            self.ui.lineEdit_message.setText(message)

        return

    def action_save_project(self):
        """
        Save project
        :return:
        """
        # read project file name
        project_file_name = QFileDialog.getSaveFileName(self, "Specify Project File", os.getcwd())
        if not project_file_name:
            return
        if isinstance(project_file_name, tuple):
            project_file_name = project_file_name[0]
        # NEXT ISSUE - consider to allow incremental project saving technique
        if os.path.exists(project_file_name):
            yes = gutil.show_message(self, "Project file %s does exist. This is supposed to be an incremental save." % project_file_name)
            if yes:
                message = "Save project to {0} in incremental way.".format(project_file_name)
            else:
                message = "Saving activity is cancelled."
        else:
            message = "Saving current project to {0}".format(project_file_name)
        self._show_message(message)

        # gather some useful information
        ui_dict = dict()
        ui_dict["exp number"] = str(self.ui.lineEdit_exp.text())
        ui_dict["local spice dir"] = str(self.ui.lineEdit_localSpiceDir.text())
        ui_dict["work dir"] = str(self.ui.lineEdit_workDir.text())
        ui_dict["survey start"] = str(self.ui.lineEdit_surveyStartPt.text())
        ui_dict["survey stop"] = str(self.ui.lineEdit_surveyEndPt.text())

        # detector-sample distance
        det_distance_str = str(self.ui.lineEdit_infoDetSampleDistance.text()).strip()
        if len(det_distance_str) > 0:
            ui_dict["det_sample_distance"] = det_distance_str

        # wave length
        wave_length_str = str(self.ui.lineEdit_infoWavelength.text()).strip()
        if len(wave_length_str) > 0:
            ui_dict["wave_length"] = wave_length_str

        # calibrated detector center
        det_center_str = str(self.ui.lineEdit_infoDetCenter.text())
        if len(det_center_str) > 0:
            ui_dict["det_center"] = det_center_str

        # register and make it as a queue for last n opened/saved project
        last_1_path = str(self.ui.label_last1Path.text())
        if last_1_path != project_file_name:
            self.ui.label_last3Path.setText(self.ui.label_last2Path.text())
            self.ui.label_last2Path.setText(self.ui.label_last1Path.text())
            self.ui.label_last1Path.setText(project_file_name)
        # END-IF

        err_msg = self._myControl.save_project(project_file_name, ui_dict)
        if err_msg is not None:
            self.pop_one_button_dialog(err_msg)

        # show user the message that the saving process is over
        information = "Project has been saved to {0}\n".format(project_file_name)
        information += "Including dictionary keys: {0}".format(ui_dict)
        self.pop_one_button_dialog(information)

        return

    def action_load_project(self):
        """
        Load project
        :return:
        """
        project_file_name = QFileDialog.getOpenFileName(self, "Choose Project File", os.getcwd())
        if not project_file_name:  # return if cancelled
            return
        if isinstance(project_file_name, tuple):
            project_file_name = project_file_name[0]

        # make it as a queue for last n opened/saved project
        last_1_path = str(self.ui.label_last1Path.text())
        if last_1_path != project_file_name:
            self.ui.label_last3Path.setText(self.ui.label_last2Path.text())
            self.ui.label_last2Path.setText(self.ui.label_last1Path.text())
            self.ui.label_last1Path.setText(last_1_path)
        # END-IF

        self.load_project(project_file_name)

    def action_load_last_project(self):
        """
        Load last project
        :return:
        """
        project_file_name = str(self.ui.label_last1Path.text())
        if os.path.exists(project_file_name) is False:
            self.pop_one_button_dialog("Last saved project %s cannot be located." % project_file_name)
        else:
            self.load_project(project_file_name)

    def closeEvent(self, QCloseEvent):
        """
        Close event
        :param QCloseEvent:
        :return:
        """
        self.menu_quit()

    def do_accept_ub(self):
        """Accept the calculated UB matrix and thus put to controller"""
        # get the experiment number
        exp_number = int(str(self.ui.lineEdit_exp.text()))

        # get matrix
        curr_ub = self.ui.tableWidget_ubMatrix.get_matrix()

        # synchronize UB matrix to tableWidget_ubInUse
        self.ui.tableWidget_ubInUse.set_from_matrix(curr_ub)

        # set UB matrix to system
        self._myControl.set_ub_matrix(exp_number, curr_ub)

    def do_add_peaks_for_ub(self):
        """In tab-survey, merge selected scans, find peaks in merged data and
         switch to UB matrix calculation tab and add to table
        :return:
        """
        # get selected scans
        selected_row_index_list = self.ui.tableWidget_surveyTable.get_selected_rows(True)
        scan_number_list = self.ui.tableWidget_surveyTable.get_scan_numbers(selected_row_index_list)
        if len(scan_number_list) == 0:
            self.pop_one_button_dialog("No scan is selected.")
            return

        # get experiment number
        status, exp_number = gutil.parse_integers_editors(self.ui.lineEdit_exp)
        if not status:
            self.pop_one_button_dialog("Unable to get experiment number\n  due to %s." % str(exp_number))
            return

        # switch to tab-3
        self.ui.tabWidget.setCurrentIndex(MainWindow.TabPage["Calculate UB"])

        # prototype for a new thread
        self.ui.progressBar_add_ub_peaks.setRange(0, len(scan_number_list))
        self._addUBPeaksThread = thread_pool.AddPeaksThread(self, exp_number, scan_number_list)
        self._addUBPeaksThread.start()

        # set the flag/notification where the indexing (HKL) from
        self.ui.lineEdit_peaksIndexedBy.setText(IndexFromSpice)

    def add_scans_ub_table(self, scan_list):
        """add scans to UB matrix construction table
        :param scan_list:
        :return:
        """
        # TODO/FIXME/ISSUE/NOW - consider to refactor with do_add_peaks_for_ub() and
        # get experiment number
        status, exp_number = gutil.parse_integers_editors(self.ui.lineEdit_exp)
        if not status:
            self.pop_one_button_dialog("Unable to get experiment number\n  due to %s." % str(exp_number))
            return

        # switch to tab-3
        # self.ui.tabWidget.setCurrentIndex(MainWindow.TabPage['Calculate UB'])

        # prototype for a new thread
        self.ui.progressBar_add_ub_peaks.setRange(0, len(scan_list))
        self._addUBPeaksThread = thread_pool.AddPeaksThread(self, exp_number, scan_list)
        self._addUBPeaksThread.start()

        # set the flag/notification where the indexing (HKL) from
        self.ui.lineEdit_peaksIndexedBy.setText(IndexFromSpice)

    def do_switch_roi_mode(self):
        """Add region of interest to 2D image
        :return:
        """
        # set the button to next mode
        if str(self.ui.pushButton_switchROIMode.text()) == "Enter ROI-Edit Mode":
            # enter adding ROI mode
            self.ui.graphicsView_detector2dPlot.enter_roi_mode(roi_state=True)
            # rename the button
            self.ui.pushButton_switchROIMode.setText("Quit ROI-Edit Mode")
        else:
            # quit editing ROI mode
            self.ui.graphicsView_detector2dPlot.enter_roi_mode(roi_state=False)
            # rename the button
            self.ui.pushButton_switchROIMode.setText("Enter ROI-Edit Mode")
        # END-IF-ELSE

    def do_add_scans_merge(self):
        """Add scans to merge
        :return:
        """
        # Get list of scans
        scan_list = gutil.parse_integer_list(str(self.ui.lineEdit_listScansSliceView.text()))
        if len(scan_list) == 0:
            self.pop_one_button_dialog("Scan list is empty.")

        # Set table
        self.ui.tableWidget_mergeScans.append_scans(scans=scan_list, allow_duplicate_scans=False)

    def do_add_ub_peaks(self):
        """
        Launch dialog to add UB peaks
        :return:
        """
        if self._addUBPeaksDialog is None:
            self._addUBPeaksDialog = FindUBUtility.AddScansForUBDialog(self)

        self._addUBPeaksDialog.show()

    # def do_add_ub_peak(self):
    #     """ Add current to ub peaks
    #     :return:
    #     """
    #     # TODO//ISSUE/Future - Find out whether this method is still needed
    #     # Add peak
    #     status, int_list = gutil.parse_integers_editors([self.ui.lineEdit_exp,
    #                                                      self.ui.lineEdit_scanNumber])
    #     if status is False:
    #         self.pop_one_button_dialog(int_list)
    #         return
    #     exp_no, scan_no = int_list
    #
    #     # Get HKL from GUI
    #     status, float_list = gutil.parse_float_editors([self.ui.lineEdit_H,
    #                                                     self.ui.lineEdit_K,
    #                                                     self.ui.lineEdit_L])
    #     if status is False:
    #         err_msg = float_list
    #         self.pop_one_button_dialog(err_msg)
    #         return
    #     h, k, l = float_list
    #
    #     try:
    #         peak_info_obj = self._myControl.get_peak_info(exp_no, scan_no)
    #     except AssertionError as ass_err:
    #         self.pop_one_button_dialog(str(ass_err))
    #         return
    #
    #     assert isinstance(peak_info_obj, r4c.PeakProcessRecord)
    #     peak_info_obj.set_hkl(h, k, l)
    #     self.set_ub_peak_table(peak_info_obj)
    #
    #     # Clear
    #     self.ui.lineEdit_scanNumber.setText('')
    #
    #     self.ui.lineEdit_sampleQx.setText('')
    #     self.ui.lineEdit_sampleQy.setText('')
    #     self.ui.lineEdit_sampleQz.setText('')
    #
    #     self.ui.lineEdit_H.setText('')
    #     self.ui.lineEdit_K.setText('')
    #     self.ui.lineEdit_L.setText('')
    #
    #     # set the flag/notification where the indexing (HKL) from
    #     self.ui.lineEdit_peaksIndexedBy.setText(IndexFromSpice)
    #
    #     return

    def do_add_k_shift_vector(self):
        """Add a k-shift vector
        :return:
        """
        # parse the k-vector
        status, ret_obj = gutil.parse_float_editors([self.ui.lineEdit_kX, self.ui.lineEdit_kY, self.ui.lineEdit_kZ], allow_blank=False)
        if status is False:
            error_message = ret_obj
            self.pop_one_button_dialog(error_message)
            return
        else:
            k_x, k_y, k_z = ret_obj

        # add to controller
        k_index = self._myControl.add_k_shift_vector(k_x, k_y, k_z)

        # add to table and combo-box
        self.ui.tableWidget_kShift.add_k_vector(k_index, k_x, k_y, k_z)

        combo_message = "%d: (%.5f, %.5f, %.5f)" % (k_index, k_x, k_y, k_z)
        self.ui.comboBox_kVectors.addItem(combo_message)

    def do_apply_k_shift(self):
        """Apply k-shift to selected reflections
        :return:
        """
        # get the selected scans
        scan_list = list()
        selected_row_numbers = self.ui.tableWidget_mergeScans.get_selected_rows(True)
        for row_index in selected_row_numbers:
            scan_number = self.ui.tableWidget_mergeScans.get_scan_number(row_index)
            scan_list.append(scan_number)

        # get the k-vector
        k_shift_message = str(self.ui.comboBox_kVectors.currentText())
        k_index = int(k_shift_message.split(":")[0])

        # set to controller
        self._myControl.set_k_shift(scan_list, k_index)

        # set k-shift to table
        # exp_number = int(self.ui.lineEdit_exp.text())
        for row_index in selected_row_numbers:
            self.ui.tableWidget_mergeScans.set_k_shift_index(row_index, k_index)
            # scan_number = self.ui.tableWidget_mergeScans.get_scan_number(row_index)

    def do_apply_setup(self):
        """
        Purpose:
         - Apply the setup to controller.
        Requirements:
         - data directory, working directory must be given; but not necessarily correct
         - URL must be given; but not necessary to be correct
        :return:
        """
        # get data directory, working directory and data server URL from GUI
        local_data_dir = str(self.ui.lineEdit_localSpiceDir.text()).strip()
        working_dir = str(self.ui.lineEdit_workDir.text()).strip()
        pre_process_dir = str(self.ui.lineEdit_preprocessedDir.text()).strip()

        # set to my controller
        status, err_msg = self._myControl.set_local_data_dir(local_data_dir)
        if not status:
            raise RuntimeError(err_msg)
        self._myControl.set_working_directory(working_dir)

        # check
        error_message = ""

        # local data dir
        if local_data_dir == "":
            error_message += "Local data directory is not specified!\n"
        elif os.path.exists(local_data_dir) is False:
            try:
                os.mkdir(local_data_dir)
            except OSError as os_error:
                error_message += "Unable to create local data directory %s due to %s.\n" % (local_data_dir, str(os_error))
                self.ui.lineEdit_localSpiceDir.setStyleSheet("color: red;")
            else:
                self.ui.lineEdit_localSpiceDir.setStyleSheet("color: green;")
        else:
            self.ui.lineEdit_localSpiceDir.setStyleSheet("color: green;")
        # END-IF-ELSE

        # working directory
        if working_dir == "":
            error_message += "Working directory is not specified!\n"
        elif os.path.exists(working_dir) is False:
            try:
                os.mkdir(working_dir)
                self.ui.lineEdit_workDir.setStyleSheet("color: green;")
            except OSError as os_error:
                error_message += "Unable to create working directory %s due to %s.\n" % (working_dir, str(os_error))
                self.ui.lineEdit_workDir.setStyleSheet("color: red;")
        else:
            self.ui.lineEdit_workDir.setStyleSheet("color: green;")
        # END-IF-ELSE

        # preprocess directory
        if len(pre_process_dir) == 0:
            # user does not specify
            pass
        # It is not allowed to set pre-processed dir to None: self._myControl.pre_processed_dir = None
        elif os.path.exists(pre_process_dir):
            # user specifies a valid directory
            self._myControl.pre_processed_dir = pre_process_dir
            self.ui.lineEdit_preprocessedDir.setStyleSheet("color: green;")
        else:
            # user specifies a non-exist directory. make an error message
            self.pop_one_button_dialog("Pre-processed directory {0} ({1}) does not exist.".format(pre_process_dir, type(pre_process_dir)))
            self._myControl.pre_processed_dir = self._myControl.get_working_directory()
            self.ui.lineEdit_preprocessedDir.setStyleSheet("color: red;")
            self.ui.lineEdit_preprocessedDir.setText(self._myControl.pre_processed_dir)
        # END-IF

        if len(error_message) > 0:
            self.pop_one_button_dialog(error_message)

    def do_browse_preprocessed_dir(self):
        """browse the pre-processed merged scans' directory
        :return:
        """
        print("Here...2")
        # determine default directory
        exp_number_str = str(self.ui.lineEdit_exp.text())
        default_pp_dir = os.path.join("/HFIR/HB3A/exp{0}/Shared/".format(exp_number_str))
        if not os.path.exists(default_pp_dir):
            default_pp_dir = os.path.expanduser("~")

        # use FileDialog to get the directory and set to preprocessedDir
        pp_dir = QFileDialog.getExistingDirectory(self, "Get Directory", default_pp_dir)
        if not pp_dir:
            return
        if isinstance(pp_dir, tuple):
            pp_dir = pp_dir[0]
        self.ui.lineEdit_preprocessedDir.setText(pp_dir)

    def do_browse_local_spice_data(self):
        """Browse local source SPICE data directory"""
        print("Here...1")
        src_spice_dir = QFileDialog.getExistingDirectory(self, "Get Directory", self._homeSrcDir)
        if isinstance(src_spice_dir, tuple):
            src_spice_dir = src_spice_dir[0]
        # Set local data directory to controller
        status, error_message = self._myControl.set_local_data_dir(src_spice_dir)
        if status is False:
            self.pop_one_button_dialog(error_message)
            return

        self._homeSrcDir = src_spice_dir
        self.ui.lineEdit_localSpiceDir.setText(src_spice_dir)

    def do_browse_working_dir(self):
        """
        Browse and set up working directory
        :return:
        """
        print("Here...2")
        work_dir = QFileDialog.getExistingDirectory(self, "Get Working Directory", self._homeDir)
        if isinstance(work_dir, tuple):
            work_dir = work_dir[0]
        status, error_message = self._myControl.set_working_directory(work_dir)
        if status is False:
            self.pop_one_button_dialog(error_message)
        else:
            self.ui.lineEdit_workDir.setText(work_dir)

    def do_cal_ub_matrix(self):
        """Calculate UB matrix by 2 or 3 reflections"""
        # Get reflections selected to calculate UB matrix
        num_rows = self.ui.tableWidget_peaksCalUB.rowCount()
        peak_info_list = list()
        status, exp_number = gutil.parse_integers_editors(self.ui.lineEdit_exp)
        assert status
        for i_row in range(num_rows):
            if self.ui.tableWidget_peaksCalUB.is_selected(i_row) is True:
                scan_num, pt_num = self.ui.tableWidget_peaksCalUB.get_exp_info(i_row)
                if pt_num < 0:
                    pt_num = None
                peak_info = self._myControl.get_peak_info(exp_number, scan_num, pt_num)
                assert isinstance(peak_info, r4c.PeakProcessRecord)
                peak_info_list.append(peak_info)
        # END-FOR

        # Get lattice
        status, ret_obj = self._get_lattice_parameters()
        if status is True:
            a, b, c, alpha, beta, gamma = ret_obj
        else:
            err_msg = ret_obj
            self.pop_one_button_dialog(err_msg)
            return

        # Calculate UB matrix
        status, ub_matrix = self._myControl.calculate_ub_matrix(peak_info_list, a, b, c, alpha, beta, gamma)

        # Deal with result
        if status is True:
            self.ui.tableWidget_ubMatrix.set_from_matrix(ub_matrix)

        else:
            err_msg = ub_matrix
            self.pop_one_button_dialog(err_msg)

    def do_change_instrument_name(self):
        """Handing the event as the instrument name is changed
        :return:
        """
        new_instrument = str(self.ui.comboBox_instrument.currentText())
        self.pop_one_button_dialog("Change of instrument during data processing is dangerous.")
        status, error_message = self._myControl.set_instrument_name(new_instrument)
        if status is False:
            self.pop_one_button_dialog(error_message)

    def do_clear_all_peaks_index_ub(self):
        """
        Set all peaks' indexes in UB matrix calculation tab to zero
        :return:
        """
        num_rows = self.ui.tableWidget_peaksCalUB.rowCount()
        for i_row in range(num_rows):
            self.ui.tableWidget_peaksCalUB.set_hkl(i_row, [0.0, 0.0, 0.0], is_spice_hkl=False)

    def do_clear_merge_table(self):
        """
        Clear the merge/peak-integration table
        :return:
        """
        # clear
        self.ui.tableWidget_mergeScans.remove_all_rows()

    def do_clear_peak_integration_canvas(self):
        """
        clear the peak integration canvas and the integrated values
        :return:
        """
        self.ui.graphicsView_integratedPeakView.clear_all_lines()

        self.ui.lineEdit_rawSinglePeakIntensity.setText("")
        self.ui.lineEdit_intensity2.setText("")
        self.ui.lineEdit_gaussianPeakIntensity.setText("")
        self.ui.lineEdit_errorIntensity1.setText("")
        self.ui.lineEdit_errorIntensity2.setText("")
        self.ui.lineEdit_errorIntensity3.setText("")

    def do_clear_survey(self):
        """
        Clear survey and survey table.
        As myController does not store any survey information,
        there is no need to clear any data structure in myController
        :return:
        """
        # Clear table
        self.ui.tableWidget_surveyTable.remove_all_rows()
        self.ui.tableWidget_surveyTable.reset()

    def do_clear_ub_peaks(self):
        """
        Clear all peaks in UB-Peak table
        :return:
        """
        num_rows = self.ui.tableWidget_peaksCalUB.rowCount()
        row_number_list = list(range(num_rows))
        self.ui.tableWidget_peaksCalUB.delete_rows(row_number_list)

    def do_convert_merged_to_hkl(self):
        """
        convert merged workspace in Q-sample frame to HKL frame
        :return:
        """
        # get experiment number
        exp_number = int(str(self.ui.lineEdit_exp.text()))

        # get the lines that are selected
        selected_row_number_list = self.ui.tableWidget_mergeScans.get_selected_rows(True)

        for row_number in selected_row_number_list:
            scan_number = self.ui.tableWidget_mergeScans.get_scan_number(row_number)
            status, ret_obj = self._myControl.get_pt_numbers(exp_number, scan_number)
            if not status:
                raise RuntimeError("It is not possible to fail to get Pt number list at this stage. Error is due to %s." % str(ret_obj))
            pt_number_list = ret_obj

            # set intensity to zero and error message if fails to get Pt.
            self._myControl.convert_merged_ws_to_hkl(exp_number, scan_number, pt_number_list)

    def do_del_roi(self):
        """Remove the current ROI
        :return:
        """
        # check whether there is mask on detector view
        if self.ui.graphicsView_detector2dPlot.is_roi_selection_drawn:
            self.ui.graphicsView_detector2dPlot.remove_roi()

        # need to draw again?
        if self._isROIApplied:
            re_plot = True
        else:
            re_plot = False

        # need to reset combo box index
        curr_index = self.ui.comboBox_viewRawDataMasks.currentIndex()
        if curr_index != 0:
            self.ui.comboBox_viewRawDataMasks.setCurrentIndex(0)
        elif re_plot:
            self.do_plot_pt_raw()

    def do_del_ub_peaks(self):
        """
        Delete a peak in UB-Peak table
        :return:
        """
        # Find out the lines to get deleted
        row_num_list = self.ui.tableWidget_peaksCalUB.get_selected_rows()

        # Delete
        self.ui.tableWidget_peaksCalUB.delete_rows(row_num_list)

    def find_peak_in_scan(self, scan_number, load_spice_hkl):
        """Find peak in a given scan and record it"""
        # Get experiment, scan and pt
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp])
        if status is True:
            exp_no = ret_obj
        else:
            self.pop_one_button_dialog(ret_obj)
            return

        # merge peak if necessary
        if self._myControl.has_merged_data(exp_no, scan_number) is False:
            status, err_msg = self._myControl.merge_pts_in_scan(
                exp_no, scan_number, [], rewrite=True, preprocessed_dir=self._myControl.pre_processed_dir
            )
            if status is False:
                self.pop_one_button_dialog(err_msg)

        # Find peak
        status, err_msg = self._myControl.find_peak(exp_no, scan_number)
        if status is False:
            self.pop_one_button_dialog(ret_obj)
            return

        # Get information from the latest (integrated) peak
        # if load_spice_hkl:
        #     # This is the first time that in the workflow to get HKL from MD workspace
        #     peak_info = self._myControl.get_peak_info(exp_no, scan_number)
        #     assert peak_info is not None, 'Unable to locate PeakProcessRecord (peak info).'
        # # END-IF

        # Set up correct values to table tableWidget_peaksCalUB
        peak_info = self._myControl.get_peak_info(exp_no, scan_number)
        assert peak_info is not None, "Unable to locate PeakProcessRecord (peak info)."

        if load_spice_hkl:
            h, k, l = peak_info.get_hkl()
            hkl = (h, k, l)
        else:
            hkl = ()

        q_x, q_y, q_z = peak_info.get_peak_centre()
        vec_q = (q_x, q_y, q_z)

        return hkl, vec_q

    def do_export_mask(self):
        """
        export selected mask to file
        :return:
        """
        # get selected mask name
        mask_name = str(self.ui.comboBox_viewRawDataMasks.currentText())
        if mask_name == "No Mask":
            self.pop_one_button_dialog("No mask is selected.  Saving is aborted.")
            return

        # get the output file name
        roi_file_name = QFileDialog.getSaveFileName(
            self, "Output mask/ROI file name", self._myControl.get_working_directory(), "XML Files (*.xml);;All Files (*.*)"
        )
        if not roi_file_name:
            return
        if isinstance(roi_file_name, tuple):
            roi_file_name = roi_file_name[0]

        # save file
        self._myControl.save_roi_to_file(None, None, mask_name, roi_file_name)

    def do_export_detector_views_to_movie(self):
        """
        go through all surveyed scans. plot all the measurements from all the scans. record the plot to PNG files,
        and possibly convert them to movies
        :return:
        """
        scan_list = self.ui.tableWidget_surveyTable.get_scan_numbers(range(self.ui.tableWidget_surveyTable.rowCount()))
        # roi_name = str(self.ui.comboBox_viewRawDataMasks.currentText())
        file_name_out = ""
        for i_scan, scan_number in enumerate(scan_list):
            # get pt numbers
            status, pt_number_list = self._myControl.get_pt_numbers(self._current_exp_number, scan_number)
            # stop this loop if unable to get Pt. numbers
            if not status:
                print("[DB...BAT] Unable to get list of Pt. number from scan {0} due to {1}.".format(scan_number, pt_number_list))
                continue
            # plot
            for pt_number in pt_number_list:
                # ROI is set to None because only the ROI rectangular shall appear on the output. But with
                # a ROI name, the detector is then masked.  It is not the purpose to examine whether all the
                # peaks are in ROI
                file_name = self.load_plot_raw_data(
                    self._current_exp_number, scan_number, pt_number, roi_name=None, save=True, remove_workspace=True
                )
                file_name_out += file_name + "\n"
            # END-FOR

            # debug break
            # if i_scan == 5:
            #     break
        # END-FOR

        # write out the file list
        list_name = os.path.join(self._myControl.get_working_directory(), "png_exp{0}_list.txt".format(self._current_exp_number))
        ofile = open(list_name, "w")
        ofile.write(file_name_out)
        ofile.close()

        message = "convert -delay 10 -loop 0 @{0} {1}.mpeg".format(list_name, "[Your Name]")
        self.pop_one_button_dialog(message)

    def do_export_selected_peaks_to_integrate(self):
        """
        export (to file or just print out) the scans that are selected for integration
        :param self:
        :return:
        """
        # get selected rows' scan numbers
        scan_tuple_list = self.ui.tableWidget_mergeScans.get_selected_scans()
        scan_number_list = list()
        for tup in scan_tuple_list:
            scan_number_list.append(tup[0])
        scan_number_list.sort()

        info_str = "{0}".format(scan_number_list)
        self._show_message("Selected scans: {0}".format(info_str))

    def do_export_to_fp(self):
        """Export selected reflections to Fullprof single crystal data file for analysis
        :return:
        """
        # get selected lines
        selected_rows = self.ui.tableWidget_mergeScans.get_selected_rows(True)
        if len(selected_rows) == 0:
            self.pop_one_button_dialog("There isn't any peak selected.")
            return

        # get the file name
        fp_name = QFileDialog.getSaveFileName(self, "Save to Fullprof File")
        if not fp_name:  # return due to cancel
            return
        if isinstance(fp_name, tuple):
            fp_name = fp_name[0]

        # collect information
        exp_number = int(self.ui.lineEdit_exp.text())
        scan_number_list = list()
        for i_row in selected_rows:
            # add both ROI and scan number
            scan_i = self.ui.tableWidget_mergeScans.get_scan_number(i_row)
            roi_i = self.ui.tableWidget_mergeScans.get_mask(i_row)
            scan_number_list.append((scan_i, roi_i))

        # write
        user_header = str(self.ui.lineEdit_fpHeader.text())
        try:
            export_absorption = self.ui.checkBox_exportAbsorptionToFP.isChecked()

            file_content = self._myControl.export_to_fullprof(
                exp_number, scan_number_list, user_header, export_absorption, fp_name, self.ui.checkBox_fpHighPrecision.isChecked()
            )
            self.ui.plainTextEdit_fpContent.setPlainText(file_content)
        except AssertionError as a_err:
            self.pop_one_button_dialog(str(a_err))
        except KeyError as key_err:
            self.pop_one_button_dialog(str(key_err))
        except RuntimeError as run_error:
            error_msg = str(run_error)
            if error_msg.count("Peak index error") > 0:
                error_msg = "You may forget to index peak\n" + error_msg
            self.pop_one_button_dialog(error_msg)

    def do_filter_sort_survey_table(self):
        """
        Sort and filter survey table by specified field
        Requirements:
        Guarantees: the original table is cleared and a new list is appended
        :return:
        """
        # Get column name
        if self.ui.radioButton_sortByScan.isChecked():
            column_name = "Scan"
        elif self.ui.radioButton_sortByCounts.isChecked():
            column_name = "Max Counts"
        elif self.ui.radioButton_sortByTemp.isChecked():
            column_name = "Sample Temp"
        elif self.ui.radioButton_sortBy2Theta.isChecked():
            column_name = "2theta"
        else:
            self.pop_one_button_dialog("No column is selected to sort.")
            return

        # Get filters
        status, ret_obj = gutil.parse_integers_editors(
            [self.ui.lineEdit_filterScanLower, self.ui.lineEdit_filterScanUpper], allow_blank=True
        )

        # return with error
        if status is False:
            self.pop_one_button_dialog(ret_obj)
            return

        # set up default with return as None
        start_scan_number = ret_obj[0]
        if start_scan_number is None:
            start_scan_number = 0
        end_scan_number = ret_obj[1]
        if end_scan_number is None:
            end_scan_number = sys.maxsize

        status, ret_obj = gutil.parse_float_editors(
            [self.ui.lineEdit_filterCountsLower, self.ui.lineEdit_filterCountsUpper], allow_blank=True
        )
        if status is False:
            # return with error message
            self.pop_one_button_dialog(ret_obj)
            return

        # set up default with return as None
        min_counts = ret_obj[0]
        if min_counts is None:
            min_counts = -0.0
        max_counts = ret_obj[1]
        if max_counts is None:
            max_counts = sys.float_info.max

        # filter and sort
        ascending_order = not self.ui.checkBox_sortDescending.isChecked()
        if ascending_order:
            sort_order = 0
        else:
            sort_order = 1
        self.ui.tableWidget_surveyTable.filter_and_sort(start_scan_number, end_scan_number, min_counts, max_counts, column_name, sort_order)

    def do_integrate_single_scan(self):
        """
        integrate a single scan in 'Peak Integration' tab
        Note: this is an experimenntal replacement for do_integrate_per_pt
        :return:
        """
        # parse experiment and scan number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp, self.ui.lineEdit_scanIntegratePeak])
        if not status:
            self.pop_one_button_dialog("Unable to integrate peak due to %s." % ret_obj)
            return
        else:
            exp_number, scan_number = ret_obj

        # parse normalization type
        normalization = str(self.ui.comboBox_ptCountType.currentText())
        if normalization.count("Time") > 0:
            norm_type = "time"
        elif normalization.count("Monitor") > 0:
            norm_type = "monitor"
        else:
            norm_type = ""

        # parse scale factor
        try:
            intensity_scale_factor = float(self.ui.lineEdit_scaleFactorScan.text())
        except ValueError:
            intensity_scale_factor = 1.0

        # calculate peak center (weighted)
        status, ret_obj = self._myControl.find_peak(exp_number, scan_number)
        if status is False:
            error_message = ret_obj
            self.pop_one_button_dialog(error_message)
            return
        else:
            this_peak_centre = ret_obj

        # mask workspace
        mask_name = str(self.ui.comboBox_maskNames2.currentText())
        if mask_name.lower() == "no mask":
            mask_name = ""

        # ui.lineEdit_backgroundPts and set default in init_widgets
        bkgd_pt_tuple = gutil.parse_integer_list(str(self.ui.lineEdit_backgroundPts.text()), 2)

        # integrate peak
        try:
            int_peak_dict = self._myControl.integrate_scan_peak(
                exp_number=exp_number,
                scan_number=scan_number,
                peak_centre=this_peak_centre,
                mask_name=mask_name,
                normalization=norm_type,
                scale_factor=intensity_scale_factor,
                background_pt_tuple=bkgd_pt_tuple,
            )
        except RuntimeError as run_error:
            self.pop_one_button_dialog("Unable to integrate peak for scan {0} due to {1}.".format(scan_number, run_error))
            return

        # plot calculated motor position (or Pt.) - integrated intensity per Pts.
        motor_pos_vec = int_peak_dict["motor positions"]
        pt_intensity_vec = int_peak_dict["pt intensities"]
        motor_std = motor_pos_vec.std()
        if motor_std > 0.005:
            self.ui.graphicsView_integratedPeakView.plot_raw_data(motor_pos_vec, pt_intensity_vec)
        else:
            # motor position fixed
            # KEEP-IN-MIND:  Make this an option from
            self.ui.graphicsView_integratedPeakView.plot_raw_data(numpy.arange(1, len(pt_intensity_vec) + 1), pt_intensity_vec)

        if self._mySinglePeakIntegrationDialog is None:
            self._mySinglePeakIntegrationDialog = message_dialog.MessageDialog(self)
        self._mySinglePeakIntegrationDialog.set_peak_integration_details(motor_pos_vec, pt_intensity_vec)

        # set calculated values
        try:
            self.ui.lineEdit_rawSinglePeakIntensity.setText("{0:.7f}".format(int_peak_dict["simple intensity"]))
            self.ui.lineEdit_errorIntensity1.setText("{0:.7f}".format(int_peak_dict["simple error"]))
            self.ui.lineEdit_avgBackground.setText("{0:.7f}".format(int_peak_dict["simple background"]))
            self.ui.lineEdit_intensity2.setText("{0:.7f}".format(int_peak_dict["intensity 2"]))
            if int_peak_dict["error 2"] is None:
                self.ui.lineEdit_errorIntensity2.setText("inf")
            else:
                self.ui.lineEdit_errorIntensity2.setText("{0:.7f}".format(int_peak_dict["error 2"]))
            self.ui.lineEdit_ptRange.setText("{0}".format(int_peak_dict["pt_range"]))
            self.ui.lineEdit_gaussianPeakIntensity.setText("{0:.7f}".format(int_peak_dict["gauss intensity"]))
            if int_peak_dict["gauss error"] is None:
                self.ui.lineEdit_errorIntensity3.setText("inf")
            else:
                self.ui.lineEdit_errorIntensity3.setText("{0:.7f}".format(int_peak_dict["gauss error"]))
            self.ui.tableWidget_covariance.set_matrix(int_peak_dict["covariance matrix"])

            fit_param_dict = int_peak_dict["gauss parameters"]
            # {'A': 1208.4097237325959, 'x0': 32.175524426773507, 'B': 23.296505385975976, 's': 0.47196665622701633}
            self.ui.lineEdit_peakBackground.setText("{0:.4f}".format(fit_param_dict["B"]))
            self.ui.lineEdit_gaussA.setText("{0:.4f}".format(fit_param_dict["A"]))
            self.ui.lineEdit_gaussSigma.setText("{0:.4f}".format(fit_param_dict["s"]))
            self.ui.lineEdit_gaussB.setText("{0:.4f}".format(fit_param_dict["B"]))

            # plot fitted Gaussian
            fit_gauss_dict = int_peak_dict["gauss parameters"]
        except KeyError as key_err:
            raise RuntimeError(
                "Peak integration result dictionary has keys {0}. Error is caused by {1}.".format(int_peak_dict.keys(), key_err)
            )
        except ValueError as value_err:
            self._show_message("[ERROR] Unable to fit by Gaussian due to {0}.".format(value_err))
        else:
            self.plot_model_data(motor_pos_vec, fit_gauss_dict)

    def plot_model_data(self, vec_x, params):
        """
        calculate the Y value by the model and plot them.
        the sparse X values will be expanded
        :return:
        """
        # check inputs
        assert isinstance(vec_x, numpy.ndarray), "vec X {0} must be a numpy.ndarray but not a {1}.".format(vec_x, type(vec_x))
        assert isinstance(params, dict), "Model parameters {0} must be given by a dictionary but not by a {1}.".format(params, type(params))

        # get parameters
        x0 = params["x0"]
        gauss_sigma = params["s"]
        gauss_a = params["A"]
        background = params["B"]
        info_str = "Gaussian fit"

        # plot the data
        # make modelX and modelY for more fine grids
        model_x = peak_integration_utility.get_finer_grid(vec_x, 10)
        model_y = peak_integration_utility.gaussian_linear_background(model_x, x0, gauss_sigma, gauss_a, background)

        # plot the model
        self.ui.graphicsView_integratedPeakView.plot_model(model_x, model_y, title=info_str)

    def do_integrate_roi(self):
        """integrate the detector counts in the region of interest

        Integrate the detector's counts (2D) along axis-0 and axis-1 respectively.
        and save the result (1D data) to file
        :return:
        """
        exp_number = str(self.ui.lineEdit_exp.text())
        scan_number = str(self.ui.lineEdit_run.text())
        pt_number = str(self.ui.lineEdit_rawDataPtNo.text())
        working_dir = str(self.ui.lineEdit_workDir.text())

        msg = self.ui.graphicsView_detector2dPlot.integrate_roi_linear(exp_number, scan_number, pt_number, working_dir)

        self.pop_one_button_dialog(msg)

    def do_integrate_peaks(self):
        """Integrate selected peaks tab-'scan processing'.

        If any scan is not merged, then it will merge the scan first.
        Integrate peaks from the table of merged peak.
        It will so the simple cuboid integration with region of interest and background subtraction.
        :return:
        """
        # get rows to merge
        row_number_list = self.ui.tableWidget_mergeScans.get_selected_rows(True)
        if len(row_number_list) == 0:
            self.pop_one_button_dialog("No scan is selected for scan")
            return

        # get experiment number
        status, ret_obj = gutil.parse_integers_editors(self.ui.lineEdit_exp, allow_blank=False)
        if status:
            exp_number = ret_obj
        else:
            self.pop_one_button_dialog("Unable to get valid experiment number due to %s." % ret_obj)
            return

        # mask workspace
        selected_mask = str(self.ui.comboBox_maskNames1.currentText())
        if selected_mask.lower().startswith("no mask"):
            selected_mask = ""
            mask_det = False
        else:
            mask_det = True

        # normalization
        norm_str = str(self.ui.comboBox_mergePeakNormType.currentText())
        if norm_str.lower().count("time") > 0:
            norm_type = "time"
        elif norm_str.lower().count("monitor") > 0:
            norm_type = "monitor"
        else:
            norm_type = ""

        # background Pt.
        # status, num_bg_pt = gutil.parse_integers_editors(self.ui.lineEdit_numPt4Background, allow_blank=False)
        # if not status or num_bg_pt == 0:
        #     self.pop_one_button_dialog('Number of Pt number for background must be larger than 0: %s!' % str(num_bg_pt))
        #     return

        # get the merging information: each item should be a tuple as (scan number, pt number list, merged)
        scan_number_list = list()
        for row_number in row_number_list:
            # get scan number and pt numbers
            scan_number = self.ui.tableWidget_mergeScans.get_scan_number(row_number)
            status, pt_number_list = self._myControl.get_pt_numbers(exp_number, scan_number)

            # set intensity to zero and error message if fails to get Pt.
            if status is False:
                error_msg = "Unable to get Pt. of experiment %d scan %d due to %s." % (exp_number, scan_number, str(pt_number_list))
                self.controller.set_zero_peak_intensity(exp_number, scan_number)
                self.ui.tableWidget_mergeScans.set_peak_intensity(row_number, scan_number, 0.0, False, integrate_method="")
                self.ui.tableWidget_mergeScans.set_status(scan_number, error_msg)
                continue

            # merge all Pt. of the scan if they are not merged.
            merged = self.ui.tableWidget_mergeScans.get_merged_status(row_number)

            # add to list
            scan_number_list.append((scan_number, pt_number_list, merged))
            self.ui.tableWidget_mergeScans.set_status(row_number, "Waiting")
        # END-FOR

        # set the progress bar
        self.ui.progressBar_mergeScans.setRange(0, len(scan_number_list))
        self.ui.progressBar_mergeScans.setValue(0)
        self.ui.progressBar_mergeScans.setTextVisible(True)
        self.ui.progressBar_mergeScans.setStatusTip("Hello")

        # process background setup
        status, ret_obj = gutil.parse_integers_editors(
            [self.ui.lineEdit_numPt4BackgroundLeft, self.ui.lineEdit_numPt4BackgroundRight], allow_blank=False
        )
        if not status:
            error_msg = str(ret_obj)
            self.pop_one_button_dialog(error_msg)
            return
        num_pt_bg_left = ret_obj[0]
        num_pt_bg_right = ret_obj[1]

        # scale factor:
        scale_factor = float(self.ui.lineEdit_scaleFactor.text())

        # initialize a thread and start
        self._myIntegratePeaksThread = thread_pool.IntegratePeaksThread(
            self,
            exp_number,
            scan_number_list,
            mask_det,
            selected_mask,
            norm_type,
            num_pt_bg_left,
            num_pt_bg_right,
            scale_factor=scale_factor,
        )
        self._myIntegratePeaksThread.start()

    def do_index_ub_peaks(self):
        """Index the peaks in the UB matrix peak table
        :return:
        """
        # Get UB matrix
        ub_matrix = self.ui.tableWidget_ubMatrix.get_matrix()
        self._show_message("[Info] Get UB matrix from table: {0}".format(ub_matrix))

        # Index all peaks
        num_peaks = self.ui.tableWidget_peaksCalUB.rowCount()
        err_msg = ""
        for i_peak in range(num_peaks):
            scan_no = self.ui.tableWidget_peaksCalUB.get_exp_info(i_peak)[0]
            status, ret_obj = self._myControl.index_peak(ub_matrix, scan_number=scan_no)
            if status is True:
                hkl_value = ret_obj[0]
                hkl_error = ret_obj[1]
                self.ui.tableWidget_peaksCalUB.set_hkl(i_peak, hkl_value, is_spice_hkl=False, error=hkl_error)
            else:
                err_msg += ret_obj + "\n"
        # END-FOR

        # error message
        if len(err_msg) > 0:
            self.pop_one_button_dialog(err_msg)

        # update the message
        self.ui.lineEdit_peaksIndexedBy.setText(IndexFromUB)

        # enable/disable push buttons
        # self.ui.pushButton_setHKL2Int.setEnabled(True)
        # self.ui.pushButton_undoSetToInteger.setEnabled(True)

    def do_load_scan_info(self):
        """Load SIICE's scan file
        :return:
        """
        # Get scan number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_run])
        if status is True:
            scan_no = ret_obj[0]
        else:
            err_msg = ret_obj
            self.pop_one_button_dialog("Unable to get scan number in raw data tab due to %s." % err_msg)
            return

        status, err_msg = self._myControl.load_spice_scan_file(exp_no=None, scan_no=scan_no)
        if status is False:
            self.pop_one_button_dialog(err_msg)

    def do_load_survey(self):
        """Load csv file containing experiment-scan survey's result.
        :return:
        """
        # check validity
        num_rows = int(self.ui.lineEdit_numSurveyOutput.text())

        # get the csv file
        file_filter = "CSV Files (*.csv);;All Files (*)"
        csv_file_name = QFileDialog.getOpenFileName(self, "Open Exp-Scan Survey File", self._homeDir, file_filter)
        if not csv_file_name:  # return if file selection is cancelled
            return
        if isinstance(csv_file_name, tuple):
            csv_file_name = csv_file_name[0]

        # call controller to load
        survey_tuple = self._myControl.load_scan_survey_file(csv_file_name)
        scan_sum_list = survey_tuple[1]
        assert isinstance(scan_sum_list, list), "Returned value from load scan survey file must be a dictionary."

        # set the table
        self.ui.tableWidget_surveyTable.set_survey_result(scan_sum_list)
        self.ui.tableWidget_surveyTable.remove_all_rows()
        self.ui.tableWidget_surveyTable.show_reflections(num_rows)

    def do_plot_pt_raw(self):
        """Plot the Pt."""
        # Get measurement pt and the file number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp, self.ui.lineEdit_run, self.ui.lineEdit_rawDataPtNo])
        if status is True:
            exp_no = ret_obj[0]
            scan_no = ret_obj[1]
            pt_no = ret_obj[2]
        else:
            self.pop_one_button_dialog(f"Unable to plot detector counts as a 2D image due to {ret_obj}")
            return

        if self.ui.checkBox_autoMask.isChecked():
            roi_index = self.ui.comboBox_viewRawDataMasks.currentIndex()
            if roi_index == 0:
                roi_name = None
            else:
                roi_name = str(self.ui.comboBox_viewRawDataMasks.currentText())
        else:
            # if auto-mask flag is off, then no need to mask data
            roi_name = None

        self.load_plot_raw_data(exp_no, scan_no, pt_no, roi_name=roi_name)

    def do_plot_prev_pt_raw(self):
        """Plot the Pt."""
        # Get measurement pt and the file number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp, self.ui.lineEdit_run, self.ui.lineEdit_rawDataPtNo])
        if status is True:
            pt_no = ret_obj[2]
        else:
            self.pop_one_button_dialog(ret_obj)
            return

        # Previous one
        pt_no -= 1
        if pt_no <= 0:
            self.pop_one_button_dialog("Pt. = 1 is the first one.")
            return
        else:
            self.ui.lineEdit_rawDataPtNo.setText("%d" % pt_no)

        # Plot
        self.do_plot_pt_raw()

    def do_plot_prev_scan(self):
        """Plot the previous scan while keeping the same Pt.
        :return:
        """
        # get current exp number, scan number and pt number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_run])
        if status is False:
            error_msg = ret_obj
            self.pop_one_button_dialog(error_msg)
            return

        scan_number = ret_obj[0]

        # get next scan
        scan_number -= 1
        if scan_number < 0:
            self.pop_one_button_dialog("Scan number cannot be negative!")
            return

        # update line edits
        self.ui.lineEdit_run.setText(str(scan_number))

        #
        self.do_plot_pt_raw()

    def do_plot_next_pt_raw(self):
        """Plot the Pt."""
        # Get measurement pt and the file number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp, self.ui.lineEdit_run, self.ui.lineEdit_rawDataPtNo])
        if status:
            exp_no = ret_obj[0]
            scan_no = ret_obj[1]
            pt_no = ret_obj[2]
        else:
            self.pop_one_button_dialog(ret_obj)
            return

        # Next Pt
        pt_no += 1
        # get last Pt. number
        status, last_pt_no = self._myControl.get_pt_numbers(exp_no, scan_no)
        if status is False:
            error_message = last_pt_no
            self.pop_one_button_dialog('Unable to access Spice table for scan %d. Reason" %s.' % (scan_no, error_message))
        if pt_no > last_pt_no:
            self.pop_one_button_dialog("Pt. = %d is the last one of scan %d." % (pt_no, scan_no))
            return
        else:
            self.ui.lineEdit_rawDataPtNo.setText("%d" % pt_no)

        # Plot
        self.do_plot_pt_raw()

        # self.load_plot_raw_data(exp_no, scan_no, pt_no)

    def do_plot_next_scan(self):
        """Plot the next scan while keeping the same Pt.
        :return:
        """
        # get current exp number, scan number and pt number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_run])
        if status is False:
            error_msg = ret_obj
            self.pop_one_button_dialog(error_msg)
            return

        scan_number = ret_obj[0]

        # get next scan
        scan_number += 1

        self.do_plot_pt_raw()

        # plot
        # try:
        #     self.load_plot_raw_data(exp_number, scan_number, pt_number)
        # except RuntimeError as err:
        #     error_msg = 'Unable to plot next scan %d due to %s.' % (scan_number, str(err))
        #     self.pop_one_button_dialog(error_msg)
        #     return

        # update line edits
        self.ui.lineEdit_run.setText(str(scan_number))

    def do_load_nth_project(self):
        """
        Load the n-th saved project listed in the last tab
        :return:
        """
        # from the radio buttons to find the project that is selected to load
        if self.ui.radioButton_useLast1.isChecked():
            project_file_name = str(self.ui.label_last1Path.text())
        elif self.ui.radioButton_useLast2.isChecked():
            project_file_name = str(self.ui.label_last2Path.text())
        elif self.ui.radioButton_useLast3.isChecked():
            project_file_name = str(self.ui.label_last3Path.text())
        else:
            raise RuntimeError("None of the saved project is selected.")

        # load project
        self.load_project(project_file_name)

    def do_mask_pt_2d(self):
        """Save current in-edit ROI Mask a Pt and re-plot with current selected ROI or others
        :return:
        """
        # # get the experiment and scan value
        # status, par_val_list = gutil.parse_integers_editors([self.ui.lineEdit_exp, self.ui.lineEdit_run])
        # if not status:
        #     raise RuntimeError('Experiment number and Scan number must be given!')
        # exp_number = par_val_list[0]
        # scan_number = par_val_list[1]

        # get the user specified name from ...
        roi_name, ok = QInputDialog.getText(self, "Input Mask Name", "Enter mask name:")

        # return if cancelled
        if not ok:
            return
        roi_name = str(roi_name)

        # check whether this ROI name is used or not. If it is, warn user if the given ROI is used already
        current_roi_names = [str(self.ui.comboBox_maskNames1.itemText(i)) for i in range(self.ui.comboBox_maskNames1.count())]
        if roi_name in current_roi_names:
            self.pop_one_button_dialog(
                "[Warning] ROI name {} is used before.  The previous ROI will be overwritten by the new defined.".format(roi_name)
            )

        # get current ROI
        ll_corner, ur_corner = self.ui.graphicsView_detector2dPlot.get_roi()

        # set ROI
        self._myControl.set_roi(roi_name, ll_corner, ur_corner)

        # set it to combo-box
        self._roiComboBoxMutex = True
        self.ui.comboBox_maskNames1.addItem(roi_name)
        self.ui.comboBox_maskNames2.addItem(roi_name)
        self.ui.comboBox_maskNamesSurvey.addItem(roi_name)
        self.ui.comboBox_viewRawDataMasks.addItem(roi_name)
        self.ui.comboBox_viewRawDataMasks.setCurrentIndex(self.ui.comboBox_viewRawDataMasks.count() - 1)
        self._roiComboBoxMutex = False

        # get experiment, scan
        status, ret_obj = gutil.parse_integers_editors(
            [self.ui.lineEdit_exp, self.ui.lineEdit_run, self.ui.lineEdit_rawDataPtNo], allow_blank=False
        )
        if status:
            exp, scan, pt = ret_obj
        else:
            self.pop_one_button_dialog(ret_obj)
            return

        # previously saved ROI
        if self._myControl.has_roi_generated(roi_name) is False:
            roi_start, roi_end = self._myControl.get_region_of_interest(roi_name)
            status, mask_ws_name = self._myControl.generate_mask_workspace(
                exp, scan, roi_start=roi_start, roi_end=roi_end, mask_tag=roi_name
            )
            if status:
                self._myControl.set_roi_workspace(roi_name, mask_ws_name)
        # END-IF

        # plot
        self.load_plot_raw_data(exp, scan, pt, roi_name=roi_name)

        # switch ROI edit mode
        self.do_switch_roi_mode()

    def do_merge_multi_scans(self):
        """
        Merge several scans to a single MDWorkspace and give suggestion for re-binning
        :return:
        """
        # find the selected scans
        selected_rows = self.ui.tableWidget_mergeScans.get_selected_rows(True)
        if len(selected_rows) < 2:
            self.pop_one_button_dialog("Merging multiple scans requires more than 1 scan to be selected.")
            return

        # find out the merged workspace is in Q or hkl
        convert_to_hkl = self.ui.radioButton_hkl.isChecked()

        # get the information from table workspace
        exp_number = int(str(self.ui.lineEdit_exp.text()))
        md_ws_list = list()
        peak_center_list = list()
        for i_row in selected_rows:
            # get scan number and md workspace number
            scan_number = self.ui.tableWidget_mergeScans.get_scan_number(i_row)
            md_ws_name = self.ui.tableWidget_mergeScans.get_merged_ws_name(i_row)
            if convert_to_hkl:
                status, ret_obj = self._myControl.get_pt_numbers(exp_number, scan_number)
                if not status:
                    raise RuntimeError("It is not possible to fail to get Pt number list at this stage.Error is due to %s." % str(ret_obj))
                pt_list = ret_obj
                md_ws_name = hb3a_util.get_merged_hkl_md_name(self._instrument, exp_number, scan_number, pt_list)
            md_ws_list.append(md_ws_name)
            # get peak center in 3-tuple
            peak_center = self._myControl.get_peak_info(exp_number, scan_number).get_peak_centre()
            assert peak_center is not None, "Exp/Scan/Pt %s does not exist in PeakInfo dictionary."
            peak_center_list.append(peak_center)
        # END-FOR

        # ask name for the merged workspace
        merged_ws_name, status = gutil.get_value(self)

        # call the controller to merge the scans
        message = self._myControl.merge_multiple_scans(md_ws_list, peak_center_list, merged_ws_name)

        # information
        self.pop_one_button_dialog(message)

    def do_merge_scans(self):
        """Merge each selected scans in the tab-merge-scans
        :return:
        """
        # Get UB matrix and set to control
        ub_matrix = self.ui.tableWidget_ubInUse.get_matrix()
        self._myControl.set_ub_matrix(exp_number=None, ub_matrix=ub_matrix)

        # Warning
        self.pop_one_button_dialog("Merging scans can take long time. Please be patient!")

        # Process
        row_number_list = self.ui.tableWidget_mergeScans.get_selected_rows(True)
        exp_number = int(str(self.ui.lineEdit_exp.text()))

        sum_error_msg = ""

        for row_number in row_number_list:
            # get row number
            scan_number = self.ui.tableWidget_mergeScans.get_scan_number(row_number)

            # check SPICE file's existence. if not, then download it
            self._myControl.download_spice_file(exp_number, scan_number, over_write=False)

            status, pt_list = self._myControl.get_pt_numbers(exp_number, scan_number)
            if status is False:
                # skip this row due to error
                sum_error_msg += "%s\n" % str(pt_list)
                continue

            self.ui.tableWidget_mergeScans.set_status(row_number, "In Processing")
            status, ret_tup = self._myControl.merge_pts_in_scan(
                exp_no=exp_number, scan_no=scan_number, pt_num_list=[], rewrite=False, preprocessed_dir=self._myControl.pre_processed_dir
            )
            # find peaks too
            status, ret_obj = self._myControl.find_peak(exp_number, scan_number)

            # process output
            if status:
                assert len(ret_tup) == 2
                merge_status = "Merged"
                merged_name = ret_tup[0]
            else:
                merge_status = "Failed. Reason: %s" % ret_tup
                merged_name = "x"

            # update table
            self.ui.tableWidget_mergeScans.set_status(row_number, merge_status)
            self.ui.tableWidget_mergeScans.set_ws_name(row_number, merged_name)
            # if peak_centre is not None:
            #     self.ui.tableWidget_mergeScans.set_peak_centre(row_number, peak_centre)

            # Sleep for a while
            time.sleep(0.1)
        # END-FOR

    def do_merge_scans_survey(self):
        """
        Merge each selected scans in the 'List Scans' tab to Q-sample space
        :return:
        """
        # get the selected scans
        scan_run_list = self.ui.tableWidget_surveyTable.get_selected_run_surveyed(required_size=None)
        if len(scan_run_list) == 0:
            self.pop_one_button_dialog("There is no run that is selected.")

        # start to add scan/run to table
        # Set table
        scan_list = list()
        for scan, pt in scan_run_list:
            scan_list.append(scan)
        scan_list.sort()
        self.ui.tableWidget_mergeScans.append_scans(scans=scan_list, allow_duplicate_scans=False)

        # switch tab
        self.ui.tabWidget.setCurrentIndex(MainWindow.TabPage["Scans Processing"])

    def do_refine_ub_indexed_peaks(self):
        """
        Refine UB matrix by indexed peaks
        :return:
        """
        # refine UB matrix by indexed peak
        peak_info_list = self._build_peak_info_list(zero_hkl=False)
        if peak_info_list is None:
            return

        # Refine UB matrix
        try:
            self._myControl.refine_ub_matrix_indexed_peaks(peak_info_list)
        except AssertionError as error:
            self.pop_one_button_dialog(str(error))
            return

        # show result
        self._show_refined_ub_result()

    def do_refine_ub_cal_indexed_peaks(self):
        """
        refine UB matrix by indexed peaks with HKL calculated
        :return:
        """
        # refine UB matrix by indexed peak
        peak_info_list = self._build_peak_info_list(zero_hkl=False, is_spice=False)
        if peak_info_list is None:
            return

        # Refine UB matrix
        try:
            self._myControl.refine_ub_matrix_indexed_peaks(peak_info_list)
        except AssertionError as error:
            self.pop_one_button_dialog(str(error))
            return

        # show result
        self._show_refined_ub_result()

    def do_refine_ub_lattice(self):
        """
        Calculate UB matrix constrained by lattice parameters
        :return:
        """
        # launch the set up window
        if self._refineConfigWindow is None:
            self._refineConfigWindow = ol_window.OptimizeLatticeWindow(self)

        self._refineConfigWindow.set_prev_ub_refine_method(False)
        self._refineConfigWindow.show()

    def load_project(self, project_file_name):
        """
        Load a saved project with all the setup loaded to memory
        :param project_file_name:
        :return:
        """
        assert isinstance(project_file_name, str), "Project file name %s must be a string but not %s." % (
            str(project_file_name),
            type(project_file_name),
        )
        assert os.path.exists(project_file_name), 'Project file "%s" cannot be found.' % project_file_name

        # load project
        ui_dict, err_msg = self._myControl.load_project(project_file_name)

        # get experiment number and IPTS number
        exp_number = int(ui_dict["exp number"])
        self.ui.lineEdit_exp.setText(str(exp_number))
        if "ipts" in ui_dict and ui_dict["ipts"] is not None:
            self.ui.lineEdit_iptsNumber.setText(str(ui_dict["ipts"]))

        # set the UI parameters to GUI
        try:
            self.ui.lineEdit_localSpiceDir.setText(ui_dict["local spice dir"])
            self.ui.lineEdit_workDir.setText(ui_dict["work dir"])
            self.ui.lineEdit_surveyStartPt.setText(ui_dict["survey start"])
            self.ui.lineEdit_surveyEndPt.setText(ui_dict["survey stop"])

            # now try to call some actions
            self.do_apply_setup()
            self.do_set_experiment()

        except KeyError:
            self._show_message("[Error] Some field cannot be found from project file {0}".format(project_file_name))

        # set experiment configurations
        # set sample distance
        if "det_sample_distance" in ui_dict and ui_dict["det_sample_distance"] is not None:
            det_sample_distance = float(ui_dict["det_sample_distance"])
            self.ui.lineEdit_infoDetSampleDistance.setText(str(det_sample_distance))
            self._myControl.set_default_detector_sample_distance(det_sample_distance)

        # set user-specified wave length
        if "wave_length" in ui_dict and ui_dict["wave_length"] is not None:
            wave_length = float(ui_dict["wave_length"])
            self.ui.lineEdit_infoWavelength.setText(str(wave_length))
            self._myControl.set_user_wave_length(exp_number, wave_length)

        if "det_center" in ui_dict and ui_dict["det_center"] is not None:
            det_center_str = ui_dict["det_center"].strip()
            terms = det_center_str.split(",")
            center_row = int(terms[0].strip())
            center_col = int(terms[1].strip())
            self.ui.lineEdit_infoDetCenter.setText("{0}, {1}".format(center_row, center_col))
            self._myControl.set_detector_center(exp_number, center_row, center_col)

        # pop out a dialog to notify the completion
        if err_msg is not None:
            self.pop_one_button_dialog("Encountered these errors from loading:\n{0}".format(err_msg))

        message = "Project from file {0} is loaded.".format(project_file_name)
        self.pop_one_button_dialog(message)

    def process_single_pt_scan_intensity(self, scan_integrate_info_dict):
        """
        process integrated single pt scan
        :param scan_integrate_info_dict:
        :return:
        """
        # check inputs
        assert isinstance(scan_integrate_info_dict, dict), "Input scan-pt pairs {0} must be in a dict but not a {1}".format(
            scan_integrate_info_dict, type(scan_integrate_info_dict)
        )

        # get intensities
        for scan_number in scan_integrate_info_dict:
            # self._single_pt_scan_intensity_dict: not been defined and used at all!
            # TODO NOW3 : Check whether the intensity is recorded in the single_measurement already???
            intensity, roi_name = scan_integrate_info_dict[scan_number]
            self.ui.tableWidget_mergeScans.add_single_measure_scan(scan_number, intensity, roi_name)
        # END-FOR

    # add slot for UB refinement configuration window's signal to connect to
    # @QtCore.pyqtSlot(int)
    def refine_ub_lattice(self, val):
        """
        Refine UB matrix by constraining on lattice type.
        Required inputs:
          1. unit cell type
          2. peak information include
        :param val:
        :return:
        """
        # check signal for hand shaking
        if val != 1000:
            raise RuntimeError("Signal value %s is not an authorized signal value (1000)." % str(val))

        # it is supposed to get the information back from the window
        unit_cell_type = self._refineConfigWindow.get_unit_cell_type()

        # get peak information list
        peak_info_list = self._build_peak_info_list(zero_hkl=False)
        if peak_info_list is None:
            return

        # get the UB matrix value
        ub_src_tab = self._refineConfigWindow.get_ub_source()
        if ub_src_tab == 3:
            self._show_message('UB matrix comes from tab "Calculate UB".')
            ub_matrix = self.ui.tableWidget_ubMatrix.get_matrix_str()
        elif ub_src_tab == 4:
            self._show_message('UB matrix comes from tab "UB Matrix".')
            ub_matrix = self.ui.tableWidget_ubInUse.get_matrix_str()
        else:
            self.pop_one_button_dialog("UB source tab %s is not supported." % str(ub_src_tab))
            return

        # refine UB matrix by constraint on lattice parameters
        status, error_message = self._myControl.refine_ub_matrix_by_lattice(peak_info_list, ub_matrix, unit_cell_type)
        if status:
            # successfully refine the lattice and UB matrix
            self._show_refined_ub_result()
        else:
            self.pop_one_button_dialog(error_message)

    def do_refine_ub_fft(self):
        """
        Refine UB matrix by calling FFT method
        :return:
        """
        dlg = refineubfftsetup.RefineUBFFTSetupDialog(self)
        if dlg.exec_():
            # Do stuff with values
            min_d, max_d, tolerance = dlg.get_values()
        else:
            # case for cancel
            return

        # launch the dialog to get min D and max D
        if (0 < min_d < max_d) is False:
            self.pop_one_button_dialog("Range of d is not correct! FYI, min D = %.5f, max D = %.5f." % (min_d, max_d))
            return

        # get PeakInfo list and check
        peak_info_list = self._build_peak_info_list(zero_hkl=True)
        if peak_info_list is None:
            return
        assert isinstance(peak_info_list, list), "PeakInfo list must be a list but not %s." % str(type(peak_info_list))
        assert len(peak_info_list) >= 3, "PeakInfo must be larger or equal to 3 (.now given %d) to refine UB matrix" % len(peak_info_list)

        # friendly suggestion
        if len(peak_info_list) <= 9:
            self.pop_one_button_dialog("It is recommended to use at least 9 reflections to refine UB matrix without prior knowledge.")

        # refine
        try:
            self._myControl.refine_ub_matrix_least_info(peak_info_list, min_d, max_d, tolerance)
        except RuntimeError as runtime_error:
            self.pop_one_button_dialog(str(runtime_error))
            return

        # set value
        self._show_refined_ub_result()

        # set HKL to zero
        self.do_clear_all_peaks_index_ub()

    def do_refresh_merged_scans_table(self):
        """Find the merged
        :return:
        """
        # find out the merged runs
        scan_info_tup_list = self._myControl.get_merged_scans()

        # get existing scan numbers
        existing_scan_number_list = self.ui.tableWidget_mergeScans.get_selected_scans()

        # append the row to the merged scan table
        for scan_info_tup in scan_info_tup_list:
            scan_number = scan_info_tup[1]
            # skip if existing
            if scan_number in existing_scan_number_list:
                continue

            exp_number = scan_info_tup[0]
            pt_number_list = scan_info_tup[2]
            ws_name = hb3a_util.get_merged_md_name("HB3A", exp_number, scan_number, pt_number_list)
            self.ui.tableWidget_mergeScans.add_new_merged_data(exp_number, scan_number, ws_name)

    def do_reset_ub_peaks_hkl(self):
        """
        Reset user specified HKL value to peak table
        :return:
        """
        # get experiment number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp])
        if not status:
            raise RuntimeError(ret_obj)
        else:
            exp_number = ret_obj[0]

        # reset all rows back to SPICE HKL
        num_rows = self.ui.tableWidget_peaksCalUB.rowCount()
        for i_row in range(num_rows):
            scan, pt = self.ui.tableWidget_peaksCalUB.get_scan_pt(i_row)
            if pt < 0:
                pt = None
            peak_info = self._myControl.get_peak_info(exp_number, scan, pt)
            h, k, l = peak_info.get_hkl(user_hkl=False)
            self.ui.tableWidget_peaksCalUB.update_hkl(i_row, h, k, l)
        # END-FOR

        # set the flag right
        self.ui.lineEdit_peaksIndexedBy.setText(IndexFromSpice)

    def do_save_survey(self):
        """
        Save the survey to a file
        :return:
        """
        # Get file name
        file_filter = "CSV Files (*.csv);;All Files (*)"
        out_file_name = QFileDialog.getSaveFileName(self, "Save scan survey result", self._homeDir, file_filter)
        if isinstance(out_file_name, tuple):
            out_file_name = out_file_name[0]

        # Save file
        self._myControl.save_scan_survey(out_file_name)

    def do_save_ub(self):
        """Save the in-use Matrix to an ASCII file
        :return:
        """
        # get file name
        file_filter = "Data Files (*.dat);;All Files (*)"
        ub_file_name = QFileDialog.getSaveFileName(self, "ASCII File To Save UB Matrix", self._homeDir, file_filter)

        # early return if user cancels selecting a file name to save
        if not ub_file_name:
            return
        if isinstance(ub_file_name, tuple):
            ub_file_name = ub_file_name[0]

        # get UB matrix
        in_use_ub = self.ui.tableWidget_ubInUse.get_matrix()
        ub_shape = in_use_ub.shape
        assert len(ub_shape) == 2 and ub_shape[0] == ub_shape[1] == 3

        # construct string
        ub_str = "# UB matrix\n# %s\n" % str(self.ui.label_ubInfoLabel.text())
        for i_row in range(3):
            for j_col in range(3):
                ub_str += "%.15f  " % in_use_ub[i_row][j_col]
            ub_str += "\n"

        # write to file
        ub_file = open(ub_file_name, "w")
        ub_file.write(ub_str)
        ub_file.close()

    def do_select_all_peaks(self):
        """
        Purpose: select all peaks in table tableWidget_peaksCalUB
        :return:
        """
        if self.ui.radioButton_ubSelectAllScans.isChecked() and self._ubPeakTableFlag != 0:
            self.ui.tableWidget_peaksCalUB.select_all_rows(True)
            self._ubPeakTableFlag = 0
        elif self.ui.radioButton_ubSelectNoScan.isChecked() and self._ubPeakTableFlag != 1:
            self.ui.tableWidget_peaksCalUB.select_all_rows(False)
            self._ubPeakTableFlag = 1
        elif self.ui.radioButton_ubAdvancedSelection.isChecked() and self._ubPeakTableFlag != 2:
            # advanced
            import FindUBUtility

            self._selectUBScanDialog = FindUBUtility.SelectUBMatrixScansDialog(self)
            self._selectUBScanDialog.show()
            self._ubPeakTableFlag = 2
        # END-IF

        # if not self._ubPeakTableFlag:
        #     # turn to deselect all
        #     self.ui.tableWidget_peaksCalUB.select_all_rows(self._ubPeakTableFlag)
        # elif self.ui.checkBox_ubNuclearPeaks.isChecked() is False:
        #     # all peaks are subjected to select
        #     self.ui.tableWidget_peaksCalUB.select_all_rows(self._ubPeakTableFlag)
        # else:
        #     # only nuclear peaks to get selected
        #     self.ui.tableWidget_peaksCalUB.select_all_nuclear_peaks()
        # # END-IF-ELSE
        #
        # # revert the flag
        # self._ubPeakTableFlag = not self._ubPeakTableFlag

    # TEST NOW
    def do_select_all_survey(self):
        """
        Select or de-select all rows in survey items
        :return:
        """
        if self._surveyTableFlag:
            # there are cases: select all or select selected few with filters
            # check whether there is any option
            if self.ui.checkBox_surveySelectNuclearPeaks.isChecked() or self.ui.checkBox_singlePtScans.isChecked():
                # deselect all rows for future selection
                self.ui.tableWidget_surveyTable.select_all_rows(False)

            # go through all rows
            num_rows = self.ui.tableWidget_surveyTable.rowCount()
            for i_row in range(num_rows):
                select_line = True

                # filter HKL (nuclear)
                if self.ui.checkBox_surveySelectNuclearPeaks.isChecked():
                    # only select nuclear peaks
                    peak_hkl = self.ui.tableWidget_surveyTable.get_hkl(i_row)
                    select_line = peak_util.is_peak_nuclear(peak_hkl[0], peak_hkl[1], peak_hkl[2]) and select_line

                if self.ui.checkBox_singlePtScans.isChecked():
                    # only select scan with single Pt
                    scan_number = self.ui.tableWidget_surveyTable.get_scan_numbers([i_row])[0]
                    status, pt_list = self._myControl.get_pt_numbers(self._current_exp_number, scan_number)
                    select_line = (status and len(pt_list) == 1) and select_line

                if select_line:
                    self.ui.tableWidget_surveyTable.select_row(i_row, True)
            # END-FOR(i_row)

        else:
            # de-select all peaks
            self.ui.tableWidget_surveyTable.select_all_rows(False)
        # END-IF-ELSE

        # flip the flag for next select
        self._surveyTableFlag = not self._surveyTableFlag

    def do_select_merged_scans(self):
        """Select or deselect all rows in the merged scan table
        :return:
        """
        # get current status
        curr_state = str(self.ui.pushButton_selectAllScans2Merge.text()).startswith("Select")

        # select/deselect all
        self.ui.tableWidget_mergeScans.select_all_rows(curr_state)

        # set the text to the push button
        if curr_state:
            self.ui.pushButton_selectAllScans2Merge.setText("Deselect All")
        else:
            self.ui.pushButton_selectAllScans2Merge.setText("Select All")

    def do_set_all_calibration(self):
        """
        set up all the calibration parameters
        :return:
        """
        self.do_set_user_detector_center()
        self.do_set_user_detector_distance()
        self.do_set_user_wave_length()
        self.do_set_detector_size()

    def do_set_detector_size(self):
        """
        set the detector size to controller
        :return:
        """
        det_size_str = str(self.ui.comboBox_detectorSize.currentText())

        if det_size_str.count("256") > 0:
            # 256 by 256 pixels
            det_size_row = 256
            det_size_col = 256
        elif det_size_str.count("512") > 0:
            # 512 x 512
            det_size_row = 512
            det_size_col = 512
        else:
            # unsupported case yet
            raise NotImplementedError("Detector with size {0} is not supported yet.".format(det_size_str))

        # set to controller
        self._myControl.set_detector_geometry(det_size_row, det_size_col)

    def do_set_ipts_number(self):
        """
        set IPTS number
        :return:
        """
        # get IPTS number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_iptsNumber])
        if status:
            # a valid IPTS number

            raise NotImplementedError("The following section commented out now will be implemented when IPTS is ready.")
            # ipts_number = ret_obj[0]
            # search archive for available experiment number under this IPTS
            # status, ret_obj = self._myControl.check_ipts(ipts_number=ipts_number)
            # if status:
            #     exp_number_list = ret_obj
            #     self._iptsNumber = ipts_number
            #     self.ui.comboBox_expInIPTS.clear()
            #     for exp_number in exp_number_list:
            #         self.ui.comboBox_expInIPTS.addItem(str(exp_number))
            # else:
            #     self.pop_one_button_dialog('Unable to locate IPTS {0} due to {1}'.format(ipts_number, ret_obj))
            #     return
        else:
            # error
            self.pop_one_button_dialog("User specified IPTS number {0} is not correct.".format(str(self.ui.lineEdit_iptsNumber.text())))

    def do_set_experiment(self):
        """Set experiment
        :return:
        """
        # get exp number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp])

        if status:
            # new experiment number
            exp_number = ret_obj[0]
            # current experiment to be replaced: warning
            curr_exp_number = self._myControl.get_experiment()
            if curr_exp_number is not None and exp_number != curr_exp_number:
                self.pop_one_button_dialog(
                    "Changing experiment to %d.  Clean previous experiment %d's result in Mantid manually." % (exp_number, curr_exp_number)
                )
            # set the new experiment number
            self._myControl.set_exp_number(exp_number)
            self.ui.lineEdit_exp.setStyleSheet("color: black")
            self.setWindowTitle("Experiment %d" % exp_number)

            # try to set the default
            if self._iptsNumber is not None:
                default_data_dir = "/HFIR/HB3A/IPTS-{0}/exp{1}/Datafiles".format(self._iptsNumber, exp_number)
            else:
                default_data_dir = "/HFIR/HB3A/exp{0}/Datafiles".format(exp_number)
            if os.path.exists(default_data_dir):
                # set the directory in
                self.ui.lineEdit_localSpiceDir.setText(default_data_dir)
                # find out the detector type
                status, ret_obj = self._myControl.find_detector_size(default_data_dir, exp_number)

            # TEST new feature
            # Check working directory.  If not set or with different exp number, then set to
            work_dir = str(self.ui.lineEdit_workDir.text()).strip()
            if work_dir != "":
                # check whether it contains exp number other than current one
                if work_dir.lower().count("exp") > 0 and work_dir.count("{0}".format(exp_number)) == 0:
                    use_default = True
                else:
                    use_default = False
            else:
                use_default = False
            if use_default:
                # set default to analysis cluster or local
                work_dir = "/HFIR/HB3A/exp{0}/Shared/".format(exp_number)
                # check whether user has writing permission
                if os.access(work_dir, os.W_OK) is False:
                    work_dir = os.path.join(os.path.expanduser("~"), "HB3A/Exp{0}".format(exp_number))
                self.ui.lineEdit_workDir.setText(work_dir)
            # END-IF

            # TEST new feature
            # Check pre-processing directory.  If not set or with different exp number, then set to
            pre_process_dir = str(self.ui.lineEdit_preprocessedDir.text()).strip()
            if pre_process_dir == "":
                # empty
                use_default = True
            elif pre_process_dir.lower().count("exp") > 0 and pre_process_dir.count("{0}".format(exp_number)) > 0:
                # doesn't sound right
                use_default = True
            else:
                # other cases
                use_default = False
            if use_default:
                # shared Example: /HFIR/HB3A/exp668/Shared/preprocessed
                pre_process_dir = "/HFIR/HB3A/exp{0}/Shared/preprocessed".format(exp_number)
                # check whether user has writing permission
                if os.access(pre_process_dir, os.W_OK) is False:
                    # use local
                    pre_process_dir = os.path.join(os.path.expanduser("~"), "HB3A/Exp{0}/preprocessed".format(exp_number))
                self.ui.lineEdit_preprocessedDir.setText(pre_process_dir)
            # END-IF

        else:
            err_msg = ret_obj
            self.pop_one_button_dialog("Unable to set experiment as %s" % err_msg)
            self.ui.lineEdit_exp.setStyleSheet("color: red")
            return

        # register experiment number
        self._current_exp_number = exp_number

        self.ui.tabWidget.setCurrentIndex(0)

        # set the instrument geometry constants
        status, ret_obj = gutil.parse_float_editors(
            [self.ui.lineEdit_defaultSampleDetDistance, self.ui.lineEdit_pixelSizeX, self.ui.lineEdit_pixelSizeY], allow_blank=False
        )
        if status:
            default_det_sample_distance, pixel_x_size, pixel_y_size = ret_obj
            self._myControl.set_default_detector_sample_distance(default_det_sample_distance)
            self._myControl.set_default_pixel_size(pixel_x_size, pixel_y_size)
        else:
            self.pop_one_button_dialog("[ERROR] Unable to parse default instrument geometry constants due to %s." % str(ret_obj))
            return

        # set the detector center
        det_center_str = str(self.ui.lineEdit_defaultDetCenter.text())
        try:
            terms = det_center_str.split(",")
            center_row = int(terms[0])
            center_col = int(terms[1])
            self._myControl.set_detector_center(exp_number, center_row, center_col, default=True)
        except (IndexError, ValueError) as error:
            self.pop_one_button_dialog("[ERROR] Unable to parse default detector center %s due to %s." % (det_center_str, str(error)))

    def do_set_ub_tab_hkl_to_integers(self):
        """
        Set all peaks' indexing (HKL) to integer in the table in "UB matrix calculation" tab.
        Change to these HKL values is only related to GUI, i.e., the table
        :return:
        """
        # get the current index source
        hkl_type_str = str(self.ui.comboBox_hklType.currentText())
        if hkl_type_str.lower().count("spice") > 0:
            # set spice HKL to integer
            is_spice = True
        else:
            is_spice = False

        # store the current value
        self.ui.tableWidget_peaksCalUB.store_current_indexing()

        # set the index to integer
        num_rows = self.ui.tableWidget_peaksCalUB.rowCount()
        for row_index in range(num_rows):
            try:
                m_h, m_l, m_k = self.ui.tableWidget_peaksCalUB.get_hkl(row_index, is_spice_hkl=is_spice)
                peak_indexing, round_error = hb3a_util.convert_hkl_to_integer(m_h, m_l, m_k, MAGNETIC_TOL)
                self.ui.tableWidget_peaksCalUB.set_hkl(row_index, peak_indexing, is_spice, round_error)
            except RuntimeError as run_err:
                scan_number, pt_number = self.ui.tableWidget_peaksCalUB.get_scan_pt(row_index)
                print("[ERROR] Unable to convert HKL to integer for scan {0} due to {1}.".format(scan_number, run_err))
        # END-FOR

        # disable the set to integer button and enable the revert/undo button
        # self.ui.pushButton_setHKL2Int.setEnabled(False)
        # self.ui.pushButton_undoSetToInteger.setEnabled(True)

    def do_toggle_table_integration(self):
        """
        change the type
        :return:
        """
        exp_number = int(self.ui.lineEdit_exp.text())

        integrate_type = self.ui.tableWidget_mergeScans.get_integration_type()
        if integrate_type == "simple":
            integrate_type = "mixed"
        elif integrate_type == "mixed":
            integrate_type = "gaussian"
        else:
            integrate_type = "simple"

        err_msg = ""
        for i_row in range(self.ui.tableWidget_mergeScans.rowCount()):
            scan_number = self.ui.tableWidget_mergeScans.get_scan_number(i_row)
            peak_info_obj = self._myControl.get_peak_info(exp_number, scan_number)
            try:
                intensity1, error1 = peak_info_obj.get_intensity(integrate_type, False)
                intensity2, error2 = peak_info_obj.get_intensity(integrate_type, True)
                self.ui.tableWidget_mergeScans.set_peak_intensity(i_row, intensity1, intensity2, error2, integrate_type)
            except RuntimeError as run_err:
                err_msg += "{0} due to {1};".format(self.ui.tableWidget_mergeScans.get_scan_number(i_row), run_err)
        # END-FOR

        if len(err_msg) > 0:
            self._show_message("Unable to get peak intensity of scan: {0}".format(err_msg))

    def do_undo_ub_tab_hkl_to_integers(self):
        """
        After the peaks' indexing are set to integer, undo the action (i.e., revert to the original value)
        :return:
        """
        # get the column
        hkl_type = str(self.ui.comboBox_hklType.currentText())
        if hkl_type.lower().count("spice") > 0:
            is_spice = True
        else:
            is_spice = False

        # restore the value
        self.ui.tableWidget_peaksCalUB.restore_cached_indexing(is_spice)

        # enable and disable the buttons
        # self.ui.pushButton_setHKL2Int.setEnabled(True)
        # self.ui.pushButton_undoSetToInteger.setEnabled(False)

    def do_index_merged_scans_peaks(self):
        """Index all peaks' HKL value in the merged-peak tab by UB matrix that is just calculated
        :return:
        """
        # get the parameters
        exp_number = int(self.ui.lineEdit_exp.text())
        hkl_src = str(self.ui.comboBox_indexFrom.currentText())

        # loop through all rows
        message = ""
        num_rows = self.ui.tableWidget_mergeScans.rowCount()
        for row_index in range(num_rows):
            # get scan number
            scan_i = self.ui.tableWidget_mergeScans.get_scan_number(row_index)

            integral_type = self.ui.tableWidget_mergeScans.get_integration_type(row_index)
            is_single_pt = False
            if integral_type == "single-pt":
                roi_name = self.ui.tableWidget_mergeScans.get_roi_name(row_index)
                peak_info_i = self._myControl.get_single_pt_info(exp_number, scan_number=scan_i, pt_number=1, roi_name=roi_name)
                is_single_pt = True
            else:
                peak_info_i = self._myControl.get_peak_info(exp_number, scan_number=scan_i)
                # skip non-merged sans
                if peak_info_i is None:
                    message += "Row {0} Scan {1} is not integrated.\n".format(row_index, scan_i)
                    continue

            # get or calculate HKL
            if is_single_pt or hkl_src == "From SPICE":
                # get HKL from SPICE (non-user-hkl)
                hkl_i = peak_info_i.get_hkl(user_hkl=False)
            else:
                # calculate HKL from SPICE
                try:
                    ub_matrix = self._myControl.get_ub_matrix(exp_number)
                except KeyError as key_err:
                    self.pop_one_button_dialog("Unable to get UB matrix due to {0}.\nCheck whether UB matrix is set.".format(key_err))
                    return
                index_status, ret_tup = self._myControl.index_peak(ub_matrix, scan_i, allow_magnetic=True)
                if index_status:
                    hkl_i = ret_tup[0]
                else:
                    # unable to index peak. use fake hkl and set error message
                    hkl_i = [0, 0, 0]
                    error_msg = "Scan %d: %s" % (scan_i, str(ret_tup))
                    self.ui.tableWidget_mergeScans.set_status(scan_i, error_msg)
                # END-IF-ELSE(index)

                # set to peak info
                peak_info_i.set_hkl_np_array(hkl_i)

            # END-IF-ELSE (hkl_from_spice)

            # show by rounding HKL?
            if self.ui.checkBox_roundHKL.isChecked():
                hkl_i = [
                    hb3a_util.round_miller_index(hkl_i[0], 0.2),
                    hb3a_util.round_miller_index(hkl_i[1], 0.2),
                    hb3a_util.round_miller_index(hkl_i[2], 0.2),
                ]

            self.ui.tableWidget_mergeScans.set_hkl(row_index, hkl_i, hkl_src)
        # END-FOR

    def do_setup_dir_default(self):
        """
        Set up default directory for storing data and working
        If directory /HFIR/HB3A exists, it means that the user can access HFIR archive server
        :return:
        """
        home_dir = os.path.expanduser("~")

        # Data cache directory
        project_cache_dir = os.path.join(home_dir, "Temp/HB3ATest")
        if os.path.exists("/HFIR/HB3A/"):
            self.ui.lineEdit_localSrcDir.setText("/HFIR/HB3A/")
        else:
            self.ui.lineEdit_localSpiceDir.setText(project_cache_dir)

        # working directory
        work_dir = os.path.join(project_cache_dir, "Workspace")
        self.ui.lineEdit_workDir.setText(work_dir)

    def set_ub_from_file(self):
        """Get UB matrix from an Ascii file
        :return:
        """
        file_filter = "Data Files (*.dat);;Text Files (*.txt);;All Files (*)"
        file_name = QFileDialog.getOpenFileName(self, "Open UB ASCII File", self._homeDir, file_filter)
        if not file_name:  # quit if cancelled
            return
        if isinstance(file_name, tuple):
            file_name = file_name[0]

        # parse file
        ub_file = open(file_name, "r")
        raw_lines = ub_file.readlines()
        ub_file.close()

        # convert to ub string
        ub_str = ""
        for line in raw_lines:
            line = line.strip()
            if len(line) == 0:
                # empty line
                continue
            elif line[0] == "#":
                # comment line
                continue
            else:
                # regular line
                ub_str += "%s " % line

        # convert to matrix
        ub_matrix = gutil.convert_str_to_matrix(ub_str, (3, 3))

        return ub_matrix

    def do_set_user_detector_distance(self):
        """
        Set up the user-defined detector distance for loading instrument with data
        :return:
        """
        user_det_distance_str = str(self.ui.lineEdit_userDetSampleDistance.text()).strip()
        if len(user_det_distance_str) == 0:
            return

        # convert to float
        try:
            user_det_distance = float(user_det_distance_str)
        except ValueError:
            self.pop_one_button_dialog("User detector-sample distance %s must be a float." % user_det_distance_str)
            return

        # check distance value because it cannot be too far
        default_det_distance = float(str(self.ui.lineEdit_defaultSampleDetDistance.text()))
        distance_tol = float(str(self.ui.lineEdit_sampleDetDistTol.text()))
        if abs((user_det_distance - default_det_distance) / default_det_distance) > distance_tol:
            self.pop_one_button_dialog("User specified sample-detector distance is not reasonable.")
            return

        # set to controller
        exp_number = int(str(self.ui.lineEdit_exp.text()))
        self._myControl.set_detector_sample_distance(exp_number, user_det_distance)

        # update the GUI for information
        self.ui.lineEdit_infoDetSampleDistance.setText("%.5f" % user_det_distance)

    def do_set_user_wave_length(self):
        """

        :return:
        """
        try:
            exp_number = int(str(self.ui.lineEdit_exp.text()))
            user_lambda = float(str(self.ui.lineEdit_userWaveLength.text()))
        except ValueError:
            self.pop_one_button_dialog("Unable to set user wave length with value %s." % str(self.ui.lineEdit_infoWavelength.text()))
            return

        self._myControl.set_user_wave_length(exp_number, user_lambda)

        # set back to GUI
        self.ui.lineEdit_infoWavelength.setText("%.5f" % user_lambda)

    def do_set_user_detector_center(self):
        """
        set the user-defined detector center
        :return:
        """
        # get information
        status, ret_obj = gutil.parse_integers_editors(
            [self.ui.lineEdit_exp, self.ui.lineEdit_detCenterPixHorizontal, self.ui.lineEdit_detCenterPixVertical], allow_blank=True
        )

        if not status:
            self.pop_one_button_dialog(str(ret_obj))
            return

        assert isinstance(ret_obj, list) and len(ret_obj) == 3, "Error!"
        exp_number, user_center_row, user_center_col = ret_obj
        assert isinstance(exp_number, int), "Experiment number must be set up."

        self._myControl.set_detector_center(exp_number, user_center_row, user_center_col)

        # apply to the GUI
        self.ui.lineEdit_infoDetCenter.setText("%d, %d" % (user_center_row, user_center_col))

    def do_show_integration_details(self):
        """
        show the details (in table) about the integration of scans
        :return:
        """
        # check whether the integration information table
        if self._peakIntegrationInfoWindow is None:
            self._peakIntegrationInfoWindow = PeaksIntegrationReport.PeaksIntegrationReportDialog(self)

        # show
        self._peakIntegrationInfoWindow.show()

        # report
        report_dict = self.generate_peaks_integration_report()
        self._peakIntegrationInfoWindow.set_report(report_dict)

    def do_show_single_peak_integration(self):
        """
        pop out a dialog box to show the detailed integration information
        :return:
        """
        if self._mySinglePeakIntegrationDialog is None:
            self._mySinglePeakIntegrationDialog = message_dialog.MessageDialog(self)

        self._mySinglePeakIntegrationDialog.show()

    def do_show_spice_file(self):
        """
        Show SPICE file in a window
        :return:
        """
        # get the files from the GUI
        exp_number = int(str(self.ui.lineEdit_exp.text()))

        row_id_list = self.ui.tableWidget_surveyTable.get_selected_rows(True)
        scan_number_list = self.ui.tableWidget_surveyTable.get_scan_numbers(row_id_list)
        if len(scan_number_list) == 0:
            return

        # read the spice file into list of lines
        spice_line_list = self._myControl.read_spice_file(exp_number, scan_number_list[0])

        self._spiceViewer = viewspicedialog.ViewSpiceDialog(self)

        # Write each line
        wbuf = ""
        for line in spice_line_list:
            wbuf += line
        self._spiceViewer.write_text(wbuf)

        # show the new window
        self._spiceViewer.show()

    def do_show_spice_file_raw(self):
        """
        show SPICE file in a window from Raw-Tab
        :return:
        """
        # get the files from the GUI
        exp_number = int(str(self.ui.lineEdit_exp.text()))

        # get the scan number
        try:
            scan_number = int(str(self.ui.lineEdit_run.text()))
        except ValueError as val_err:
            error_msg = "Scan number {0} in raw-data-view-tab is invalid. FYI: {1}.".format(self.ui.lineEdit_run.text(), val_err)
            self.pop_one_button_dialog(error_msg)
            return

        # get spice file
        spice_line_list = self._myControl.read_spice_file(exp_number, scan_number)

        # launch SPICE view
        if self._spiceViewer is None:
            # create SPICE viewer if it does not exist
            self._spiceViewer = viewspicedialog.ViewSpiceDialog(self)

        # form the buffer
        spice_buffer = ""
        for line in spice_line_list:
            spice_buffer += line

        # write out the value
        self._spiceViewer.write_text(spice_buffer)

        # show
        self._spiceViewer.show()

    def do_show_ub_in_box(self):
        """Get UB matrix in table tableWidget_ubMergeScan and write to plain text edit plainTextEdit_ubInput
        :return:
        """
        ub_matrix = self.ui.tableWidget_ubInUse.get_matrix()

        text = ""
        for i in range(3):
            for j in range(3):
                text += "%.7f, " % ub_matrix[i][j]
            text += "\n"

        self.ui.plainTextEdit_ubInput.setPlainText(text)

    def do_show_workspaces(self):
        """
        pop out a dialog to show the workspace names that are selected
        :return:
        """
        # get number of rows that are selected
        row_number_list = self.ui.tableWidget_mergeScans.get_selected_rows(True)

        message = ""
        for row_number in row_number_list:
            scan_number = self.ui.tableWidget_mergeScans.get_scan_number(row_number)
            md_qample_ws_name = self.ui.tableWidget_mergeScans.get_merged_ws_name(row_number)
            message += "Scan %03d: %s\n" % (scan_number, md_qample_ws_name)
        # END-FOR

        gutil.show_message(message=message)

    def do_survey(self):
        """
        Purpose: survey for the strongest reflections
        :return:
        """
        # Get experiment number
        exp_number = int(self.ui.lineEdit_exp.text())
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_surveyStartPt, self.ui.lineEdit_surveyEndPt])
        if status is False:
            err_msg = ret_obj
            self.pop_one_button_dialog(err_msg)
        start_scan = ret_obj[0]
        end_scan = ret_obj[1]

        # maximum number to show on the table
        max_str = str(self.ui.lineEdit_numSurveyOutput.text())
        if max_str.isdigit():
            max_number = int(max_str)
        else:
            max_number = end_scan - start_scan + 1
            self.ui.lineEdit_numSurveyOutput.setText(str(max_number))

        # Get value
        status, ret_obj, err_msg = self._myControl.survey(exp_number, start_scan, end_scan)
        if status is False:
            self.pop_one_button_dialog(ret_obj)
            return
        elif err_msg != "":
            self.pop_one_button_dialog(err_msg)
        scan_sum_list = ret_obj
        self.ui.tableWidget_surveyTable.set_survey_result(scan_sum_list)
        self.ui.tableWidget_surveyTable.show_reflections(max_number)

    def do_switch_tab_peak_int(self):
        """Switch to tab 'Peak Integration' to set up and learn how to do peak integration
        :return:
        """
        # switch tab
        self.ui.tabWidget.setCurrentIndex(MainWindow.TabPage["Peak Integration"])

        # set up value
        selected_scans = self.ui.tableWidget_mergeScans.get_selected_scans()
        if len(selected_scans) > 0:
            # set the first one.  remember that the return is a list of tuple
            self.ui.lineEdit_scanIntegratePeak.setText(str(selected_scans[0][0]))

    def do_sync_ub(self):
        """Purpose: synchronize UB matrix in use with UB matrix calculated.
        Requirements: One of these must be given
        1. a valid UB matrix from tableWidget_ubMatrix
        2. an ascii file that contains 9 float
        3. from text edit
        :return:
        """
        # get the radio button that is checked
        if self.ui.radioButton_ubFromTab1.isChecked():
            # get ub matrix from tab 'Calculate UB Matrix'
            ub_matrix = self.ui.tableWidget_ubMatrix.get_matrix()

        # elif self.ui.radioButton_loadUBmatrix.isChecked():
        #     # load ub matrix from a file
        # ISSUE 001 VZ-FUTURE: Implement this next!
        #     raise NotImplementedError('This tab is not implemented, because the file format has not been decided.')

        elif self.ui.radioButton_ubFromList.isChecked():
            # load ub matrix from text editor
            ub_matrix = self.get_ub_from_text()

        else:
            raise RuntimeError("None radio button is selected for UB")

        # set to in-use UB matrix and control
        self.ui.tableWidget_ubInUse.set_from_matrix(ub_matrix)

        exp_no = int(str(self.ui.lineEdit_exp.text()))
        self._myControl.set_ub_matrix(exp_number=exp_no, ub_matrix=ub_matrix)

    def do_view_data_set_3d(self):
        """
        Launch the sub window to view merged data in 3D.
        :return:
        """
        # get a list of rows that are selected
        row_index_list = self.ui.tableWidget_peaksCalUB.get_selected_rows(True)
        assert len(row_index_list) > 0, "There is no row that is selected to view."

        # get the information from the selected row
        status, exp_number = gutil.parse_integers_editors(self.ui.lineEdit_exp)
        assert status, "Experiment number is not a valid integer."

        # create window
        if self._my3DWindow is None:
            self._my3DWindow = plot3dwindow.Plot3DWindow(self)

        for i_row in row_index_list:
            exp_info = self.ui.tableWidget_peaksCalUB.get_exp_info(i_row)
            scan_number = exp_info[0]

            # prepare data
            ret_obj = self._prepare_view_merged(exp_number, scan_number)
            assert len(ret_obj) == 5
            md_file_name, weight_peak_centers, weight_peak_intensities, avg_peak_centre, avg_peak_intensity = ret_obj

            # add the 3D window
            self._my3DWindow.initialize_group("%s" % scan_number)
            self._my3DWindow.add_plot_by_file(md_file_name)
            self._my3DWindow.add_plot_by_array(weight_peak_centers, weight_peak_intensities)
            self._my3DWindow.add_plot_by_array(avg_peak_centre, avg_peak_intensity)
            self._my3DWindow.close_session()

        # END-FOR

        # Show
        self._my3DWindow.show()

    def do_view_data_3d(self):
        """
        View merged scan data in 3D after FindPeaks.
        :return:
        """
        # get experiment and scan number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp])
        if status:
            exp_number = ret_obj[0]
        else:
            self.pop_one_button_dialog(ret_obj)
            return
        scan_number_list = self.ui.tableWidget_peaksCalUB.get_selected_scans()
        if len(scan_number_list) != 1:
            self.pop_one_button_dialog(
                "To view scan data in 3D, one and only one scan can be selected. Now there are {0} scans that are selected.".format(
                    len(scan_number_list)
                )
            )
            return
        else:
            scan_number = scan_number_list[0]

        # Check
        if self._myControl.has_merged_data(exp_number, scan_number) is False:
            self.pop_one_button_dialog("No merged MD workspace found for %d, %d" % (exp_number, scan_number))
            return

        # Generate data by writing out temp file
        ret_obj = self._prepare_view_merged(exp_number, scan_number)
        assert len(ret_obj) == 5
        md_file_name, weight_peak_centers, weight_peak_intensities, avg_peak_centre, avg_peak_intensity = ret_obj

        # Plot
        if self._my3DWindow is None:
            self._my3DWindow = plot3dwindow.Plot3DWindow(self)

        # print('[INFO] Write file to %s' % md_file_name)
        self._my3DWindow.add_plot_by_file(md_file_name)
        self._my3DWindow.add_plot_by_array(weight_peak_centers, weight_peak_intensities)
        self._my3DWindow.add_plot_by_array(avg_peak_centre, avg_peak_intensity)

        # Show
        self._my3DWindow.show()

    def _prepare_view_merged(self, exp_number, scan_number):
        """
        Prepare the data to view 3D for merged scan
        :return:
        """
        # check
        assert isinstance(exp_number, int) and isinstance(scan_number, int)

        # get file name for 3D data
        base_file_name = "md_%d.dat" % random.randint(1, 1001)
        md_file_name = self._myControl.export_md_data(exp_number, scan_number, base_file_name)
        peak_info = self._myControl.get_peak_info(exp_number, scan_number)

        # peak center
        weight_peak_centers, weight_peak_intensities = peak_info.get_weighted_peak_centres()
        qx, qy, qz = peak_info.get_peak_centre()

        # convert from list to ndarray
        num_pt_peaks = len(weight_peak_centers)
        assert num_pt_peaks == len(weight_peak_intensities)

        pt_peak_centre_array = numpy.ndarray(shape=(num_pt_peaks, 3), dtype="float")
        pt_peak_intensity_array = numpy.ndarray(shape=(num_pt_peaks,), dtype="float")
        for i_pt in range(num_pt_peaks):
            for j in range(3):
                pt_peak_centre_array[i_pt][j] = weight_peak_centers[i_pt][j]
            pt_peak_intensity_array[i_pt] = weight_peak_intensities[i_pt]

        avg_peak_centre = numpy.ndarray(shape=(1, 3), dtype="float")
        avg_peak_intensity = numpy.ndarray(shape=(1,), dtype="float")
        avg_peak_centre[0][0] = qx
        avg_peak_centre[0][1] = qy
        avg_peak_centre[0][2] = qz
        # integrated peak intensity
        avg_peak_intensity[0] = sum(pt_peak_intensity_array)

        return md_file_name, pt_peak_centre_array, pt_peak_intensity_array, avg_peak_centre, avg_peak_intensity

    def do_view_merged_scans_3d(self):
        """Get selected merged scan and plot them individually in 3D
        :return:
        """
        # collect the selected rows and thus workspace names
        row_index_list = self.ui.tableWidget_mergeScans.get_selected_rows(True)
        exp_scan_list = list()
        for row_index in row_index_list:
            exp_number, scan_number = self.ui.tableWidget_mergeScans.get_exp_scan(row_index)
            pt_number_list = self._myControl.get_pt_numbers(exp_number, scan_number)
            md_data = self._myControl.get_merged_data(exp_number, scan_number, pt_number_list)
            exp_scan_list.append((scan_number, md_data))

        # initialize 3D plot
        if self._my3DWindow is None:
            self._my3DWindow = plot3dwindow.Plot3DWindow(self)
        self._my3DWindow.set_merged_data_set(exp_scan_list)
        self._my3DWindow.show()

    def do_view_ub(self):
        """View UB matrix in tab 'UB matrix'
        :return:
        """
        self.ui.tabWidget.setCurrentIndex(MainWindow.TabPage["UB Matrix"])

    def do_view_survey_peak(self):
        """View selected peaks from survey table
        Requirements: one and only 1 run is selected
        Guarantees: the scan number and pt number that are selected will be set to
            tab 'View Raw' and the tab is switched.
        :return:
        """
        # get values
        try:
            scan_num, pt_num = self.ui.tableWidget_surveyTable.get_selected_run_surveyed()
        except RuntimeError as err:
            self.pop_one_button_dialog(str(err))
            return

        # clear selection
        self.ui.tableWidget_surveyTable.select_all_rows(False)

        # switch tab
        self.ui.tabWidget.setCurrentIndex(MainWindow.TabPage["View Raw Data"])
        self.ui.lineEdit_run.setText(str(scan_num))
        self.ui.lineEdit_rawDataPtNo.setText(str(pt_num))

    def evt_change_norm_type(self):
        """
        handling the event that the detector counts normalization method is changed
        :return:
        """
        # read the current normalization type
        new_norm_type = str(self.ui.comboBox_mergePeakNormType.currentText()).lower()
        print("[DB...BAT] Change Norm Type is Triggered.  Type: {0}".format(new_norm_type))

        # set the scale factor according to new norm type
        if new_norm_type.count("time") > 0:
            scale_factor = 1.0
        elif new_norm_type.count("monitor") > 0:
            scale_factor = 1000.0
        else:
            scale_factor = 1.0

        self.ui.lineEdit_scaleFactor.setText("{0}".format(scale_factor))

    def evt_change_normalization(self):
        """
        Integrate Pt. vs integrated intensity of detectors of that Pt. if it is not calculated before
        and then plot pt vs. integrated intensity on
        :return:
        """
        # integrate any how
        # self.do_integrate_per_pt()
        self.do_integrate_single_scan()

    def evt_change_roi(self):
        """handing event of ROI selected in combobox is changed
        :return:
        """
        if self._roiComboBoxMutex:
            return

        # get target ROI name
        curr_roi_name = str(self.ui.comboBox_viewRawDataMasks.currentText()).strip()

        # set to 'no ROI', i.e., removing ROI and plot data without mask
        self.ui.graphicsView_detector2dPlot.remove_roi()

        if len(curr_roi_name) == 0:
            # re-plot
            self.do_plot_pt_raw()

        else:
            # set to another ROI
            self.do_plot_pt_raw()

            if self.ui.graphicsView_detector2dPlot.is_roi_selection_drawn is False:
                # shall apply ROI/rectangular to 2D plot
                lower_left, upper_right = self._myControl.get_region_of_interest(curr_roi_name)
                self.ui.graphicsView_detector2dPlot.set_roi(lower_left, upper_right)
                self.ui.graphicsView_detector2dPlot.plot_roi()
            # END-IF

        # END-IF-ELSE

    def evt_new_roi(self, lower_left_x, lower_left_y, upper_right_x, upper_right_y):
        """
        handling event that a new ROI is defined
        :param lower_left_x:
        :param lower_left_y:
        :param upper_right_x:
        :param upper_right_y:
        :return:
        """
        # a new ROI is defined so combo box is set to item 0
        self.ui.comboBox_viewRawDataMasks.setCurrentIndex(0)

        self.ui.lineEdit_message.setText(
            "New selected ROI: ({0}, {1}), ({2}, {3})".format(lower_left_x, lower_left_y, upper_right_x, upper_right_y)
        )

    def evt_show_survey(self):
        """
        Show survey result
        :return:
        """
        if self.ui.tableWidget_surveyTable.rowCount() == 0:
            # do nothing if the table is empty
            return

        max_number_str = str(self.ui.lineEdit_numSurveyOutput.text()).strip()
        if max_number_str == "":
            # empty: select all
            surveyed_scan_list = self._myControl.get_surveyed_scans()
            max_number = len(surveyed_scan_list)

        else:
            # get maximum number and
            max_number = int(max_number_str)

        # ignore the situation that this line edit is cleared
        if max_number <= 0:
            return

        # reset row number
        if max_number != self.ui.tableWidget_surveyTable.rowCount():
            # re-show survey
            self.ui.tableWidget_surveyTable.remove_all_rows()
            self.ui.tableWidget_surveyTable.show_reflections(max_number)

    def generate_peaks_integration_report(self):
        """
        generate a report for all integrated peaks get MergeScan table and related PeakInfo instance
        :return:
        """
        # get experiment number
        exp_number = int(self.ui.lineEdit_exp.text())

        # get all the selected peaks from table
        row_number_list = self.ui.tableWidget_mergeScans.get_selected_rows()

        # collection all the information
        report_dict = dict()
        for row_number in row_number_list:
            scan_number = self.ui.tableWidget_mergeScans.get_scan_number(row_number)
            peak_info = self._myControl.get_peak_info(exp_number, scan_number)
            peak_integrate_dict = peak_info.generate_integration_report()
            report_dict[scan_number] = peak_integrate_dict
        # END-FOR

        return report_dict

    def get_ub_from_text(self):
        """Purpose: Set UB matrix in use from plain text edit plainTextEdit_ubInput.
        Requirements:
          1. the string in the plain text edit must be able to be split to 9 floats by ',', ' ', '\t' and '\n'
        Guarantees: the matrix will be set up the UB matrix in use
        :return:
        """
        # get the string for ub matrix
        ub_str = str(self.ui.plainTextEdit_ubInput.toPlainText())

        # check the float list string
        status, ret_obj = gutil.parse_float_array(ub_str)
        # check whether the ub matrix in text editor is valid
        if status is False:
            # unable to parse to float arrays
            self.pop_one_button_dialog(ret_obj)
            return
        elif len(ret_obj) != 9:
            # number of floats is not 9
            self.pop_one_button_dialog("Requiring 9 floats for UB matrix.  Only %d are given." % len(ret_obj))
            return

        # in good UB matrix format
        if self.ui.radioButton_ubMantidStyle.isChecked():
            # UB matrix in mantid style
            mantid_ub = gutil.convert_str_to_matrix(ub_str, (3, 3))

        elif self.ui.radioButton_ubSpiceStyle.isChecked():
            # UB matrix in SPICE style
            spice_ub = gutil.convert_str_to_matrix(ub_str, (3, 3))
            mantid_ub = r4c.convert_spice_ub_to_mantid(spice_ub)

        else:
            # not defined
            self.pop_one_button_dialog("Neither Mantid or SPICE-styled UB is checked!")
            return

        return mantid_ub

    def load_session(self, filename=None):
        """
        To load a session, i.e., read it back:
        :param filename:
        :return:
        """
        if filename is None:
            filename = "session_backup.csv"
            filename = os.path.join(os.path.expanduser("~/.mantid/"), filename)

        in_file = open(filename, "r")
        reader = csv.reader(in_file)
        my_dict = dict(x for x in reader)

        # set the data from saved file
        for key, value in my_dict.items():
            if key.startswith("lineEdit") is True:
                self.ui.__getattribute__(key).setText(value)
            elif key.startswith("plainText") is True:
                self.ui.__getattribute__(key).setPlainText(value)
            elif key.startswith("comboBox") is True:
                self.ui.__getattribute__(key).setCurrentIndex(int(value))
            else:
                self.pop_one_button_dialog("Error! Widget name %s is not supported" % key)
        # END-FOR

        # set the experiment
        self._myControl.set_local_data_dir(str(self.ui.lineEdit_localSpiceDir.text()))
        self._myControl.set_working_directory(str(self.ui.lineEdit_workDir.text()))

        self._show_message("Session {0} has been loaded.".format(filename))

    def pop_one_button_dialog(self, message):
        """Pop up a one-button dialog
        :param message:
        :return:
        """
        assert isinstance(message, str), "Input message %s must a string but not %s." % (str(message), type(message))
        QMessageBox.information(self, "4-circle Data Reduction", message)

    def report_peak_addition(self, exp_number, error_message):
        """

        :param self:
        :param exp_number:
        :param error_message:
        :return:
        """
        self.pop_one_button_dialog("Exp: %d\n%s" % (exp_number, error_message))

    def save_current_session(self, filename=None):
        """Save current session/value setup to
        :return:
        """
        # Set up dictionary
        save_dict = dict()

        # Setup
        save_dict["lineEdit_localSpiceDir"] = str(self.ui.lineEdit_localSpiceDir.text())
        save_dict["lineEdit_workDir"] = str(self.ui.lineEdit_workDir.text())

        # Experiment
        save_dict["lineEdit_exp"] = str(self.ui.lineEdit_exp.text())

        # Lattice
        save_dict["lineEdit_a"] = str(self.ui.lineEdit_a.text())
        save_dict["lineEdit_b"] = str(self.ui.lineEdit_b.text())
        save_dict["lineEdit_c"] = str(self.ui.lineEdit_c.text())
        save_dict["lineEdit_alpha"] = str(self.ui.lineEdit_alpha.text())
        save_dict["lineEdit_beta"] = str(self.ui.lineEdit_beta.text())
        save_dict["lineEdit_gamma"] = str(self.ui.lineEdit_gamma.text())

        # Merge scan
        save_dict["lineEdit_listScansSliceView"] = str(self.ui.lineEdit_listScansSliceView.text())

        # Save to csv file
        if filename is None:
            # default
            save_dir = os.path.expanduser("~/.mantid/")
            if os.path.exists(save_dir) is False:
                os.mkdir(save_dir)
            filename = os.path.join(save_dir, "session_backup.csv")
        # END-IF

        ofile = open(filename, "w")
        writer = csv.writer(ofile)
        for key, value in save_dict.items():
            writer.writerow([key, value])
        ofile.close()

    def menu_download_data(self):
        """launch a dialog for user to download data
        :return:
        """
        # create the dialog instance if it is not created
        if self._dataDownloadDialog is None:
            self._dataDownloadDialog = DataDownloadDialog(self)

        # set the experiment number
        exp_number = int(self.ui.lineEdit_exp.text())
        self._dataDownloadDialog.set_experiment_number(exp_number)

        # show the dialog
        self._dataDownloadDialog.show()

    def menu_integrate_peak_single_pt(self):
        """Handle the event to integrate single-pt peak from menu operation
        The operation includes
        1. initialize the single-pt scan integration window if it is not initialized
        2. add the selected scans to the table in single-pt scan integration window
        3. show the window
        :return:
        """
        if self._single_pt_peak_integration_window is None:
            self._single_pt_peak_integration_window = IntegrateSingePtSubWindow.IntegrateSinglePtIntensityWindow(self)
        self._single_pt_peak_integration_window.show()

        # set experiment
        self._single_pt_peak_integration_window.set_experiment(self._current_exp_number)

        # collect selected Scan/Pt combo
        scan_pt_tup_list = self.ui.tableWidget_surveyTable.get_selected_scan_pt()
        # get other information
        for scan_number, pt_number in scan_pt_tup_list:
            # check HKL and 2theta
            h_i = self._myControl.get_sample_log_value(self._current_exp_number, scan_number, pt_number, "_h")
            k_i = self._myControl.get_sample_log_value(self._current_exp_number, scan_number, pt_number, "_k")
            l_i = self._myControl.get_sample_log_value(self._current_exp_number, scan_number, pt_number, "_l")
            hkl_str = "{0}, {1}, {2}".format(h_i, k_i, l_i)
            two_theta = self._myControl.get_sample_log_value(self._current_exp_number, scan_number, pt_number, "2theta")
            self._single_pt_peak_integration_window.add_scan(scan_number, pt_number, hkl_str, two_theta)

    def menu_load_mask(self):
        """Load Mask and apply to both workspaces and GUI
        hb3a_clean_ui_21210
        """
        # get the XML file to load
        file_filter = "XML Files (*.xml);;All Files (*)"
        mask_file_name = QFileDialog.getOpenFileName(self, "Open Masking File", self._myControl.get_working_directory(), file_filter)
        if isinstance(mask_file_name, tuple):
            mask_file_name = mask_file_name[0]

        # generate a mask name and load by calling controller to load mask XML
        roi_name = os.path.basename(mask_file_name).split(".")[0]
        lower_left_corner, upper_right_corner = self._myControl.load_mask_file(mask_file_name, roi_name)

        # set UI
        self.ui.comboBox_maskNames1.addItem(roi_name)
        self.ui.comboBox_maskNames2.addItem(roi_name)
        self.ui.comboBox_maskNamesSurvey.addItem(roi_name)
        self.ui.comboBox_maskNamesSurvey.setCurrentIndex(self.ui.comboBox_maskNames1.count() - 1)
        self.ui.comboBox_viewRawDataMasks.addItem(roi_name)
        self.ui.comboBox_viewRawDataMasks.setCurrentIndex(self.ui.comboBox_viewRawDataMasks.count() - 1)

        # set ROI to controller
        self._myControl.set_roi(roi_name, lower_left_corner, upper_right_corner)

        # plot ROI on 2D plot
        self.ui.graphicsView_detector2dPlot.remove_roi()
        self.ui.graphicsView_detector2dPlot.set_roi(lower_left_corner, upper_right_corner)

    def menu_pop_2theta_sigma_window(self):
        """
        pop out a general figure plot window for 2theta - sigma plot
        :return:
        """
        # create new window or clear existing window
        if self._general_1d_plot_window is None:
            self._general_1d_plot_window = generalplotview.GeneralPlotWindow(self)
            pass
        else:
            self._general_1d_plot_window.reset_window()
        # show
        self._general_1d_plot_window.show()

        # set up the window
        try:
            output_arrays = self._myControl.get_peak_integration_parameters(xlabel="2theta", ylabel=["sigma", "scan"])
            # convert matrix to arrays
            xye_matrix = output_arrays.transpose()
            vec_2theta = xye_matrix[0]
            vec_sigma = xye_matrix[1]
            vec_sigma_error = xye_matrix[2]

            vec_scan_number = xye_matrix[3]
            notes = list()
            for i_scan in range(len(vec_scan_number)):
                notes.append("{}".format(int(vec_scan_number[i_scan])))

            # get the latest (cached) vec_x and vec_y
            self._general_1d_plot_window.plot_data(
                vec_2theta, vec_sigma, vec_sigma_error, "2theta", "Gaussian-Sigma", annotation_list=notes
            )
        except RuntimeError as run_err:
            self.pop_one_button_dialog(str(run_err))

    def menu_save_2theta_sigma(self):
        """
        save 2theta - sigma - scan number to a csv file
        :return:
        """
        # get file name
        out_file_name = QFileDialog.getSaveFileName(
            self,
            caption="Select a file to save 2theta-Sigma-Scan",
            directory=self._myControl.get_working_directory(),
            filter="Data Files (*.dat);;All File (*.*)",
        )
        if not out_file_name:  # cancelled
            return
        if isinstance(out_file_name, tuple):
            out_file_name = out_file_name[0]

        # construct the table
        try:
            output_arrays = self._myControl.get_peak_integration_parameters(xlabel="2theta", ylabel=["sigma", "scan"], with_error=True)
        except RuntimeError as run_err:
            self.pop_one_button_dialog(str(run_err))
            return
        else:
            # xye_matrix = output_arrays.transpose()
            # vec_2theta = xye_matrix[0]
            # vec_sigma = xye_matrix[1]
            # vec_sigma_error = xye_matrix[2]
            # vec_scan_number = xye_matrix[3]
            pass

        # write to the output file
        wbuf = "# 2theta\tsigam\terror\tscan number\n"
        for index in range(output_arrays.shape[0]):
            # wbuf += '{:.5f}\t{:5f}\t{:5f}\t{}\n'.format(vec_2theta[index], vec_sigma[index], vec_sigma_error[index],
            #                                             vec_scan_number[index])
            wbuf += "{:.5f}\t{:5f}\t{:5f}\t{}\n".format(
                output_arrays[index][0], output_arrays[index][1], output_arrays[index][2], output_arrays[index][3]
            )
        # END-FOR

        out_file = open(out_file_name, "w")
        out_file.write(wbuf)
        out_file.close()

    def menu_quit(self):
        """

        :return:
        """
        self.save_settings()
        self.close()

    def menu_pre_process(self):
        """handling action to trigger menu pre-process
        :return:
        """
        # initialize the pre processing window if it is not initialized
        reset_pre_process_window = False
        if self._preProcessWindow is None:
            # initialize the instance
            self._preProcessWindow = PreprocessWindow.ScanPreProcessWindow(self)
            self._preProcessWindow.setup(self._myControl)
            reset_pre_process_window = True

        # show the window
        self._preProcessWindow.show()

        # setup the parameters
        # TODO/FUTURE - Add a push button somewhere to force pre-processing menu to synchronize with main UI for
        # TODO          instrument calibration
        if reset_pre_process_window:
            exp_number = int(str(self.ui.lineEdit_exp.text()))
            # detector size/pixel numbers
            det_size_str = str(self.ui.comboBox_detectorSize.currentText())
            det_size = int(det_size_str.split()[0])
            # detector center
            det_center = str(self.ui.lineEdit_infoDetCenter.text())
            # sample detector distance
            det_sample_dist = str(self.ui.lineEdit_infoDetSampleDistance.text())
            try:
                det_sample_dist = float(det_sample_dist)
            except ValueError:
                det_sample_dist = None
            # wave length
            wave_length = str(self.ui.lineEdit_infoWavelength.text())
            try:
                wave_length = float(wave_length)
            except ValueError:
                wave_length = None

            # set up the other calibration parameters
            self._preProcessWindow.set_instrument_calibration(
                exp_number=exp_number,
                det_size=det_size,
                det_center=det_center,
                det_sample_distance=det_sample_dist,
                wave_length=wave_length,
            )
        # END-IF

    def menu_sort_survey_2theta(self):
        """
        sort survey table by 2theta
        :return:
        """
        self.ui.tableWidget_surveyTable.filter_and_sort(
            start_scan=0, end_scan=100000, min_counts=0.0, max_counts=10000000000.0, sort_by_column="2theta", sort_order=0
        )

    # TESTME - recently implemented
    def menu_sort_by_pt_number(self):
        """
        sort survey table by pt number (with the maximum counts in the scan)
        :return:
        """
        self.ui.tableWidget_surveyTable.filter_and_sort(
            start_scan=0, end_scan=100000, min_counts=0.0, max_counts=10000000000.0, sort_by_column="Max Counts Pt", sort_order=0
        )

    def show_scan_pt_list(self):
        """Show the range of Pt. in a scan
        :return:
        """
        # Get parameters
        status, inp_list = gutil.parse_integers_editors([self.ui.lineEdit_exp, self.ui.lineEdit_run])
        if status is False:
            self.pop_one_button_dialog(inp_list)
            return
        else:
            exp_no = inp_list[0]
            scan_no = inp_list[1]

        status, ret_obj = self._myControl.get_pt_numbers(exp_no, scan_no)

        # Form message
        if status is False:
            # Failed to get Pt. list
            error_message = ret_obj
            self.pop_one_button_dialog(error_message)
        else:
            # Form message
            pt_list = sorted(ret_obj)
            num_pts = len(pt_list)
            info = "Exp %d Scan %d has %d Pt. ranging from %d to %d.\n" % (exp_no, scan_no, num_pts, pt_list[0], pt_list[-1])
            num_miss_pt = pt_list[-1] - pt_list[0] + 1 - num_pts
            if num_miss_pt > 0:
                info += "There are %d Pt. skipped.\n" % num_miss_pt

            self.pop_one_button_dialog(info)

    def set_ub_peak_table(self, peak_info):
        """
        Set up the table of peaks to calculate UB matrix
        Requirements: peak_info is a valid PeakInfo instance
        :param peak_info:
        :return:
        """
        # Check requirement
        assert isinstance(peak_info, r4c.PeakProcessRecord), "Peak information instance must be a PeakProcessedRecord but not a {0}".format(
            type(peak_info)
        )

        # Get data
        exp_number, scan_number = peak_info.get_experiment_info()
        h, k, l = peak_info.get_hkl(user_hkl=False)
        q_x, q_y, q_z = peak_info.get_peak_centre()
        # wave length
        m1 = self._myControl.get_sample_log_value(exp_number, scan_number, 1, "_m1")
        user_wave_length = self._myControl.get_calibrated_wave_length(exp_number)
        if user_wave_length is None:
            # no user specified wave length
            wave_length = hb3a_util.convert_to_wave_length(m1_position=m1)
        else:
            # user specified is found
            wave_length = user_wave_length

        # Set to table
        status, err_msg = self.ui.tableWidget_peaksCalUB.add_peak(scan_number, (h, k, l), (q_x, q_y, q_z), m1, wave_length)
        if status is False:
            self.pop_one_button_dialog(err_msg)

    def save_settings(self):
        """
        Save settings (parameter set) upon quitting
        :return:
        """
        settings = QSettings()

        # directories
        local_spice_dir = str(self.ui.lineEdit_localSpiceDir.text())
        settings.setValue("local_spice_dir", local_spice_dir)
        work_dir = str(self.ui.lineEdit_workDir.text())
        settings.setValue("work_dir", work_dir)

        # experiment number
        exp_num = str(self.ui.lineEdit_exp.text())
        settings.setValue("exp_number", exp_num)

        # lattice parameters
        lattice_a = str(self.ui.lineEdit_a.text())
        settings.setValue("a", lattice_a)
        lattice_b = str(self.ui.lineEdit_b.text())
        settings.setValue("b", lattice_b)
        lattice_c = str(self.ui.lineEdit_c.text())
        settings.setValue("c", lattice_c)
        lattice_alpha = str(self.ui.lineEdit_alpha.text())
        settings.setValue("alpha", lattice_alpha)
        lattice_beta = str(self.ui.lineEdit_beta.text())
        settings.setValue("beta", lattice_beta)
        lattice_gamma = str(self.ui.lineEdit_gamma.text())
        settings.setValue("gamma", lattice_gamma)

        # calibrated instrument configurations
        user_wave_length = str(self.ui.lineEdit_userWaveLength.text())
        settings.setValue("wave_length", user_wave_length)

        det_row_center = str(self.ui.lineEdit_detCenterPixHorizontal.text())
        settings.setValue("row_center", det_row_center)

        det_col_center = str(self.ui.lineEdit_detCenterPixVertical.text())
        settings.setValue("col_center", det_col_center)

        det_sample_distance_str = str(self.ui.lineEdit_userDetSampleDistance.text())
        settings.setValue("det_sample_distance", det_sample_distance_str)

        # last project
        last_1_project_path = str(self.ui.label_last1Path.text())
        settings.setValue("last1path", last_1_project_path)

        # survey
        survey_start = str(self.ui.lineEdit_surveyStartPt.text())
        survey_stop = str(self.ui.lineEdit_surveyEndPt.text())
        settings.setValue("survey_start_scan", survey_start)
        settings.setValue("survey_stop_scan", survey_stop)

    def load_settings(self):
        """
        Load QSettings from previous saved file
        :return:
        """
        settings = QSettings()

        # directories
        try:
            spice_dir = settings.value("local_spice_dir", "", type=str)
            self.ui.lineEdit_localSpiceDir.setText(str(spice_dir))
            work_dir = settings.value("work_dir", type=str)
            self.ui.lineEdit_workDir.setText(str(work_dir))

            # experiment number
            exp_num = settings.value("exp_number")
            self.ui.lineEdit_exp.setText(str(exp_num))

            # lattice parameters
            lattice_a = settings.value("a", type=str)
            self.ui.lineEdit_a.setText(str(lattice_a))
            lattice_b = settings.value("b", type=str)
            self.ui.lineEdit_b.setText(str(lattice_b))
            lattice_c = settings.value("c", type=str)
            self.ui.lineEdit_c.setText(str(lattice_c))
            lattice_alpha = settings.value("alpha", type=str)
            self.ui.lineEdit_alpha.setText(str(lattice_alpha))
            lattice_beta = settings.value("beta", type=str)
            self.ui.lineEdit_beta.setText(str(lattice_beta))
            lattice_gamma = settings.value("gamma", type=str)
            self.ui.lineEdit_gamma.setText(str(lattice_gamma))

            # last project
            last_1_project_path = settings.value("last1path", type=str)
            self.ui.label_last1Path.setText(last_1_project_path)

            # calibrated instrument configurations
            user_wave_length = settings.value("wave_length", type=str)
            self.ui.lineEdit_userWaveLength.setText(user_wave_length)

            det_row_center = settings.value("row_center", type=str)
            self.ui.lineEdit_detCenterPixHorizontal.setText(det_row_center)

            det_col_center = settings.value("col_center", type=str)
            self.ui.lineEdit_detCenterPixVertical.setText(det_col_center)

            det_sample_distance = settings.value("det_sample_distance", type=str)
            self.ui.lineEdit_userDetSampleDistance.setText(det_sample_distance)

            # survey
            survey_start = settings.value("survey_start_scan", type=str)
            self.ui.lineEdit_surveyStartPt.setText(survey_start)
            survey_stop = settings.value("survey_stop_scan", type=str)
            self.ui.lineEdit_surveyEndPt.setText(survey_stop)

        except TypeError as err:
            self.pop_one_button_dialog("Failed to load previous session successfully due to {0}".format(err))
            return

    def _get_lattice_parameters(self):
        """
        Get lattice parameters from GUI
        :return: (Boolean, Object).  True, 6-tuple as a, b, c, alpha, beta, gamm
                                     False: error message
        """
        status, ret_list = gutil.parse_float_editors(
            [
                self.ui.lineEdit_a,
                self.ui.lineEdit_b,
                self.ui.lineEdit_c,
                self.ui.lineEdit_alpha,
                self.ui.lineEdit_beta,
                self.ui.lineEdit_gamma,
            ]
        )
        if status is False:
            err_msg = ret_list
            err_msg = "Unable to parse unit cell due to %s" % err_msg
            return False, err_msg

        a, b, c, alpha, beta, gamma = ret_list

        return True, (a, b, c, alpha, beta, gamma)

    def load_spice_file(self, exp_number, scan_number, overwrite):
        """
        load spice file
        :param exp_number:
        :param scan_number:
        :param overwrite:
        :return:
        """
        # check inputs
        assert isinstance(exp_number, int), "Exp number {0} must be an integer but not of type {1}".format(exp_number, type(exp_number))
        assert isinstance(scan_number, int), "Scan number {0} must be an integer but not of type {1}".format(scan_number, type(scan_number))

        # Check and load SPICE table file
        load_spice = False
        if not overwrite:
            load_spice = not self._myControl.does_spice_loaded(exp_number, scan_number)

        # load if necessary
        if load_spice:
            # Download data
            status, error_message = self._myControl.download_spice_file(exp_number, scan_number, over_write=False)
            if status:
                status, error_message = self._myControl.load_spice_scan_file(exp_number, scan_number)
                if status is False and self._allowDownload is False:
                    self.pop_one_button_dialog(error_message)
                    return
            else:
                self.pop_one_button_dialog(error_message)
                return
        # END-IF(does_exist)

    def load_plot_raw_data(self, exp_no, scan_no, pt_no, roi_name=None, save=False, remove_workspace=False):
        """
        Plot raw workspace from XML file for a measurement/pt.
        :param exp_no:
        :param scan_no:
        :param pt_no:
        :param roi_name: string (mask loaded data) or None (do nothing)
        :param save: flag to save the ROI
        :param remove_workspace: Flag to remove the raw data workspace
        :return:
        """
        # check inputs
        assert isinstance(exp_no, int), "Exp number {0} must be an integer but not of type {1}".format(exp_no, type(exp_no))
        assert isinstance(scan_no, int), "Scan number {0} must be an integer but not of type {1}".format(scan_no, type(scan_no))
        assert isinstance(pt_no, int), "Pt number {0} must be an integer but not of type {1}".format(pt_no, type(pt_no))

        # check data loaded with mask information
        does_loaded = self._myControl.does_raw_loaded(exp_no, scan_no, pt_no, roi_name)
        if not does_loaded:
            # check and load SPICE file if necessary
            self.load_spice_file(exp_no, scan_no, overwrite=False)
            # load Pt xml
            self._myControl.load_spice_xml_file(exp_no, scan_no, pt_no)
            # mask detector if required
            if roi_name is not None:
                self._myControl.apply_mask(exp_no, scan_no, pt_no, roi_name=roi_name)
        # END-IF

        # Get data and plot
        raw_det_data = self._myControl.get_raw_detector_counts(exp_no, scan_no, pt_no)
        this_title = "Exp {} Scan {} Pt {} ROI {}".format(exp_no, scan_no, pt_no, roi_name)
        self.ui.graphicsView_detector2dPlot.plot_detector_counts(raw_det_data, title=this_title)
        if save:
            image_file = os.path.join(self.working_directory, "exp{}_scan{}_pt{}_{}.png".format(exp_no, scan_no, pt_no, roi_name))
            self.ui.graphicsView_detector2dPlot.save_figure(image_file)
        else:
            image_file = None

        # Information
        info = "%-10s: %d\n%-10s: %d\n%-10s: %d\n" % ("Exp", exp_no, "Scan", scan_no, "Pt", pt_no)
        self.ui.plainTextEdit_rawDataInformation.setPlainText(info)

        if remove_workspace:
            self._myControl.remove_pt_xml_workspace(exp_no, scan_no, pt_no)

        return image_file

    def update_adding_peaks_status(self, exp_number, scan_number, progress):
        """
        Update the status for adding peak to UB matrix calculating
        :param exp_number:
        :param scan_number:
        :param progress:
        :return:
        """
        # show message to bar
        if scan_number < 0:
            message = "Peak processing finished"
        else:
            message = "Processing experiment %d scan %d starting from %s." % (exp_number, scan_number, str(datetime.datetime.now()))
        self.ui.statusbar.showMessage(message)

        # update progress bar
        self.ui.progressBar_add_ub_peaks.setValue(progress)

    def update_merge_value(self, exp_number, scan_number, sig_value, peak_centre, mode):
        """
        update the values of result from merging/integrating peaks
        :param exp_number:
        :param scan_number:
        :param sig_value:
        :param peak_centre:
        :param mode:
        :return:
        """
        # Process signals according to mode
        if mode == 0:
            # start of processing one peak
            progress = int(sig_value - 0.5)
            if progress == 0:
                # run start
                self._startMeringScans = time.process_time()
                self._errorMessageEnsemble = ""

            self.ui.progressBar_mergeScans.setValue(progress)

        elif mode == 1:
            # receive signal from the end of processing one peak: complete the row
            # get the peak object
            peak_info_obj = self._myControl.get_peak_info(exp_number, scan_number)

            # get row number to set up the table
            try:
                row_number = self.ui.tableWidget_mergeScans.get_row_by_scan(scan_number)
            except RuntimeError as run_err:
                raise RuntimeError("Unable to find scan {0} in Peak-Processing table due to {1}.".format(scan_number, run_err))

            # get peak: simple summation intensity
            intensity, int_std_dev = peak_info_obj.get_intensity("simple intensity", False)

            # check intensity value
            is_error = False
            if intensity < 0:
                # set to status
                error_msg = "Negative intensity (simple): %.3f" % intensity
                self.ui.tableWidget_mergeScans.set_status(row_number=row_number, status=error_msg)
                # reset intensity to 0.
                intensity = 0.0
                is_error = True
                int_std_dev = 0.0

            # get the corrected value
            corrected_intensity = intensity * peak_info_obj.lorentz_correction_factor
            corrected_std_dev = int_std_dev * peak_info_obj.lorentz_correction_factor

            # status, error_msg = self._myControl.set_peak_intensity(exp_number, scan_number, intensity)
            # if status:
            #     # set the value to table
            self.ui.tableWidget_mergeScans.set_peak_intensity(
                row_number=row_number,
                peak_intensity=intensity,
                corrected_intensity=corrected_intensity,
                standard_error=corrected_std_dev,
                integrate_method="simple",
            )

            if is_error:
                self.ui.tableWidget_mergeScans.set_status(row_number, "Integration Error")
            else:
                self.ui.tableWidget_mergeScans.set_status(row_number, "Integrated")

        elif mode == 2:
            # get signal from the end of all selected scans being integrated

            # apply Lorentz correction
            # self.ui_apply_lorentz_correction_mt()

            # set progress bar
            progress = int(sig_value + 0.5)
            self.ui.progressBar_mergeScans.setValue(progress)

            # set message to status bar
            merge_run_end = time.process_time()
            elapsed = merge_run_end - self._startMeringScans
            message = "Peak integration is over. Used %.2f seconds" % elapsed
            self.ui.statusbar.showMessage(message)

            # pop error message if there is any
            if len(self._errorMessageEnsemble) > 0:
                self.pop_one_button_dialog(self._errorMessageEnsemble)

            # delete thread
            del self._myIntegratePeaksThread
            self._myIntegratePeaksThread = None

        # END-IF-ELSE (mode)

    def update_merge_message(self, exp_number, scan_number, mode, message):
        """
        Update the merge-scan table for message such as error or etc.
        Note: the string passed from PyQt message is of type unicode but not string!
        :param exp_number:
        :param scan_number:
        :param mode:
        :param message:
        :return:
        """
        # check
        assert isinstance(exp_number, int), "Experiment number must be integer."
        assert isinstance(scan_number, int), "Scan number must be integer."
        assert isinstance(mode, int), "Mode %s must be integer but not %s." % (str(mode), type(mode))
        assert isinstance(message, str) or isinstance(message, unicode), "Message %s must be a string/unicode but not %s." % (
            str(message),
            type(message),
        )

        # passed value from PyQt signal might be a unicode code
        message = str(message)

        try:
            row_number = self.ui.tableWidget_mergeScans.get_row_by_scan(scan_number)
        except RuntimeError as run_err:
            self.pop_one_button_dialog(str(run_err))
            return

        # set intensity, state to table
        if mode == 0:
            # error message
            self.ui.tableWidget_mergeScans.set_peak_intensity(
                row_number, peak_intensity=0.0, corrected_intensity=0.0, standard_error=0.0, integrate_method="simple"
            )
            self.ui.tableWidget_mergeScans.set_status(row_number=row_number, status=message)

            # set peak value
            status, ret_message = self._myControl.set_zero_peak_intensity(exp_number, scan_number)
            if not status:
                self.pop_one_button_dialog(ret_message)

        elif mode == 1:
            # merged workspace name
            merged_ws_name = message
            self.ui.tableWidget_mergeScans.set_ws_name(row_number=row_number, merged_md_name=merged_ws_name)

        else:
            raise RuntimeError("Peak-merging mode %d is not supported." % mode)

    def update_peak_added_info(self, int_msg, int_msg2):
        """
        Update the peak-being-added information
        :param int_msg:
        :param int_msg2:
        :return:
        """
        # get parameters passed
        exp_number = int_msg
        scan_number = int_msg2

        # get PeakInfo
        peak_info = self._myControl.get_peak_info(exp_number, scan_number)
        assert isinstance(peak_info, r4c.PeakProcessRecord)

        # add to table
        self.set_ub_peak_table(peak_info)

    @property
    def ub_matrix_processing_table(self):
        """
        return the handler to the UB matrix
        :return:
        """
        return self.ui.tableWidget_peaksCalUB

    @property
    def working_directory(self):
        """
        return the current working directory
        :return:
        """
        return self._myControl._workDir
