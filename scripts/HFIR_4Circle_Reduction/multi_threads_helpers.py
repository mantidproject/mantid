#pylint: disable=W0403,R0913,R0902
from PyQt4 import QtCore
from PyQt4.QtCore import QThread

import reduce4circleControl as r4c
import peak_integration_utility


class AddPeaksThread(QThread):
    """
    A QThread class to add peaks to Mantid to calculate UB matrix
    """
    # signal for a peak is added: int_0 = experiment number, int_1 = scan number
    peakAddedSignal = QtCore.pyqtSignal(int, int)
    # signal for status: int_0 = experiment number, int_1 = scan number, int_2 = progress (0...)
    peakStatusSignal = QtCore.pyqtSignal(int, int, int)
    # signal for final error report: int_0 = experiment number, str_1 = error message
    peakAddedErrorSignal = QtCore.pyqtSignal(int, str)

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
        self.peakAddedErrorSignal.connect(self._mainWindow.report_peak_addition)

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
            status, err_msg = self._mainWindow.controller.merge_pts_in_scan(
                self._expNumber, scan_number, [])

            # continue to the next scan if there is something wrong
            if status is False:
                failed_list.append((scan_number, err_msg))
                continue

            # find peak
            self._mainWindow.controller.find_peak(self._expNumber, scan_number)

            # get PeakInfo
            peak_info = self._mainWindow.controller.get_peak_info(self._expNumber, scan_number)
            assert isinstance(peak_info, r4c.PeakProcessRecord)

            # send signal to main window for peak being added
            self.peakAddedSignal.emit(self._expNumber, scan_number)
        # END-FOR

        # send signal with unphysical scan number to flag the end of operation.
        self.peakStatusSignal.emit(self._expNumber, -1, len(self._scanNumberList))

        # construct a final error message for main GUI
        # TEST: Exp 423 Scan 82
        if len(failed_list) > 0:
            failed_scans_str = 'Unable to merge scans: '
            sum_error_str = ''
            for fail_tup in failed_list:
                failed_scans_str += '%d, ' % fail_tup[0]
                sum_error_str += '%s\n' % fail_tup[1]
            # END-FOR

            self.peakAddedErrorSignal.emit(self._expNumber, failed_scans_str + '\n' + sum_error_str)
        # END-IF

        return


