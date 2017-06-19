import os
from PyQt4 import QtGui, QtCore
import ui_preprocess_window


class ScanPreProcessWindow(QtGui.QMainWindow):
    """
    Main window class to pre-process scans
    """
    def __init__(self, parent):
        """
        initialization
        :param parent:
        """
        super(ScanPreProcessWindow, self).__init__(parent)

        # define UI
        self.ui = ui_preprocess_window.Ui_PreprocessWindow()
        self.ui.setupUi(self)

        # initialize the widgets
        self.enable_calibration_settings(False)

        # define event handling
        self.connect(self.ui.pushButton_browseOutputDir, QtCore.SIGNAL('clicked()'),
                     self.do_browse_output_dir)
        self.connect(self.ui.pushButton_preProcessScan, QtCore.SIGNAL('clicked()'),
                     self.do_start_pre_process)
        self.connect(self.ui.pushButton_changeSettings, QtCore.SIGNAL('clicked()'),
                     self.do_change_calibration_settings)
        self.connect(self.ui.pushButton_fixSettings, QtCore.SIGNAL('clicked()'),
                     self.do_fix_calibration_settings)

        return

    def do_browse_output_dir(self):
        """
        browse the output directory
        :return:
        """
        # get scan number or numbers
        try:
            scan_numbers = self.get_scan_numbers()
        except RuntimeError as run_err:
            gui_util.pop_error_dialog(self, run_err)
            return

        if len(scan_numbers) != 1:
            default_dir = '/HFIR/HB3A/'
        else:
            default_dir = os.path.join('/HFIR/HB3A/Exp{0}/shared/'.format(scan_numbers[0]))

        # get output directory
        output_dir = str(QtGui.QFileDialog.getExistingDirectory(self,
                                                                'Outputs for pre-processed scans',
                                                                default_dir))
        if output_dir is None or len(output_dir) == 0:
            return

        self.ui.lineEdit_outputDir.setText(output_dir)

        return

    def do_change_calibration_settings(self):
        """
        enable the settings to be modifiable
        :return:
        """
        self.enable_calibration_settings(True)

        return

    def do_fix_calibration_settings(self):
        """
        disable the settings to be modifiable
        :return:
        """
        self.enable_calibration_settings(False)

        return

    def do_start_pre_process(self):
        """
        start the pre-precessing scans
        :return:
        """
        # TODO/ISSUE/NOWNOW

        # get all the information
        exp_number = int(self.ui.lineEdit_ipts.text())
        scan_list = self.get_scan_numbers()

        status, pt_list = self._myControl.get_pt_numbers(exp_number, scan_number)
        if status is False:
            # skip this row due to error
            sum_error_msg += '%s\n' % str(pt_list)
            continue

        self.ui.tableWidget_mergeScans.set_status(row_number, 'In Processing')
        status, ret_tup = self._myControl.merge_pts_in_scan(exp_no=exp_number, scan_no=scan_number,
                                                            pt_num_list=[])

        self._myControl.set_exp_number()
        self._myControl.set_default_detector_sample_distance()
        set_detector_sample_distance
        self._myControl.set_default_pixel_size()
        self._myControl.set_detector_center()
        self._myControl.set_default_pixel_size()
        self._myControl.save_merged_scan()

        return

    def enable_calibration_settings(self, to_enable):
        """
        enable or disable the calibration settings
        :param to_enable:
        :return:
        """
        self.ui.lineEdit_infoDetCenter.setEnabled(to_enable)
        self.ui.lineEdit_infoWavelength.setEnabled(to_enable)
        self.ui.lineEdit_infoDetSampleDistance.setEnabled(to_enable)
        self.ui.comboBox_detSize.setEnabled(to_enable)

        return

    def get_scan_numbers(self):
        """
        parse the scan numbers from the scan numbers line editor
        :return: a list of scan numbers. if no scan is given, an empty string is returned
        """
        # scan list
        scan_list = list()

        scans_str = str(self.ui.lineEdit_scans.text()).strip()
        if len(scans_str) == 0:
            return scan_list

        # replace ',' by ' ' such that the scans can be accepted by different type of deliminators
        scan_str = scans_str.replace(',', '')
        scan_str_list = scans_str.split()

        for scan_str in scan_str_list:
            # skip empty string
            if len(scans_str) == 0:
                continue

            if scan_str.isdigit():
                scan = int(scan_str)
                scan_list.append(scan)
            else:
                raise RuntimeError('{0} in scan list {1} cannot be converted to integer.'.format(scan_str, scans_str))
        # END-FOR

        return scan_list

    def setup(self, exp_number, det_size, det_center, det_sample_distance, wave_length):
        """
        set up the calibration parameters
        :param exp_number:
        :param det_size:
        :param det_center:
        :param det_sample_distance:
        :param wave_length:
        :return:
        """
        # experiment number
        assert isinstance(exp_number, int), 'Experiment number {0} must be an integer'.format(exp_number)
        self.ui.lineEdit_ipts.setText('{0}'.format(exp_number))

        # detector size: must be given
        assert isinstance(det_size, int), 'Square detector size {0} must be an integer'.format(det_size)
        if det_size == 256:
            self.ui.comboBox_detSize.setCurrentIndex(0)
        elif det_size == 512:
            self.ui.comboBox_detSize.setCurrentIndex(1)
        else:
            self.ui.comboBox_detSize.setCurrentIndex(2)

        # set up detector center
        if det_center is not None:
            self.ui.lineEdit_infoDetCenter.setText('{0}'.format(det_center))

        # set up detector-sample distance
        if det_sample_distance is not None:
            self.ui.lineEdit_infoDetSampleDistance.setText('{0}'.format(det_sample_distance))

        # set up wave length
        if wave_length is not None:
            self.ui.lineEdit_infoWavelength.setText('{0}'.format(wave_length))

        return
