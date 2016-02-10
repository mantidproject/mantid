#pylint: disable=C0103,R0904
# N(DAV)TableWidget
#

from PyQt4 import QtGui, QtCore

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s


class NTableWidget(QtGui.QTableWidget):
    """
    NdavTableWidget inherits from QTableWidget by extending the features
    for easy application.
    """
    def __init__(self, parent):
        """

        :param parent:
        :return:
        """
        QtGui.QTableWidget.__init__(self, parent)

        self._myParent = parent

        self._myColumnNameList = None
        self._myColumnTypeList = None

        self._statusColName = 'Status'

        return

    def append_row(self, row_value_list, type_list=None):
        """

        :param row_value_list:
        :return: 2-tuple as (boolean, message)
        """
        # Check input
        assert isinstance(row_value_list, list)
        if type_list is not None:
            assert isinstance(type_list, list)
            assert len(row_value_list) == len(type_list)
        else:
            type_list = self._myColumnTypeList
        if len(row_value_list) != self.columnCount():
            ret_msg = 'Input number of values (%d) is different from ' \
                      'column number (%d).' % (len(row_value_list), self.columnCount())
            return False, ret_msg
        else:
            ret_msg = ''

        # Insert new row
        row_number = self.rowCount()
        self.insertRow(row_number)

        # Set values
        for i_col in xrange(min(len(row_value_list), self.columnCount())):
            item = QtGui.QTableWidgetItem()
            item.setText(_fromUtf8(str(row_value_list[i_col])))
            item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
            if type_list[i_col] == 'checkbox':
                self.set_check_box(row_number, i_col, False)
            else:
                self.setItem(row_number, i_col, item)
        # END-FOR(i_col)

        return True, ret_msg

    def delete_rows(self, row_number_list):
        """ Delete rows
        :param row_number_list:
        :return:
        """
        # Check and re-order row numbers
        assert isinstance(row_number_list, list)
        row_number_list.sort(reverse=True)

        for row_number in row_number_list:
            self.removeRow(row_number)

        return

    def get_selected_rows(self, status=True):
        """ Get the rows whose status is same as given status
        Requirements: given status must be a boolean
        Guarantees: a list of row indexes are constructed for those rows that meet the requirement.
        :param status:
        :return: list of row indexes that are selected
        """
        # check
        assert isinstance(status, bool)
        index_status = self._myColumnNameList.index(self._statusColName)

        # loop over all the rows
        row_index_list = list()
        for i_row in xrange(self.rowCount()):
            # check status
            is_checked = self.get_cell_value(i_row, index_status)
            if is_checked == status:
                row_index_list.append(i_row)

        return row_index_list

    def get_cell_value(self, row_index, col_index):
        """
        Purpose: Get cell value
        Requirements: row index and column index are integer and within range.
        Guarantees: the cell value with correct type is returned
        :param row_index:
        :param col_index:
        :return:
        """
        # check
        assert isinstance(row_index, int)
        assert isinstance(col_index, int)
        assert 0 <= row_index < self.rowCount()
        assert 0 <= col_index < self.columnCount()

        # get cell type
        cell_data_type = self._myColumnTypeList[col_index]

        if cell_data_type == 'checkbox':
            # Check box
            cell_i_j = self.cellWidget(row_index, col_index)
            assert isinstance(cell_i_j, QtGui.QCheckBox)

            return_value = cell_i_j.isChecked()
        else:
            # Regular cell for int, float and string
            item_i_j = self.item(row_index, col_index)
            assert isinstance(item_i_j, QtGui.QTableWidgetItem)

            return_value = str(item_i_j.text())
            if cell_data_type == 'int':
                return_value = int(return_value)
            elif cell_data_type == 'float':
                return_value = float(return_value)

        return return_value

    def get_row_value(self, row_index):
        """
        :param row_index:
        :return: list of objects
        """
        if row_index < 0 or row_index >= self.rowCount():
            raise IndexError('Index of row (%d) is out of range.' % row_index)

        ret_list = list()
        for i_col in xrange(len(self._myColumnTypeList)):
            c_type = self._myColumnTypeList[i_col]

            if c_type == 'checkbox':
                # Check box
                cell_i_j = self.cellWidget(row_index, i_col)
                assert isinstance(cell_i_j, QtGui.QCheckBox)
                is_checked = cell_i_j.isChecked()
                ret_list.append(is_checked)
            else:
                # Regular cell
                item_i_j = self.item(row_index, i_col)
                assert isinstance(item_i_j, QtGui.QTableWidgetItem)
                value = str(item_i_j.text()).strip()
                if len(value) > 0:
                    if c_type == 'int':
                        value = int(value)
                    elif c_type == 'float':
                        value = float(value)
                else:
                    value = None

                ret_list.append(value)
            # END-IF-ELSE
        # END-FOR

        return ret_list

    def init_setup(self, column_tup_list):
        """ Initial setup
        :param column_tup_list: list of 2-tuple as string (column name) and string (data type)
        :return:
        """
        # Define column headings
        num_cols = len(column_tup_list)

        # Class variables
        self._myColumnNameList = list()
        self._myColumnTypeList = list()

        for c_tup in column_tup_list:
            c_name = c_tup[0]
            c_type = c_tup[1]
            self._myColumnNameList.append(c_name)
            self._myColumnTypeList.append(c_type)

        self.setColumnCount(num_cols)
        self.setHorizontalHeaderLabels(self._myColumnNameList)

        return

    def init_size(self, num_rows, num_cols):
        """

        :return:
        """
        self.setColumnCount(num_cols)
        self.setRowCount(num_rows)

        return

    def remove_all_rows(self):
        """
        Remove all rows
        :return:
        """
        num_rows = self.rowCount()
        for i_row in xrange(1, num_rows+1):
            self.removeRow(num_rows - i_row)

        return

    def select_all_rows(self, status):
        """
        Purpose: select or deselect all rows in the table if applied
        Requirements:
          (1) status/selection column name must be set right;
          (2) status (input arguments) must be a boolean
        Guarantees: all rows will be either selected (status is True) or deselected (status is false)
        :param status:
        :return: 2-tuple as (True, None) or (False, error message)
        """
        # get column  index
        try:
            status_col_index = self._myColumnNameList.index(self._statusColName)
        except ValueError as e:
            # status column name is not properly set up
            return False, str(e)

        # Loop over all rows. If any row's status is not same as target status, then set it
        num_rows = self.rowCount()
        for row_index in xrange(num_rows):
            if self.get_cell_value(row_index, status_col_index) != status:
                self.update_cell_value(row_index, status_col_index, status)
        # END-FOR

        return

    def set_check_box(self, row, col, state):
        """ function to add a new select checkbox to a cell in a table row
        won't add a new checkbox if one already exists
        """
        # Check input
        assert isinstance(state, bool)

        # Check if cellWidget exists
        if self.cellWidget(row, col):
            # existing: just set the value
            self.cellWidget(row, col).setChecked(state)
        else:
            # case to add checkbox
            checkbox = QtGui.QCheckBox()
            checkbox.setText('')
            checkbox.setChecked(state)

            # Adding a widget which will be inserted into the table cell
            # then centering the checkbox within this widget which in turn,
            # centers it within the table column :-)
            self.setCellWidget(row, col, checkbox)
        # END-IF-ELSE

        return

    def set_status_column_name(self, name):
        """
        Purpose: if the status column's name is not 'Status' of this table,
                 then re-set the colun name for the status row
        Requirements: given name must be a string and in _header
        :param name:
        :return:
        """
        # check
        assert isinstance(name, str), 'Given status column name must be an integer,' \
                                      'but not %s.' % str(type(name))
        assert name in self._myColumnNameList

        # set value
        self._statusColName = name

        return

    def set_value_cell(self, row, col, value=''):
        """
        Set value to a cell with integer, float or string
        :param row:
        :param col:
        :param value:
        :return:
        """
        # Check
        if row < 0 or row >= self.rowCount() or col < 0 or col >= self.columnCount():
            raise IndexError('Input row number or column number is out of range.')

        # Init cell
        cell_item = QtGui.QTableWidgetItem()
        cell_item.setText(_fromUtf8(str(value)))
        cell_item.setFlags(cell_item.flags() & ~QtCore.Qt.ItemIsEditable)

        self.setItem(row, col, cell_item)

        return

    def update_cell_value(self, row, col, value):
        """

        :param row:
        :param col:
        :param value:
        :return:
        """
        cell_item = self.item(row, col)
        cell_widget = self.cellWidget(row, col)

        if cell_item is not None and cell_widget is None:
            # TableWidgetItem
            assert isinstance(cell_item, QtGui.QTableWidgetItem)
            if isinstance(value, float):
                cell_item.setText(_fromUtf8('%.7f' % value))
            else:
                cell_item.setText(_fromUtf8(str(value)))
        elif cell_item is None and cell_widget is not None:
            # TableCellWidget
            if isinstance(cell_widget, QtGui.QCheckBox) is True:
                cell_widget.setChecked(value)
            else:
                raise TypeError('Cell of type %s is not supported.' % str(type(cell_item)))
        else:
            raise TypeError('Table cell (%d, %d) is in an unsupported situation!' % (row, col))

        return
