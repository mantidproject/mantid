#
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

        self._myHeaderList = None
        self._myColumnTypeList = None

        return

    def append_row(self, row_value_list, type_list=None):
        """

        :param row_value_list:
        :return:
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
        else:
            ret_msg = ''

        # Insert new row
        row_number = self.rowCount()
        self.insertRow(row_number)

        # Set values
        for i_col in xrange(min(len(row_value_list), self.columnCount())):
            item = QtGui.QTableWidgetItem()
            item.setText(_fromUtf8(row_value_list[i_col]))
            item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
            if type_list[i_col] == 'checkbox':
                self.set_check_box(row_number, i_col, False)
            else:
                self.setItem(row_number, i_col, item)
        # END-FOR(i_col)

        return ret_msg

    def get_selected_rows(self):
        """

        :return:
        """
        rows_list = list()
        index_status = self._myColumnTypeList.index('checkbox')
        for i_row in xrange(self.rowCount()):
            is_checked = self.get_row_value(i_row)[index_status]
            if is_checked:
                rows_list.append(i_row)

        return rows_list

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
                value = str(item_i_j.text())
                if c_type == 'int':
                    value = int(value)
                elif c_type == 'float':
                    value = float(value)

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
        self._myHeaderList = list()
        self._myColumnTypeList = list()

        for c_tup in column_tup_list:
            c_name = c_tup[0]
            c_type = c_tup[1]
            self._myHeaderList.append(c_name)
            self._myColumnTypeList.append(c_type)

        self.setColumnCount(num_cols)
        self.setHorizontalHeaderLabels(self._myHeaderList)

        return

    def set_check_box(self, row, col, state):
        """ function to add a new select checkbox to a cell in a table row
        won't add a new checkbox if one already exists
        """
        # Check input
        assert isinstance(state, bool)

        # Check if cellWidget exists
        if self.cellWidget(row,col):
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
