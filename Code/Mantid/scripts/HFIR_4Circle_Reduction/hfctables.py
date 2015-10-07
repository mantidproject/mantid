#pylint: disable=W0403,C0103,R0901,R0904
import numpy
import HFIR_4Circle_Reduction.NTableWidget as tableBase


class IntegratePeaksTableWidget(tableBase.NTableWidget):
    """
    Extended table widget for peak integration
    """
    def __init__(self, parent):
        """
        :param parent:
        """
        tableBase.NTableWidget.__init__(self, parent)

        return

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_setup(... ...)

        return


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
        self._matrix = numpy.ndarray( (3, 3), float )
        for i in xrange(3):
            for j in xrange(3):
                self._matrix[i][j] = 0.

        return

    def _set_to_table(self):
        """
        TODO/DOC
        :return:
        """
        for i_row in xrange(3):
            for j_col in xrange(3):
                self.update_cell_value(i_row, j_col, self._matrix[i_row][j_col])

        return

    def get_matrix(self):
        """
        Get the copy of the matrix
        :return:
        """
        print '[DB] MatrixTable: _Matrix = ', self._matrix
        return self._matrix.copy()

    def set_from_list(self, element_array):
        """
        TODO/DOC
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
        TODO - DOC
        :param matrix:
        :return:
        """
        # Check
        assert isinstance(matrix, numpy.ndarray)
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
                       ('Use', 'checkbox'),
                       ('m1', 'float')]


class UBMatrixPeakTable(tableBase.NTableWidget):
    """
    Extended table for peaks used to calculate UB matrix
    """
    def __init__(self, parent):
        """

        :param parent:
        :return:
        """
        tableBase.NTableWidget.__init__(self, parent)

        return

    def get_exp_info(self, row_index):
        """
        Get experiment information from a row
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

    def is_selected(self, row_index):
        """

        :return:
        """
        if row_index < 0 or row_index >= self.rowCount():
            raise IndexError('Input row number %d is out of range [0, %d)' % (row_index, self.rowCount()))

        col_index = UB_Peak_Table_Setup.index(('Use', 'checkbox'))

        return self.get_cell_value(row_index, col_index)

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_setup(UB_Peak_Table_Setup)

        return

    def set_hkl(self, i_row, hkl):
        """
        Set HKL to table
        :param irow:
        :param hkl:
        """
        # Check
        assert isinstance(i_row, int)
        assert isinstance(hkl, list)

        i_col_h = UB_Peak_Table_Setup.index(('H', 'float'))
        i_col_k = UB_Peak_Table_Setup.index(('K', 'float'))
        i_col_l = UB_Peak_Table_Setup.index(('L', 'float'))

        self.update_cell_value(i_row, i_col_h, hkl[0])
        self.update_cell_value(i_row, i_col_k, hkl[1])
        self.update_cell_value(i_row, i_col_l, hkl[2])

        return

# Processing status table
Process_Table_Setup = [('Scan', 'int'),
                       ('Number Pt', 'int'),
                       ('Status', 'str')]


class ProcessTableWidget(tableBase.NTableWidget):
    """
    Extended table for peaks used to calculate UB matrix
    """
    def __init__(self, parent):
        """

        :param parent:
        :return:
        """
        tableBase.NTableWidget.__init__(self, parent)

        return

    def append_scans(self, scans):
        """ Append rows
        :param scans:
        :return:
        """
        # Check
        assert isinstance(scans, list)

        # Append rows
        for scan in scans:
            row_value_list = [scan, 0, 'In Queue']
            self.append_row(row_value_list)

        return

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_setup(Process_Table_Setup)

        return

    def set_scan_pt(self, scan_no, pt_list):
        """
        :param scan_no:
        :param pt_list:
        :return:
        """
        # Check
        assert isinstance(scan_no, int)

        num_rows = self.rowCount()
        set_done = False
        for i_row in xrange(num_rows):
            tmp_scan_no = self.get_cell_value(i_row, 0)
            if scan_no == tmp_scan_no:
                self.update_cell_value(i_row, 1, len(pt_list))
                set_done = True
                break
        # END-FOR

        if set_done is False:
            return 'Unable to find scan %d in table.' % scan_no

        return ''

    def set_status(self, scan_no, status):
        """
        TODO/Doc
        :param status:
        :return:
        """
        # Check
        assert isinstance(scan_no, int)

        num_rows = self.rowCount()
        set_done = False
        for i_row in xrange(num_rows):
            tmp_scan_no = self.get_cell_value(i_row, 0)
            if scan_no == tmp_scan_no:
                self.update_cell_value(i_row, 2, status)
                set_done = True
                break
        # END-FOR

        if set_done is False:
            return 'Unable to find scan %d in table.' % scan_no

        return ''
