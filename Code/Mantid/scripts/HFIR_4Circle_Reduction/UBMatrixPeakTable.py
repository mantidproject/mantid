#pylint: disable=W0403,C0103,R0901,R0904

import NTableWidget as base

# UB peak information table
UB_Peak_Table_Setup = [('Scan', 'int'),
                       ('Pt', 'int'),
                       ('H', 'float'),
                       ('K', 'float'),
                       ('L', 'float'),
                       ('Q_x', 'float'),
                       ('Q_y', 'float'),
                       ('Q_z', 'float'),
                       ('Status', 'checkbox')]


class UBMatrixPeakTable(base.NTableWidget):
    """
    Extended table for peaks used to calculate UB matrix
    """
    def __init__(self, parent):
        """

        :param parent:
        :return:
        """
        base.NTableWidget.__init__(self, parent)

        return

    def get_exp_info(self, row_index):
        """
        Get experiment information from a row
        :return:
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

        col_index = UB_Peak_Table_Setup.index(('Status', 'checkbox'))

        return self.get_cell_value(row_index, col_index)

    def setup(self):
        """
        Init setup
        :return:
        """
        self.init_setup(UB_Peak_Table_Setup)

        return
