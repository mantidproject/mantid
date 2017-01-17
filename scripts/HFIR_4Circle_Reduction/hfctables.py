#pylint: disable=W0403,C0103,R0901,R0904,R0913,C0302
import numpy
import sys
import fourcircle_utility

import NTableWidget as tableBase


class KShiftTableWidget(tableBase.NTableWidget):
    """ Extended table widget for show the K-shift vectors set to the output Fullprof file
    """
    # Table set up
    TableSetup = [('Index', 'int'),
                  ('Kx', 'float'),
                  ('Ky', 'float'),
                  ('Kz', 'float'),
                  ('Selected', 'checkbox')]

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
        """ Delete a row
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
        """ Set up the table
        :return:
        """
        self.init_setup(self.TableSetup)

        self._iColKIndex = self.TableSetup.index(('Index', 'int'))

        return


class PeakIntegrationTableWidget(tableBase.NTableWidget):
    """
    Extended table widget for studying peak integration of scan on various Pts.
    """

    # UB peak information table
    Table_Setup = [('Pt', 'int'),
                   ('Raw', 'float'),
                   ('Masked', 'float'),
                   ('Selected', 'checkbox')]

    def __init__(self, parent):
        """
        :param parent:
        """
        tableBase.NTableWidget.__init__(self, parent)

        self._expNumber = -1
        self._scanNumber = -1

        self._intensityColIndex = None

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
        assert isinstance(pt_number, int)
        assert isinstance(raw_signal, int) or isinstance(raw_signal, float)
        assert isinstance(masked_signal, float)

        # form a new row and append
        status, msg = self.append_row([pt_number, raw_signal, masked_signal, False])
        if status is False:
            msg = 'Unable to append row to peak integration table due to %s' % msg

        return status, msg

    def get_exp_info(self):
        """
        Get experiment information of the data written in the table now
        :return:
        """
        return self._expNumber, self._scanNumber

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_setup(self.Table_Setup)

        self._statusColName = 'Selected'

        # set columns' width
        self.setColumnWidth(0, 35)
        self.setColumnWidth(1, 60)
        self.setColumnWidth(2, 90)
        self.setColumnWidth(3, 90)

        # Set others...
        self._intensityColIndex = self._myColumnNameList.index('Masked')

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
        hkl = '0, 0, 0'
        q = '0, 0, 0'
        signal = -1

        num_rows = len(pt_vec)
        for i_row in xrange(num_rows):
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
        index_q_x = self.Table_Setup.index(('Q_x', 'float'))
        for j in xrange(3):
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
        assert self.rowCount() > 0, 'Table is empty!'
        assert isinstance(background, float) and background >= 0.

        # Integrate
        sum_intensity = 0.
        for i_row in range(self.rowCount()):
            intensity_i = self.get_cell_value(i_row, self._intensityColIndex)
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
        for i in xrange(3):
            for j in xrange(3):
                self._matrix[i][j] = 0.

        return

    def _set_to_table(self):
        """
        Set values in holder '_matrix' to TableWidget
        :return:
        """
        for i_row in xrange(3):
            for j_col in xrange(3):
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
        matrix_string = ''
        for i in range(3):
            for j in range(3):
                matrix_string += '%.10f' % self._matrix[i][j]
                if not (i == 2 and j == 2):
                    matrix_string += ','
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
        for i in xrange(3):
            for j in xrange(3):
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
        assert isinstance(matrix, numpy.ndarray), 'Input matrix must be numpy.ndarray, but not %s' % str(type(matrix))
        assert matrix.shape == (3, 3)

        for i in xrange(3):
            for j in xrange(3):
                self._matrix[i][j] = matrix[i][j]

        self._set_to_table()

        return

    def setup(self):
        """
        Init setup
        :return:
        """
        # self.init_size(3, 3)

        for i in xrange(3):
            for j in xrange(3):
                self.set_value_cell(i, j)

        self._set_to_table()

        return


# UB peak information table
UB_Peak_Table_Setup = [('Scan', 'int'),
                       ('Pt', 'int'),
                       ('H', 'float'),
                       ('K', 'float'),
                       ('L', 'float'),
                       ('Q_x', 'float'),
                       ('Q_y', 'float'),
                       ('Q_z', 'float'),
                       ('Selected', 'checkbox'),
                       ('m1', 'float'),
                       ('lambda', 'float'),  # wave length
                       ('Error', 'float')]


class UBMatrixPeakTable(tableBase.NTableWidget):
    """
    Extended table for peaks used to calculate UB matrix
    """

    def __init__(self, parent):
        """
        Initialization
        :param parent:
        :return:
        """
        tableBase.NTableWidget.__init__(self, parent)

        # define class variables
        self._storedHKL = dict()

        return

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

    def get_hkl(self, row_index):
        """
        Get reflection's miller index
        :param row_index:
        :return:
        """
        assert isinstance(row_index, int)

        m_h = self.get_cell_value(row_index, 2)
        m_k = self.get_cell_value(row_index, 3)
        m_l = self.get_cell_value(row_index, 4)

        assert isinstance(m_h, float)
        assert isinstance(m_k, float)
        assert isinstance(m_l, float)

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

    def is_selected(self, row_index):
        """ Check whether a row is selected.
        :param row_index:
        :return:
        """
        if row_index < 0 or row_index >= self.rowCount():
            raise IndexError('Input row number %d is out of range [0, %d)' % (row_index, self.rowCount()))

        col_index = UB_Peak_Table_Setup.index(('Selected', 'checkbox'))

        return self.get_cell_value(row_index, col_index)

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_setup(UB_Peak_Table_Setup)
        self._statusColName = 'Selected'

        return

    def select_all_nuclear_peaks(self):
        """
        select all nuclear peaks, i.e., set the flag on on 'select' for all rows if their HKL indicates that
        they are nuclear peaks
        :return: string as error message
        """
        num_rows = self.rowCount()
        error_message = ''

        for row_index in range(num_rows):
            # get the reading of HKL
            try:
                hkl_tuple = self.get_hkl(row_index)
                if fourcircle_utility.is_peak_nuclear(hkl_tuple[0], hkl_tuple[1], hkl_tuple[2]):
                    self.select_row(row_index, status=True)
            except RuntimeError as error:
                error_message += 'Unable to parse HKL of line %d due to %s.' % (row_index, str(error))
        # END-FOR

        return error_message

    def set_hkl(self, i_row, hkl, error=None):
        """
        Set HKL to table
        :param i_row:
        :param hkl: HKL is a list of tuple
        :param error: error of HKL
        """
        # Check
        assert isinstance(i_row, int), 'Row number (index) must be integer but not %s.' % type(i_row)

        if isinstance(hkl, list) or isinstance(hkl, tuple):
            assert len(hkl) == 3, 'In case HKL is list of tuple, its size must be equal to 3 but not %d.' \
                                  '' % len(hkl)
        elif isinstance(hkl, numpy.ndarray):
            assert hkl.shape == (3,), 'In case HKL is numpy array, its shape must be (3,) but not %s.' \
                                      '' % str(hkl.shape)
        else:
            raise AssertionError('HKL of type %s is not supported. Supported types include list, tuple '
                                 'and numpy array.' % type(hkl))

        # get columns
        i_col_h = UB_Peak_Table_Setup.index(('H', 'float'))
        i_col_k = UB_Peak_Table_Setup.index(('K', 'float'))
        i_col_l = UB_Peak_Table_Setup.index(('L', 'float'))

        self.update_cell_value(i_row, i_col_h, hkl[0])
        self.update_cell_value(i_row, i_col_k, hkl[1])
        self.update_cell_value(i_row, i_col_l, hkl[2])

        if error is not None:
            i_col_error = UB_Peak_Table_Setup.index(('Error', 'float'))
            self.update_cell_value(i_row, i_col_error, error)

        return

    def restore_cached_indexing(self):
        """
        Restore the previously saved value to HKL
        :return:
        """
        # check first such that all the stored value are to be
        stored_line_index = sorted(self._storedHKL.keys())
        assert len(stored_line_index) == self.rowCount(), 'The current rows and cached row counts do not match.'

        # restore
        for row_index in stored_line_index:
            hkl = self._storedHKL[row_index]
            self.set_hkl(row_index, hkl)
        # END-FOR

        # clear
        self._storedHKL.clear()

        return

    def store_current_indexing(self):
        """
        Store the current indexing for reverting
        :return:
        """
        # clear the previous value
        self._storedHKL.clear()

        # store
        num_rows = self.rowCount()
        for row_index in range(num_rows):
            peak_indexing = self.get_hkl(row_index)
            self._storedHKL[row_index] = peak_indexing
        # END-FOR

        return

    def update_hkl(self, i_row, h, k, l):
        """ Update HKL value
        :param i_row: index of the row to have HKL updated
        :param h:
        :param k:
        :param l:
        """
        assert isinstance(i_row, int)

        self.update_cell_value(i_row, 2, h)
        self.update_cell_value(i_row, 3, k)
        self.update_cell_value(i_row, 4, l)

        return


class ProcessTableWidget(tableBase.NTableWidget):
    """
    Extended table for peaks used to calculate UB matrix
    """
    TableSetup = [('Scan', 'int'),
                  ('Intensity', 'float'),
                  ('Corrected', 'float'),
                  ('Status', 'str'),
                  ('Peak', 'str'),  # peak center can be either HKL or Q depending on the unit
                  ('HKL', 'str'),
                  ('Index From', 'str'),  # source of HKL index, either SPICE or calculation
                  ('Motor', 'str'),
                  ('Motor Step', 'str'),
                  ('Wavelength', 'float'),
                  ('Workspace', 'str'),
                  ('K-Index', 'int'),
                  ('Select', 'checkbox')]

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
        self._colIndexHKL = None
        self._colIndexStatus = None
        self._colIndexPeak = None
        self._colIndexIndexFrom = None
        self._colIndexMotor = None
        self._colIndexMotorStep = None
        self._colIndexWaveLength = None
        self._colIndexKIndex = None
        self._colIndexWorkspace = None

        return

    @staticmethod
    def _generate_empty_row(scan_number, status='In-Queue', ws_name=''):
        """ Generate a list for empty row with scan number
        :param scan_number:
        :param status:
        :param frame: HKL or QSample
        :param ws_name
        :return:
        """
        # check inputs
        assert isinstance(scan_number, int)
        assert isinstance(status, str)

        intensity = None
        corr_int = None
        motor_name = None
        motor_step = None
        wave_length = 0
        peak_center = ''
        hkl = ''
        hkl_from = ''

        new_row = [scan_number, intensity, corr_int, status, peak_center, hkl, hkl_from,
                   motor_name, motor_step, wave_length, ws_name, 0, False]

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
        assert isinstance(exp_number, int)
        assert isinstance(scan_number, int)
        assert isinstance(ws_name, str)

        # construct a row
        new_row = self._generate_empty_row(scan_number, ws_name=ws_name)
        self.append_row(new_row)

        return

    def append_scans(self, scans, allow_duplicate_scans):
        """ Append rows for merge in future
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

    def get_row_by_scan(self, scan_number):
        """
        get the row number for a gien scan
        :param scan_number:
        :return:
        """
        assert isinstance(scan_number, int) and scan_number >= 0,\
            'Scan number %s (type %s) is invalid.  It must be a positive integer.' \
            '' % (str(scan_number), type(scan_number))
        num_rows = self.rowCount()
        ret_row_number = None
        for i_row in xrange(num_rows):
            tmp_scan_no = self.get_cell_value(i_row, self._colIndexScan)
            if scan_number == tmp_scan_no:
                ret_row_number = i_row
                break
        # END-FOR

        if ret_row_number is None:
            raise RuntimeError('Scan number %d does not exist in merge-scan-table.' % scan_number)

        return ret_row_number

    def get_rows_by_state(self, target_state):
        """ Get the rows' indexes by status' value (state)
        Requirements: target_state is a string
        Guarantees: a list of integers as row indexes are returned for all rows with state as target_state
        :param target_state:
        :return:
        """
        # Check
        assert isinstance(target_state, str), 'blabla'

        # Loop around to check
        return_list = list()
        num_rows = self.rowCount()
        for i_row in xrange(num_rows):
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
        assert isinstance(row_index, int) and row_index >= 0, 'Row index %s of type %s is not acceptable.' \
                                                              '' % (str(row_index), type(row_index))

        # retrieve value of HKL as string and then split them into floats
        hkl_str = self.get_cell_value(row_index, self._colIndexHKL)
        hkl_str = hkl_str.strip()
        if len(hkl_str) == 0:
            return None

        hkl_str_list = hkl_str.split(',')
        try:
            peak_index_h = float(hkl_str_list[0])
            peak_index_k = float(hkl_str_list[1])
            peak_index_l = float(hkl_str_list[2])
        except IndexError:
            raise RuntimeError('Row %d\' HKL value %s is not value.' % (row_index, hkl_str))
        except ValueError:
            raise RuntimeError('Row %d\' HKL value %s is not value.' % (row_index, hkl_str))

        return peak_index_h, peak_index_k, peak_index_l

    def get_merged_status(self, row_number):
        """ Get the status whether it is merged
        :param row_number:
        :return: boolean
        """
        # check
        assert isinstance(row_number, int)
        assert 0 <= row_number < self.rowCount()

        # get value
        merge_status_col_index = self._myColumnNameList.index('Status')
        status_str = self.get_cell_value(row_number, merge_status_col_index)

        if status_str.lower() == 'done':
            return True

        return False

    def get_merged_ws_name(self, i_row):
        """
        Get merged workspace name
        :param i_row:
        :return:
        """
        return self.get_cell_value(i_row, self._colIndexWorkspace)

    def get_scan_list(self, output_row_number=True):
        """
        Get all scans that are already listed in the table.
        :param output_row_number:
        :return: list of 2-tuple or integer according to value of output_row_number
        """
        scan_list = list()
        num_rows = self.rowCount()

        for i_row in xrange(num_rows):
            scan_num = self.get_cell_value(i_row, self._colIndexScan)
            if output_row_number:
                scan_list.append((scan_num, i_row))
            else:
                scan_list.append(scan_num)
        # END-FOR (i_row)

        return scan_list

    def get_selected_scans(self):
        """ Get list of selected scans to merge from table
        :return: list of 2-tuples (scan number, row number)
        """
        scan_list = list()
        num_rows = self.rowCount()
        col_select_index = self._myColumnNameList.index('Select')

        for i_row in xrange(num_rows):
            if self.get_cell_value(i_row, col_select_index) is True:
                scan_num = self.get_cell_value(i_row, self._colIndexScan)
                scan_list.append((scan_num, i_row))

        return scan_list

    def get_scan_number(self, row_number):
        """ Get scan number of a row
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
        error_message = ''

        for row_index in range(num_rows):
            # get the reading of HKL
            try:
                hkl_tuple = self.get_hkl(row_index)
                if hkl_tuple is None:
                    error_message += 'Row %d has no HKL showed.' % row_index
                    continue
                if fourcircle_utility.is_peak_nuclear(hkl_tuple[0], hkl_tuple[1], hkl_tuple[2]):
                    self.select_row(row_index)
            except RuntimeError as error:
                error_message += 'Unable to parse HKL of line %d due to %s.' % (row_index, str(error))
        # END-FOR

        return error_message

    def set_hkl(self, row_number, hkl, hkl_source=None):
        """ Set Miller index HKL to a row
        :param row_number: row number
        :param hkl:
        :param hkl_source:
        :return:
        """
        # check
        assert isinstance(row_number, int) and 0 <= row_number < self.rowCount(),\
            'Row number %s is out of range.' % str(row_number)
        assert len(hkl) == 3, 'HKL must be a sequence with 3 items but not %s.' % len(hkl)

        # update the cell
        hkl_str = '%.3f, %.3f, %.3f' % (hkl[0], hkl[1], hkl[2])
        self.update_cell_value(row_number, self._colIndexHKL, hkl_str)

        if hkl_source is not None:
            self.update_cell_value(row_number, self._colIndexIndexFrom, hkl_source)

        return

    def set_k_shift_index(self, row_number, k_index):
        """ Set k-shift index to a row
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
        assert isinstance(row_number, int) and 0 <= row_number < self.rowCount(), 'Input row number is out of range.'
        assert len(motor_move_tup) == 3

        # get motor information and construct the string
        motor_name = motor_move_tup[0]
        motor_move = '%.3f (%.2E)' % (motor_move_tup[1], motor_move_tup[2])

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
        assert isinstance(row_number, int) and 0 <= row_number < self.rowCount(), \
            'Row number %s is not supported or out of boundary.' % str(row_number)
        assert isinstance(peak_centre, str) or len(peak_centre) == 3,\
            'Peak centre %s must be a string or a container with size 3.' % str(peak_centre)

        # set value of peak center
        if isinstance(peak_centre, str):
            # string no need to change
            value_to_set = peak_centre
        else:
            # construct the value
            value_to_set = '%.3f, %.3f, %.3f' % (peak_centre[0], peak_centre[1], peak_centre[2])

        self.update_cell_value(row_number, self._colIndexPeak, value_to_set)

        return

    def set_peak_intensity(self, row_number, peak_intensity, lorentz_corrected=False):
        """ Set peak intensity to a row or scan
        Requirement: Either row number or scan number must be given
        Guarantees: peak intensity is set
        :param row_number:
        :param peak_intensity:
        :param lorentz_corrected:
        :return:
        """
        # check requirements
        assert isinstance(peak_intensity, float), 'Peak intensity must be a float.'

        if lorentz_corrected:
            col_index = self._colIndexCorrInt
        else:
            col_index = self._colIndexIntensity

        return self.update_cell_value(row_number, col_index, peak_intensity)

    def set_status(self, row_number, status):
        """
        Set the status for merging scan to QTable
        :param row_number: scan number
        :param status:
        :return:
        """
        # Check
        assert isinstance(status, str), 'Status (%s) must be a string, but not %s.' % (str(status), type(status))

        return self.update_cell_value(row_number, self._colIndexStatus, status)

    def set_wave_length(self, row_number, wave_length):
        """ Set wave length to a row
        :param row_number:
        :param wave_length:
        :return:
        """
        # check
        assert isinstance(row_number, int) and 0 <= row_number < self.rowCount(), 'Input row number is out of range.'
        assert isinstance(wave_length, float) and wave_length >= 0.

        # set
        col_index = self.TableSetup.index(('Wavelength', 'float'))
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
        assert isinstance(merged_md_name, str), 'Merged MDWorkspace name must be a string.'

        self.update_cell_value(row_number, self._colIndexWorkspace, merged_md_name)

        return

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_setup(self.TableSetup)
        self._statusColName = 'Select'

        # set up column index
        self._colIndexScan = ProcessTableWidget.TableSetup.index(('Scan', 'int'))
        self._colIndexIntensity = self.TableSetup.index(('Intensity', 'float'))
        self._colIndexCorrInt = self.TableSetup.index(('Corrected', 'float'))
        self._colIndexStatus = self.TableSetup.index(('Status', 'str'))
        self._colIndexHKL = ProcessTableWidget.TableSetup.index(('HKL', 'str'))
        self._colIndexPeak = self.TableSetup.index(('Peak', 'str'))
        self._colIndexIndexFrom = self.TableSetup.index(('Index From', 'str'))
        self._colIndexMotor = ProcessTableWidget.TableSetup.index(('Motor', 'str'))
        self._colIndexMotorStep = ProcessTableWidget.TableSetup.index(('Motor Step', 'str'))
        self._colIndexWaveLength = self.TableSetup.index(('Wavelength', 'float'))
        self._colIndexKIndex = self.TableSetup.index(('K-Index', 'int'))
        self._colIndexWorkspace = self.TableSetup.index(('Workspace', 'str'))

        return


