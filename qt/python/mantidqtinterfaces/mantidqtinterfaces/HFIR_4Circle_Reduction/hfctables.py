# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=W0403,C0103,R0901,R0904,R0913,C0302
# ruff: noqa: E741  # Ambiguous variable name
import numpy
import sys
from mantidqtinterfaces.HFIR_4Circle_Reduction import fourcircle_utility
from mantidqtinterfaces.HFIR_4Circle_Reduction import guiutility
from qtpy import QtCore
import math
import mantidqtinterfaces.HFIR_4Circle_Reduction.NTableWidget as tableBase
import os


class KShiftTableWidget(tableBase.NTableWidget):
    """Extended table widget for show the K-shift vectors set to the output Fullprof file"""

    # Table set up
    TableSetup = [("Index", "int"), ("Kx", "float"), ("Ky", "float"), ("Kz", "float"), ("Selected", "checkbox")]

    def __init__(self, parent):
        """
        Initialization
        :param parent::
        :return:
        """
        tableBase.NTableWidget.__init__(self, parent)

        # column index of k-index
        self._iColKIndex = None

        return

    def add_k_vector(self, k_index, kx, ky, kz):
        """
        Add K-vector to the table
        :param k_index:
        :param kx:
        :param ky:
        :param kz:
        :return:
        """
        # check
        assert isinstance(k_index, int) and k_index >= 0
        assert isinstance(kx, float) and isinstance(ky, float) and isinstance(kz, float)

        new_row = [k_index, kx, ky, kz, False]

        self.append_row(new_row)

        return

    def delete_k_vector(self, k_index):
        """Delete a row
        :param k_index:
        :return:
        """
        # check
        assert isinstance(k_index, int)

        # find and delete
        found_and_delete = False
        for i_row in range(self.rowCount()):
            k_index_i = self.get_cell_value(i_row, self._iColKIndex)
            if k_index == k_index_i:
                self.delete_rows([i_row])
                found_and_delete = True

        return found_and_delete

    def setup(self):
        """Set up the table
        :return:
        """
        self.init_setup(self.TableSetup)

        self._iColKIndex = self.TableSetup.index(("Index", "int"))

        return


class MatrixTable(tableBase.NTableWidget):
    """ """

    def __init__(self, parent):
        """

        :param parent:
        """
        super(MatrixTable, self).__init__(parent)

        return

    def setup(self, num_rows, num_cols):
        """
        set up a table for matrix
        :param num_rows:
        :param num_cols:
        :return:
        """
        # check inputs
        assert isinstance(num_rows, int) and num_rows > 0, "Number of rows larger than 0."
        assert isinstance(num_cols, int) and num_cols > 0, "Number of columns larger than 0."

        self.init_size(4, 4)

        # think of reset
        if self.rowCount() != num_rows or self.columnCount() != num_cols:
            errmsg = (
                "Number of rows to set {0} is not equal to current number of rows {1} or "
                "Number of columns to set {2} is not equal to current number of columns {3}"
                "".format(self.rowCount(), num_rows, self.columnCount(), num_cols)
            )
            raise RuntimeError(errmsg)

        return

    def set_matrix(self, matrix):
        """

        :param matrix:
        :return:
        """
        # check inputs
        assert isinstance(matrix, numpy.ndarray) and matrix.shape == (4, 4), "Matrix {0} must be ndarray with {1}." "".format(
            matrix, matrix.shape
        )
        for i in range(matrix.shape[0]):
            for j in range(matrix.shape[1]):
                self.set_value_cell(i, j, matrix[i, j])

        return


class PeaksIntegrationSpreadSheet(tableBase.NTableWidget):
    """
    Detailed peaks integration information table. Each row is for a peak measured in a scan containing multiple Pts.
    It can be converted to a csv file for user to check the integration details.
    Note: all the intensities shown below are corrected by by Lorentzian and absorption if either of them is
          calculated and applied.
    """

    Table_Setup = [
        ("Scan", "int"),
        ("HKL (S)", "str"),
        ("HKL (C)", "str"),
        ("Mask", "str"),
        ("Intensity (R)", "float"),
        ("Error (R)", "float"),
        ("Intensity 2", "float"),
        ("Error (2)", "float"),
        ("Intensity (G)", "float"),
        ("Error (G)", "float"),
        ("Lorentz", "float"),
        ("Bkgd (E)", "float"),
        ("Bkgd (G)", "float"),
        ("Sigma", "float"),
        ("A", "float"),
        ("Motor Name", "str"),
        ("Motor Step", "float"),
        ("K-shift", "str"),
        ("Absorption", "float"),
    ]

    def __init__(self, parent):
        """
        initialization
        :param parent:
        """
        super(PeaksIntegrationSpreadSheet, self).__init__(parent)

        # define column indexes
        self._colIndexScan = None
        self._colIndexSpiceHKL = None
        self._colIndexMantidHKL = None
        self._colIndexMask = None
        self._colIndexRawIntensity = None
        self._colIndexRawError = None
        self._colIndexIntensity2 = None
        self._colIndexError2 = None
        self._colIndexIntensity3 = None
        self._colIndexError3 = None
        self._colIndexBkgdE = None
        self._colIndexBkgdG = None
        self._colIndexMotorName = None
        self._colIndexMotorStep = None
        self._colIndexAbsorption = None
        self._colIndexKShift = None
        self._colIndexLorentz = None
        self._colIndexSigma = None
        self._colIndexA = None

        return

    def add_scan_information(
        self,
        scan_number,
        s_hkl,
        m_hkl,
        mask,
        raw_intensity,
        raw_error,
        intensity2,
        error2,
        intensity3,
        error3,
        lorentz,
        bkgd_e,
        bkgd_g,
        gauss_s,
        gauss_a,
        motor_name,
        motor_step,
        k_shift,
        absorption,
    ):
        """
        add the detailed integrating information to table
        :param scan_number:
        :param s_hkl:
        :param m_hkl:
        :param mask:
        :param raw_intensity:
        :param raw_error:
        :param intensity2:
        :param error2:
        :param intensity3:
        :param error3:
        :param lorentz:
        :param bkgd_e:
        :param bkgd_g:
        :param gauss_s:
        :param gauss_a:
        :param motor_name:
        :param motor_step:
        :param k_shift:
        :param absorption:
        :return:
        """
        # append an empty row
        row_list = [None] * len(self.Table_Setup)
        status, msg = self.append_row(row_list)
        if not status:
            print("[ERROR] Unable to append a new row due to {0}.".format(msg))
        else:
            row_list[0] = 123
            row_list[1] = ""
            row_list[2] = ""
        last_row_number = self.rowCount() - 1

        # set value
        self.update_cell_value(last_row_number, self._colIndexScan, scan_number)
        self.update_cell_value(last_row_number, self._colIndexSpiceHKL, s_hkl)
        self.update_cell_value(last_row_number, self._colIndexMantidHKL, m_hkl)
        self.update_cell_value(last_row_number, self._colIndexMask, mask)
        self.update_cell_value(last_row_number, self._colIndexRawIntensity, raw_intensity)
        self.update_cell_value(last_row_number, self._colIndexRawError, raw_error)
        self.update_cell_value(last_row_number, self._colIndexIntensity2, intensity2)
        self.update_cell_value(last_row_number, self._colIndexIntensity3, intensity3)
        self.update_cell_value(last_row_number, self._colIndexError2, error2)
        self.update_cell_value(last_row_number, self._colIndexError3, error3)
        self.update_cell_value(last_row_number, self._colIndexLorentz, lorentz)
        self.update_cell_value(last_row_number, self._colIndexBkgdE, bkgd_e)
        self.update_cell_value(last_row_number, self._colIndexBkgdG, bkgd_g)
        self.update_cell_value(last_row_number, self._colIndexSigma, gauss_s)
        self.update_cell_value(last_row_number, self._colIndexA, gauss_a)
        self.update_cell_value(last_row_number, self._colIndexKShift, k_shift)
        self.update_cell_value(last_row_number, self._colIndexAbsorption, absorption)
        self.update_cell_value(last_row_number, self._colIndexMotorName, motor_name)
        self.update_cell_value(last_row_number, self._colIndexMotorStep, motor_step)

        return

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_setup(self.Table_Setup)

        # get column names
        col_name_list = self._myColumnNameList

        self._colIndexScan = col_name_list.index("Scan")
        self._colIndexSpiceHKL = self.Table_Setup.index(("HKL (S)", "str"))
        self._colIndexMantidHKL = self.Table_Setup.index(("HKL (C)", "str"))
        self._colIndexMask = self.Table_Setup.index(("Mask", "str"))
        self._colIndexRawIntensity = self.Table_Setup.index(("Intensity (R)", "float"))
        self._colIndexRawError = self.Table_Setup.index(("Error (R)", "float"))
        self._colIndexIntensity2 = self.Table_Setup.index(("Intensity 2", "float"))
        self._colIndexError2 = self.Table_Setup.index(("Error (2)", "float"))
        self._colIndexIntensity3 = self.Table_Setup.index(("Intensity (G)", "float"))
        self._colIndexError3 = self.Table_Setup.index(("Error (G)", "float"))
        self._colIndexLorentz = self.Table_Setup.index(("Lorentz", "float"))
        self._colIndexBkgdE = self.Table_Setup.index(("Bkgd (E)", "float"))
        self._colIndexBkgdG = self.Table_Setup.index(("Bkgd (G)", "float"))
        self._colIndexMotorName = self.Table_Setup.index(("Motor Name", "str"))
        self._colIndexMotorStep = self.Table_Setup.index(("Motor Step", "float"))
        self._colIndexKShift = self.Table_Setup.index(("K-shift", "str"))
        self._colIndexAbsorption = self.Table_Setup.index(("Absorption", "float"))
        self._colIndexSigma = self.Table_Setup.index(("Sigma", "float"))
        self._colIndexA = self.Table_Setup.index(("A", "float"))

        return


