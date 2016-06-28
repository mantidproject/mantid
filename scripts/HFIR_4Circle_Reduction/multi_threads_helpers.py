import datetime
import time
from PyQt4 import QtCore
from PyQt4.QtCore import QThread

import reduce4circleControl as r4c

class getPostsThread(QThread):
    def __init__(self, main_window):
        QThread.__init__(self)
        self.main_window = main_window
        print '[DB...Prototype] Thread init...'
        self.start_time = None
        self.num_loops = 0
        self.stopSignal = False

    def __del__(self):
        self.wait()

    def stop(self):
        print 'set stop to ', self.stop
        self.stopSignal = True

    def run(self):
        if self.start_time is None:
            self.start_time = datetime.datetime.now()

        while not self.stopSignal:
            msg = 'loop %d: %s ... %s' % (self.num_loops, (datetime.datetime.now()), str(self.stopSignal))
            print '....................................', msg
            self.sleep(1)
            self.num_loops += 1
            # self.main_window.ui.label_message.setText(msg)

        return


class AddPeaksThread(QThread):
    """
    A QThread class to add peaks to Mantid to calculate UB matrix
    """
    # signal for a peak is added: int_0 = experiment number, int_1 = scan number
    peakAddedSignal = QtCore.pyqtSignal(int, int)
    # signal for status: int_0 = experiment number, int_1 = scan number, int_2 = progress (0...)
    peakStatusSignal = QtCore.pyqtSignal(int, int, int)

    def __init__(self, main_window, exp_number, scan_number_list):
        """
        Initialization
        :param main_window:
        :param exp_number:
        :param scan_number_list:
        """
        QThread.__init__(self)

        # check
        assert main_window is not None, 'Main window cannot be None'
        assert isinstance(exp_number, int), 'Experiment number must be an integer.'
        assert isinstance(scan_number_list, list), 'Scan number list must be a list but not %s.' \
                                                   '' % str(type(scan_number_list))

        # set values
        self._mainWindow = main_window
        self._expNumber = exp_number
        self._scanNumberList = scan_number_list

        # connect to the updateTextEdit slot defined in app1.py
        self.peakAddedSignal.connect(self._mainWindow.update_peak_added_info)
        self.peakStatusSignal.connect(self._mainWindow.update_adding_peaks_status)

        return

    def __del__(self):
        """
        Delete signal
        :return:
        """
        self.wait()

        return

    def run(self):
        """
        method for thread is running
        :return:
        """
        # declare list of failed
        failed_list = list()

        # loop over all scan numbers
        for index, scan_number in enumerate(self._scanNumberList):
            # update state
            self.peakStatusSignal.emit(self._expNumber, scan_number, index)

            # merge peak
            status, err_msg = self._mainWindow._myControl.merge_pts_in_scan(
                self._expNumber, scan_number, [], 'q-sample')

            # continue to the next scan if there is something wrong
            if status is False:
                failed_list.append((scan_number, err_msg))
                continue

            # find peak
            self._mainWindow._myControl.find_peak(self._expNumber, scan_number)

            # get PeakInfo
            peak_info = self._mainWindow._myControl.get_peak_info(self._expNumber, scan_number)
            assert isinstance(peak_info, r4c.PeakProcessHelper)

            # send signal to main window for peak being added
            self.peakAddedSignal.emit(self._expNumber, scan_number)

            # retrieve and set HKL from spice table
            # peak_info.retrieve_hkl_from_spice_table()
            # add to table
            # self.main_window.set_ub_peak_table(peak_info)
        # END-FOR

        self.peakStatusSignal.emit(self._expNumber, -1, len(self._scanNumberList))

        # pop error if there is any scan that is not reduced right
        if len(failed_list) > 0:
            failed_scans_str = 'Unable to merge scans: '
            sum_error_str = ''
            for fail_tup in failed_list:
                failed_scans_str += '%d, ' % fail_tup[0]
                sum_error_str += '%s\n' % fail_tup[1]
            # END-FOR

            self._mainWindow.pop_one_button_dialog(failed_scans_str)
            self._mainWindow.pop_one_button_dialog(sum_error_str)
        # END-IF

        return