class ScanSurveyTable(tableBase.NTableWidget):
    """
    Extended table widget for peak integration
    """
    Table_Setup = [('Scan', 'int'),
                   ('Max Counts Pt', 'int'),
                   ('Max Counts', 'float'),
                   ('H', 'float'),
                   ('K', 'float'),
                   ('L', 'float'),
                   ('Q-range', 'float'),
                   ('Sample Temp', 'float'),
                   ('Selected', 'checkbox')]

    def __init__(self, parent):
        """
        :param parent:
        """
        tableBase.NTableWidget.__init__(self, parent)

        self._myScanSummaryList = list()

        self._currStartScan = 0
        self._currEndScan = sys.maxint
        self._currMinCounts = 0.
        self._currMaxCounts = sys.float_info.max

        self._colIndexH = None
        self._colIndexK = None
        self._colIndexL = None

        return

    def filter_and_sort(self, start_scan, end_scan, min_counts, max_counts,
                        sort_by_column, sort_order):
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
        assert isinstance(sort_by_column, str), \
            'sort_by_column requires a string but not %s.' % str(type(sort_by_column))
        assert isinstance(sort_order, int), \
            'sort_order requires an integer but not %s.' % str(type(sort_order))

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
        if start_scan == self._currStartScan and end_scan == self._currEndScan \
                and min_counts == self._currMinCounts and max_counts == self._currMaxCounts:
            # same filter set up, return
            return

        # clear the table
        self.remove_all_rows()

        # go through all rows in the original list and then reconstruct
        for index in xrange(len(self._myScanSummaryList)):
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
        scan_col_index = self.Table_Setup.index(('Scan', 'int'))
        for row_index in row_index_list:
            scan_number_i = self.get_cell_value(row_index, scan_col_index)
            scan_list.append(scan_number_i)
        scan_list.sort()

        return scan_list

    def get_selected_run_surveyed(self):
        """
        Purpose: Get selected pt number and run number that is set as selected
        Requirements: there must be one and only one run that is selected
        Guarantees: a 2-tuple for integer for return as scan number and Pt. number
        :return: a 2-tuple of integer
        """
        # get the selected row indexes and check
        row_index_list = self.get_selected_rows(True)
        assert len(row_index_list) == 1, 'There must be exactly one run that is selected. Now' \
                                         'there are %d runs that are selected' % len(row_index_list)

        # get scan and pt
        row_index = row_index_list[0]
        scan_number = self.get_cell_value(row_index, 0)
        pt_number = self.get_cell_value(row_index, 1)

        return scan_number, pt_number

    def show_reflections(self, num_rows):
        """
        :param num_rows:
        :return:
        """
        assert isinstance(num_rows, int)
        assert num_rows > 0
        assert len(self._myScanSummaryList) > 0

        for i_ref in xrange(min(num_rows, len(self._myScanSummaryList))):
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
        self.set_status_column_name('Selected')

        self._colIndexH = ScanSurveyTable.Table_Setup.index(('H', 'float'))
        self._colIndexK = ScanSurveyTable.Table_Setup.index(('K', 'float'))
        self._colIndexL = ScanSurveyTable.Table_Setup.index(('L', 'float'))

        return

    def reset(self):
        """ Reset the inner survey summary table
        :return:
        """
        self._myScanSummaryList = list()