class PeakIntegrationTableWidget(tableBase.NTableWidget):
    """
    Extended table widget for studying peak integration of a single scan on various Pts.
    """

    Table_Setup = [("Pt", "int"), ("Raw", "float"), ("Masked", "float"), ("Selected", "checkbox")]

    def __init__(self, parent):
        """
        :param parent:
        """
        tableBase.NTableWidget.__init__(self, parent)

        self._expNumber = -1
        self._scanNumber = -1

        self._rawIntensityColIndex = None
        self._maskedIntensityColIndex = None

        return

    def append_pt(self, pt_number, raw_signal, masked_signal):
        """
        Append a new row for the signal/intensity of a Pt.
        :param pt_number:
        :param raw_signal:
        :param masked_signal:
        :return: 2-tuple as boolean and error message
        """
        # check requirements
        assert isinstance(pt_number, int), "Error 920X"
        assert isinstance(raw_signal, int) or isinstance(raw_signal, float) or raw_signal is None, "Error 920A"
        assert isinstance(masked_signal, float) or isinstance(masked_signal, int) or masked_signal is None, "Error 920B"

        # form a new row and append
        status, msg = self.append_row([pt_number, raw_signal, masked_signal, False])
        if status is False:
            msg = "Unable to append row to peak integration table due to %s" % msg

        return status, msg

    def get_exp_info(self):
        """
        Get experiment information of the data written in the table now
        :return:
        """
        return self._expNumber, self._scanNumber

    def sum_raw_intensity(self):
        """
        sum raw intensities of all Pts.
        :return:
        """
        num_rows = self.rowCount()

        count_sum = 0.0
        for i_row in range(num_rows):
            pt_count = self.get_cell_value(i_row, self._rawIntensityColIndex)
            count_sum += pt_count
        # END-FOR

        return count_sum

    def sum_masked_intensity(self):
        """
        sum masked intensities of all Pts.
        :return:
        """
        num_rows = self.rowCount()

        count_sum = 0.0
        for i_row in range(num_rows):
            pt_count = self.get_cell_value(i_row, self._maskedIntensityColIndex)
            count_sum += pt_count
        # END-FOR

        return count_sum

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_setup(self.Table_Setup)

        self._statusColName = "Selected"

        # set columns' width
        self.setColumnWidth(0, 35)
        self.setColumnWidth(1, 60)
        self.setColumnWidth(2, 90)
        self.setColumnWidth(3, 90)

        # Set others...
        self._rawIntensityColIndex = self._myColumnNameList.index("Raw")
        self._maskedIntensityColIndex = self._myColumnNameList.index("Masked")

        return

    def set_exp_info(self, exp_no, scan_no):
        """
        Set experiment number and scan number to this table for the data that are written to it
        :param exp_no:
        :param scan_no:
        :return:
        """
        assert isinstance(exp_no, int)
        assert isinstance(scan_no, int)

        self._expNumber = exp_no
        self._scanNumber = scan_no

        return

    def set_integrated_values(self, pt_vec, intensity_vec):
        """

        :param pt_vec:
        :param intensity_vec:
        :return:
        """
        # check
        assert len(pt_vec) == len(intensity_vec)

        # common value
        hkl = "0, 0, 0"
        q = "0, 0, 0"
        signal = -1

        num_rows = len(pt_vec)
        for i_row in range(num_rows):
            pt = int(pt_vec[i_row])
            intensity = intensity_vec[i_row]
            item_list = [pt, hkl, q, signal, intensity]
            self.append_row(item_list)

        return

    def set_q(self, row_index, vec_q):
        """
        Set Q to rows
        :param row_index:
        :param vec_q: a list or array with size 3
        :return:
        """
        assert len(vec_q) == 3

        # locate
        index_q_x = self.Table_Setup.index(("Q_x", "float"))
        for j in range(3):
            col_index = j + index_q_x
            self.update_cell_value(row_index, col_index, vec_q[j])
        # END-FOR (j)

        return

    def simple_integrate_peak(self, background):
        """
        Integrate peak in a simple way. Refer to documentation of this interface
        :param background:
        :return:
        """
        # Check
        assert self.rowCount() > 0, "Table is empty!"
        assert isinstance(background, float) and background >= 0.0

        # Integrate
        sum_intensity = 0.0
        for i_row in range(self.rowCount()):
            intensity_i = self.get_cell_value(i_row, self._maskedIntensityColIndex)
            sum_intensity += intensity_i - background

        return sum_intensity