class IntegratePeaksThread(QThread):
    """
    A thread to integrate peaks
    """
    # signal to emit before a merge/integration status: exp number, scan number, progress, mode
    peakMergeSignal = QtCore.pyqtSignal(int, int, int, int)

    def __init__(self, main_window, exp_number, row_number_list, mask_det, mask_name, norm_type):
        """

        :param main_window:
        :param exp_number:
        :param row_number_list:
        :param mask_det:
        :param mask_name:
        :param norm_type: type of normalization
        """
        QThread.__init__(self)

        # check
        assert main_window is not None, 'Main window cannot be None'
        assert isinstance(exp_number, int), 'Experiment number must be an integer.'
        assert isinstance(row_number_list, list), 'Scan number list must be a list but not %s.' \
                                                  '' % str(type(row_number_list))
        assert isinstance(mask_det, bool), 'Parameter mask_det must be a boolean but not %s.' \
                                           '' % str(type(mask_det))
        assert isinstance(mask_name, str), 'Name of mask must be a string but not %s.' % str(type(mask_name))
        assert isinstance(norm_type, str), 'Normalization type must be a string but not %s.' \
                                           '' % str(type(norm_type))

        # set values
        self._mainWindow = main_window
        self._expNumber = exp_number
        self._rowNumberList = row_number_list[:]
        self._maskDetector = mask_det
        self._normalizeType = norm_type
        self._selectedMaskName = mask_name

        # link signals
        self.peakMergeSignal.connect(self._mainWindow.upate_merge_status)

        return

    def __del__(self):
        """
        Delete signal
        :return:
        """
        self.wait()

        return

    def run(self):
        """
        Execute the thread!
        :return:
        """
        # TODO/NEXT - consider to move the following section of codes to reduce4circleGUI
        # get the merging information: each item should be a tuple as (scan number, pt number list, merged)
        scan_number_list = list()

        for row_number in self._rowNumberList:
            # get scan number and pt numbers
            scan_number = self._mainWindow.ui.tableWidget_mergeScans.get_scan_number(row_number)
            status, pt_number_list = self._mainWindow._myControl.get_pt_numbers(self._expNumber, scan_number)

            # set intensity to zero and error message
            if status is False:
                error_msg = 'Unable to get Pt. of experiment %d scan %d due to %s.' % (
                    self._expNumber, scan_number, str(pt_number_list))
                self._mainWindow._myControl.set_peak_intensity(self._expNumber, scan_number, 0.)
                self._mainWindow.ui.tableWidget_mergeScans.set_peak_intensity(row_number, scan_number, 0., False)
                self._mainWindow.ui.tableWidget_mergeScans.set_status(scan_number, error_msg)
                continue

            # merge all Pt. of the scan if they are not merged.
            merged = self._mainWindow.ui.tableWidget_mergeScans.get_merged_status(row_number)

            # add to list
            scan_number_list.append((scan_number, pt_number_list, merged))
            self._mainWindow.ui.tableWidget_mergeScans.set_status_by_row(row_number, 'Waiting')

        # END-FOR

        grand_error_message = ''
        for index, scan_tup in enumerate(scan_number_list):
            """
            # get scan number and pt numbers
            scan_number = self._mainWindow.ui.tableWidget_mergeScans.get_scan_number(row_number)
            status, pt_number_list = self._mainWindow._myControl.get_pt_numbers(self._expNumber, scan_number)

            # set intensity to zero and error message
            if status is False:
                error_msg = 'Unable to get Pt. of experiment %d scan %d due to %s.' % (
                    self._expNumber, scan_number, str(pt_number_list))
                self._mainWindow._myControl.set_peak_intensity(self._expNumber, scan_number, 0.)
                self._mainWindow.ui.tableWidget_mergeScans.set_peak_intensity(row_number, scan_number, 0., False)
                self._mainWindow.ui.tableWidget_mergeScans.set_status(scan_number, error_msg)
                continue

            # merge all Pt. of the scan if they are not merged.
            merged = self._mainWindow.ui.tableWidget_mergeScans.get_merged_status(row_number)
            """
            assert isinstance(scan_tup, tuple) and len(scan_tup) == 3
            scan_number, pt_number_list, merged = scan_tup

            # emit signal for run start (mode 0)
            mode = int(0)
            self.peakMergeSignal.emit(self._expNumber, scan_number, index, mode)

            # merge if not merged
            if merged is False:
                self._mainWindow._myControl.merge_pts_in_scan(exp_no=self._expNumber, scan_no=scan_number,
                                                              pt_num_list=pt_number_list,
                                                              target_frame='q-sample')
                self._mainWindow.ui.tableWidget_mergeScans.set_status_by_row(row_number, 'Done')
            # END-IF

            # calculate peak center
            try:
                status, ret_obj = self._mainWindow._myControl.calculate_peak_center(self._expNumber, scan_number, pt_number_list)
            except RuntimeError as run_err:
                status = False
                ret_obj = 'RuntimeError: %s.' % str(run_err)
            except AssertionError as ass_err:
                status = False
                ret_obj = 'AssertionError: %s.' % str(ass_err)

            if status:
                center_i = ret_obj
            else:
                error_msg = 'Unable to find peak for exp %d scan %d: %s.' % (self._expNumber, scan_number, str(ret_obj))
                self._mainWindow._myControl.set_peak_intensity(self._expNumber, scan_number, 0.)
                self._mainWindow.ui.tableWidget_mergeScans.set_peak_intensity(row_number, scan_number, 0., False)
                self._mainWindow.ui.tableWidget_mergeScans.set_status(scan_number, error_msg)
                continue

            # check given mask workspace
            if self._maskDetector:
                self._mainWindow._myControl.check_generate_mask_workspace(self._expNumber, scan_number,
                                                                          self._selectedMaskName)

            # integrate peak
            status, ret_obj = self._mainWindow._myControl.integrate_scan_peaks(exp=self._expNumber,
                                                                               scan=scan_number,
                                                                               peak_radius=1.0,
                                                                               peak_centre=center_i,
                                                                               merge_peaks=False,
                                                                               use_mask=self._maskDetector,
                                                                               normalization=self._normalizeType,
                                                                               mask_ws_name=self._selectedMaskName)
            # handle integration error
            # FIXME/TODO/NOW - Make this right
            if status:
                # get PT dict
                pt_dict = ret_obj
            else:
                # integration failed
                error_msg = str(ret_obj)
                self.errorSignal.emit(self._expNumber, scan_number, error_msg)
                """ for main window method...

                self._mainWindow._myControl.set_peak_intensity(self._expNumber, scan_number, 0.)
                self._mainWindow.ui.tableWidget_mergeScans.set_peak_intensity(row_number, scan_number, 0., False)
                self._mainWindow.ui.tableWidget_mergeScans.set_status(scan_number, error_msg)
                """
                continue

            # FIXME/TODO/NOW - Make this right
            num_bg_pt = 2
            background_pt_list = pt_number_list[:num_bg_pt] + pt_number_list[-num_bg_pt:]
            avg_bg_value = self._mainWindow._myControl.estimate_background(pt_dict, background_pt_list)
            intensity_i = self._mainWindow._myControl.simple_integrate_peak(pt_dict, avg_bg_value)

            self.peakMergeSignal.emit(self._expNumber, scan_number, int(intensity_i), 1)

            continue

            # check intensity value
            if intensity_i < 0:
                # set to status
                error_msg = 'Negative intensity: %.3f' % intensity_i
                self._mainWindow.tableWidget_mergeScans.set_status(scan_no=scan_number, status=error_msg)
                # reset intensity to 0.
                intensity_i = 0.

            # set the calculated peak intensity to _peakInfoDict
            status, error_msg = self._mainWindow._myControl.set_peak_intensity(exp_number, scan_number, intensity_i)
            if status is False:
                grand_error_message += error_msg + '\n'
                continue

            # set the value to table
            self._mainWindow.ui.tableWidget_mergeScans.set_peak_intensity(row_number, None, intensity_i)
        # END-FOR

        # pop error message if there is any
        if len(grand_error_message) > 0:
            self.pop_one_button_dialog(grand_error_message)

        self._mainWindow.ui.tableWidget_mergeScans.select_all_rows(False)

        # count time
        integrate_peak_time_end = time.clock()
        elapsed = integrate_peak_time_end - integrate_peak_time_start
        self._mainWindow.ui.statusbar.showMessage('Peak integration is finished in %.2f seconds' % elapsed)

        return
