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
        self._controller = parent.get_controller()

        # init UI
        self.ui = window_ui.Ui_MainWindow()
        self.ui.setupUi(self)

        # initialize widgets

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
        self._scan_number = None
        self._pt_number = 1

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

    # TODO FIXME NOW NOW2 - From here
    def do_integrate_single_pt(self, exp_number, scan_number, pt_number):
        """
        integrate the 2D data inside region of interest along both axis-0 and axis-1 individually.
        and the result (as 1D data) will be saved to ascii file.
        the X values will be the corresponding pixel index either along axis-0 or axis-1
        :return:
        """
        roi_name = str(self.ui.comboBox_roiList.currentText())
        if len(roi_name) == 0:
            raise RuntimeError('No ROI is selected or set')

        self._controller.integrate_detector_image(self._exp_number, self._scan_number,
                                                  self._pt_number, roi_name, fit_gaussian=True)






        

    def do_ok(self):
        """
        User decide to go on and then send a signal to parent
        :return:
        """

        tolerance = self.get_tolerance()
        if tolerance is None:
            raise RuntimeError('Tolerance cannot be left blank!')

        # set up a hand-shaking signal
        signal_value = 1000
        self.mySignal.emit(signal_value)

        # quit
        self.do_quit()

        return

    def do_quit(self):
        """
        Quit the window
        :return:
        """
        self.close()

        return

    def get_unit_cell_type(self):
        """
        Get the tolerance
        :return:
        """
        unit_cell_type = str(self.ui.comboBox_unitCellTypes.currentText())

        return unit_cell_type

    def get_tolerance(self):
        """
        Get the tolerance for refining UB matrix with unit cell type.
        :return:
        """
        tol_str = str(self.ui.lineEdit_tolerance.text()).strip()

        if len(tol_str) == 0:
            # blank: return None
            tol = None
        else:
            tol = float(tol_str)

        return tol

    def get_ub_source(self):
        """
        Get the index of the tab where the UB matrix comes from
        :return:
        """
        source = str(self.ui.comboBox_ubSource.currentText())

        if source == 'Tab - Calculate UB Matrix':
            tab_index = 3
        else:
            tab_index = 4

        return tab_index

    def set_prev_ub_refine_method(self, use_fft=False):
        """

        :param use_fft:
        :return:
        """
        self._prevIndexByFFT = use_fft

        return