class UBMatrixTable(tableBase.NTableWidget):
    """
    Extended table for UB matrix
    """

    def __init__(self, parent):
        """

        :param parent:
        :return:
        """
        tableBase.NTableWidget.__init__(self, parent)

        # Matrix
        self._matrix = numpy.ndarray((3, 3), float)
        for i in range(3):
            for j in range(3):
                self._matrix[i][j] = 0.0

        return

    def _set_to_table(self):
        """
        Set values in holder '_matrix' to TableWidget
        :return:
        """
        for i_row in range(3):
            for j_col in range(3):
                self.update_cell_value(i_row, j_col, self._matrix[i_row][j_col])

        return

    def get_matrix(self):
        """
        Get the copy of the matrix
        Guarantees: return a 3 x 3 ndarray
        :return:
        """
        return self._matrix.copy()

    def get_matrix_str(self):
        """
        Get the 3 x 3 matrix and format it to a 9 float strings from (0, 0) to (2, 2)
        :return: a string
        """
        matrix_string = ""
        for i in range(3):
            for j in range(3):
                matrix_string += "%.10f" % self._matrix[i][j]
                if not (i == 2 and j == 2):
                    matrix_string += ","
            # END-FOR (j)
        # END-FOR (i)

        return matrix_string

    def set_from_list(self, element_array):
        """
        Set table value including holder and QTable from a 1D numpy array
        :param element_array:
        :return:
        """
        # Check
        assert isinstance(element_array, list)
        assert len(element_array) == 9

        # Set value
        i_array = 0
        for i in range(3):
            for j in range(3):
                self._matrix[i][j] = element_array[i_array]
                i_array += 1

        # Set to table
        self._set_to_table()

        return

    def set_from_matrix(self, matrix):
        """
        Set value to both holder and QTable from a numpy 3 x 3 matrix
        :param matrix:
        :return:
        """
        # Check
        assert isinstance(matrix, numpy.ndarray), "Input matrix must be numpy.ndarray, but not %s" % str(type(matrix))
        assert matrix.shape == (3, 3)

        for i in range(3):
            for j in range(3):
                self._matrix[i][j] = matrix[i][j]

        self._set_to_table()

        return

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_size(3, 3)

        for i in range(3):
            for j in range(3):
                self.set_value_cell(i, j)

        self._set_to_table()

        return