class IntegratePeaksThread(QThread):
    """
    A thread to integrate peaks
    """
    # signal to emit before a merge/integration status: exp number, scan number, progress, mode
    peakMergeSignal = QtCore.pyqtSignal(int, int, float, list, int)
    # signal to report state: (1) experiment, (2) scan, (3) mode, (4) message
    mergeMsgSignal = QtCore.pyqtSignal(int, int, int, str)

    def __init__(self, main_window, exp_number, scan_tuple_list, mask_det, mask_name, norm_type, num_pt_bg_left,
                 num_pt_bg_right, scale_factor=1.000):
        """

        :param main_window:
        :param exp_number:
        :param scan_tuple_list: list of tuples for scan as (scan number, pt number list, state as merged)
        :param mask_det:
        :param mask_name:
        :param norm_type: type of normalization
        :param num_pt_bg_left: number of Pt in the left
        :param num_pt_bg_right: number of Pt for background in the right
        """
        # start thread
        QThread.__init__(self)

        # check
        assert main_window is not None, 'Main window cannot be None'
        assert isinstance(exp_number, int), 'Experiment number must be an integer.'
        assert isinstance(scan_tuple_list, list), 'Scan (info) tuple list must be a list but not %s.' \
                                                  '' % str(type(scan_tuple_list))
        assert isinstance(mask_det, bool), 'Parameter mask_det must be a boolean but not %s.' \
                                           '' % str(type(mask_det))
        assert isinstance(mask_name, str), 'Name of mask must be a string but not %s.' % str(type(mask_name))
        assert isinstance(norm_type, str), 'Normalization type must be a string but not %s.' \
                                           '' % str(type(norm_type))
        assert isinstance(num_pt_bg_left, int) and num_pt_bg_left >= 0,\
            'Number of Pt at left for background {0} must be non-negative integers but not of type {1}.' \
            ''.format(num_pt_bg_left, type(num_pt_bg_left))
        assert isinstance(num_pt_bg_right, int) and num_pt_bg_right >= 0,\
            'Number of Pt at right for background {0} must be non-negative integers but not of type {1}.' \
            ''.format(num_pt_bg_right, type(num_pt_bg_right))

        # set values
        self._mainWindow = main_window
        self._expNumber = exp_number
        self._scanTupleList = scan_tuple_list[:]
        self._maskDetector = mask_det
        self._normalizeType = norm_type
        self._selectedMaskName = mask_name
        self._numBgPtLeft = num_pt_bg_left
        self._numBgPtRight = num_pt_bg_right
        self._scaleFactor = scale_factor

        # link signals
        self.peakMergeSignal.connect(self._mainWindow.update_merge_value)
        self.mergeMsgSignal.connect(self._mainWindow.update_merge_message)

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
        for index, scan_tup in enumerate(self._scanTupleList):
            # check
            assert isinstance(scan_tup, tuple) and len(scan_tup) == 3
            scan_number, pt_number_list, merged = scan_tup

            # emit signal for run start (mode 0)
            mode = int(0)
            self.peakMergeSignal.emit(self._expNumber, scan_number, float(index), [0., 0., 0.], mode)

            # merge if not merged
            if merged is False:
                merged_ws_name = 'X'
                try:
                    status, ret_tup = self._mainWindow.controller.merge_pts_in_scan(exp_no=self._expNumber,
                                                                                    scan_no=scan_number,
                                                                                    pt_num_list=pt_number_list)
                    if status:
                        merged_ws_name = str(ret_tup[0])
                        error_message = ''
                    else:
                        error_message = str(ret_tup)
                except RuntimeError as run_err:
                    status = False
                    error_message = str(run_err)

                # continue to
                if status:
                    # successfully merge peak
                    assert isinstance(merged_ws_name, str), 'Merged workspace %s must be a string but not %s.' \
                                                            '' % (str(merged_ws_name), type(merged_ws_name))
                    self.mergeMsgSignal.emit(self._expNumber, scan_number, 1, merged_ws_name)
                else:
                    self.mergeMsgSignal.emit(self._expNumber, scan_number, 0, error_message)
                    continue
                # self._mainWindow.ui.tableWidget_mergeScans.set_status(scan_number, 'Merged')
            else:
                # merged
                pass
            # END-IF

            # calculate peak center
            try:
                # status, ret_obj = self._mainWindow.controller.calculate_peak_center(self._expNumber, scan_number,
                #                                                                     pt_number_list)
                status, ret_obj = self._mainWindow.controller.find_peak(self._expNumber, scan_number, pt_number_list)

            except RuntimeError as run_err:
                status = False
                ret_obj = 'RuntimeError: %s.' % str(run_err)
            except AssertionError as ass_err:
                status = False
                ret_obj = 'AssertionError: %s.' % str(ass_err)

            if status:
                center_i = ret_obj   # 3-tuple
            else:
                error_msg = 'Unable to find peak for exp %d scan %d: %s.' % (self._expNumber, scan_number, str(ret_obj))
                # no need... self._mainWindow.controller.set_peak_intensity(self._expNumber, scan_number, 0.)
                self._mainWindow.ui.tableWidget_mergeScans.set_peak_intensity(None, scan_number, 0., False)
                self._mainWindow.ui.tableWidget_mergeScans.set_status(scan_number, error_msg)
                continue

            # check given mask workspace
            if self._maskDetector:
                self._mainWindow.controller.check_generate_mask_workspace(self._expNumber, scan_number,
                                                                          self._selectedMaskName, check_throw=True)

            bkgd_pt_list = (self._numBgPtLeft, self._numBgPtRight)
            # integrate peak
            try:
                status, ret_obj = self._mainWindow.controller.integrate_scan_peaks(exp=self._expNumber,
                                                                                   scan=scan_number,
                                                                                   peak_radius=1.0,
                                                                                   peak_centre=center_i,
                                                                                   merge_peaks=False,
                                                                                   use_mask=self._maskDetector,
                                                                                   normalization=self._normalizeType,
                                                                                   mask_ws_name=self._selectedMaskName,
                                                                                   scale_factor=self._scaleFactor,
                                                                                   background_pt_tuple=bkgd_pt_list)
            except ValueError as val_err:
                status = False
                ret_obj = 'Unable to integrate scan {0} due to {1}.'.format(scan_number, str(val_err))
            except RuntimeError as run_err:
                status = False
                ret_obj = 'Unable to integrate scan {0}: {1}.'.format(scan_number, run_err)

            # handle integration error
            if status:
                # get PT dict
                pt_dict = ret_obj
                assert isinstance(pt_dict, dict), 'dictionary must'
                self.set_integrated_peak_info(scan_number, pt_dict)
                # information setup include
                # - lorentz correction factor
                # - peak integration dictionary
                # - motor information: peak_info_obj.set_motor(motor_name, motor_step, motor_std_dev)
            else:
                # integration failed
                error_msg = str(ret_obj)
                self.mergeMsgSignal.emit(self._expNumber, scan_number, 0, error_msg)
                continue

            intensity1 = pt_dict['simple intensity']
            peak_centre = self._mainWindow.controller.get_peak_info(self._expNumber, scan_number).get_peak_centre()

            # emit signal to main app for peak intensity value
            mode = 1
            # center_i
            self.peakMergeSignal.emit(self._expNumber, scan_number, float(intensity1), list(peak_centre), mode)
        # END-FOR

        # terminate the process
        mode = int(2)
        self.peakMergeSignal.emit(self._expNumber, -1, len(self._scanTupleList), [0, 0, 0], mode)
        # self._mainWindow.ui.tableWidget_mergeScans.select_all_rows(False)

        return

    def set_integrated_peak_info(self, scan_number, peak_integration_dict):
        """
        set the integrated peak information including
        * calculate Lorentz correction
        * add the integration result dictionary
        * add motor step information
        :return:
        """
        # print '[DB...BAT] Set Integrated Peak Info is called for exp {0} scan {1}.' \
        #       ''.format(self._expNumber, scan_number)

        # get peak information
        peak_info_obj = self._mainWindow.controller.get_peak_info(self._expNumber, scan_number)

        # get Q-vector of the peak center and calculate |Q| from it
        peak_center_q = peak_info_obj.get_peak_centre_v3d().norm()
        # get wave length
        wavelength = self._mainWindow.controller.get_wave_length(self._expNumber, [scan_number])

        # get motor step (choose from omega, phi and chi)
        try:
            motor_move_tup = self._mainWindow.controller.get_motor_step(self._expNumber, scan_number)
            motor_name, motor_step, motor_std_dev = motor_move_tup
        except RuntimeError as run_err:
            return str(run_err)
        except AssertionError as ass_err:
            return str(ass_err)

        # calculate lorentz correction
        lorentz_factor = peak_integration_utility.calculate_lorentz_correction_factor(peak_center_q, wavelength,
                                                                                      motor_step)

        peak_info_obj.lorentz_correction_factor = lorentz_factor
        # set motor
        peak_info_obj.set_motor(motor_name, motor_step, motor_std_dev)
        # set peak integration dictionary
        peak_info_obj.set_integration(peak_integration_dict)

        return
