#pylint: disable=C0103
from __future__ import (absolute_import, division, print_function)
from PyQt4 import QtGui, QtCore
import HFIR_4Circle_Reduction.ui_SinglePtIntegrationWindow as window_ui
from PyQt4.QtGui import QMainWindow, QFileDialog
import numpy as np
import os


class IntegrateSinglePtIntensityWindow(QMainWindow):
    """
    Main window widget to set up parameters to optimize
    """
    # establish signal for communicating from App2 to App1 - must be defined before the constructor
    # mySignal = QtCore.pyqtSignal(int)

    def __init__(self, parent=None):
        """
        Initialization
        :param parent:
        :return:
        """
        # init
        super(IntegrateSinglePtIntensityWindow, self).__init__(parent)
        self._controller = parent.controller

        # init UI
        self.ui = window_ui.Ui_MainWindow()
        self.ui.setupUi(self)

        # initialize widgets
        self.ui.tableView_summary.setup()

        # define event handlers for widgets
        self.ui.pushButton_exportIntensityToFile.clicked.connect(self.do_save_intensity)
        self.ui.pushButton_exportIntensityToTable.clicked.connect(self.do_export_intensity_to_parent)
        self.ui.pushButton_refreshROI.clicked.connect(self.do_refresh_roi)
        self.ui.pushButton_retrieveFWHM.clicked.connect(self.do_retrieve_fwhm)
        self.ui.pushButton_integratePeaks.clicked.connect(self.do_integrate_single_pt)

        # menu bar
        self.ui.menuQuit.triggered.connect(self.do_close)

        # class variable
        self._working_dir = os.path.expanduser('~')
        self._exp_number = None
        self._roiMutex = False

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
        blabla
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
        roi_list = self._controller.get_roi_list()

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
