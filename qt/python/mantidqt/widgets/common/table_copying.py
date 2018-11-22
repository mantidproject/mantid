from __future__ import (absolute_import, division, print_function)

from qtpy import QtGui
from qtpy.QtCore import QPoint
from qtpy.QtGui import QCursor, QFont, QFontMetrics
from qtpy.QtWidgets import (QTableView, QToolTip)

NO_SELECTION_MESSAGE = "No selection"
COPY_SUCCESSFUL_MESSAGE = "Copy Successful"


def copy_spectrum_values(table, ws_read):
    """
    Copies the values selected by the user to the system's clipboard

    :param table: Table from which the selection will be read
    :param view:
    :param ws_read: The workspace read function, that is used to access the data directly
    """
    selection_model = table.selectionModel()
    if not selection_model.hasSelection():
        show_no_selection_to_copy_toast()
        return
    selected_rows = selection_model.selectedRows()  # type: list
    row_data = []

    for index in selected_rows:
        row = index.row()
        data = "\t".join(map(str, ws_read(row)))

        row_data.append(data)

    copy_to_clipboard("\n".join(row_data))
    show_successful_copy_toast()


def show_no_selection_to_copy_toast():
    show_mouse_toast(NO_SELECTION_MESSAGE)


def show_successful_copy_toast():
    show_mouse_toast(COPY_SUCCESSFUL_MESSAGE)


def copy_bin_values(table, ws_read, num_rows):
    selection_model = table.selectionModel()
    if not selection_model.hasSelection():
        show_no_selection_to_copy_toast()
        return
    selected_columns = selection_model.selectedColumns()  # type: list

    # Qt gives back a QModelIndex, we need to extract the column from it
    column_data = []
    for index in selected_columns:
        column = index.column()
        data = [str(ws_read(row)[column]) for row in range(num_rows)]
        column_data.append(data)

    all_string_rows = []
    for i in range(num_rows):
        # Appends ONE value from each COLUMN, this is because the final string is being built vertically
        # the noqa disables a 'data' variable redefined warning
        all_string_rows.append("\t".join([data[i] for data in column_data]))  # noqa: F812

    # Finally all rows are joined together with a new line at the end of each row
    final_string = "\n".join(all_string_rows)
    copy_to_clipboard(final_string)
    show_successful_copy_toast()


def copy_cells(table):
    """
    :type table: QTableView
    :param table: The table from which the data will be copied.
    :return:
    """
    selectionModel = table.selectionModel()
    if not selectionModel.hasSelection():
        show_no_selection_to_copy_toast()
        return

    selection = selectionModel.selection()
    # TODO show a warning if copying more cells than some number (100? 200? 300? 400??)
    selectionRange = selection.first()

    top = selectionRange.top()
    bottom = selectionRange.bottom()
    left = selectionRange.left()
    right = selectionRange.right()

    data = []
    index = selectionModel.currentIndex()
    for i in range(top, bottom + 1):
        for j in range(left, right):
            data.append(index.sibling(i, j).data())
            data.append("\t")
        data.append(index.sibling(i, right).data())
        data.append("\n")

    # strip the string to remove the trailing new line
    copy_to_clipboard("".join(data).strip())
    show_successful_copy_toast()


def keypress_copy(table, view, ws_read, num_rows):
    selectionModel = table.selectionModel()
    if not selectionModel.hasSelection():
        show_no_selection_to_copy_toast()
        return

    if len(selectionModel.selectedRows()) > 0:
        copy_spectrum_values(table, ws_read)
    elif len(selectionModel.selectedColumns()) > 0:
        copy_bin_values(table, ws_read, num_rows)
    else:
        copy_cells(table)


def show_mouse_toast(message):
    # Creates a text with empty space to get the height of the rendered text - this is used
    # to provide the same offset for the tooltip, scaled relative to the current resolution and zoom.
    font_metrics = QFontMetrics(QFont(" "))
    # The height itself is divided by 2 just to reduce the offset so that the tooltip is
    # reasonably position relative to the cursor
    QToolTip.showText(QCursor.pos() + QPoint(font_metrics.height() / 2, 0), message)


def copy_to_clipboard(data):
    """
    Uses the QGuiApplication to copy to the system clipboard.

    :type data: str
    :param data: The data that will be copied to the clipboard
    :return:
    """
    cb = QtGui.QGuiApplication.clipboard()
    cb.setText(data, mode=cb.Clipboard)
