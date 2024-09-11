# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import time
import csv
from mantidqtinterfaces.HFIR_4Circle_Reduction import reduce4circleControl
from mantidqtinterfaces.HFIR_4Circle_Reduction import guiutility as gui_util
from mantidqtinterfaces.HFIR_4Circle_Reduction import fourcircle_utility as fourcircle_utility
from mantidqtinterfaces.HFIR_4Circle_Reduction import NTableWidget
from qtpy.QtWidgets import QFileDialog, QMainWindow
from mantid.kernel import Logger

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("HFIR_4Circle_Reduction").information("Using legacy ui importer")
    from mantidplot import load_ui
from qtpy.QtWidgets import QVBoxLayout


class ScanPreProcessWindow(QMainWindow):
    """
    Main window class to pre-process scans
    """

    def __init__(self, parent):
        """
        initialization
        :param parent:
        """
        super(ScanPreProcessWindow, self).__init__(parent)

        # class variables
        self._myController = None
        self._myMergePeaksThread = None
        self._rowScanDict = dict()

        # mutex and data structure that can be in contention
        self._recordLogMutex = False
        self._scansToProcess = set()
        self._mdFileDict = dict()
        self._scanNumbersProcessed = set()

        # current experiment number in processing
        self._currExpNumber = None
        self._outputDir = None

        # define UI
        ui_path = "preprocess_window.ui"
        self.ui = load_ui(__file__, ui_path, baseinstance=self)
        self._promote_widgets()

        # initialize the widgets
        self.enable_calibration_settings(False)

        # detector sizes
        self.ui.comboBox_detSize.addItem("256 x 256")
        self.ui.comboBox_detSize.addItem("512 x 512")
        self.ui.comboBox_detSize.addItem("(512 x 9) x (512 x 9)")
        self.ui.comboBox_detSize.setCurrentIndex(0)

        self.ui.checkBox_saveToDataServer.setChecked(True)

        # initialize table
        self.ui.tableView_scanProcessState.setup()
        # try to make column nicer
        self.ui.tableView_scanProcessState.resizeColumnsToContents()

        # define event handling
        self.ui.pushButton_browseOutputDir.clicked.connect(self.do_browse_output_dir)
        self.ui.pushButton_preProcessScan.clicked.connect(self.do_start_pre_process)
        self.ui.pushButton_changeSettings.clicked.connect(self.do_change_calibration_settings)
        self.ui.pushButton_fixSettings.clicked.connect(self.do_fix_calibration_settings)
        self.ui.actionExit.triggered.connect(self.do_quit)

    def _promote_widgets(self):
        tableView_scanProcessState_layout = QVBoxLayout()
        self.ui.frame_tableView_scanProcessState.setLayout(tableView_scanProcessState_layout)
        self.ui.tableView_scanProcessState = ScanPreProcessStatusTable(self)
        tableView_scanProcessState_layout.addWidget(self.ui.tableView_scanProcessState)

    @property
    def controller(self):
        """
        get access to controller
        :return:
        """
        assert self._myController is not None, "Controller cannot be None"

        return self._myController

    def do_browse_output_dir(self):
        """
        browse the output directory
        :return:
        """
        # get scan number or numbers
        try:
            exp_number = self.get_exp_number()
        except RuntimeError as run_err:
            gui_util.show_message(self, "[ERROR] {0}".format(run_err))
            return

        default_dir = os.path.join("/HFIR/HB3A/Exp{0}/shared/".format(exp_number))

        # get output directory
        output_dir = QFileDialog.getExistingDirectory(self, "Outputs for pre-processed scans", default_dir)
        if not output_dir:
            return
        if isinstance(output_dir, tuple):
            output_dir = output_dir[0]
        self.ui.lineEdit_outputDir.setText(output_dir)

    def do_change_calibration_settings(self):
        """
        enable the settings to be modifiable
        :return:
        """
        self.enable_calibration_settings(True)

    def do_fix_calibration_settings(self):
        """
        disable the settings to be modifiable
        :return:
        """
        self.enable_calibration_settings(False)

    def do_quit(self):
        """close window properly

        :return:
        """
        print("[INFO] Closing {0}".format(self.objectName()))

        if self._myMergePeaksThread is not None:
            self._myMergePeaksThread.terminate()

    def do_start_pre_process(self):
        """
        start the pre-precessing scans
        :return:
        """
        import multi_threads_helpers

        # check inputs
        assert isinstance(self._myController, reduce4circleControl.CWSCDReductionControl), (
            "Reduction controller of type {0} is not accepted. It must be a CWSCDReductionControl instance." "".format(
                self._myController.__class__.__name__
            )
        )

        # check whether it is well setup for reduction
        if self._myController is None:
            raise RuntimeError("Reduction controller has not been set up yet.  It is required for pre-processing.")

        # get all the information
        exp_number = self.get_exp_number()
        scan_list = self.get_scan_numbers()

        # set up calibration information
        self.set_calibration_to_reduction_controller(exp_number)

        # set up GUI
        self._rowScanDict = self.ui.tableView_scanProcessState.add_new_scans(scan_list)

        # form the output files
        output_dir = str(self.ui.lineEdit_outputDir.text())
        if os.path.exists(output_dir) is False:
            # create output directory and change to all accessible
            os.mkdir(output_dir)
            os.chmod(output_dir, 0o777)
        self._outputDir = output_dir

        file_list = list()
        for scan in scan_list:
            # md_file_name = os.path.join(output_dir, 'Exp{0}_Scan{1}_MD.nxs'.format(exp_number, scan))
            md_file_name = fourcircle_utility.pre_processed_file_name(exp_number, scan, output_dir)
            file_list.append(md_file_name)

        # launch the multiple threading to scans
        self._myMergePeaksThread = multi_threads_helpers.MergePeaksThread(self, exp_number, scan_list, file_list)
        self._scansToProcess = set(scan_list)
        self._scanNumbersProcessed = set()

        self._currExpNumber = exp_number
        self._myMergePeaksThread.start()

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
        scans_str = scans_str.replace(",", " ")
        scan_str_list = scans_str.split()

        for single_scan_str in scan_str_list:
            # skip empty string
            if len(single_scan_str) == 0:
                continue

            if single_scan_str.isdigit():
                scan = int(single_scan_str)
                scan_list.append(scan)
            elif single_scan_str.count("-") == 1:
                terms = single_scan_str.split("-")
                if len(terms) == 2 and terms[0].isdigit() and terms[1].isdigit():
                    start_scan = int(terms[0])
                    end_scan = int(terms[1])
                    scan_list.extend(range(start_scan, end_scan + 1))
                else:
                    raise RuntimeError("{0} in scan list {1} cannot be converted to integer list." "".format(single_scan_str, scans_str))
            else:
                raise RuntimeError("{0} in scan list {1} cannot be converted to integer." "".format(single_scan_str, scans_str))
        # END-FOR

        return scan_list

    def get_exp_number(self):
        """
        get experiment number
        :exception: RuntimeError if input is not correct
        :return:
        """
        exp_num_str = str(self.ui.lineEdit_expNumber.text())
        if len(exp_num_str) == 0:
            raise RuntimeError("Experiment number is not given")
        if exp_num_str.isdigit() is False:
            raise RuntimeError("Experiment number {0} is not a valid integer.".format(exp_num_str))

        return int(exp_num_str)

    def set_calibration_to_reduction_controller(self, exp_number):
        """set user-specified instrument calibrations to the my controller
        :param exp_number:
        :return:
        """
        # set up the experiment number if it is different
        if exp_number != self._myController.get_experiment():
            self._myController.set_exp_number(exp_number)
            self._myController.set_default_detector_sample_distance(0.3750)
            self._myController.set_default_pixel_number(256, 256)

        # set up the calibration
        # wave length
        user_wavelength_str = str(self.ui.lineEdit_infoWavelength.text()).strip()
        if len(user_wavelength_str) > 0:
            try:
                user_wavelength = float(user_wavelength_str)
                self._myController.set_user_wave_length(exp_number, user_wavelength)
            except ValueError:
                gui_util.show_message(
                    self, "[ERROR] User-specified wave length {0} cannot be converted to float." "".format(user_wavelength_str)
                )
                return
        # END-IF

        # detector center
        user_det_center_str = str(self.ui.lineEdit_infoDetCenter.text()).strip()
        user_det_center_str = user_det_center_str.replace("x", ",")
        if len(user_det_center_str) > 0:
            try:
                det_center = gui_util.parse_integer_list(user_det_center_str, 2)
            except RuntimeError as run_err:
                gui_util.show_message(self, "Unable to parse detector center {0} due to {1}" "".format(user_det_center_str, run_err))
                return
            self._myController.set_detector_center(exp_number, det_center[0], det_center[1])
        # END-IF

        # detector sample distance
        status, ret_obj = gui_util.parse_float_editors([self.ui.lineEdit_infoDetSampleDistance], allow_blank=True)

        if not status:
            error_message = ret_obj
            gui_util.show_message(self, "[ERROR] {0}".format(error_message))
            return
        user_det_sample_distance = ret_obj[0]
        if user_det_sample_distance is not None:
            self._myController.set_detector_sample_distance(exp_number, user_det_sample_distance)

        # detector size
        curr_det_size_index = self.ui.comboBox_detSize.currentIndex()
        if curr_det_size_index > 2:
            gui_util.show_message(self, "Detector {0} is not supported by now!" "".format(str(self.ui.comboBox_detSize.currentText())))
            return
        det_size = [256, 512][curr_det_size_index]
        self._myController.set_detector_geometry(det_size, det_size)

    def set_instrument_calibration(self, exp_number, det_size, det_center, det_sample_distance, wave_length):
        """set up the calibration parameters

        :param exp_number:
        :param det_size:
        :param det_center: ok with string
        :param det_sample_distance: ok with string
        :param wave_length: ok with string
        :return:
        """
        # experiment number
        assert isinstance(exp_number, int), "Experiment number {0} must be an integer".format(exp_number)
        self.ui.lineEdit_expNumber.setText("{0}".format(exp_number))

        # detector size: must be given
        assert isinstance(det_size, int), "Square detector size {0} must be an integer".format(det_size)
        if det_size == 256:
            self.ui.comboBox_detSize.setCurrentIndex(0)
        elif det_size == 512:
            self.ui.comboBox_detSize.setCurrentIndex(1)
        else:
            self.ui.comboBox_detSize.setCurrentIndex(2)

        # set up detector center
        if det_center is not None:
            self.ui.lineEdit_infoDetCenter.setText("{0}".format(det_center))

        # set up detector-sample distance
        if det_sample_distance is not None:
            self.ui.lineEdit_infoDetSampleDistance.setText("{0}".format(det_sample_distance))

        # set up wave length
        if wave_length is not None:
            self.ui.lineEdit_infoWavelength.setText("{0}".format(wave_length))

        # set up the default output directory and create it if it does not exist
        if self.ui.checkBox_saveToDataServer.isChecked():
            default_output_dir = os.path.join("/HFIR/HB3A/exp{0}".format(exp_number), "Shared/MergedScans")
            if os.path.exists(default_output_dir) is False:
                try:
                    os.mkdir(default_output_dir)
                    self.ui.lineEdit_outputDir.setText(default_output_dir)
                except OSError:
                    self.ui.lineEdit_outputDir.setText("/tmp")
                    # default_output_dir = '/tmp'
            # END-IF
        # END-IF

    def setup(self, controller):
        """
        setup the 4-circle reduction controller
        :param controller:
        :return:
        """
        assert isinstance(controller, reduce4circleControl.CWSCDReductionControl), (
            "Reduction controller must be an instance of reduce4circleControl.CWSCDReductionControl but not a {0}." "".format(
                controller.__class__.__name__
            )
        )

        self._myController = controller

    def update_file_name(self, scan_number, file_name):
        """Update merged file name
        :param scan_number:
        :param file_name:
        :return:
        """
        # file is written, then check whether it is time to write a record file
        counter = 0
        while self._recordLogMutex:
            # waiting for the mutex to be released
            time.sleep(0.1)
            counter += 1
            if counter > 600:  # 60 seconds... too long
                raise RuntimeError("It is too long to wait for mutex released.  There must be a bug!")
        # END-WHILE

        # update processed scan numbers
        self._recordLogMutex = True
        self._scanNumbersProcessed.add(scan_number)
        self._mdFileDict[scan_number] = file_name
        self._recordLogMutex = False

        # check whether it is time to write all the scans to file
        if len(self._scansToProcess) == len(self._scanNumbersProcessed):
            self.update_record_file(self._currExpNumber, check_duplicates=False, scan_list=self._scanNumbersProcessed)
            if self._scansToProcess != self._scanNumbersProcessed:
                raise RuntimeWarning(
                    "Scans to process {0} is not same as scans processed {1}." "".format(self._scansToProcess, self._scanNumbersProcessed)
                )
        # END-IF

        row_number = self._rowScanDict[scan_number]
        self.ui.tableView_scanProcessState.set_file_name(row_number, file_name)
        self.ui.tableView_scanProcessState.resizeColumnsToContents()

    def update_merge_value(self, scan_number, message):
        """update merged signal
        :param scan_number:
        :param message:
        :return:
        """
        row_number = self._rowScanDict[scan_number]
        self.ui.tableView_scanProcessState.set_status(row_number, message)
        self.ui.tableView_scanProcessState.resizeColumnsToContents()

    def update_record_file(self, exp_number, check_duplicates, scan_list):
        """update the record file
        it is an option to append file or check and remove duplication.
        duplication can be removed in the record file loading method by checking the time stamp
        :param exp_number:
        :param check_duplicates:
        :param scan_list:
        :return:
        """
        # check inputs
        assert len(self._scanNumbersProcessed) > 0, "Processed scan number set cannot be empty!"

        # get calibration information
        det_sample_distance = self._myController.get_calibrated_det_sample_distance(exp_number=exp_number)
        det_center_x, det_center_y = self._myController.get_calibrated_det_center(exp_number)
        user_wave_length = self._myController.get_calibrated_wave_length(exp_number)

        record_file_name = fourcircle_utility.pre_processed_record_file(exp_number, self._outputDir)
        if os.path.exists(record_file_name):
            write_header = False
        else:
            write_header = True

        with open(record_file_name, "a") as csvfile:
            fieldnames = fourcircle_utility.pre_processed_record_header()
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

            # write header for the first time
            if write_header:
                writer.writeheader()

            for scan_number in scan_list:
                record = fourcircle_utility.pre_processed_record_make(
                    scan_number, self._mdFileDict[scan_number], det_sample_distance, det_center_x, det_center_y, user_wave_length
                )
                writer.writerow(record)
            # END-FOR

        # END-WITH