class ProcessTableWidget(tableBase.NTableWidget):
    """
    Extended table for peaks used to process scans including peak integration, scan merging and etc.
    """

    TableSetup = [
        ("Scan", "int"),
        ("Status", "str"),
        ("Intensity", "float"),
        ("F2", "float"),  # Lorenzian corrected
        ("F2 Error", "float"),
        ("Integrate", "str"),  # integration type, Gaussian fit / simple summation
        ("Mask", "str"),  # '' for no mask
        ("HKL", "str"),
        ("Motor", "str"),
        ("Motor Step", "str"),
        ("Wavelength", "float"),
        ("K-Index", "int"),
        ("Select", "checkbox"),
    ]

    def __init__(self, parent):
        """
        Initialization
        :param parent:
        :return:
        """
        tableBase.NTableWidget.__init__(self, parent)

        # some commonly used column index
        self._colIndexScan = None
        self._colIndexIntensity = None
        self._colIndexCorrInt = None
        self._colIndexErrorBar = None
        self._colIndexMask = None
        self._colIndexIntType = None
        self._colIndexHKL = None
        self._colIndexStatus = None
        self._colIndexPeak = None
        # self._colIndexIndexFrom = None
        self._colIndexMotor = None
        self._colIndexMotorStep = None
        self._colIndexWaveLength = None
        self._colIndexKIndex = None

        # cache dictionaries
        self._workspaceCacheDict = dict()

        return

    @staticmethod
    def _generate_empty_row(scan_number, status="In-Queue", ws_name=""):
        """Generate a list for empty row with scan number
        :param scan_number:
        :param status:
        :param ws_name
        :return:
        """
        # check inputs
        assert isinstance(scan_number, int)
        assert isinstance(status, str)

        intensity = None
        corr_int = None
        error = None
        mask = ""
        integrate_type = "sum"
        motor_name = None
        motor_step = None
        wave_length = 0
        hkl = ""

        new_row = [
            scan_number,
            status,
            intensity,
            corr_int,
            error,
            integrate_type,
            mask,  # peak_center,
            hkl,
            motor_name,
            motor_step,
            wave_length,
            0,
            False,
        ]

        return new_row

    def add_new_merged_data(self, exp_number, scan_number, ws_name):
        """
        Append a new row with merged data
        :param exp_number:
        :param scan_number:
        :param ws_name:
        :return:
        """
        # check
        assert isinstance(exp_number, int), "Experiment number {0} must be an integer but not a {1}." "".format(
            exp_number, type(exp_number)
        )
        assert isinstance(scan_number, int), "Scan number {0} must be an integer but not a {1}." "".format(scan_number, type(scan_number))
        assert isinstance(ws_name, str), "Workspace name {0} must be a string but not a {1}." "".format(ws_name, type(ws_name))

        # construct a row
        new_row = self._generate_empty_row(scan_number, ws_name=ws_name)
        self.append_row(new_row)

        return

    def add_single_measure_scan(self, scan_number, intensity, roi_name):
        """
        add a single measurement peak scan
        :param scan_number:
        :param intensity:
        :param roi_name:
        :return:
        """
        # construct a new row
        new_row = self._generate_empty_row(scan_number, ws_name="single-pt")
        self.append_row(new_row)

        # set peak intensity
        row_number = self.rowCount() - 1
        self.set_peak_intensity(
            row_number=row_number,
            peak_intensity=intensity,
            corrected_intensity=intensity,
            standard_error=math.sqrt(abs(intensity)),
            integrate_method="single-pt",
        )

        # ROI: use the unused workspace column for this information
        self.update_cell_value(row_number, self._colIndexMask, roi_name)

        return

    def append_scans(self, scans, allow_duplicate_scans):
        """Append rows for merge in future
        :param scans:
        :param allow_duplicate_scans: does not allow duplicate scan
        :return:
        """
        # Check
        assert isinstance(scans, list)

        if allow_duplicate_scans is False:
            scan_list = self.get_scan_list(output_row_number=False)
        else:
            scan_list = list()

        # set value as default
        # Append rows
        for scan in scans:
            # add a new row for the scan
            if allow_duplicate_scans is False and scan in scan_list:
                # skip is duplicate scan is not allowed
                continue

            # add scans to new row
            new_row = self._generate_empty_row(scan_number=scan)
            status, err = self.append_row(new_row)
            if status is False:
                raise RuntimeError(err)

            # set unit
        # END-FOR

        return

    def get_integration_type(self, row_index):
        """
        get the peak integration type
        :return:
        """
        if self.rowCount() == 0:
            raise RuntimeError("Empty table!")

        integrate_type = self.get_cell_value(row_index, self._colIndexIntType)

        return integrate_type

    def get_row_by_scan(self, scan_number):
        """
        get the row number for a gien scan
        :param scan_number:
        :return:
        """
        assert (
            isinstance(scan_number, int) and scan_number >= 0
        ), f"Scan number {scan_number} (type {type(scan_number)}) is invalid.  It must be a positive integer."
        num_rows = self.rowCount()
        ret_row_number = None
        for i_row in range(num_rows):
            tmp_scan_no = self.get_cell_value(i_row, self._colIndexScan)
            if scan_number == tmp_scan_no:
                ret_row_number = i_row
                break
        # END-FOR

        if ret_row_number is None:
            raise RuntimeError("Scan number %d does not exist in merge-scan-table." % scan_number)

        return ret_row_number

    def get_rows_by_state(self, target_state):
        """Get the rows' indexes by status' value (state)
        Requirements: target_state is a string
        Guarantees: a list of integers as row indexes are returned for all rows with state as target_state
        :param target_state:
        :return:
        """
        # Check
        assert isinstance(target_state, str), "State {0} must be a string but not a {1}." "".format(target_state, type(target_state))

        # Loop around to check
        return_list = list()
        num_rows = self.rowCount()
        for i_row in range(num_rows):
            status_i = self.get_cell_value(i_row, self._colIndexStatus)
            if status_i == target_state:
                return_list.append(i_row)
        # END-FOR (i_row)

        return return_list

    def get_hkl(self, row_index):
        """
        Get peak index, HKL or a row
        :param row_index: row index (aka number)
        :return: 3-float-tuple or None (not defined)
        """
        # check input's validity
        assert isinstance(row_index, int) and row_index >= 0, "Row index %s of type %s is not acceptable." "" % (
            str(row_index),
            type(row_index),
        )

        # retrieve value of HKL as string and then split them into floats
        hkl_str = self.get_cell_value(row_index, self._colIndexHKL)
        hkl_str = hkl_str.strip()
        if len(hkl_str) == 0:
            return None

        hkl_str_list = hkl_str.split(",")
        try:
            peak_index_h = float(hkl_str_list[0])
            peak_index_k = float(hkl_str_list[1])
            peak_index_l = float(hkl_str_list[2])
        except IndexError:
            raise RuntimeError("Row %d' HKL value %s is not value." % (row_index, hkl_str))
        except ValueError:
            raise RuntimeError("Row %d' HKL value %s is not value." % (row_index, hkl_str))

        return peak_index_h, peak_index_k, peak_index_l

    def get_mask(self, row_index):
        """
        get the mask/ROI name that this integration is based on
        :param row_index:
        :return:
        """
        return self.get_cell_value(row_index, self._colIndexMask)

    def get_merged_status(self, row_number):
        """Get the status whether it is merged
        :param row_number:
        :return: boolean
        """
        # check
        assert isinstance(row_number, int)
        assert 0 <= row_number < self.rowCount()

        # get value
        merge_status_col_index = self._myColumnNameList.index("Status")
        status_str = self.get_cell_value(row_number, merge_status_col_index)

        if status_str.lower() == "done":
            return True

        return False

    def get_merged_ws_name(self, i_row):
        """
        Get merged workspace name
        :param i_row:
        :return:
        """
        #  return self.get_cell_value(i_row, self._colIndexWorkspace)
        return self._workspaceCacheDict[i_row]

    def get_roi_name(self, row_index):
        """
        get ROI name if it is a single-pt scan
        :except RuntimeError: if it is not!
        :param row_index:
        :return:
        """
        integral_type = self.get_integration_type(row_index)
        if integral_type != "single-pt":
            raise RuntimeError("Non-single-pt is not applied to get roi name")

        roi_name = self.get_cell_value(row_index, self._colIndexMask)

        return roi_name

    def get_scan_list(self, output_row_number=True):
        """
        Get all scans that are already listed in the table.
        :param output_row_number:
        :return: list of 2-tuple or integer according to value of output_row_number
        """
        scan_list = list()
        num_rows = self.rowCount()

        for i_row in range(num_rows):
            scan_num = self.get_cell_value(i_row, self._colIndexScan)
            if output_row_number:
                scan_list.append((scan_num, i_row))
            else:
                scan_list.append(scan_num)
        # END-FOR (i_row)

        return scan_list

    def get_selected_scans(self):
        """Get list of selected scans to merge from table
        :return: list of 2-tuples (scan number, row number)
        """
        scan_list = list()
        num_rows = self.rowCount()
        col_select_index = self._myColumnNameList.index("Select")

        for i_row in range(num_rows):
            if self.get_cell_value(i_row, col_select_index) is True:
                scan_num = self.get_cell_value(i_row, self._colIndexScan)
                scan_list.append((scan_num, i_row))

        return scan_list

    def get_scan_number(self, row_number):
        """Get scan number of a row
        Guarantees: get scan number of a row
        :param row_number:
        :return:
        """
        return self.get_cell_value(row_number, self._colIndexScan)

    def select_all_nuclear_peaks(self):
        """
        select all nuclear peaks, i.e., set the flag on on 'select' for all rows if their HKL indicates that
        they are nuclear peaks
        :return: string as error message
        """
        num_rows = self.rowCount()
        error_message = ""

        for row_index in range(num_rows):
            # get the reading of HKL
            try:
                hkl_tuple = self.get_hkl(row_index)
                if hkl_tuple is None:
                    error_message += "Row %d has no HKL showed." % row_index
                    continue
                if fourcircle_utility.is_peak_nuclear(hkl_tuple[0], hkl_tuple[1], hkl_tuple[2]):
                    self.select_row(row_index)
            except RuntimeError as error:
                error_message += "Unable to parse HKL of line %d due to %s." % (row_index, str(error))
        # END-FOR

        return error_message

    def set_hkl(self, row_number, hkl, hkl_source=None):
        """Set Miller index HKL to a row
        :param row_number: row number
        :param hkl:
        :param hkl_source:
        :return:
        """
        # check
        assert isinstance(row_number, int) and 0 <= row_number < self.rowCount(), "Row number %s is out of range." % str(row_number)
        assert len(hkl) == 3, "HKL must be a sequence with 3 items but not %s." % len(hkl)

        # update the cell
        hkl_str = "%.3f, %.3f, %.3f" % (hkl[0], hkl[1], hkl[2])
        self.update_cell_value(row_number, self._colIndexHKL, hkl_str)

        return

    def set_k_shift_index(self, row_number, k_index):
        """Set k-shift index to a row
        :param row_number:
        :param k_index:
        :return:
        """
        assert isinstance(k_index, int)

        self.update_cell_value(row_number, self._colIndexKIndex, k_index)

        return

    def set_motor_info(self, row_number, motor_move_tup):
        """
        Set the motor step information to the 'Motor' cell
        :param row_number:
        :param motor_move_tup:
        :return:
        """
        # check
        assert isinstance(row_number, int) and 0 <= row_number < self.rowCount(), "Input row number is out of range."
        assert len(motor_move_tup) == 3

        # get motor information and construct the string
        motor_name = motor_move_tup[0]
        motor_move = "%.3f (%.2E)" % (motor_move_tup[1], motor_move_tup[2])

        # set motor step information string to the table cell.
        self.update_cell_value(row_number, self._colIndexMotor, motor_name)
        self.update_cell_value(row_number, self._colIndexMotorStep, motor_move)

        return

    def set_peak_centre(self, row_number, peak_centre):
        """
        set peak centre value
        :param row_number:
        :param peak_centre:
        :return:
        """
        # check input's validity
        assert isinstance(row_number, int) and 0 <= row_number < self.rowCount(), (
            "Row number %s is not supported or out of boundary." % str(row_number)
        )
        assert isinstance(peak_centre, str) or len(peak_centre) == 3, "Peak centre %s must be a string or a container with size 3." % str(
            peak_centre
        )

        # set value of peak center
        if isinstance(peak_centre, str):
            # string no need to change
            value_to_set = peak_centre
        else:
            # construct the value
            value_to_set = "%.3f, %.3f, %.3f" % (peak_centre[0], peak_centre[1], peak_centre[2])

        self.update_cell_value(row_number, self._colIndexPeak, value_to_set)

        return

    def set_peak_intensity(self, row_number, peak_intensity, corrected_intensity, standard_error, integrate_method):
        """
        Set peak intensity to a row in the table
        Guarantees: peak intensity is set
        :param row_number:
        :param peak_intensity:
        :param corrected_intensity:
        :param standard_error:
        :param integrate_method: must be '', simple or gaussian for simple counts summation or Gaussian fit, respectively
        :return:
        """
        # check requirements
        assert isinstance(peak_intensity, float), "Peak intensity must be a float."
        assert isinstance(integrate_method, str), "Integrated method {0} must be a string but not {1}." "".format(
            integrate_method, type(integrate_method)
        )
        if integrate_method not in ["", "simple", "mixed", "gaussian", "single-pt"]:
            raise RuntimeError(
                'Peak integration {0} not in list. Method must be in ["" (Not defined), "simple"' ', "gaussian"]'.format(integrate_method)
            )

        self.update_cell_value(row_number, self._colIndexIntensity, peak_intensity)
        self.update_cell_value(row_number, self._colIndexIntType, integrate_method)
        self.update_cell_value(row_number, self._colIndexCorrInt, corrected_intensity)
        self.update_cell_value(row_number, self._colIndexErrorBar, standard_error)

        return

    def set_status(self, row_number, status):
        """
        Set the status for merging scan to QTable
        :param row_number: scan number
        :param status:
        :return:
        """
        # Check
        assert isinstance(status, str), "Status (%s) must be a string, but not %s." % (str(status), type(status))

        return self.update_cell_value(row_number, self._colIndexStatus, status)

    def set_wave_length(self, row_number, wave_length):
        """Set wave length to a row
        :param row_number:
        :param wave_length:
        :return:
        """
        # check
        assert isinstance(row_number, int) and 0 <= row_number < self.rowCount(), "Input row number is out of range."
        assert isinstance(wave_length, float) and wave_length >= 0.0

        # set
        col_index = self.TableSetup.index(("Wavelength", "float"))
        self.update_cell_value(row_number, col_index, wave_length)

        return

    def set_ws_name(self, row_number, merged_md_name):
        """
        Set the output workspace and workspace group's names to QTable
        :param row_number:
        :param merged_md_name:
        :return:
        """
        # Check
        assert isinstance(merged_md_name, str), "Merged MDWorkspace name must be a string."

        #  self.update_cell_value(row_number, self._colIndexWorkspace, merged_md_name)

        self._workspaceCacheDict[row_number] = merged_md_name

        return

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_setup(self.TableSetup)
        self._statusColName = "Select"

        # set up column index
        self._colIndexScan = ProcessTableWidget.TableSetup.index(("Scan", "int"))
        self._colIndexIntensity = self.TableSetup.index(("Intensity", "float"))
        self._colIndexCorrInt = self.TableSetup.index(("F2", "float"))
        self._colIndexErrorBar = self.TableSetup.index(("F2 Error", "float"))
        self._colIndexMask = self.TableSetup.index(("Mask", "str"))
        self._colIndexIntType = self.TableSetup.index(("Integrate", "str"))
        self._colIndexStatus = self.TableSetup.index(("Status", "str"))
        self._colIndexHKL = ProcessTableWidget.TableSetup.index(("HKL", "str"))
        # self._colIndexPeak = self.TableSetup.index(('Peak', 'str'))
        # self._colIndexIndexFrom = self.TableSetup.index(('Index From', 'str'))
        self._colIndexMotor = ProcessTableWidget.TableSetup.index(("Motor", "str"))
        self._colIndexMotorStep = ProcessTableWidget.TableSetup.index(("Motor Step", "str"))
        self._colIndexWaveLength = self.TableSetup.index(("Wavelength", "float"))
        self._colIndexKIndex = self.TableSetup.index(("K-Index", "int"))
        # self._colIndexWorkspace = self.TableSetup.index(('Workspace', 'str'))

        return


