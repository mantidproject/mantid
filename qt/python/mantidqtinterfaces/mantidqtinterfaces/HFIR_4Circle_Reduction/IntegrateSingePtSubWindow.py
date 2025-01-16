# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0103
from mantidqtinterfaces.HFIR_4Circle_Reduction.hfctables import SinglePtIntegrationTable
from mantidqtinterfaces.HFIR_4Circle_Reduction.integratedpeakview import SinglePtIntegrationView
import mantidqtinterfaces.HFIR_4Circle_Reduction.guiutility as guiutility
import os
from qtpy.QtWidgets import QMainWindow, QFileDialog
from qtpy.QtCore import Signal as pyqtSignal
from mantid.kernel import Logger

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("HFIR_4Circle_Reduction").information("Using legacy ui importer")
    from mantidplot import load_ui
from qtpy.QtWidgets import QVBoxLayout


class IntegrateSinglePtIntensityWindow(QMainWindow):
    """
    Main window widget to set up parameters to optimize
    """

    # establish signal for communicating from App2 to App1 - must be defined before the constructor
    scanIntegratedSignal = pyqtSignal(dict, name="SinglePtIntegrated")

    def __init__(self, parent=None):
        """
        Initialization
        :param parent:
        :return:
        """
        # init
        super(IntegrateSinglePtIntensityWindow, self).__init__(parent)

        assert parent is not None, "Parent window cannot be None to set"
        self._parent_window = parent
        self._controller = parent.controller

        # connect signal handler
        self.scanIntegratedSignal.connect(self._parent_window.process_single_pt_scan_intensity)

        # init UI
        ui_path = "SinglePtIntegrationWindow.ui"
        self.ui = load_ui(__file__, ui_path, baseinstance=self)
        self._promote_widgets()

        # initialize widgets
        self.ui.tableView_summary.setup()
        self.ui.graphicsView_integration1DView.set_parent_window(self)

        # define event handlers for widgets
        self.ui.pushButton_integrteDetectorCounts.clicked.connect(self.do_integrate_detector_counts)
        self.ui.pushButton_load2thetaSigmaFile.clicked.connect(self.menu_load_gauss_sigma_file)

        self.ui.pushButton_exportIntensityToFile.clicked.connect(self.do_save_intensity)
        self.ui.pushButton_exportIntensityToTable.clicked.connect(self.do_export_intensity_to_parent)
        # self.ui.pushButton_refreshROI.clicked.connect(self.do_refresh_roi)
        self.ui.pushButton_retrieveFWHM.clicked.connect(self.do_retrieve_fwhm)
        self.ui.pushButton_integratePeaks.clicked.connect(self.do_integrate_single_pt)
        self.ui.pushButton_plot.clicked.connect(self.do_plot_integrated_pt)
        self.ui.pushButton_exportToMovie.clicked.connect(self.do_export_to_movie)

        # TODO - 20180809 - Implement the following...calling change_scan_number
        self.ui.pushButton_rewindPlot.clicked.connect(self.do_plot_previous_scan)
        self.ui.pushButton_forwardPlot.clicked.connect(self.do_plot_next_scan)
        self.ui.actionDefine_2theta_FWHM_Function.triggered.connect(self.do_define_2theta_fwhm_function)

        # menu bar
        self.ui.menuQuit.triggered.connect(self.do_close)
        self.ui.actionSelect_All.triggered.connect(self.menu_table_select_all)
        self.ui.actionDe_select_All.triggered.connect(self.menu_table_select_none)
        self.ui.actionLoad_Gaussian_Sigma_File.triggered.connect(self.menu_load_gauss_sigma_file)
        self.ui.actionLoad_Peak_Info_File.triggered.connect(self.do_load_peak_integration_table)
        self.ui.actionRefresh_ROI_List.triggered.connect(self.do_refresh_roi)

        # class variable
        self._working_dir = self._controller.get_working_directory()
        self._exp_number = None
        self._roiMutex = False

        # other things to do
        self.do_refresh_roi()

        return

    def _promote_widgets(self):
        graphicsView_integration1DView_layout = QVBoxLayout()
        self.ui.frame_graphicsView_integration1DView.setLayout(graphicsView_integration1DView_layout)
        self.ui.graphicsView_integration1DView = SinglePtIntegrationView(self)
        graphicsView_integration1DView_layout.addWidget(self.ui.graphicsView_integration1DView)

        tableView_summary_layout = QVBoxLayout()
        self.ui.frame_tableView_summary.setLayout(tableView_summary_layout)
        self.ui.tableView_summary = SinglePtIntegrationTable(self)
        tableView_summary_layout.addWidget(self.ui.tableView_summary)

        return

    def do_close(self):
        """
        Quit the window
        :return:
        """
        self.close()

        return

    def do_define_2theta_fwhm_function(self):
        """
        pop out a dialog for user to input the 2theta-FWHM formula
        :return:
        """
        formula = guiutility.get_value_from_dialog(
            parent=self,
            title="Input 2theta-FWHM function",
            details="Example: y = 4.0 * x**2 - 1.2 * x + 1./x]=\nwhere y is FWHM and x is 2theta",
            label_name="Equation: ",
        )

        if formula is None:
            # return if user cancels operation
            return

        print("[DB...BAT] User input 2theta formula: {}".format(formula))
        state, error_message = self._controller.check_2theta_fwhm_formula(formula)
        if not state:
            guiutility.show_message(self, message=error_message, message_type="error")
            return

        return

    def do_export_intensity_to_parent(self):
        """
        export the integrated intensity to parent window's peak processing table
        :return:
        """
        # collect all scan/pt from table. value including intensity and ROI
        intensity_dict = self.ui.tableView_summary.get_peak_intensities()

        # add to table including calculate peak center in Q-space
        self.scanIntegratedSignal.emit(intensity_dict)

        return

    # TESTME -  20180727 - Complete it!
    def do_export_to_movie(self):
        """
        export the complete list of single-pt experiment to a movie
        :return:
        """
        # find out the directory to save the PNG files for making a move
        movie_dir = self._controller.get_working_directory()
        roi_name = str(self.ui.comboBox_roiList.currentText())
        direction = str(self.ui.comboBox_integrateDirection.currentText()).lower()
        movie_dir = os.path.join(movie_dir, "{}_{}".format(roi_name, direction))
        os.mkdir(movie_dir)

        # go through each line to plot and save the data
        num_rows = self.ui.tableView_summary.rowCount()
        file_list_str = ""
        for i_row in range(num_rows):
            # get run number and set to plot
            scan_number = self.ui.tableView_summary.get_scan_number(i_row)
            self.ui.lineEdit_Scan.setText("{}".format(scan_number))
            png_file_name = os.path.join(movie_dir, "Scan{0:04d}_ROI{1}_{2}.png".format(scan_number, roi_name, direction))
            self.do_plot_integrated_pt(show_plot=False, save_plot_to=png_file_name)
            file_list_str += "{}\n".format(png_file_name)
        # END-IF

        # write out
        png_list_file = open(os.path.join(movie_dir, "MoviePNGList.txt"), "w")
        png_list_file.write(file_list_str)
        png_list_file.close()

        # prompt how to make a movie
        command_linux = 'ffmpeg -framerate 8 -pattern_type glob -i "*.png" -r 30 test.mp4'
        guiutility.show_message(self, command_linux)

        return

    def do_integrate_detector_counts(self):
        """
        sum over the (selected) scan's detector counts by ROI
        :return:
        """
        # get ROI
        roi_name = str(self.ui.comboBox_roiList.currentText())
        if roi_name is None or roi_name == "":
            guiutility.show_message("A region-of-interest must be chosen in order to integrate detector counts.")
            return

        # integration direction and fit
        direction = str(self.ui.comboBox_integrateDirection.currentText()).lower()
        fit_gaussian = self.ui.checkBox_fitPeaks.isChecked()

        num_rows = self.ui.tableView_summary.rowCount()
        print("[DB...BAT] Number of rows = {}".format(num_rows))
        scan_number_list = list()
        for row_number in range(num_rows):
            # integrate counts on detector
            scan_number = self.ui.tableView_summary.get_scan_number(row_number)
            scan_number_list.append(scan_number)
        # END-FOR
        print("[DB...BAT] Scan numbers: {}".format(scan_number_list))

        peak_height_dict = self._controller.integrate_single_pt_scans_detectors_counts(
            self._exp_number, scan_number_list, roi_name, direction, fit_gaussian
        )

        # set the value to the row  to table
        for row_number in range(self.ui.tableView_summary.rowCount()):
            scan_number = self.ui.tableView_summary.get_scan_number(row_number)
            pt_number = 1
            peak_height = peak_height_dict[scan_number]
            self.ui.tableView_summary.set_peak_height(scan_number, pt_number, peak_height, roi_name)
        # END-FOR (row_number)

        return

    def do_integrate_single_pt(self):
        """
        integrate the 2D data inside region of interest along both axis-0 and axis-1 individually.
        and the result (as 1D data) will be saved to ascii file.
        the X values will be the corresponding pixel index either along axis-0 or axis-1
        :return:
        """
        # get ROI
        roi_name = str(self.ui.comboBox_roiList.currentText())
        if roi_name is None or roi_name == "":
            guiutility.show_message("A region-of-interest must be chosen in order to integrate detector counts.")
            return

        for row_number in range(self.ui.tableView_summary.rowCount()):
            # integrate counts on detector
            scan_number = self.ui.tableView_summary.get_scan_number(row_number)
            pt_number = self.ui.tableView_summary.get_pt_number(row_number)

            # calculate peak intensity
            ref_fwhm = self.ui.tableView_summary.get_fwhm(row_number)

            intensity = self._controller.calculate_intensity_single_pt(
                self._exp_number, scan_number, pt_number, roi_name, ref_fwhm=ref_fwhm, is_fwhm=False
            )

            # add to table
            self.ui.tableView_summary.set_intensity(scan_number, pt_number, intensity)
        # END-FOR

        return

    # TESTME - Load a previously save integrated peaks file
    # Question: What kind of peak integrtion table??? Need to find out and well documented!
    def do_load_peak_integration_table(self):
        """
        load peak integration table CSV file saved from peak integration table
        :return:
        """
        # get table file name
        table_file = QFileDialog.getOpenFileName(self, "Peak Integration Table", self._working_dir)
        if not table_file:
            return
        if isinstance(table_file, tuple):
            table_file = table_file[0]
        if not os.path.exists(table_file):
            return

        # load
        status, error_msg = self._controller.load_peak_integration_table(table_file)
        if not status:
            raise RuntimeError(error_msg)

    def do_plot_integrated_pt(self, show_plot=True, save_plot_to=None):
        """plot integrated Pt with model and raw data
        1. selection include: 2-theta FWHM Model, Summed Single Pt. Counts (horizontal),
        Summed Single Pt. Counts (vertical) from comboBox_plotType
        :return:
        """
        plot_type = str(self.ui.comboBox_plotType.currentText())

        # reset the canvas
        self.ui.graphicsView_integration1DView.clear_all_lines()

        if plot_type == "2-theta FWHM Model":
            self.plot_2theta_fwhm_model()
        else:
            # plot summed single pt scan
            self.plot_summed_single_pt_scan_counts(is_vertical_summed=plot_type.lower().count("vertical"), figure_file=save_plot_to)

        return

    def do_plot_previous_scan(self):
        """plot previous scan if not in 2theta FWHM model
        :return:
        """
        plot_type = str(self.ui.comboBox_plotType.currentText())
        if plot_type == "2-theta FWHM Model":
            return

        scan_number_str = str(self.ui.lineEdit_Scan.text()).strip()
        if scan_number_str == "":
            return

        scan_number = int(scan_number_str)
        row_number = self.ui.tableView_summary.get_row_number_by_scan(scan_number)
        if row_number == 0:
            row_number = self.ui.tableView_summary.rowCount() - 1
        scan_number = self.ui.tableView_summary.get_scan_number(row_number)
        self.ui.lineEdit_Scan.setText(scan_number)

        self.do_plot_integrated_pt()

        return

    def do_plot_next_scan(self):
        """plot next scan if not in 2theta FWHM model
        :return:
        """
        plot_type = str(self.ui.comboBox_plotType.currentText())
        if plot_type == "2-theta FWHM Model":
            return

        scan_number_str = str(self.ui.lineEdit_Scan.text()).strip()
        if scan_number_str == "":
            return

        scan_number = int(scan_number_str)
        row_number = self.ui.tableView_summary.get_row_number_by_scan(scan_number)
        if row_number == self.ui.tableView_summary.rowCount() - 1:
            row_number = 0
        scan_number = self.ui.tableView_summary.get_scan_number(row_number)
        self.ui.lineEdit_Scan.setText(scan_number)

        self.do_plot_integrated_pt()

        return

    def plot_summed_single_pt_scan_counts(self, is_vertical_summed, figure_file=None, pop_error=False):
        """
        plot single pt scanned counts
        :param is_vertical_summed:
        :param figure_file:
        :param pop_error:
        :return:
        """
        # get scan number
        scan_num_str = str(self.ui.lineEdit_Scan.text()).strip()
        if len(scan_num_str) == 0:
            scan_number = self.ui.tableView_summary.get_scan_number(0)
            self.ui.lineEdit_Scan.setText("{}".format(scan_number))
        else:
            scan_number = int(scan_num_str)
        roi_name = str(self.ui.comboBox_roiList.currentText())
        if is_vertical_summed:
            direction = "vertical"
        else:
            direction = "horizontal"

        # get data: pt number is always 1 as it is a single Pt. measurement
        model_y = None
        if self.ui.checkBox_fitPeaks.isChecked():
            try:
                vec_x, model_y = self._controller.get_single_scan_pt_model(
                    self._exp_number, scan_number, pt_number=1, roi_name=roi_name, integration_direction=direction
                )
            except RuntimeError as run_err:
                err_msg = "Unable to get single-pt scan model for {} {} {} due to {}".format(
                    self._exp_number, scan_number, roi_name, run_err
                )
                if pop_error:
                    raise RuntimeError(err_msg)
                else:
                    print(err_msg)
            # END-TRY-EXCEPT
        # END-IF

        # get original data
        vec_x, vec_y = self._controller.get_single_scan_pt_summed(
            self._exp_number, scan_number, pt_number=1, roi_name=roi_name, integration_direction=direction
        )

        # plot
        self.ui.graphicsView_integration1DView.add_observed_data(vec_x, vec_y, label="Summed (raw) counts", update_plot=False)
        if model_y is not None:
            self.ui.graphicsView_integration1DView.add_fit_data(vec_x, model_y, label="Gaussian model", update_plot=True)

        # title
        self.ui.graphicsView_integration1DView.set_title("Scan {} Pt {} {} Integration.".format(scan_number, 1, direction))

        # save plot?
        if figure_file is not None:
            self.ui.graphicsView_integration1DView.canvas.save_figure(figure_file)

        return

    def plot_2theta_fwhm_model(self):
        """plot the loaded 2theta-FWHM model
        :return:
        """
        # TODO - 20180815 - Need to parse the range from self.ui.lineEdit_Scan
        # default
        two_theta_range = 10, 2.0, 110  # start, resolution, stop

        vec_2theta, vec_fwhm, vec_model = self._controller.get_2theta_fwhm_data(two_theta_range[0], two_theta_range[1], two_theta_range[2])

        self.ui.graphicsView_integration1DView.plot_2theta_model(vec_2theta, vec_fwhm, vec_model)

        return

    def do_refresh_roi(self):
        """
        refresh ROI list from parent
        :return:
        """
        roi_list = self._controller.get_region_of_interest_list()

        # add ROI
        self._roiMutex = True

        self.ui.comboBox_roiList.clear()
        for roi_name in sorted(roi_list):
            self.ui.comboBox_roiList.addItem(roi_name)

        self._roiMutex = False

        return

    def do_save_intensity(self):
        """
        save intensity to file
        :return:
        """
        # get output file
        out_file_name = QFileDialog.getSaveFileName(self, "File to save integrated intensities", self._working_dir)
        if not out_file_name:
            return
        if isinstance(out_file_name, tuple):
            out_file_name = out_file_name[0]

        self.ui.tableView_summary.save_intensities_to_file(out_file_name)

    def do_retrieve_fwhm(self):
        """
        Get FWHM from integrated 'STRONG' peaks according to 2theta value
        :return:
        """
        row_number = self.ui.tableView_summary.rowCount()
        error_messages = ""
        for row_index in range(row_number):
            # check whether FWHM value is set up
            fwhm_i = self.ui.tableView_summary.get_fwhm(row_index)
            if fwhm_i is not None and fwhm_i > 1.0e-10:
                continue

            # use interpolation to curve
            two_theta = self.ui.tableView_summary.get_two_theta(row_index)
            try:
                gauss_sigma = self._controller.calculate_peak_integration_sigma(two_theta)
                scan_number = self.ui.tableView_summary.get_scan_number(row_index)
                pt_number = 1
                roi_name = self.ui.tableView_summary.get_region_of_interest_name(row_index)
                self.ui.tableView_summary.set_gaussian_sigma(row_index, gauss_sigma)
                self._controller.set_single_measure_peak_width(
                    self._exp_number, scan_number, pt_number, roi_name, gauss_sigma, is_fhwm=False
                )

            except RuntimeError as err:
                # error!
                error_messages += "Unable to calculate sigma of row {0} due to {1}\n".format(row_index, err)
                continue
            # END-IF-ELSE

        # show error message if necessary
        if len(error_messages) > 0:
            guiutility.show_message(self, error_messages)

    def menu_load_gauss_sigma_file(self):
        """
        load a Gaussian sigma curve for interpolation or matching
        :return:
        """
        # get the column ascii file name
        file_filter = "Data Files (*.dat);;All Files (*.*)"
        twotheta_sigma_file_name = QFileDialog.getOpenFileName(self, self._working_dir, "2theta Gaussian-Sigma File", file_filter)
        if not twotheta_sigma_file_name:
            return
        if isinstance(twotheta_sigma_file_name, tuple):
            twotheta_sigma_file_name = twotheta_sigma_file_name[0]

        # set the file to controller
        try:
            vec_x, vec_y = self._controller.import_2theta_gauss_sigma_file(twotheta_sigma_file_name)
            self.ui.graphicsView_integration1DView.plot_2theta_model(vec_x, vec_y)
        except RuntimeError as run_err:
            guiutility.show_message(self, str(run_err))

    def menu_table_select_all(self):
        """
        select all rows in table
        :return:
        """
        self.ui.tableView_summary.select_all_rows(True)

    def menu_table_select_none(self):
        """
        de-select all rows in the able
        :return:
        """
        self.ui.tableView_summary.select_all_rows(False)

    def add_scans(self, scan_pt_list):
        """
        add scans' information to table, i.e., add line
        :param scan_pt_list:
        :return:
        """
        # check input
        assert isinstance(scan_pt_list, list), "Scan-Pt-Infos {} must be a list but not a {}.".format(scan_pt_list, type(scan_pt_list))

        # sort the scans
        scan_pt_list = sorted(scan_pt_list)

        for scan_pt_info in scan_pt_list:
            scan_number, pt_number, hkl, two_theta = scan_pt_info
            self.ui.tableView_summary.add_scan_pt(scan_number, pt_number, hkl, two_theta)
        # END-FOR

        return

    def add_scan(self, scan_number, pt_number, hkl_str, two_theta):
        """
        add single scan
        :param scan_number:
        :param pt_number:
        :param hkl_str:
        :param two_theta:
        :return:
        """
        self.ui.tableView_summary.add_scan_pt(scan_number, pt_number, hkl_str, two_theta)

    def change_scan_number(self, increment):
        """
        change the scan number in the
        :param increment:
        :return:
        """
        # FIXME - 20180809 - This behaviors weird... Need debugging output - TODO
        # get the list of scan number from the table, in future, a real-time updated list shall be used.
        run_number_list = list()
        for irow in range(self.ui.tableView_summary.rowCount()):
            run_number_list.append(self.ui.tableView_summary.get_scan_number(irow))
        curr_scan = int(self.ui.lineEdit_Scan.text())
        try:
            curr_scan_index = run_number_list.index(curr_scan)
        except IndexError:
            curr_scan_index = 0

        next_scan_index = curr_scan_index + increment
        next_scan_index = (next_scan_index + len(run_number_list)) % len(run_number_list)

        # set
        self.ui.lineEdit_Scan.setText("{}".format(run_number_list[next_scan_index]))

        return

    def set_experiment(self, exp_number):
        """set experiment number to this window for convenience
        :param exp_number:
        :return:
        """
        assert isinstance(exp_number, int) and exp_number > 0, "Experiment number {} (of type {} now) must be a positive integer".format(
            exp_number, type(exp_number)
        )
        self._exp_number = exp_number

        return
