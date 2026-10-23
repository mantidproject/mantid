# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=W0403,R0913,R0902
import os
from qtpy.QtCore import Signal as pyqtSignal
from qtpy.QtCore import QThread
import mantidqtinterfaces.HFIR_4Circle_Reduction.reduce4circleControl as r4c


class AddPeaksThread(QThread):
    """
    A QThread class to add peaks to Mantid to calculate UB matrix
    """

    # signal for a peak is added: int_0 = experiment number, int_1 = scan number
    peakAddedSignal = pyqtSignal(int, int)
    # signal for status: int_0 = experiment number, int_1 = scan number, int_2 = progress (0...)
    peakStatusSignal = pyqtSignal(int, int, int)
    # signal for final error report: int_0 = experiment number, str_1 = error message
    peakAddedErrorSignal = pyqtSignal(int, str)

    def __init__(self, main_window, exp_number, scan_number_list):
        """
        Initialization
        :param main_window:
        :param exp_number:
        :param scan_number_list:
        """
        # check
        assert main_window is not None, "Main window cannot be None"
        assert isinstance(exp_number, int), "Experiment number must be an integer."
        assert isinstance(scan_number_list, list), "Scan number list must be a list but not %s." % str(type(scan_number_list))

        # init thread
        super(AddPeaksThread, self).__init__()

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
                self._expNumber, scan_number, [], False, self._mainWindow.controller.pre_processed_dir
            )

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
            failed_scans_str = "Unable to merge scans: "
            sum_error_str = ""
            for fail_tup in failed_list:
                failed_scans_str += "%d, " % fail_tup[0]
                sum_error_str += "%s\n" % fail_tup[1]
            # END-FOR

            self.peakAddedErrorSignal.emit(self._expNumber, failed_scans_str + "\n" + sum_error_str)
        # END-IF

        return


class MergePeaksThread(QThread):
    """A thread to integrate peaks"""

    # signal to report state: (1) scan, (2) message
    mergeMsgSignal = pyqtSignal(int, str)
    saveMsgSignal = pyqtSignal(int, str)

    def __init__(self, main_window, exp_number, scan_number_list, md_file_list):
        """Initialization

        :param main_window:
        :param exp_number:
        :param scan_number_list: list of tuples for scan as (scan number, pt number list, state as merged)
        :param md_file_list:
        """
        # check
        assert main_window is not None, "Main window cannot be None"
        assert isinstance(exp_number, int), "Experiment number must be an integer."
        assert isinstance(scan_number_list, list), "Scan (info) tuple list {0} must be a list but not {1}.".format(
            scan_number_list, type(scan_number_list)
        )
        assert isinstance(md_file_list, list) or md_file_list is None, (
            "Output MDWorkspace file name list {0} must be either a list or None but not {1}.".format(md_file_list, type(md_file_list))
        )

        if md_file_list is not None and len(scan_number_list) != len(md_file_list):
            raise RuntimeError(
                "If MD file list is not None, then it must have the same size ({0}) as the scans ({1}) to merge.".format(
                    len(md_file_list), len(scan_number_list)
                )
            )

        # start thread
        QThread.__init__(self)

        # set values
        self._mainWindow = main_window
        self._expNumber = exp_number
        self._scanNumberList = scan_number_list[:]
        self._outputMDFileList = None
        if md_file_list is not None:
            self._outputMDFileList = md_file_list[:]

        # other about preprocessed options
        self._checkPreprocessedScans = False
        self._preProcessedDir = None
        self._redoMerge = True

        # link signals
        self.mergeMsgSignal.connect(self._mainWindow.update_merge_value)
        self.saveMsgSignal.connect(self._mainWindow.update_file_name)

        return

    def __del__(self):
        """Delete signal

        :return:
        """
        self.wait()

        return

    def run(self):
        """Execute the thread!

        i.e., merging the scans
        :return:
        """
        if self._outputMDFileList is None or len(self._outputMDFileList) == 0:
            save_file = False
        else:
            save_file = True

        for index, scan_number in enumerate(self._scanNumberList):
            # set up merging parameters
            pt_number_list = list()

            # emit signal for run start (mode 0)
            # self.peakMergeSignal.emit(scan_number, 'In merging')
            self.mergeMsgSignal.emit(scan_number, "Being merged")

            # merge if not merged
            merged_ws_name = None
            out_file_name = "No File To Save"
            try:
                status, ret_tup = self._mainWindow.controller.merge_pts_in_scan(
                    exp_no=self._expNumber,
                    scan_no=scan_number,
                    pt_num_list=pt_number_list,
                    rewrite=self._redoMerge,
                    preprocessed_dir=self._preProcessedDir,
                )
                if status:
                    merged_ws_name = str(ret_tup[0])
                    error_message = ""
                else:
                    error_message = str(ret_tup)

                # save
                if save_file:
                    out_file_name = self._outputMDFileList[index]
                    self._mainWindow.controller.save_merged_scan(
                        exp_number=self._expNumber,
                        scan_number=scan_number,
                        pt_number_list=pt_number_list,
                        merged_ws_name=merged_ws_name,
                        output=out_file_name,
                    )
                # END-IF-ELSE

            except RuntimeError as run_err:
                # error
                status = False
                error_message = "Failed: {0}".format(run_err)

            # continue to
            if status:
                # successfully merge peak
                assert merged_ws_name is not None, "Impossible situation"
                self.mergeMsgSignal.emit(scan_number, merged_ws_name)
                self.saveMsgSignal.emit(scan_number, out_file_name)
            else:
                # merging error
                self.mergeMsgSignal.emit(scan_number, error_message)
                continue
            # END-IF

        return

    def set_pre_process_options(self, option_to_use, pre_process_dir):
        """
        set the pre-process options
        :param option_to_use:
        :param pre_process_dir:
        :return:
        """
        # check
        assert isinstance(option_to_use, bool), "Option to use pre-process must be a boolean but not a {0}.".format(type(option_to_use))

        self._checkPreprocessedScans = option_to_use

        if self._checkPreprocessedScans:
            assert isinstance(pre_process_dir, str), (
                f"Directory {pre_process_dir} to store preprocessed data must be a string " + f"but not a {type(pre_process_dir)}."
            )
            if os.path.exists(pre_process_dir) is False:
                raise RuntimeError("Directory {0} does not exist.".format(pre_process_dir))
            self._preProcessedDir = pre_process_dir
        # END-IF

        return

    def set_rewrite(self, flag):
        """
        set the flag to re-merge the scan regardless whether the target workspace is in memory
        or a pre-processed MD workspace does exist.
        :param flag:
        :return:
        """
        assert isinstance(flag, bool), "Re-merge/re-write flag must be a boolean but not a {0}".format(type(flag))

        self._redoMerge = flag

        return