class ScanSurveyTable(tableBase.NTableWidget):
    """
    Extended table widget for peak integration
    """

    Table_Setup = [
        ("Scan", "int"),
        ("Max Counts Pt", "int"),
        ("Max Counts", "float"),
        ("H", "float"),
        ("K", "float"),
        ("L", "float"),
        ("Q-range", "float"),
        ("Sample Temp", "float"),
        ("2theta", "float"),
        ("Selected", "checkbox"),
    ]

    def __init__(self, parent):
        """
        :param parent:
        """
        tableBase.NTableWidget.__init__(self, parent)

        self._myScanSummaryList = list()

        self._currStartScan = 0
        self._currEndScan = sys.maxsize
        self._currMinCounts = 0.0
        self._currMaxCounts = sys.float_info.max

        self._colIndexH = None
        self._colIndexK = None
        self._colIndexL = None

        self._colIndex2Theta = None

        return

    def filter_and_sort(self, start_scan, end_scan, min_counts, max_counts, sort_by_column, sort_order):
        """
        Filter the survey table and sort
        Note: it might not be efficient here because the table will be refreshed twice
        :param start_scan:
        :param end_scan:
        :param min_counts:
        :param max_counts:
        :param sort_by_column:
        :param sort_order: 0 for ascending, 1 for descending
        :return:
        """
        # check
        assert isinstance(start_scan, int) and isinstance(end_scan, int) and end_scan >= start_scan
        assert isinstance(min_counts, float) and isinstance(max_counts, float) and min_counts < max_counts
        assert isinstance(sort_by_column, str), "sort_by_column requires a string but not %s." % str(type(sort_by_column))
        assert isinstance(sort_order, int), "sort_order requires an integer but not %s." % str(type(sort_order))

        # get column index to sort
        col_index = self.get_column_index(column_name=sort_by_column)

        # filter on the back end row contents list first
        self.filter_rows(start_scan, end_scan, min_counts, max_counts)

        # order
        self.sort_by_column(col_index, sort_order)

        return

    def filter_rows(self, start_scan, end_scan, min_counts, max_counts):
        """
        Filter by scan number, detector counts on self._myScanSummaryList
        and reset the table via the latest result
        :param start_scan:
        :param end_scan:
        :param min_counts:
        :param max_counts:
        :return:
        """
        # check whether it can be skipped
        if (
            start_scan == self._currStartScan
            and end_scan == self._currEndScan
            and min_counts == self._currMinCounts
            and max_counts == self._currMaxCounts
        ):
            # same filter set up, return
            return

        # clear the table
        self.remove_all_rows()

        # go through all rows in the original list and then reconstruct
        for index in range(len(self._myScanSummaryList)):
            sum_item = self._myScanSummaryList[index]
            # check
            assert isinstance(sum_item, list)
            assert len(sum_item) == len(self._myColumnNameList) - 1
            # check with filters: original order is counts, scan, Pt., ...
            scan_number = sum_item[1]
            if scan_number < start_scan or scan_number > end_scan:
                continue
            counts = sum_item[0]
            if counts < min_counts or counts > max_counts:
                continue

            # modify for appending to table
            row_items = sum_item[:]
            counts = row_items.pop(0)
            row_items.insert(2, counts)
            row_items.append(False)

            # append to table
            self.append_row(row_items)
        # END-FOR (index)

        # Update
        self._currStartScan = start_scan
        self._currEndScan = end_scan
        self._currMinCounts = min_counts
        self._currMaxCounts = max_counts

        return

    def get_hkl(self, row_index):
        """
        Get peak index (HKL) from survey table (i.e., SPICE file)
        :param row_index:
        :return:
        """
        index_h = self.get_cell_value(row_index, self._colIndexH)
        index_k = self.get_cell_value(row_index, self._colIndexK)
        index_l = self.get_cell_value(row_index, self._colIndexL)

        return index_h, index_k, index_l

    def get_scan_numbers(self, row_index_list):
        """
        Get scan numbers with specified rows
        :param row_index_list:
        :return:
        """
        scan_list = list()
        scan_col_index = self.Table_Setup.index(("Scan", "int"))
        for row_index in row_index_list:
            scan_number_i = self.get_cell_value(row_index, scan_col_index)
            scan_list.append(scan_number_i)
        scan_list.sort()

        return scan_list

    def get_selected_scan_pt(self):
        """
        get selected row's scan number and pt number
        :return:
        """
        selected_row_list = self.get_selected_rows()

        selected_scan_pt_list = list()
        for row_number in selected_row_list:
            scan_number = self.get_cell_value(row_number, 0)
            pt_number = self.get_cell_value(row_number, 1)
            selected_scan_pt_list.append((scan_number, pt_number))
        # END-FOR

        return selected_scan_pt_list

    def get_selected_run_surveyed(self, required_size=1):
        """
        Purpose: Get selected pt number and run number that is set as selected
        Requirements: there must be one and only one run that is selected
        Guarantees: a 2-tuple for integer for return as scan number and Pt. number
        :param required_size: if specified as an integer, then if the number of selected rows is different,
                              an exception will be thrown.
        :return: a 2-tuple of integer if required size is 1 (as old implementation) or a list of 2-tuple of integer
        """
        # check required size?
        assert isinstance(required_size, int) or required_size is None, (
            "Required number of runs {0} must be None " "or an integer but not a {1}." "".format(required_size, type(required_size))
        )

        # get the selected row indexes and check
        row_index_list = self.get_selected_rows(True)

        if required_size is not None and required_size != len(row_index_list):
            raise RuntimeError(
                "It is required to have {0} runs selected, but now there are {1} runs that are selected.".format(
                    required_size, row_index_list
                )
            )

        # get all the scans and rows that are selected
        scan_run_list = list()
        for i_row in row_index_list:
            # get scan and pt.
            scan_number = self.get_cell_value(i_row, 0)
            pt_number = self.get_cell_value(i_row, 1)
            scan_run_list.append((scan_number, pt_number))

        # special case for only 1 run that is selected
        if len(row_index_list) == 1 and required_size is not None:
            # get scan and pt
            return scan_run_list[0]
        # END-IF

        return scan_run_list

    def show_reflections(self, num_rows):
        """
        :param num_rows:
        :return:
        """
        assert isinstance(num_rows, int)
        assert num_rows > 0
        assert len(self._myScanSummaryList) > 0

        for i_ref in range(min(num_rows, len(self._myScanSummaryList))):
            # get counts
            scan_summary = self._myScanSummaryList[i_ref]
            # check
            assert isinstance(scan_summary, list)
            assert len(scan_summary) == len(self._myColumnNameList) - 1
            # modify for appending to table
            row_items = scan_summary[:]
            max_count = row_items.pop(0)
            row_items.insert(2, max_count)
            row_items.append(False)
            # append
            self.append_row(row_items)
        # END-FOR

        return

    def set_survey_result(self, scan_summary_list):
        """

        :param scan_summary_list:
        :return:
        """
        # check
        assert isinstance(scan_summary_list, list)

        # Sort and set to class variable
        scan_summary_list.sort(reverse=True)
        self._myScanSummaryList = scan_summary_list

        return

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_setup(ScanSurveyTable.Table_Setup)
        self.set_status_column_name("Selected")

        self._colIndexH = ScanSurveyTable.Table_Setup.index(("H", "float"))
        self._colIndexK = ScanSurveyTable.Table_Setup.index(("K", "float"))
        self._colIndexL = ScanSurveyTable.Table_Setup.index(("L", "float"))

        self._colIndex2Theta = ScanSurveyTable.Table_Setup.index(("2theta", "float"))

        return

    def reset(self):
        """Reset the inner survey summary table
        :return:
        """
        self._myScanSummaryList = list()