class ScanPreProcessStatusTable(NTableWidget.NTableWidget):
    """
    Extended table widget for scans to process
    """

    Table_Setup = [("Scan", "int"), ("Status", "str"), ("File", "str"), ("Note", "str")]

    def __init__(self, parent):
        """
        Initialization
        :param parent::
        :return:
        """
        super(ScanPreProcessStatusTable, self).__init__(parent)

        # column index of k-index
        self._iColScanNumber = None
        self._iColStatus = None
        self._iColFile = None
        self._iColNote = None

        # a quick-reference list
        self._scanRowDict = dict()

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_setup(self.Table_Setup)

        # set columns' width
        self.setColumnWidth(0, 35)
        self.setColumnWidth(1, 60)
        self.setColumnWidth(2, 90)
        self.setColumnWidth(3, 120)

        # set the column index
        self._iColScanNumber = self.TableSetup.index(("Scan", "int"))
        self._iColStatus = self.TableSetup.index(("Status", "str"))
        self._iColFile = self.TableSetup.index(("File", "str"))
        self._iColNote = self.TableSetup.index(("Note", "str"))

    def add_new_scans(self, scan_numbers):
        """add scans to the table

        :param scan_numbers:
        :return:
        """
        # check input
        assert isinstance(scan_numbers, list), "Scan numbers {0} must be a list but not a {1}." "".format(scan_numbers, type(scan_numbers))

        # sort
        scan_numbers.sort()
        part_dict = dict()

        # append to table
        for scan_number in scan_numbers:
            # skip the scan number that has been added to table
            if scan_number in self._scanRowDict:
                continue

            # append scan
            self.append_row([scan_number, "", "", "Queued"])
            num_rows = self.rowCount()
            self._scanRowDict[scan_number] = num_rows - 1
            part_dict[scan_number] = num_rows - 1
        # END-FOR

        return part_dict

    def set_status(self, row_number, status):
        """set the status of the scan

        :param row_number:
        :param status:
        :return:
        """
        # check
        self.update_cell_value(row_number, self._iColStatus, status)
