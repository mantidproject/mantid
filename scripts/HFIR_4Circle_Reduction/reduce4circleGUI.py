#pylint: disable=invalid-name,relative-import,W0611,R0921,R0902,R0904,R0921,C0302
################################################################################
#
# MainWindow application for reducing HFIR 4-circle
#
################################################################################
import os
import math
import csv
import time

from PyQt4 import QtCore, QtGui
from mantidqtpython import MantidQt

import reduce4circleControl as r4c
import guiutility as gutil
import fourcircle_utility as fcutil

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

# import line for the UI python class
from ui_MainWindow import Ui_MainWindow


class MainWindow(QtGui.QMainWindow):
    """ Class of Main Window (top)
    """
    def __init__(self, parent=None):
        """ Initialization and set up
        """
        # Base class
        QtGui.QMainWindow.__init__(self,parent)

        # UI Window (from Qt Designer)
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        # Make UI scrollable
        self._scrollbars = MantidQt.API.WidgetScrollbarDecorator(self)
        self._scrollbars.setEnabled(True) # Must follow after setupUi(self)!

        # Mantid configuration
        self._instrument = str(self.ui.comboBox_instrument.currentText())
        # config = ConfigService.Instance()
        # self._instrument = config["default.instrument"]

        # Event handling definitions
        # Top
        self.connect(self.ui.pushButton_setExp, QtCore.SIGNAL('clicked()'),
                     self.do_set_experiment)

        # Tab 'Data Access'
        self.connect(self.ui.pushButton_applySetup, QtCore.SIGNAL('clicked()'),
                     self.do_apply_setup)
        self.connect(self.ui.pushButton_browseLocalDataDir, QtCore.SIGNAL('clicked()'),
                     self.do_browse_local_spice_data)
        self.connect(self.ui.pushButton_testURLs, QtCore.SIGNAL('clicked()'),
                     self.do_test_url)
        self.connect(self.ui.pushButton_ListScans, QtCore.SIGNAL('clicked()'),
                     self.do_list_scans)
        self.connect(self.ui.pushButton_downloadExpData, QtCore.SIGNAL('clicked()'),
                     self.do_download_spice_data)
        self.connect(self.ui.comboBox_mode, QtCore.SIGNAL('currentIndexChanged(int)'),
                     self.change_data_access_mode)

        # Tab 'View Raw Data'
        self.connect(self.ui.pushButton_setScanInfo, QtCore.SIGNAL('clicked()'),
                     self.do_load_scan_info)
        self.connect(self.ui.pushButton_plotRawPt, QtCore.SIGNAL('clicked()'),
                     self.do_plot_pt_raw)
        self.connect(self.ui.pushButton_prevPtNumber, QtCore.SIGNAL('clicked()'),
                     self.do_plot_prev_pt_raw)
        self.connect(self.ui.pushButton_nextPtNumber, QtCore.SIGNAL('clicked()'),
                     self.do_plot_next_pt_raw)
        self.connect(self.ui.pushButton_showPtList, QtCore.SIGNAL('clicked()'),
                     self.show_scan_pt_list)
        self.connect(self.ui.pushButton_usePt4UB, QtCore.SIGNAL('clicked()'),
                     self.do_add_peak_to_find)

        # Tab 'calculate ub matrix'
        self.connect(self.ui.pushButton_findPeak, QtCore.SIGNAL('clicked()'),
                     self.do_find_peak)
        self.connect(self.ui.pushButton_addPeakToCalUB, QtCore.SIGNAL('clicked()'),
                     self.do_add_ub_peak)
        self.connect(self.ui.pushButton_calUB, QtCore.SIGNAL('clicked()'),
                     self.do_cal_ub_matrix)
        self.connect(self.ui.pushButton_acceptUB, QtCore.SIGNAL('clicked()'),
                     self.doAcceptCalUB)
        self.connect(self.ui.pushButton_indexUBPeaks, QtCore.SIGNAL('clicked()'),
                     self.do_index_ub_peaks)
        self.connect(self.ui.pushButton_deleteUBPeak, QtCore.SIGNAL('clicked()'),
                     self.do_del_ub_peaks)
        self.connect(self.ui.pushButton_clearUBPeakTable, QtCore.SIGNAL('clicked()'),
                     self.do_clear_ub_peaks)
        self.connect(self.ui.pushButton_resetPeakHKLs, QtCore.SIGNAL('clicked()'),
                     self.do_reset_ub_peaks_hkl)

        # Tab 'Slice View'
        self.connect(self.ui.pushButton_setUBSliceView, QtCore.SIGNAL('clicked()'),
                     self.do_set_ub_sv)
        self.connect(self.ui.pushButton_process4SliceView, QtCore.SIGNAL('clicked()'),
                     self.do_merge_scans)

        # Tab 'Advanced'
        self.connect(self.ui.pushButton_useDefaultDir, QtCore.SIGNAL('clicked()'),
                     self.do_setup_dir_default)
        self.connect(self.ui.pushButton_browseLocalCache, QtCore.SIGNAL('clicked()'),
                     self.do_browse_local_cache_dir)
        self.connect(self.ui.pushButton_browseWorkDir, QtCore.SIGNAL('clicked()'),
                     self.do_browse_working_dir)
        self.connect(self.ui.comboBox_instrument, QtCore.SIGNAL('currentIndexChanged(int)'),
                     self.change_instrument_name)

        # Refine UB matrix
        self.connect(self.ui.pushButton_addToRefine, QtCore.SIGNAL('clicked()'),
                     self.do_refine_ub)
        self.connect(self.ui.pushButton_addAllRefineUB, QtCore.SIGNAL('clicked()'),
                     self.do_refine_ub)
        self.connect(self.ui.pushButton_acceptRefinedUB, QtCore.SIGNAL('clicked()'),
                     self.do_refine_ub)
        self.connect(self.ui.pushButton_resetRefinedUB, QtCore.SIGNAL('clicked()'),
                     self.do_refine_ub)

        # Tab 'Integrate Peaks'
        self.connect(self.ui.pushButton_integratePeak, QtCore.SIGNAL('clicked()'),
                     self.do_integrate_peaks)

        # Menu
        self.connect(self.ui.actionExit, QtCore.SIGNAL('triggered()'),
                     self.menu_quit)

        self.connect(self.ui.actionSave_Session, QtCore.SIGNAL('triggered()'),
                     self.save_current_session)
        self.connect(self.ui.actionLoad_Session, QtCore.SIGNAL('triggered()'),
                     self.load_session)

        # Event handling for tab 'refine ub matrix'
        self.connect(self.ui.pushButton_addToRefine, QtCore.SIGNAL('clicked()'),
                     self.doAddScanPtToRefineUB)

        # Validator ... (NEXT)

        # Declaration of class variable
        # some configuration
        self._homeSrcDir = os.getcwd()
        self._homeDir = os.getcwd()

        # Control
        self._myControl = r4c.CWSCDReductionControl(self._instrument)
        self._allowDownload = True
        self._dataAccessMode = 'Download'

        # Initial setup
        self.ui.tabWidget.setCurrentIndex(0)
        self.ui.tabWidget.setTabEnabled(4, False)
        self.ui.tabWidget.setTabEnabled(5, False)
        self._init_ub_table()
        self.ui.radioButton_ubFromTab1.setChecked(True)

        # Tab 'Access'
        self.ui.lineEdit_url.setText('http://neutron.ornl.gov/user_data/hb3a/')
        self.ui.comboBox_mode.setCurrentIndex(0)
        self.ui.lineEdit_localSpiceDir.setEnabled(True)
        self.ui.pushButton_browseLocalDataDir.setEnabled(True)

        return

    def do_integrate_peaks(self):
        """

        :return:
        """
        raise RuntimeError('ASAP')

    def do_refine_ub(self):
        """

        :return:
        """
        raise RuntimeError('Next Release')

    def _init_ub_table(self):
        """ DOC
        :return:
        """
        # UB-peak table
        # NOTE: have to call this because pyqt set column and row to 0 after __init__
        #       thus a 2-step initialization has to been adopted
        self.ui.tableWidget_peaksCalUB.setup()

        self.ui.tableWidget_ubMatrix.setup()
        self.ui.tableWidget_ubSiceView.setup()
        self.ui.tableWidget_refinedUB.setup()

        self.ui.tableWidget_sliceViewProgress.setup()

        return

    def change_data_access_mode(self):
        """ Change data access mode between downloading from server and local
        Event handling methods
        :return:
        """
        new_mode = str(self.ui.comboBox_mode.currentText())
        self._dataAccessMode = new_mode

        if new_mode.startswith('Local') is True:
            self.ui.lineEdit_localSpiceDir.setEnabled(True)
            self.ui.pushButton_browseLocalDataDir.setEnabled(True)
            self.ui.lineEdit_url.setEnabled(False)
            self.ui.lineEdit_localSrcDir.setEnabled(False)
            self.ui.pushButton_browseLocalCache.setEnabled(False)
            self._allowDownload = False
        else:
            self.ui.lineEdit_localSpiceDir.setEnabled(False)
            self.ui.pushButton_browseLocalDataDir.setEnabled(False)
            self.ui.lineEdit_url.setEnabled(True)
            self.ui.lineEdit_localSrcDir.setEnabled(True)
            self.ui.pushButton_browseLocalCache.setEnabled(True)
            self._allowDownload = True

        return

    def change_instrument_name(self):
        """ Handing the event as the instrument name is changed
        :return:
        """
        new_instrument = str(self.ui.comboBox_instrument.currentText())
        self.pop_one_button_dialog('Change of instrument during data processing is dangerous.')
        status, error_message = self._myControl.set_instrument_name(new_instrument)
        if status is False:
            self.pop_one_button_dialog(error_message)

        return

    def do_add_ub_peak(self):
        """ Add current to ub peaks
        :return:
        """
        # Add peak
        status, int_list = gutil.parse_integers_editors([self.ui.lineEdit_exp,
                                                         self.ui.lineEdit_scanNumber,
                                                         self.ui.lineEdit_ptNumber])
        if status is False:
            self.pop_one_button_dialog(int_list)
        exp_no, scan_no, pt_no = int_list

        # Get HKL from GUI
        status, float_list = gutil.parse_float_editors([self.ui.lineEdit_H,
                                                        self.ui.lineEdit_K,
                                                        self.ui.lineEdit_L])
        if status is False:
            err_msg = float_list
            self.pop_one_button_dialog(err_msg)
            return
        h, k, l = float_list

        status, peak_info_obj = self._myControl.get_peak_info(exp_no, scan_no, pt_no)
        if status is False:
            error_message = peak_info_obj
            self.pop_one_button_dialog(error_message)
            return
        assert isinstance(peak_info_obj, r4c.PeakInfo)

        if self.ui.checkBox_roundHKLInt.isChecked():
            h = math.copysign(1, h)*int(abs(h)+0.5)
            k = math.copysign(1, k)*int(abs(k)+0.5)
            l = math.copysign(1, l)*int(abs(l)+0.5)
        peak_info_obj.set_user_hkl(h, k, l)
        self.set_ub_peak_table(peak_info_obj)

        # Clear
        self.ui.lineEdit_scanNumber.setText('')
        self.ui.lineEdit_ptNumber.setText('')

        self.ui.lineEdit_sampleQx.setText('')
        self.ui.lineEdit_sampleQy.setText('')
        self.ui.lineEdit_sampleQz.setText('')

        self.ui.lineEdit_H.setText('')
        self.ui.lineEdit_K.setText('')
        self.ui.lineEdit_L.setText('')

        return

    def doAcceptCalUB(self):
        """ Accept the calculated UB matrix
        """
        raise RuntimeError('ASAP')
        return

    def doAddScanPtToRefineUB(self):
        """ Add scan/pt numbers to the list of data points for refining ub matrix

        And the added scan number and pt numbers will be reflected in the (left sidebar)

        """
        raise RuntimeError("ASAP")

    def do_add_peak_to_find(self):
        """
        Add the scan/pt to the next
        :return:
        """
        scan_no = self.ui.lineEdit_run.text()
        pt_no = self.ui.lineEdit_rawDataPtNo.text()

        self.ui.lineEdit_scanNumber.setText(scan_no)
        self.ui.lineEdit_ptNumber.setText(pt_no)

        self.ui.tabWidget.setCurrentIndex(2)

    def do_browse_local_cache_dir(self):
        """ Browse local cache directory
        :return:
        """
        local_cache_dir = str(QtGui.QFileDialog.getExistingDirectory(self,
                                                                     'Get Local Cache Directory',
                                                                     self._homeSrcDir))

        # Set local directory to control
        status, error_message = self._myControl.set_local_data_dir(local_cache_dir)
        if status is False:
            self.pop_one_button_dialog(error_message)
            return

        # Synchronize to local data/spice directory and local cache directory
        if str(self.ui.lineEdit_localSpiceDir.text()) != '':
            prev_dir = str(self.ui.lineEdit_localSrcDir.text())
            self.pop_one_button_dialog('Local data directory was set up as %s' %
                                       prev_dir)
        self.ui.lineEdit_localSrcDir.setText(local_cache_dir)
        self.ui.lineEdit_localSpiceDir.setText(local_cache_dir)

        return

    def do_browse_local_spice_data(self):
        """ Browse local source SPICE data directory
        """
        src_spice_dir = str(QtGui.QFileDialog.getExistingDirectory(self, 'Get Directory',
                                                                   self._homeSrcDir))
        # Set local data directory to controller
        status, error_message = self._myControl.set_local_data_dir(src_spice_dir)
        if status is False:
            self.pop_one_button_dialog(error_message)
            return

        self._homeSrcDir = src_spice_dir
        self.ui.lineEdit_localSpiceDir.setText(src_spice_dir)

        return

    def do_browse_working_dir(self):
        """
        Browse and set up working directory
        :return:
        """
        work_dir = str(QtGui.QFileDialog.getExistingDirectory(self, 'Get Working Directory', self._homeDir))
        status, error_message = self._myControl.set_working_directory(work_dir)
        if status is False:
            self.pop_one_button_dialog(error_message)
        else:
            self.ui.lineEdit_workDir.setText(work_dir)

        return

    def do_cal_ub_matrix(self):
        """ Calculate UB matrix by 2 or 3 reflections
        """
        # Get reflections
        num_rows = self.ui.tableWidget_peaksCalUB.rowCount()
        peak_info_list = list()
        status, exp_number = gutil.parse_integers_editors(self.ui.lineEdit_exp)
        for i_row in xrange(num_rows):
            if self.ui.tableWidget_peaksCalUB.is_selected(i_row) is True:
                scan_num, pt_num = self.ui.tableWidget_peaksCalUB.get_exp_info(i_row)
                status, peak_info = self._myControl.get_peak_info(exp_number, scan_num, pt_num)
                peak_info.set_peak_ws_hkl_from_user()
                if status is False:
                    self.pop_one_button_dialog(peak_info)
                    return
                assert isinstance(peak_info, r4c.PeakInfo)
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
        status, ub_matrix = self._myControl.calculate_ub_matrix(peak_info_list, a, b, c,
                                                                alpha, beta, gamma)

        # Deal with result
        if status is True:
            self._show_ub_matrix(ub_matrix)
        else:
            err_msg = ub_matrix
            self.pop_one_button_dialog(err_msg)

        return

    def do_clear_ub_peaks(self):
        """
        Clear all peaks in UB-Peak table
        :return:
        """
        self.ui.tableWidget_peaksCalUB.clear()

        return

    def do_del_ub_peaks(self):
        """
        Delete a peak in UB-Peak table
        :return:
        """
        # Find out the lines to get deleted
        row_num_list = self.ui.tableWidget_peaksCalUB.get_selected_rows()
        print '[DB] Row %s are selected' % str(row_num_list)

        # Delete
        self.ui.tableWidget_peaksCalUB.delete_rows(row_num_list)

        return

    def do_download_spice_data(self):
        """ Download SPICE data
        :return:
        """
        # Check scans to download
        scan_list_str = str(self.ui.lineEdit_downloadScans.text())
        if len(scan_list_str) > 0:
            # user specifies scans to download
            valid, scan_list = fcutil.parse_int_array(scan_list_str)
            if valid is False:
                error_message = scan_list
                self.pop_one_button_dialog(error_message)
        else:
            # Get all scans
            status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp])
            if status is False:
                self.pop_one_button_dialog(ret_obj)
                return
            exp_no = ret_obj
            assert isinstance(exp_no, int)
            server_url = str(self.ui.lineEdit_url.text())
            scan_list = fcutil.get_scans_list(server_url, exp_no, return_list=True)
        self.pop_one_button_dialog('Going to download scans %s.' % str(scan_list))

        # Check location
        destination_dir = str(self.ui.lineEdit_localSrcDir.text())
        status, error_message = self._myControl.set_local_data_dir(destination_dir)
        if status is False:
            self.pop_one_button_dialog(error_message)
        else:
            self.pop_one_button_dialog('Spice files will be downloaded to %s.' % destination_dir)

        # Set up myControl for downloading data
        exp_no = int(self.ui.lineEdit_exp.text())
        self._myControl.set_exp_number(exp_no)

        server_url = str(self.ui.lineEdit_url.text())
        status, error_message = self._myControl.set_server_url(server_url)
        if status is False:
            self.pop_one_button_dialog(error_message)
            return

        # Download
        self._myControl.download_data_set(scan_list)

        return

    def do_find_peak(self):
        """ Find peak in a given scan/pt and record it
        """
        # Get experiment, scan and pt
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp,
                                                        self.ui.lineEdit_scanNumber,
                                                        self.ui.lineEdit_ptNumber])
        if status is True:
            exp_no, scan_no, pt_no = ret_obj
        else:
            self.pop_one_button_dialog(ret_obj)
            return

        # Find peak
        status, err_msg = self._myControl.find_peak(exp_no, scan_no, pt_no)
        if status is False:
            self.pop_one_button_dialog(ret_obj)
            return
        if self.ui.checkBox_loadHKLfromFile.isChecked() is True:
            # This is the first time that in the workflow to get HKL from MD workspace
            status, err_msg = self._myControl.set_hkl_to_peak(exp_no, scan_no, pt_no)
            if status is False:
                self.pop_one_button_dialog('Unable to locate peak info due to %s.' % err_msg)

        # Set up correct values to table tableWidget_peaksCalUB
        self._myControl.add_peak_info(exp_no, scan_no, pt_no)
        status, peak_info = self._myControl.get_peak_info(exp_no, scan_no, pt_no)
        if status is False:
            err_msg = peak_info
            raise KeyError(err_msg)
        assert isinstance(peak_info, r4c.PeakInfo)

        # Set the HKL value from PeakInfo directly
        # BAD PROGRAMMING! THERE ARE TOO MANY WAYS TO ACCESS STORED HKL
        h, k, l = peak_info.get_peak_ws_hkl()
        self.ui.lineEdit_H.setText('%.2f' % h)
        self.ui.lineEdit_K.setText('%.2f' % k)
        self.ui.lineEdit_L.setText('%.2f' % l)

        q_sample = peak_info.getQSample()
        self.ui.lineEdit_sampleQx.setText('%.5E' % q_sample[0])
        self.ui.lineEdit_sampleQy.setText('%.5E' % q_sample[1])
        self.ui.lineEdit_sampleQz.setText('%.5E' % q_sample[2])

        # self.set_ub_peak_table(peak_info)

        return

    def do_index_ub_peaks(self):
        """ Index the peaks in the UB matrix peak table
        :return:
        """
        # Get UB matrix
        ub_matrix = self.ui.tableWidget_ubMatrix.get_matrix()
        print '[DB] Get UB matrix ', ub_matrix

        # Do it for each peak
        num_peaks = self.ui.tableWidget_peaksCalUB.rowCount()
        err_msg = ''
        for i_peak in xrange(num_peaks):
            scan_no, pt_no = self.ui.tableWidget_peaksCalUB.get_exp_info(i_peak)
            status, ret_obj = self._myControl.index_peak(ub_matrix, scan_number=scan_no,
                                                         pt_number=pt_no)
            if status is True:
                new_hkl = ret_obj[0]
                error = ret_obj[1]
                self.ui.tableWidget_peaksCalUB.set_hkl(i_peak, new_hkl, error)
            else:
                err_msg += ret_obj + '\n'
        # END-FOR

        if len(err_msg) > 0:
            self.pop_one_button_dialog(err_msg)

        return

    def do_list_scans(self):
        """ List all scans available
        :return:
        """
        # Experiment number
        exp_no = int(self.ui.lineEdit_exp.text())

        access_mode = str(self.ui.comboBox_mode.currentText())
        if access_mode == 'Local':
            spice_dir = str(self.ui.lineEdit_localSpiceDir.text())
            message = fcutil.get_scans_list_local_disk(spice_dir, exp_no)
        else:
            url = str(self.ui.lineEdit_url.text())
            message = fcutil.get_scans_list(url, exp_no)

        self.pop_one_button_dialog(message)

        return

    def do_load_scan_info(self):
        """ Load SIICE's scan file
        :return:
        """
        # Get scan number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_run])
        if status is True:
            scan_no = ret_obj[0]
        else:
            err_msg = ret_obj
            self.pop_one_button_dialog('Unable to get scan number in raw data tab due to %s.' % err_msg)
            return

        status, err_msg = self._myControl.load_spice_scan_file(exp_no=None, scan_no=scan_no)
        if status is False:
            self.pop_one_button_dialog(err_msg)

        return

    def do_plot_pt_raw(self):
        """ Plot the Pt.
        """
        # Get measurement pt and the file number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp,
                                                        self.ui.lineEdit_run,
                                                        self.ui.lineEdit_rawDataPtNo])
        if status is True:
            exp_no = ret_obj[0]
            scan_no = ret_obj[1]
            pt_no = ret_obj[2]
        else:
            self.pop_one_button_dialog(ret_obj)
            return

        # Call to plot 2D
        self._plot_raw_xml_2d(exp_no, scan_no, pt_no)

        return

    def do_plot_prev_pt_raw(self):
        """ Plot the Pt.
        """
        # Get measurement pt and the file number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp,
                                                        self.ui.lineEdit_run,
                                                        self.ui.lineEdit_rawDataPtNo])
        if status is True:
            exp_no = ret_obj[0]
            scan_no = ret_obj[1]
            pt_no = ret_obj[2]
        else:
            self.pop_one_button_dialog(ret_obj)
            return

        # Previous one
        pt_no -= 1
        if pt_no <= 0:
            self.pop_one_button_dialog('Pt. = 1 is the first one.')
            return
        else:
            self.ui.lineEdit_rawDataPtNo.setText('%d' % pt_no)

        # Plot
        self._plot_raw_xml_2d(exp_no, scan_no, pt_no)

        return

    def do_plot_next_pt_raw(self):
        """ Plot the Pt.
        """
        # Get measurement pt and the file number
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp,
                                                        self.ui.lineEdit_run,
                                                        self.ui.lineEdit_rawDataPtNo])
        if status is True:
            exp_no = ret_obj[0]
            scan_no = ret_obj[1]
            pt_no = ret_obj[2]
        else:
            self.pop_one_button_dialog(ret_obj)
            return

        # Previous one
        pt_no += 1
        # get last Pt. number
        status, last_pt_no = self._myControl.get_pt_numbers(exp_no, scan_no)
        if status is False:
            error_message = last_pt_no
            self.pop_one_button_dialog('Unable to access Spice table for scan %d. Reason" %s.' % (
                scan_no, error_message))
        if pt_no > last_pt_no:
            self.pop_one_button_dialog('Pt. = %d is the last one of scan %d.' % (pt_no, scan_no))
            return
        else:
            self.ui.lineEdit_rawDataPtNo.setText('%d' % pt_no)

        # Plot
        self._plot_raw_xml_2d(exp_no, scan_no, pt_no)

        return

    def do_merge_scans(self):
        """ Process data for slicing view
        :return:
        """
        # Get UB matrix
        ub_matrix = self.ui.tableWidget_ubSiceView.get_matrix()
        self._myControl.set_ub_matrix(exp_number=None, ub_matrix=ub_matrix)

        # Get list of scans
        scan_list = gutil.parse_integer_list(str(self.ui.lineEdit_listScansSliceView.text()))
        if len(scan_list) == 0:
            self.pop_one_button_dialog('Scan list is empty.')

        # Set table
        self.ui.tableWidget_sliceViewProgress.append_scans(scans=scan_list)

        # Warning
        self.pop_one_button_dialog('Data processing is long. Be patient!')

        # Process
        base_name = str(self.ui.lineEdit_baseMergeMDName.text())
        scan_list.sort()
        frame = str(self.ui.comboBox_mergeScanFrame.currentText())
        for scan_no in scan_list:
            # Download/check SPICE file
            self._myControl.download_spice_file(None, scan_no, over_write=False)

            # Get some information
            status, pt_list = self._myControl.get_pt_numbers(None, scan_no, load_spice_scan=True)
            if status is False:
                err_msg = pt_list
                self.pop_one_button_dialog('Failed to get Pt. number: %s' % err_msg)
                return
            else:
                # Set information to table
                err_msg = self.ui.tableWidget_sliceViewProgress.set_scan_pt(scan_no, pt_list)
                if len(err_msg) > 0:
                    self.pop_one_button_dialog(err_msg)

            out_ws_name = base_name + '%04d' % scan_no
            self.ui.tableWidget_sliceViewProgress.set_scan_pt(scan_no, 'In Processing')
            try:
                ret_tup = self._myControl.merge_pts_in_scan(exp_no=None, scan_no=scan_no,
                                                            target_ws_name=out_ws_name,
                                                            target_frame=frame)
                merge_status = 'Done'
                merged_name = ret_tup[0]
                group_name = ret_tup[1]
            except RuntimeError as e:
                merge_status = 'Failed. Reason: %s' % str(e)
                merged_name = ''
                group_name = ''
            finally:
                self.ui.tableWidget_sliceViewProgress.set_status(scan_no, merge_status)
                self.ui.tableWidget_sliceViewProgress.set_ws_names(scan_no, merged_name, group_name)

            # Sleep for a while
            time.sleep(0.1)
        # END-FOR

        return

    def do_reset_ub_peaks_hkl(self):
        """
        Reset user specified HKL value to peak table
        :return:
        """
        num_rows = self.ui.tableWidget_peaksCalUB.rowCount()
        for i_row in xrange(num_rows):
            print '[DB] Update row %d' % (i_row)
            scan, pt = self.ui.tableWidget_peaksCalUB.get_scan_pt(i_row)
            status, peak_info = self._myControl.get_peak_info(None, scan, pt)
            if status is False:
                error_message = peak_info
                raise RuntimeError(error_message)
            h, k, l = peak_info.get_user_hkl()
            self.ui.tableWidget_peaksCalUB.update_hkl(i_row, h, k, l)
        # END-FOR

        return

    def do_set_experiment(self):
        """ Set experiment
        :return:
        """
        status, ret_obj = gutil.parse_integers_editors([self.ui.lineEdit_exp])
        if status is True:
            exp_number = ret_obj[0]
            curr_exp_number = self._myControl.get_experiment()
            if curr_exp_number is not None and exp_number != curr_exp_number:
                self.pop_one_button_dialog('Changing experiment to %d.  Clean previous experiment %d\'s result'
                                           ' in Mantid manually.' % (exp_number, curr_exp_number))
            self._myControl.set_exp_number(exp_number)
            self.ui.lineEdit_exp.setStyleSheet('color: black')
        else:
            err_msg = ret_obj
            self.pop_one_button_dialog('Unable to set experiment as %s' % err_msg)
            self.ui.lineEdit_exp.setStyleSheet('color: red')

        self.ui.tabWidget.setCurrentIndex(0)

        return

    def do_set_ub_sv(self):
        """ Set UB matrix in Slice view
        :return:
        """
        if self.ui.radioButton_ubFromTab1.isChecked():
            self.ui.tableWidget_ubSiceView.set_from_matrix(self.ui.tableWidget_ubMatrix.get_matrix())
        elif self.ui.radioButton_ubFromTab3.isChecked():
            self.ui.tableWidget_ubSiceView.set_from_matrix(self.ui.tableWidget_refinedUB.get_matrix())
        elif self.ui.radioButton_ubFromList.isChecked():
            status, ret_obj = gutil.parse_float_array(str(self.ui.plainTextEdit_ubInput.toPlainText()))
            if status is False:
                self.pop_one_button_dialog(ret_obj)
            elif len(ret_obj) != 9:
                self.pop_one_button_dialog('Requiring 9 floats for UB matrix.  Only %d are given.' % len(ret_obj))
            else:
                self.ui.tableWidget_ubSiceView.set_from_list(ret_obj)
        else:
            self.pop_one_button_dialog('None is selected to set UB matrix.')

        return

    def do_setup_dir_default(self):
        """
        Set up default directory for storing data and working
        :return:
        """
        home_dir = os.path.expanduser('~')

        # Data cache directory
        data_cache_dir = os.path.join(home_dir, 'Temp/HB3ATest')
        self.ui.lineEdit_localSpiceDir.setText(data_cache_dir)
        self.ui.lineEdit_localSrcDir.setText(data_cache_dir)

        work_dir = os.path.join(data_cache_dir, 'Workspace')
        self.ui.lineEdit_workDir.setText(work_dir)

        return

    def do_apply_setup(self):
        """
        Apply set up ...
        :return:
        """
        # Local data directory
        local_data_dir = str(self.ui.lineEdit_localSpiceDir.text())
        if os.path.exists(local_data_dir) is False:
            try:
                os.mkdir(local_data_dir)
            except OSError as os_error:
                self.pop_one_button_dialog('Unable to create local data directory %s due to %s.' % (
                    local_data_dir, str(os_error)))
                self.ui.lineEdit_localSpiceDir.setStyleSheet("color: red;")
                return
            else:
                self.ui.lineEdit_localSpiceDir.setStyleSheet("color: black;")
        # END-IF

        # Working directory
        working_dir = str(self.ui.lineEdit_workDir.text())
        if os.path.exists(working_dir) is False:
            try:
                os.mkdir(working_dir)
            except OSError as os_error:
                self.pop_one_button_dialog('Unable to create working directory %s due to %s.' % (
                    working_dir, str(os_error)))
                self.ui.lineEdit_workDir.setStyleSheet("color: red;")
                return
            else:
                self.ui.lineEdit_workDir.setStyleSheet("color: black;")
        # END-IF

        # Server URL
        data_server = str(self.ui.lineEdit_url.text())
        url_is_good = self.do_test_url()
        if url_is_good is False:
            self.ui.lineEdit_url.setStyleSheet("color: red;")
            return
        else:
            self.ui.lineEdit_url.setStyleSheet("color: black;")

        # Set to control
        self._myControl.set_local_data_dir(local_data_dir)
        self._myControl.set_working_directory(working_dir)
        self._myControl.set_server_url(data_server)

        return

    def do_test_url(self):
        """ Test whether the root URL provided specified is good
        """
        url = str(self.ui.lineEdit_url.text())

        url_is_good, err_msg = fcutil.check_url(url)
        if url_is_good is True:
            self.pop_one_button_dialog("URL %s is valid." % url)
        else:
            self.pop_one_button_dialog(err_msg)

        return url_is_good

    def pop_one_button_dialog(self, message):
        """ Pop up a one-button dialog
        :param message:
        :return:
        """
        assert isinstance(message, str)
        QtGui.QMessageBox.information(self, '4-circle Data Reduction', message)

        return

    def save_current_session(self, filename=None):
        """ Save current session/value setup to
        :return:
        """
        # Set up dictionary
        save_dict = dict()

        # Setup
        save_dict['lineEdit_localSpiceDir'] = str(self.ui.lineEdit_localSpiceDir.text())
        save_dict['lineEdit_url'] = str(self.ui.lineEdit_url.text())
        save_dict['lineEdit_workDir']= str(self.ui.lineEdit_workDir.text())

        # Experiment
        save_dict['lineEdit_exp'] = str(self.ui.lineEdit_exp.text())
        save_dict['lineEdit_scanNumber'] = self.ui.lineEdit_scanNumber.text()
        save_dict['lineEdit_ptNumber'] = str(self.ui.lineEdit_ptNumber.text())

        # Lattice
        save_dict['lineEdit_a'] = str(self.ui.lineEdit_a.text())
        save_dict['lineEdit_b'] = str(self.ui.lineEdit_b.text())
        save_dict['lineEdit_c'] = str(self.ui.lineEdit_c.text())
        save_dict['lineEdit_alpha'] = str(self.ui.lineEdit_alpha.text())
        save_dict['lineEdit_beta'] = str(self.ui.lineEdit_beta.text())
        save_dict['lineEdit_gamma'] = str(self.ui.lineEdit_gamma.text())

        # Merge scan
        save_dict['plainTextEdit_ubInput'] = str(self.ui.plainTextEdit_ubInput.toPlainText())
        save_dict['lineEdit_listScansSliceView'] = str(self.ui.lineEdit_listScansSliceView.text())
        save_dict['lineEdit_baseMergeMDName'] = str(self.ui.lineEdit_baseMergeMDName.text())

        # Save to csv file
        if filename is None:
            filename = 'session_backup.csv'
        ofile = open(filename, 'w')
        writer = csv.writer(ofile)
        for key, value in save_dict.items():
            writer.writerow([key, value])
        ofile.close()

        return

    def load_session(self, filename=None):
        """
        To load a session, i.e., read it back:
        :param filename:
        :return:
        """
        if filename is None:
            filename = 'session_backup.csv'

        in_file = open(filename, 'r')
        reader = csv.reader(in_file)
        my_dict = dict(x for x in reader)

        # ...
        for key, value in my_dict.items():
            if key.startswith('lineEdit') is True:
                self.ui.__getattribute__(key).setText(value)
            elif key.startswith('plainText') is True:
                self.ui.__getattribute__(key).setPlainText(value)
            elif key.startswith('comboBox') is True:
                self.ui.__getattribute__(key).setCurrentIndex(int(value))
            else:
                self.pop_one_button_dialog('Error! Widget name %s is not supported' % key)
        # END-FOR

        # ...
        self._myControl.set_local_data_dir(str(self.ui.lineEdit_localSpiceDir.text()))

        return

    def menu_quit(self):
        """

        :return:
        """
        self.close()

    def show_scan_pt_list(self):
        """ Show the range of Pt. in a scan
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
            info = 'Exp %d Scan %d has %d Pt. ranging from %d to %d.\n' % (exp_no, scan_no, num_pts,
                                                                           pt_list[0], pt_list[-1])
            num_miss_pt = pt_list[-1] - pt_list[0] + 1 - num_pts
            if num_miss_pt > 0:
                info += 'There are %d Pt. skipped.\n' % num_miss_pt

            self.pop_one_button_dialog(info)

        return

    def set_ub_peak_table(self, peakinfo):
        """
        DOC
        :param peak_info:
        :return:
        """
        assert isinstance(peakinfo, r4c.PeakInfo)

        # Get data
        exp_number, scan_number, pt_number = peakinfo.getExpInfo()
        h, k, l = peakinfo.get_user_hkl()
        q_sample = peakinfo.getQSample()
        m1 = self._myControl.get_sample_log_value(exp_number, scan_number, pt_number, '_m1')

        # Set to table
        status, err_msg = self.ui.tableWidget_peaksCalUB.append_row(
            [scan_number, pt_number, h, k, l, q_sample[0], q_sample[1], q_sample[2], False, m1, ''])
        if status is False:
            self.pop_one_button_dialog(err_msg)

        return

    def _get_lattice_parameters(self):
        """
        Get lattice parameters from GUI
        :return: (Boolean, Object).  True, 6-tuple as a, b, c, alpha, beta, gamm
                                     False: error message
        """
        status, ret_list = gutil.parse_float_editors([self.ui.lineEdit_a,
                                                      self.ui.lineEdit_b,
                                                      self.ui.lineEdit_c,
                                                      self.ui.lineEdit_alpha,
                                                      self.ui.lineEdit_beta,
                                                      self.ui.lineEdit_gamma])
        if status is False:
            err_msg = ret_list
            err_msg = 'Unable to parse unit cell due to %s' % err_msg
            return False, err_msg

        a, b, c, alpha, beta, gamma = ret_list

        return True, (a, b, c, alpha, beta, gamma)

    def _plot_raw_xml_2d(self, exp_no, scan_no, pt_no):
        """ Plot raw workspace from XML file for a measurement/pt.
        """
        # Check and load SPICE table file
        does_exist = self._myControl.does_spice_loaded(exp_no, scan_no)
        if does_exist is False:
            # Download data
            status, error_message = self._myControl.download_spice_file(exp_no, scan_no, over_write=False)
            if status is True:
                status, error_message = self._myControl.load_spice_scan_file(exp_no, scan_no)
                if status is False and self._allowDownload is False:
                    self.pop_one_button_dialog(error_message)
                    return
            else:
                self.pop_one_button_dialog(error_message)
                return
        # END-IF(does_exist)

        # Load Data for Pt's xml file
        does_exist = self._myControl.does_raw_loaded(exp_no, scan_no, pt_no)

        if does_exist is False:
            # Check whether needs to download
            status, error_message = self._myControl.download_spice_xml_file(scan_no, pt_no, exp_no=exp_no)
            if status is False:
                self.pop_one_button_dialog(error_message)
                return
            # Load SPICE xml file
            status, error_message = self._myControl.load_spice_xml_file(exp_no, scan_no, pt_no)
            if status is False:
                self.pop_one_button_dialog(error_message)
                return

        # Convert a list of vector to 2D numpy array for imshow()
        # Get data and plot
        raw_det_data = self._myControl.get_raw_detector_counts(exp_no, scan_no, pt_no)
        self.ui.graphicsView.clear_canvas()
        self.ui.graphicsView.add_plot_2d(raw_det_data, x_min=0, x_max=256, y_min=0, y_max=256,
                                         hold_prev_image=False)

        return

    def _show_ub_matrix(self, ubmatrix):
        """ Show UB matrix
        :param ubmatrix:
        :return:
        """
        assert ubmatrix.shape == (3, 3)

        self.ui.tableWidget_ubMatrix.set_from_matrix(ubmatrix)

        return