class SinglePtIntegrationTable(tableBase.NTableWidget):
    """
    Extended QTable for integration on single Pt with previously calculated FWHM
    """

    Table_Setup = [
        ("Scan", "int"),
        ("Pt", "int"),
        ("HKL", "str"),
        ("PeakHeight", "float"),
        ("2theta", "float"),
        ("FWHM", "float"),
        ("Intensity", "float"),
        ("Pt-Sigma", "float"),
        ("Pt-I", "float"),
        ("Pt-B", "float"),
        ("ROI", "str"),  # name of ROI used to integrate counts on detector (single measurement)
        ("Reference Scans", "str"),
        ("Selected", "checkbox"),
    ]

    def __init__(self, parent):
        """
        initialization
        :param parent:
        """
        super(SinglePtIntegrationTable, self).__init__(parent)

        # class variables
        self._scan_index = None
        self._pt_index = None
        self._hkl_index = None
        self._height_index = None
        self._2theta_index = None
        self._fwhm_index = None
        self._intensity_index = None
        self._2thta_scans_index = None
        self._ref_scans_index = None
        self._roi_index = None

        self._pt_row_dict = dict()

        return

    def add_scan_pt(self, scan_number, pt_number, hkl_str, two_theta):
        """add a new scan/pt to the table
        :param scan_number:
        :param pt_number:
        :param hkl_str:
        :param two_theta:
        :return:
        """
        # check
        if (scan_number, pt_number) in self._pt_row_dict:
            raise RuntimeError("Pt number {0} exists in the table already.".format(pt_number))

        # check inputs
        assert isinstance(scan_number, int), "Scan number {0} must be an integer but not a {1}" "".format(scan_number, type(scan_number))
        assert isinstance(pt_number, int), "Pt number {0} must be an integer".format(pt_number)
        assert isinstance(hkl_str, str), "HKL {0} must be given as a string.".format(hkl_str)
        assert isinstance(two_theta, float), "2theta {0} must be a float"

        # add a new row to the table
        status, error_msg = self.append_row([scan_number, pt_number, hkl_str, 0.0, two_theta, 0.0, 0.0, 0.0, 0.0, 0.0, "", "", False])
        if not status:
            raise RuntimeError(error_msg)

        # register
        self._pt_row_dict[scan_number, pt_number] = self.rowCount() - 1

        # set scan editable
        item_i = self.item(self.rowCount() - 1, self._ref_scans_index)
        item_i.setFlags(item_i.flags() | QtCore.Qt.ItemIsEditable)

        return

    def get_fwhm(self, row_index):
        """get reference scan's FWHM
        :param row_index:
        :return:
        """
        return self.get_cell_value(row_index, self._fwhm_index)

    def get_peak_intensities(self):
        """get the summary on all peaks' intensities
        :return: dictionary as scan number and peak intensity
        """
        peak_intensity_dict = dict()

        for row_index in range(self.rowCount()):
            scan_number = self.get_cell_value(row_index, self._scan_index)
            intensity_i = self.get_cell_value(row_index, self._intensity_index)
            roi_i = self.get_cell_value(row_index, self._roi_index)
            peak_intensity_dict[scan_number] = intensity_i, roi_i
        # END-FOR

        return peak_intensity_dict

    def get_pt_number(self, row_index):
        """
        get PT number
        :param row_index:
        :return:
        """
        return self.get_cell_value(row_index, self._pt_index)

    def get_reference_scans(self, row_index):
        """
        get the reference scans (i.e., the scans having same/close 2theta to the single Pt scan
        :param row_index:
        :return:
        """
        # get value
        scans_str = self.get_cell_value(row_index, self._ref_scans_index)

        # no matched scan can be found!
        if scans_str is None:
            return None
        elif scans_str == "No match":
            return None
        elif scans_str == "":
            return None

        # split and parse to integers
        terms = scans_str.split(",")
        ref_scan_list = [int(term) for term in terms]

        return ref_scan_list

    def get_region_of_interest_name(self, row_index):
        """
        get ROI name
        :param row_index:
        :return:
        """
        return self.get_cell_value(row_index, self._roi_index)

    def get_scan_number(self, row_index):
        """
        get scan number of the specified row
        :param row_index:
        :return:
        """
        return self.get_cell_value(row_index, self._scan_index)

    def get_scan_pt_list(self):
        """
        get a list of current scan and pt pair in the table
        :return:
        """
        return self._pt_row_dict.keys()

    def get_two_theta(self, row_index):
        """
        get two-theta value
        :param row_index:
        :return:
        """
        return self.get_cell_value(row_index, self._2theta_index)

    def save_intensities_to_file(self, out_file_name):
        """
        save integrated peaks intensities to file
        :param out_file_name:
        :return:
        """
        # check inputs ..
        assert isinstance(out_file_name, str), "Output file name {0} must be a string but not a {1}" "".format(
            out_file_name, type(out_file_name)
        )
        if not os.access(out_file_name, os.W_OK):
            raise RuntimeError("Use specified output file {0} is not writable.".format(out_file_name))

        out_buffer = ""
        for i_row in range(self.rowCount()):
            scan_number = self.get_cell_value(i_row, self._scan_index)
            intensity = self.get_cell_value(i_row, self._intensity_index)
            out_buffer += "{0} \t{1}\n".format(scan_number, intensity)

        out_file = open(out_file_name, "w")
        out_file.write(out_buffer)
        out_file.close()

        return

    def set_gaussian_sigma(self, row_index, sigma):
        """
        set the (Gaussian) sigma value to a row
        :param row_index:
        :param sigma: sigma value of Gaussian
        :return: None
        """
        self.update_cell_value(row_index, self._fwhm_index, sigma)

        return

    def set_reference_scan_numbers(self, row_index, ref_numbers):
        """set reference scan numbers or warning as 'No match'
        :param row_index:
        :param ref_numbers:
        :return:
        """
        # process the reference number
        if isinstance(ref_numbers, str):
            scans_str = ref_numbers
        elif isinstance(ref_numbers, list):
            scans_str = ""
            for index, ref_number in enumerate(ref_numbers):
                if index > 0:
                    scans_str += ","
                scans_str += "{0}".format(ref_number)

        else:
            raise AssertionError("Reference scan numbers {0} of type {1} is not supported." "".format(ref_numbers, type(ref_numbers)))

        # add to table
        self.update_cell_value(row_index, self._ref_scans_index, scans_str)

    def set_two_theta(self, row_index, two_theta):
        """
        set 2theta value of the scan
        :param row_index:
        :param two_theta:
        :return:
        """
        assert isinstance(two_theta, float), "2theta {0} must be a float.".format(two_theta)
        self.update_cell_value(row_index, self._2theta_index, two_theta)

        return

    def setup(self):
        """
        set up the table with default columns
        :return:
        """
        # set up columns
        self.init_setup(SinglePtIntegrationTable.Table_Setup)

        # set up column index
        self._scan_index = SinglePtIntegrationTable.Table_Setup.index(("Scan", "int"))
        self._pt_index = SinglePtIntegrationTable.Table_Setup.index(("Pt", "int"))
        self._hkl_index = SinglePtIntegrationTable.Table_Setup.index(("HKL", "str"))
        self._height_index = SinglePtIntegrationTable.Table_Setup.index(("PeakHeight", "float"))
        self._2theta_index = SinglePtIntegrationTable.Table_Setup.index(("2theta", "float"))
        self._fwhm_index = SinglePtIntegrationTable.Table_Setup.index(("FWHM", "float"))
        self._intensity_index = SinglePtIntegrationTable.Table_Setup.index(("Intensity", "float"))
        self._ref_scans_index = SinglePtIntegrationTable.Table_Setup.index(("Reference Scans", "str"))
        self._roi_index = SinglePtIntegrationTable.Table_Setup.index(("ROI", "str"))

        return

    def set_fwhm(self, scan_number, pt_number, fwhm):
        """

        :param pt_number:
        :param fwhm:
        :return:
        """
        row_number = self._pt_row_dict[scan_number, pt_number]

        self.update_cell_value(row_number, self._fwhm_index, fwhm)

        return

    def set_intensity(self, scan_number, pt_number, intensity):
        """

        :param scan_number:
        :param pt_number:
        :param intensity:
        :return:
        """
        row_number = self._pt_row_dict[scan_number, pt_number]

        self.update_cell_value(row_number, self._intensity_index, intensity)

        return

    def set_peak_height(self, scan_number, pt_number, peak_height, roi_name):
        """set the intensity of single measurement from the counts on the detector.
        In the view as 3D peak, it is the cut on the center plane as the peak shape can be modeled by 3D Gaussian.
        Thus the integrated value is used as the Gaussian's height.
        :param scan_number:
        :param pt_number:
        :param peak_height:
        :param roi_name: ROI is closed related to how a peak/single measurement's intensity is calculated
        :return:
        """
        row_number = self._pt_row_dict[scan_number, pt_number]

        self.update_cell_value(row_number, self._height_index, peak_height)
        self.update_cell_value(row_number, self._roi_index, roi_name)

        return


