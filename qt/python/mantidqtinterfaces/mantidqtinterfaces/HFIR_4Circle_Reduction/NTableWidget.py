# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0103,R0904
# N(DAV)TableWidget
import csv
from qtpy.QtWidgets import QCheckBox, QTableWidget, QTableWidgetItem
from qtpy import QtCore
import qtpy  # noqa


def _fromUtf8(s):
    return s


class NTableWidget(QTableWidget):
    """
    NdavTableWidget inherits from QTableWidget by extending the features
    for easy application.
    """

    # List of supported cell types (all in lower cases)
    Supported_Cell_Types = ["checkbox", "string", "str", "integer", "int", "float", "double"]

    def __init__(self, parent):
        """

        :param parent:
        :return:
        """
        QTableWidget.__init__(self, parent)

        self._myParent = parent

        self._myColumnNameList = None
        self._myColumnTypeList = None
        self._editableList = list()

        self._statusColName = "Status"
        self._colIndexSelect = None

        return

    def append_row(self, row_value_list, type_list=None):
        """
        append a row to the table
        :param row_value_list: row_value_list
        :param type_list:
        :return:  2-tuple as (boolean, message)
        """
        # Check input
        assert isinstance(row_value_list, list), "Row values {0} must be given by a list but not a {1}".format(
            row_value_list, type(row_value_list)
        )
        if type_list is not None:
            assert isinstance(type_list, list), "Value types {0} must be given by a list but not a {1}".format(type_list, type(type_list))
            if len(row_value_list) != len(type_list):
                raise RuntimeError(
                    "If value types are given, then they must have the same numbers ({0}) and values ({1})".format(
                        len(row_value_list), len(type_list)
                    )
                )
        else:
            type_list = self._myColumnTypeList

        if len(row_value_list) != self.columnCount():
            ret_msg = "Input number of values (%d) is different from column number (%d)." % (len(row_value_list), self.columnCount())
            return False, ret_msg
        else:
            ret_msg = ""

        # Insert new row
        row_number = self.rowCount()
        self.insertRow(row_number)

        # Set values
        for i_col in range(min(len(row_value_list), self.columnCount())):
            item = QTableWidgetItem()
            if row_value_list[i_col] is None:
                item_value = ""
            else:
                item_value = str(row_value_list[i_col])
            item.setText(_fromUtf8(item_value))
            item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
            # Set editable flag! item.setFlags(item.flags() | ~QtCore.Qt.ItemIsEditable)
            if type_list[i_col] == "checkbox":
                self.set_check_box(row_number, i_col, False)
            else:
                self.setItem(row_number, i_col, item)
        # END-FOR(i_col)

        return True, ret_msg

    def delete_rows(self, row_number_list):
        """Delete rows
        :param row_number_list:
        :return:
        """
        # Check and re-order row numbers
        assert isinstance(row_number_list, list)
        row_number_list.sort(reverse=True)

        for row_number in row_number_list:
            self.removeRow(row_number)

        return

    def export_table_csv(self, csv_file_name):
        """Export table to a CSV fie
        :param csv_file_name:
        :return:
        """
        # get title as header
        col_names = self._myColumnNameList[:]
        # col_names_str = '{0}'.format(col_names)
        # col_names_str = col_names_str.replace(', ', ' ')
        # headeder = col_names_str

        num_columns = self.columnCount()

        num_rows = self.rowCount()
        content_line_list = list()
        for i_row in range(num_rows):
            line_items = list()
            for j_col in range(num_columns):
                item_value = self.get_cell_value(i_row, j_col)
                if isinstance(item_value, str):
                    # remove tab because tab will be used as delimiter
                    item_value = item_value.replace("\t", "")
                elif item_value is None:
                    item_value = ""
                line_items.append(item_value)
            # END-FOR
            content_line_list.append(line_items)
        # END-FOR (row)

        with open(csv_file_name, "w") as csv_file:
            csv_writer = csv.writer(csv_file, delimiter="\t", quoting=csv.QUOTE_MINIMAL)
            # write header
            csv_writer.writerow(col_names)
            # write content
            for line_items in content_line_list:
                csv_writer.writerow(line_items)
            # END-FOR
        # END-WITH

        return

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
        assert isinstance(row_index, int), "Row index {0} must be an integer".format(row_index)
        assert isinstance(col_index, int), "Column index {0} must be an integer".format(col_index)
        if not 0 <= row_index < self.rowCount():
            raise RuntimeError("Row index {0} is out of range [0, {1})".format(row_index, self.rowCount()))
        if not 0 <= col_index < self.columnCount():
            raise RuntimeError("Column index {0} is out of range [0, {1})".format(col_index, self.columnCount()))

        # get cell type
        cell_data_type = self._myColumnTypeList[col_index]

        if cell_data_type == "checkbox":
            # Check box
            cell_i_j = self.cellWidget(row_index, col_index)
            # PyQt5 compatible issue!
            assert isinstance(cell_i_j, QCheckBox), "Cell {0} {1} must be of type QCheckBox but not a {2}".format(
                row_index, col_index, type(cell_i_j)
            )

            return_value = cell_i_j.isChecked()
        else:
            # Regular cell for int, float or string
            item_i_j = self.item(row_index, col_index)
            assert isinstance(item_i_j, QTableWidgetItem), "Cell {0} {1} must be of type QTableWidgetItem but not a {2}".format(
                row_index, col_index, type(item_i_j)
            )

            # get the string of the cell
            return_value = str(item_i_j.text()).strip()

            # cast to supported
            if return_value == "None" or len(return_value) == 0:
                # None case
                return_value = None
            elif cell_data_type.startswith("str"):
                # case as str of string
                pass
            elif cell_data_type.startswith("int"):
                # integer
                try:
                    return_value = int(return_value)
                except ValueError as val_err:
                    raise RuntimeError(
                        'Unable to convert cell ({0}, {1}) with value "{2}" to integer due to {3}.'.format(
                            row_index, col_index, return_value, val_err
                        )
                    )
            elif cell_data_type == "float" or cell_data_type == "double":
                # float or double
                try:
                    return_value = float(return_value)
                except ValueError as val_err:
                    raise RuntimeError(
                        'Unable to convert cell ({0}, {1}) with value "{2}" to float due to {3}.' "".format(
                            row_index, col_index, return_value, val_err
                        )
                    )
            # END-IF-ELSE
        # END-IF-ELSE

        return return_value

    def get_column_index(self, column_name):
        """
        Get column index by column name
        Guarantees: return column index
        :param column_name:
        :return:
        """
        assert isinstance(column_name, str)

        return self._myColumnNameList.index(column_name)

    def get_row_value(self, row_index):
        """
        :param row_index:
        :return: list of objects
        """
        if row_index < 0 or row_index >= self.rowCount():
            raise IndexError("Index of row (%d) is out of range." % row_index)

        ret_list = list()
        for i_col in range(len(self._myColumnTypeList)):
            c_type = self._myColumnTypeList[i_col]

            if c_type == "checkbox":
                # Check box
                cell_i_j = self.cellWidget(row_index, i_col)
                assert isinstance(cell_i_j, QCheckBox)
                is_checked = cell_i_j.isChecked()
                ret_list.append(is_checked)
            else:
                # Regular cell
                item_i_j = self.item(row_index, i_col)
                assert isinstance(item_i_j, QTableWidgetItem)
                value = str(item_i_j.text()).strip()
                if len(value) > 0:
                    if c_type == "int":
                        value = int(value)
                    elif c_type == "float":
                        value = float(value)
                else:
                    value = None

                ret_list.append(value)
            # END-IF-ELSE
        # END-FOR

        return ret_list

    def get_selected_rows(self, status=True):
        """Get the rows whose status is same as given status
        Requirements: given status must be a boolean
        Guarantees: a list of row indexes are constructed for those rows that meet the requirement.
        :param status:
        :return: list of row indexes that are selected
        """
        # check
        assert isinstance(status, bool)
        assert self._statusColName is not None
        index_status = self._myColumnNameList.index(self._statusColName)

        # loop over all the rows
        row_index_list = list()
        for i_row in range(self.rowCount()):
            # check status
            is_checked = self.get_cell_value(i_row, index_status)
            if is_checked == status:
                row_index_list.append(i_row)

        return row_index_list

    def init_setup(self, column_tup_list):
        """Initial setup
        :param column_tup_list: list of 2-tuple as string (column name) and string (data type)
        :return:
        """
        # Check requirements
        assert isinstance(column_tup_list, list)
        assert len(column_tup_list) > 0

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

        # Set the editable flags
        self._editableList = [False] * num_cols

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
        for i_row in range(1, num_rows + 1):
            self.removeRow(num_rows - i_row)

        return

    def remove_rows(self, row_number_list):
        """Remove row number
        :param row_number_list:
        :return: string as error message
        """
        assert isinstance(row_number_list, list)
        row_number_list.sort(reverse=True)

        error_message = ""
        for row_number in row_number_list:
            if row_number >= self.rowCount():
                error_message += "Row %d is out of range.\n" % row_number
            else:
                self.removeRow(row_number)
        # END-FOR

        return error_message

    def revert_selection(self):
        """
        revert the selection of rows
        :return:
        """
        # check
        if self._colIndexSelect is None:
            raise RuntimeError("Column for selection is not defined yet. Unable to revert selection")

        num_rows = self.rowCount()
        for i_row in range(num_rows):
            curr_selection = self.get_cell_value(i_row, self._colIndexSelect)
            self.update_cell_value(i_row, self._colIndexSelect, not curr_selection)
        # END-FOR

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
        for row_index in range(num_rows):
            if self.get_cell_value(row_index, status_col_index) != status:
                self.update_cell_value(row_index, status_col_index, status)
        # END-FOR

        return

    def select_row(self, row_index, status=True):
        """
        Select a row
        :param row_index:
        :param status:
        :return:
        """
        # get column  index
        try:
            status_col_index = self._myColumnNameList.index(self._statusColName)
        except ValueError as e:
            # status column name is not properly set up
            return False, str(e)

        # Loop over all rows. If any row's status is not same as target status, then set it
        num_rows = self.rowCount()
        assert isinstance(row_index, int) and 0 <= row_index < num_rows, "Row number %s of type %s is not right." % (
            str(row_index),
            type(row_index),
        )

        if self.get_cell_value(row_index, status_col_index) != status:
            self.update_cell_value(row_index, status_col_index, status)

        return

    def select_rows_by_column_value(self, column_index, target_value, value_tolerance, keep_current_selection):
        """
        select row
        :param column_index:
        :param target_value:
        :param value_tolerance:
        :param keep_current_selection:
        :return:
        """
        # check inputs
        assert (
            isinstance(column_index, int) and 0 <= column_index < self.columnCount()
        ), "Column index {0} must be an integer (now {1}) and in range (0, {2}]".format(
            column_index, type(column_index), self.columnCount()
        )
        if self._colIndexSelect is None:
            raise RuntimeError("Column for selection is never set up.")

        # loop over lines
        num_rows = self.rowCount()
        for i_row in range(num_rows):
            if keep_current_selection and self.get_cell_value(i_row, self._colIndexSelect) is False:
                # in case to keep and based on current selection, and this row is not selected, skip
                continue

            value_i = self.get_cell_value(i_row, column_index)
            if isinstance(target_value, str) and value_i == target_value:
                # in case of string
                self.update_cell_value(i_row, self._colIndexSelect, True)
            elif (isinstance(target_value, float) or isinstance(target_value, int)) and abs(value_i - target_value) < value_tolerance:
                # in case of integer or float, then test with consideration of tolerance
                self.update_cell_value(i_row, self._colIndexSelect, True)
        # END-FOR

        return

    def set_check_box(self, row, col, state):
        """function to add a new select checkbox to a cell in a table row
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
            checkbox = QCheckBox()
            checkbox.setText("")
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
        assert isinstance(name, str), "Given status column name must be an integer, but not %s." % str(type(name))
        if name not in self._myColumnNameList:
            raise RuntimeError("Input selection/status name {0} is not in column names list {1}.".format(name, self._myColumnNameList))

        # set value
        self._statusColName = name

        # set the column index
        self._colIndexSelect = self._myColumnNameList.index(name)

        return

    def set_value_cell(self, row, col, value=""):
        """
        Set value to a cell with integer, float or string
        :param row:
        :param col:
        :param value:
        :return:
        """
        # Check
        if row < 0 or row >= self.rowCount() or col < 0 or col >= self.columnCount():
            raise IndexError("Input row number or column number is out of range.")

        # Init cell
        cell_item = QTableWidgetItem()
        cell_item.setText(_fromUtf8(str(value)))
        cell_item.setFlags(cell_item.flags() & ~QtCore.Qt.ItemIsEditable)

        self.setItem(row, col, cell_item)

        return

    def sort_by_column(self, column_index, sort_order=0):
        """
        Requirements:
            1. column index must be an integer within valid column range
            2. sort order will be either 0 (ascending) or 1 (descending)
        :param column_index:
        :param sort_order: 0 for ascending, 1 for descending
        :return:
        """
        # check
        assert isinstance(column_index, int), "column_index must be an integer but not %s." % str(type(column_index))
        if column_index < 0:
            column_index += len(self._myColumnNameList)
        assert 0 <= column_index < len(self._myColumnNameList), "Column index %d is out of range." % column_index

        assert isinstance(sort_order, int), "sort_order must be integer but not %s." % str(type(sort_order))
        assert sort_order == 0 or sort_order == 1

        # get rows
        num_rows = self.rowCount()
        row_content_dict = dict()
        for i_row in range(num_rows):
            row_items = self.get_row_value(i_row)
            key_value = self.get_cell_value(i_row, column_index)
            row_content_dict[key_value] = row_items
        # END-FOR

        # sort keys
        reverse_order = False
        if sort_order == 1:
            reverse_order = True
        key_list = row_content_dict.keys()
        key_list.sort(reverse=reverse_order)

        # clear all rows
        self.remove_all_rows()

        # add rows back
        for key_value in key_list:
            self.append_row(row_content_dict[key_value])
        # END-FOR

        return

    def update_cell_value(self, row, col, value):
        """
        Update (NOT reset) the value of a cell
        :param row:
        :param col:
        :param value:
        :return: None
        """
        # Check
        assert isinstance(row, int) and 0 <= row < self.rowCount(), "Row %s (%s) must be an integer between 0 and %d." % (
            str(row),
            type(row),
            self.rowCount(),
        )
        assert isinstance(col, int) and 0 <= col < self.columnCount()

        cell_item = self.item(row, col)
        cell_widget = self.cellWidget(row, col)

        if cell_item is not None and cell_widget is None:
            # TableWidgetItem
            assert isinstance(cell_item, QTableWidgetItem)
            if isinstance(value, float):
                cell_item.setText(_fromUtf8("%.7f" % value))
            else:
                cell_item.setText(_fromUtf8(str(value)))
        elif cell_item is None and cell_widget is not None:
            # TableCellWidget
            if isinstance(cell_widget, QCheckBox) is True:
                cell_widget.setChecked(value)
            else:
                raise TypeError("Cell of type %s is not supported." % str(type(cell_item)))
        else:
            raise TypeError("Table cell (%d, %d) is in an unsupported situation!" % (row, col))

        return
