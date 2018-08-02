#pylint: disable=C0103
from __future__ import (absolute_import, division, print_function)
from PyQt4.QtCore import pyqtSignal
import HFIR_4Circle_Reduction.ui_SinglePtIntegrationWindow as window_ui
import HFIR_4Circle_Reduction.guiutility as guiutility
from PyQt4.QtGui import QMainWindow, QFileDialog
import os


class IntegrateSinglePtIntensityWindow(QMainWindow):
    """
    Main window widget to set up parameters to optimize
    """
    # establish signal for communicating from App2 to App1 - must be defined before the constructor
    scanIntegratedSignal = pyqtSignal(dict, name='SinglePtIntegrated')

    def __init__(self, parent=None):
        """
        Initialization
        :param parent:
        :return:
        """
        # init
        super(IntegrateSinglePtIntensityWindow, self).__init__(parent)

        assert parent is not None, 'Parent window cannot be None to set'
        self._parent_window = parent
        self._controller = parent.controller

        # connect signal handler
        self.scanIntegratedSignal.connect(self._parent_window.process_single_pt_scan_intensity)

        # init UI
        self.ui = window_ui.Ui_MainWindow()
        self.ui.setupUi(self)

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

        # menu bar
        self.ui.menuQuit.triggered.connect(self.do_close)
        self.ui.actionSelect_All.triggered.connect(self.menu_table_select_all)
        self.ui.actionDe_select_All.triggered.connect(self.menu_table_select_none)
        self.ui.actionLoad_Gaussian_Sigma_File.triggered.connect(self.menu_load_gauss_sigma_file)
        self.ui.actionLoad_Peak_Info_File.triggered.connect(self.do_load_peak_integration_table)
        self.ui.actionRefresh_ROI_List.triggered.connect(self.do_refresh_roi)

        # class variable
        self._working_dir = os.path.expanduser('~')
        self._exp_number = None
        self._roiMutex = False

        # other things to do
        self.do_refresh_roi()

        return

    def do_close(self):
        """
        Quit the window
        :return:
        """
        self.close()

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
        movie_dir = os.path.join(movie_dir, '{}_{}'.format(roi_name, direction))
        os.mkdir(movie_dir)

        # go through each line to plot and save the data
        num_rows = self.ui.tableView_summary.rowCount()
        file_list_str = ''
        for i_row in range(num_rows):
            # get run number and set to plot
            scan_number = self.ui.tableView_summary.get_scan_number(i_row)
            self.ui.lineEdit_Scan.setText('{}'.format(scan_number))
            png_file_name = os.path.join(movie_dir, 'Scan{0:04d}_ROI{1}_{2}.png'.format(scan_number, roi_name, direction))
            self.do_plot_integrated_pt(show_plot=False, save_plot_to=png_file_name)
            file_list_str += '{}\n'.format(png_file_name)
        # END-IF

        # write out
        png_list_file = open(os.path.join(movie_dir, 'MoviePNGList.txt'), 'w')
        png_list_file.write(file_list_str)
        png_list_file.close()

        return

    def do_integrate_detector_counts(self):
        """
        integrate the (selected) scan's detector counts by ROI
        :return:
        """
        # get ROI
        roi_name = str(self.ui.comboBox_roiList.currentText())
        if roi_name is None or roi_name == '':
            guiutility.show_message('A region-of-interest must be chosen in order to integrate detector counts.')
            return

        # integration direction and fit
        direction = str(self.ui.comboBox_integrateDirection.currentText()).lower()
        fit_gaussian = self.ui.checkBox_fitPeaks.isChecked()

        scan_number_list = list()
        for row_number in range(self.ui.tableView_summary.rowCount()):
            # integrate counts on detector
            scan_number = self.ui.tableView_summary.get_scan_number(row_number)
            scan_number_list.append(scan_number)
        # END-FOR

        peak_height_dict = self._controller.integrate_single_pt_scans_detectors_counts(self._exp_number,
                                                                                       scan_number_list,
                                                                                       roi_name, direction,
                                                                                       fit_gaussian)

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
        if roi_name is None or roi_name == '':
            guiutility.show_message('A region-of-interest must be chosen in order to integrate detector counts.')
            return

        for row_number in range(self.ui.tableView_summary.rowCount()):
            # integrate counts on detector
            scan_number = self.ui.tableView_summary.get_scan_number(row_number)
            pt_number = self.ui.tableView_summary.get_pt_number(row_number)

            # calculate peak intensity
            ref_fwhm = self.ui.tableView_summary.get_fwhm(row_number)

            intensity = self._controller.calculate_intensity_single_pt(self._exp_number, scan_number, pt_number,
                                                                       roi_name, ref_fwhm=ref_fwhm, is_fwhm=False)

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
        table_file = str(QFileDialog.getOpenFileName(self, 'Peak Integration Table', self._working_dir))
        if len(table_file) == 0 or os.path.exists(table_file) is False:
            return

        # load
        status, error_msg = self._controller.load_peak_integration_table(table_file)
        if not status:
            raise RuntimeError(error_msg)

        return

    def do_plot_integrated_pt(self, show_plot=True, save_plot_to=None):
        """ plot integrated Pt with model and raw data
        1. selection include: 2-theta FWHM Model, Summed Single Pt. Counts (horizontal),
        Summed Single Pt. Counts (vertical) from comboBox_plotType
        :return:
        """
        # get scan number
        # TODO TODO TODO - 20180801 - Start from here
        # TODO  - 20180801 - If line edit empty, then use the first scan in the table
        scan_number = int(self.ui.lineEdit_Scan.text())
        roi_name = str(self.ui.comboBox_roiList.currentText())
        direction = str(self.ui.comboBox_integrateDirection.currentText()).lower()

        # get data: pt number is always 1 as it is a single Pt. measurement
        vec_x, vec_y, model_y = self._controller.get_single_scan_pt_result(self._exp_number, scan_number,
                                                                           pt_number=1, roi_name=roi_name,
                                                                           integration_direction=direction)

        self.ui.graphicsView_integration1DView.add_observed_data(vec_x, vec_y, label='integrated counts',
                                                                 update_plot=False)
        # TODO - 20180801 - Model might be an option
        self.ui.graphicsView_integration1DView.add_fit_data(vec_x, model_y, label='Gaussian model',
                                                            update_plot=show_plot)
        # title
        self.ui.graphicsView_integration1DView.set_title('Scan {} Pt {} {} Integration.'
                                                         ''.format(scan_number, 1, direction))

        # save plot?
        if save_plot_to is not None:
            self.ui.graphicsView_integration1DView.canvas.save_figure(save_plot_to)

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
        out_file_name = str(QFileDialog.getSaveFileName(self, 'File to save integrated intensities', self._working_dir))
        if len(out_file_name) == 0:
            return

        self.ui.tableView_summary.save_intensities_to_file(out_file_name)

        return

    def do_retrieve_fwhm(self):
        """
        Get FWHM from integrated 'STRONG' peaks according to 2theta value
        :return:
        """
        row_number = self.ui.tableView_summary.rowCount()
        error_messages = ''
        for row_index in range(row_number):
            # check whether FWHM value is set up
            fwhm_i = self.ui.tableView_summary.get_fwhm(row_index)
            if fwhm_i is not None and fwhm_i > 1.E-10:
                continue

            # use interpolation to curve
            two_theta = self.ui.tableView_summary.get_two_theta(row_index)
            try:
                gauss_sigma = self._controller.calculate_peak_integration_sigma(two_theta)
                scan_number = self.ui.tableView_summary.get_scan_number(row_index)
                pt_number = 1
                roi_name = self.ui.tableView_summary.get_region_of_interest_name(row_index)
                self.ui.tableView_summary.set_gaussian_sigma(row_index, gauss_sigma)
                self._controller.set_single_measure_peak_width(self._exp_number, scan_number, pt_number,
                                                               roi_name, gauss_sigma, is_fhwm=False)

            except RuntimeError as err:
                # error!
                error_messages += 'Unable to calculate sigma of row {0} due to {1}\n'.format(row_index, err)
                continue
            # END-IF-ELSE

        # show error message if necessary
        if len(error_messages) > 0:
            guiutility.show_message(self, error_messages)

        return

    def menu_load_gauss_sigma_file(self):
        """
        load a Gaussian sigma curve for interpolation or matching
        :return:
        """
        # get the column ascii file name
        file_filter = 'Data Files (*.dat);;All Files (*.*)'
        twotheta_sigma_file_name = str(QFileDialog.getOpenFileName(self, self._working_dir,
                                                                   '2theta Gaussian-Sigma File',
                                                                   file_filter))
        if len(twotheta_sigma_file_name) == 0 or twotheta_sigma_file_name == 'None':
            return

        # set the file to controller
        try:
            vec_x, vec_y = self._controller.import_2theta_gauss_sigma_file(twotheta_sigma_file_name)
            self.ui.graphicsView_integration1DView.plot_2theta_model(vec_x, vec_y)
        except RuntimeError as run_err:
            guiutility.show_message(self, str(run_err))
            return

        return

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
        assert isinstance(scan_pt_list, list), 'Scan-Pt-Infos {} must be a list but not a {}.' \
                                               ''.format(scan_pt_list, type(scan_pt_list))

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

    def change_scan_number(self, incremental):
        """
        change the scan number in the
        :param incremental:
        :return:
        """
        # get the list of scan number from the table, in future, a real-time updated list shall be used.
        run_number_list = list()
        for irow in range(self.ui.tableView_summary.rowCount()):
            run_number_list.append(self.ui.tableView_summary.get_scan_number(irow))
        curr_scan = int(self.ui.lineEdit_Scan.text())
        try:
            curr_scan_index = run_number_list.index(curr_scan)
        except IndexError:
            curr_scan_index = 0

        next_scan_index = curr_scan_index + incremental
        next_scan_index = (next_scan_index + len(run_number_list)) % len(run_number_list)

        # set
        self.ui.lineEdit_Scan.setText('{}'.format(run_number_list[next_scan_index]))

        return

    def set_experiment(self, exp_number):
        """ set experiment number to this window for convenience
        :param exp_number:
        :return:
        """
        assert isinstance(exp_number, int) and exp_number > 0, 'Experiment number {} (of type {} now) must be a ' \
                                                               'positive integer'.format(exp_number, type(exp_number))
        self._exp_number = exp_number

        return