class UBMatrixPeakTable(tableBase.NTableWidget):
    """
    Extended table for peaks used to calculate UB matrix
    """

    # UB peak information table
    UB_Peak_Table_Setup = [
        ("Scan", "int"),
        ("Pt", "int"),
        ("Spice HKL", "str"),
        ("Calculated HKL", "str"),
        ("Q-Sample", "str"),
        ("Selected", "checkbox"),
        ("m1", "float"),
        ("Wavelength", "float"),  # wave length
        ("Error", "float"),
    ]

    def __init__(self, parent):
        """
        Initialization
        :param parent:
        :return:
        """
        tableBase.NTableWidget.__init__(self, parent)

        # define class variables
        self._cachedSpiceHKL = dict()

        # class variables for column indexes
        self._colIndexScan = None
        self._colIndexSpiceHKL = None
        self._colIndexCalculatedHKL = None
        self._colIndexQSample = None
        self._colIndexWavelength = None
        self._colIndexError = None

        return

    def add_peak(self, scan_number, spice_hkl, q_sample, m1, wave_length):
        """

        :param scan_number:
        :param spice_hkl:
        :param q_sample:
        :param m1:
        :param wave_length:
        :return:
        """
        # check inputs
        assert isinstance(scan_number, int), "Scan number integer"
        assert len(spice_hkl) == 3, "Spice HKL"
        assert len(q_sample) == 3, "Q-sample"
        assert isinstance(m1, float) or m1 is None, "m1"
        assert isinstance(wave_length, float) or wave_length is None, "wave length"

        # spice_hkl_str = '{0:.4f}, {1:.4f}, {2:.4f}'.format(spice_hkl[0], spice_hkl[1], spice_hkl[2])
        # q_sample_str = '{0:.4f}, {1:.4f}, {2:.4f}'.format(q_sample[0], q_sample[1], q_sample[2])
        spice_hkl_str = self.format_array(spice_hkl)
        q_sample_str = self.format_array(q_sample)
        self.append_row([scan_number, -1, spice_hkl_str, "", q_sample_str, False, m1, wave_length, ""])

        return True, ""

    @staticmethod
    def format_array(array):
        """
        output a formatted array with limited precision of float
        :param array:
        :return:
        """
        format_str = ""
        for index, number in enumerate(array):
            if index > 0:
                format_str += ", "
            if isinstance(number, float):
                format_str += "{0:.4f}".format(number)
            else:
                format_str += "{0}".format(number)
        # END-FOR

        return format_str

    def get_exp_info(self, row_index):
        """
        Get experiment information from a row
        :param row_index:
        :return: scan number, pt number
        """
        assert isinstance(row_index, int)

        scan_number = self.get_cell_value(row_index, 0)
        assert isinstance(scan_number, int)
        pt_number = self.get_cell_value(row_index, 1)
        assert isinstance(pt_number, int)

        return scan_number, pt_number

    def get_hkl(self, row_index, is_spice_hkl):
        """
        Get reflection's miller index
        :except RuntimeError:
        :param row_index:
        :param is_spice_hkl:
        :return: 3-tuple as H, K, L
        """
        # check input
        assert isinstance(row_index, int), "Row index {0} must be an integer but not a {1}." "".format(row_index, type(row_index))

        # get the HKL either parsed from SPICE file or from calculation
        if is_spice_hkl:
            hkl_str = self.get_cell_value(row_index, self._colIndexSpiceHKL)
        else:
            hkl_str = self.get_cell_value(row_index, self._colIndexCalculatedHKL)

        # convert the recorded string to HKL
        status, ret_obj = guiutility.parse_float_array(hkl_str)
        if not status:
            raise RuntimeError("Unable to parse hkl (str) due to {0}".format(ret_obj))
        elif len(ret_obj) != 3:
            raise RuntimeError('Unable to convert array "{0}" to 3 floating points.'.format(hkl_str))
        else:
            m_h, m_k, m_l = ret_obj

        return m_h, m_k, m_l

    def get_scan_pt(self, row_number):
        """
        Get Scan and Pt from a row
        :param row_number:
        :return:
        """
        scan_number = self.get_cell_value(row_number, 0)
        pt_number = self.get_cell_value(row_number, 1)

        return scan_number, pt_number

    def get_selected_scans(self):
        """
        get the scan numbers that are selected
        :return:
        """
        selected_rows = self.get_selected_rows(True)

        scan_list = list()
        for i_row in selected_rows:
            scan_number = self.get_cell_value(i_row, self._colIndexScan)
            scan_list.append(scan_number)

        return scan_list

    def is_selected(self, row_index):
        """Check whether a row is selected.
        :param row_index:
        :return:
        """
        if row_index < 0 or row_index >= self.rowCount():
            raise IndexError("Input row number %d is out of range [0, %d)" % (row_index, self.rowCount()))

        col_index = UBMatrixPeakTable.UB_Peak_Table_Setup.index(("Selected", "checkbox"))

        return self.get_cell_value(row_index, col_index)

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_setup(UBMatrixPeakTable.UB_Peak_Table_Setup)
        self.set_status_column_name("Selected")

        # define all the _colIndex
        self._colIndexScan = self._myColumnNameList.index("Scan")
        self._colIndexSpiceHKL = self._myColumnNameList.index("Spice HKL")
        self._colIndexCalculatedHKL = self._myColumnNameList.index("Calculated HKL")
        self._colIndexQSample = self._myColumnNameList.index("Q-Sample")
        self._colIndexWavelength = self._myColumnNameList.index("Wavelength")
        self._colIndexError = self._myColumnNameList.index("Error")

        # set up the width of some columns
        self.setColumnWidth(self._colIndexSpiceHKL, 240)
        self.setColumnWidth(self._colIndexCalculatedHKL, 240)
        self.setColumnWidth(4, 240)

        return

    def select_nuclear_peak_rows(self, tolerance):
        """
        select all nuclear peaks, i.e., set the flag on on 'select' for all rows if their HKL indicates that
        they are nuclear peaks
        :param tolerance:
        :return: string as error message
        """
        num_rows = self.rowCount()
        error_message = ""

        for row_index in range(num_rows):
            # get the reading of HKL
            try:
                hkl_tuple = self.get_hkl(row_index, is_spice_hkl=True)
                if fourcircle_utility.is_peak_nuclear(hkl_tuple[0], hkl_tuple[1], hkl_tuple[2], tolerance):
                    self.select_row(row_index, status=True)
            except RuntimeError as error:
                error_message += "Unable to parse HKL of line %d due to %s." % (row_index, str(error))
        # END-FOR

        return error_message

    def select_scans(self, select_all=False, nuclear_peaks=False, hkl_tolerance=None, wave_length=None, wave_length_tolerance=None):
        """
        select scans in the UB matrix table
        :param select_all:
        :param nuclear_peaks:
        :param hkl_tolerance:
        :param wave_length:
        :param wave_length_tolerance:
        :return:
        """
        if select_all:
            # select all
            self.select_all_rows(True)

        elif nuclear_peaks or wave_length_tolerance is not None:
            # using filters
            if nuclear_peaks:
                self.select_nuclear_peak_rows(hkl_tolerance)
            if wave_length_tolerance is not None:
                self.select_rows_by_column_value(self._colIndexWavelength, wave_length, wave_length_tolerance, keep_current_selection=True)
        else:
            raise RuntimeError("Must pick up one option to do filter.")

        return

    def set_hkl(self, i_row, hkl, is_spice_hkl, error=None):
        """
        Set HKL to a row in the table. Show H/K/L with 4 decimal pionts
        :param i_row:
        :param hkl: HKL is a list of tuple
        :param is_spice_hkl: If true, then set input to cell for SPICE-imported HKL. Otherwise to calculated HKL.
        :param error: error of HKL
        """
        # Check
        assert isinstance(i_row, int), f"Row number (index) must be integer but not {type(i_row)}."

        if isinstance(hkl, list) or isinstance(hkl, tuple):
            assert len(hkl) == 3, "In case HKL is list of tuple, its size must be equal to 3 but not %d." "" % len(hkl)
        elif isinstance(hkl, numpy.ndarray):
            assert hkl.shape == (3,), "In case HKL is numpy array, its shape must be (3,) but not %s." "" % str(hkl.shape)
        else:
            raise AssertionError("HKL of type %s is not supported. Supported types include list, tuple " "and numpy array." % type(hkl))
        assert isinstance(is_spice_hkl, bool), "Flag {0} for SPICE-HKL must be a boolean but not a {1}." "".format(
            is_spice_hkl, type(is_spice_hkl)
        )

        # convert to a string with 4 decimal points
        hkl_str = "%.4f, %.4f, %.4f" % (hkl[0], hkl[1], hkl[2])

        if is_spice_hkl:
            self.update_cell_value(i_row, self._colIndexSpiceHKL, hkl_str)
        else:
            self.update_cell_value(i_row, self._colIndexCalculatedHKL, hkl_str)

        # If error message is shown, then write error message to Table cell
        if error is not None:
            i_col_error = UBMatrixPeakTable.UB_Peak_Table_Setup.index(("Error", "float"))
            self.update_cell_value(i_row, i_col_error, error)

        return

    def restore_cached_indexing(self, is_spice=True):
        """
        Restore the previously saved value to HKL
        :return:
        """
        # check first such that all the stored value are to be
        stored_line_index = sorted(self._cachedSpiceHKL.keys())
        assert len(stored_line_index) == self.rowCount(), "The current rows and cached row counts do not match."

        # restore
        for row_index in stored_line_index:
            hkl = self._cachedSpiceHKL[row_index]
            self.set_hkl(row_index, hkl, is_spice_hkl=is_spice)
        # END-FOR

        # clear
        self._cachedSpiceHKL.clear()

        return

    def store_current_indexing(self):
        """
        Store the current indexing for reverting
        :return:
        """
        # clear the previous value
        self._cachedSpiceHKL.clear()

        # store
        num_rows = self.rowCount()
        for row_index in range(num_rows):
            peak_indexing = self.get_hkl(row_index, is_spice_hkl=True)
            self._cachedSpiceHKL[row_index] = peak_indexing
        # END-FOR

        return

    def update_hkl(self, i_row, h, k, l):
        """Update HKL value
        :param i_row: index of the row to have HKL updated
        :param h:
        :param k:
        :param l:
        """
        assert isinstance(i_row, int), "row number {0} must be an integer but not a {1}." "".format(i_row, type(i_row))

        self.update_cell_value(i_row, self._colIndexCalculatedHKL, self.format_array([h, k, l]))

        return
