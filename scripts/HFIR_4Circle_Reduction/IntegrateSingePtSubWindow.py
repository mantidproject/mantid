#pylint: disable=C0103
from __future__ import (absolute_import, division, print_function)
from PyQt4.QtCore import pyqtSignal
import HFIR_4Circle_Reduction.ui_SinglePtIntegrationWindow as window_ui
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
        self.ui.pushButton_exportIntensityToFile.clicked.connect(self.do_save_intensity)
        self.ui.pushButton_exportIntensityToTable.clicked.connect(self.do_export_intensity_to_parent)
        self.ui.pushButton_refreshROI.clicked.connect(self.do_refresh_roi)
        self.ui.pushButton_retrieveFWHM.clicked.connect(self.do_retrieve_fwhm)
        self.ui.pushButton_integratePeaks.clicked.connect(self.do_integrate_single_pt)
        self.ui.pushButton_plot.clicked.connect(self.do_plot_integrated_pt)

        # menu bar
        self.ui.menuQuit.triggered.connect(self.do_close)

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
        # collect all scan/pt from table
        scan_pt_list = self.ui.tableView_summary.get_scan_pt_list()

        # add to table including calculate peak center in Q-space
        self.scanIntegratedSignal.emit(scan_pt_list)

        return

    def do_plot_integrated_pt(self):
        """
        plot integrated Pt with model
        :return:
        """

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

    def do_integrate_single_pt(self):
        """
        integrate the 2D data inside region of interest along both axis-0 and axis-1 individually.
        and the result (as 1D data) will be saved to ascii file.
        the X values will be the corresponding pixel index either along axis-0 or axis-1
        :return:
        """
        roi_name = str(self.ui.comboBox_roiList.currentText())
        if len(roi_name) == 0:
            raise RuntimeError('No ROI is selected or set')

        for scan_number, pt_number in self.ui.tableView_summary.get_scan_pt_list():
            # integration on image for I
            peak_height = self._controller.integrate_detector_image(self._exp_number, scan_number, pt_number, roi_name,
                                                                    fit_gaussian=True)

            # calculate peak intensity
            intensity = self._controller.calculate_intensity_single_pt(self._exp_number, scan_number, pt_number,
                                                                       roi_name)

            # add to table
            self.ui.tableView_summary.set_peak_height(scan_number, pt_number, peak_height)
            self.ui.tableView_summary.set_intensity(scan_number, pt_number, intensity)
        # END-FOR

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

    def do_retrieve_fwhm(self):
        """
        get FWHM from integrated 'STRONG' peaks according to 2theta value
        :return:
        """
        for (scan_number, pt_number) in self.ui.tableView_summary.get_scan_pt_list():
            two_theta = self._controller.get_log_value(self._exp_number, scan_number, pt_number)
            self.ui.tableView_summary.set_two_theta(scan_number, pt_number, two_theta)

        return

    def add_scans(self, scan_pt_list):
        """
        add scans' information to table, i.e., add line
        :param scan_pt_list:
        :return:
        """
        # check ... blabla
        print('[DB...BAT] Add scan_pt_info: {0}'.format(scan_pt_list))

        for scan_pt_info in scan_pt_list:
            scan_number, pt_number, hkl, two_theta = scan_pt_info
            print ('[DB...BAT] Add scan_pt_info')
            self.ui.tableView_summary.add_scan_pt(scan_number, pt_number, hkl, two_theta)
        # END-FOR

        return

    def set_experiment(self, exp_number):
        """
        blabla
        :param exp_number:
        :return:
        """
        # check .. blabla

        self._exp_number = exp_number

        return
