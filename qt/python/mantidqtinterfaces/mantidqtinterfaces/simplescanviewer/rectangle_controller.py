# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QTableWidget, QTableWidgetItem, QWidget, QVBoxLayout
from qtpy.QtCore import Qt, Signal

from matplotlib import patches

from mantid.kernel import logger


class RectanglesManager(QWidget):
    """
    A widget holding all the rectangle controllers currently active
    """

    # signal sent when a rectangle is updated, with the updated patch
    sig_controller_updated = Signal(patches.Rectangle)

    def __init__(self, parent=None):
        super(RectanglesManager, self).__init__(parent=parent)

        self.rectangles = []  # list of all the rectangles currently managed
        self.current_rectangle_index = -1

        # function which returns additional info to put in the peak table
        self.additional_peaks_info = lambda *args: dict()

        self.table = QTableWidget()
        self.table.setColumnCount(2)

        self.setLayout(QVBoxLayout())
        self.layout().addWidget(self.table)

        self.table.cellChanged.connect(self.on_field_changed)
        self.table.itemSelectionChanged.connect(self.save_current_cell_value)
        self.manage_table_width()

    def add_rectangle(self, rectangle: patches.Rectangle):
        """
        Add a rectangle controller with given initial values
        @param rectangle: the rectangle patch to add
        """
        controller = RectangleController(*get_rectangle_corners(rectangle))
        controller.insert_in(self.table)
        self.rectangles.append([controller, rectangle])
        self.current_rectangle_index = len(self.rectangles) - 1
        self.manage_table_width()

    def find_controller(self, x0: float, y0: float, x1: float, y1: float) -> int:
        """
        Find the controller with given coordinates. If there are multiple at the same position,
        only the first one will be returned.
        @param x0: the x coordinate of a corner of the rectangle
        @param y0: the y coordinate of that corner
        @param x1: the x coordinate of the opposing corner
        @param y1: the y coordinate of that opposing corner
        @return the position of the controller in the list of held controllers, or -1 if not there
        """
        expected_controller = RectangleController(x0, y0, x1, y1)
        for index, (controller, _) in enumerate(self.rectangles):
            if controller == expected_controller:
                return index
        return -1

    def find_controllers(self, x0: float, y0: float, x1: float, y1: float) -> list:
        """
        Find all the controllers with given coordinates. Useful when wanting to update all ROIs at the exact same
        position (e.g. not often but can happen).
        @param x0: the x coordinate of a corner of the rectangle
        @param y0: the y coordinate of that corner
        @param x1: the x coordinate of the opposing corner
        @param y1: the y coordinate of that opposing corner
        @return a list of the controllers' position in the list of held controllers, or [] if not there
        """
        expected_controller = RectangleController(x0, y0, x1, y1)
        return [index for index, (controller, _) in enumerate(self.rectangles) if controller == expected_controller]

    @staticmethod
    def find_rectangle_index_at_row(row: int) -> int:
        """
        Return the index of the rectangle which owns that row
        @param row: the row of the item looked for
        @return the index of the rectangle
        """
        return row // RectangleController.NUMBER_OF_ROWS

    def set_as_current_rectangle(self, rectangle: patches.Rectangle):
        """
        Set the controller at given coordinates as the current one.
        @param rectangle: the new current rectangle
        """
        index = self.find_controller(*get_rectangle_corners(rectangle))
        self.current_rectangle_index = index

    def on_current_updated(self):
        """
        Move the currently selected rectangle to the new position
        """
        if self.current_rectangle_index == -1:
            return

        controller, rectangle = self.rectangles[self.current_rectangle_index]
        controller.update_values(*get_rectangle_corners(rectangle))

    def save_current_cell_value(self):
        """
        When a field is selected by the user, its current value is stored in case the user provides a bad input
        (e.g. text)
        """
        if len(self.table.selectedIndexes()) == 0:
            return
        selected_cell = self.table.selectedIndexes()[0]
        row, col = selected_cell.row(), selected_cell.column()

        if col != 1:
            return

        controller, _ = self.rectangles[self.find_rectangle_index_at_row(row)]
        controller.save_old_values()

    def on_field_changed(self, row: int, _):
        """
        Slot triggered when a field of the table is changed. Updates the patch accordingly.
        @param row: the row in which the item changed is
        """
        index = self.find_rectangle_index_at_row(row)
        if index >= len(self.rectangles):
            return

        controller, rectangle = self.rectangles[index]
        x0, y0, x1, y1 = controller.get_values()
        controller.save_old_values()
        rectangle.set_bounds(x0, y0, x1 - x0, y1 - y0)
        self.sig_controller_updated.emit(rectangle)

    def manage_table_width(self):
        """
        Manage the table fixed width, depending if there are rows, so it takes exactly the width it needs.
        """

        if self.table.rowCount() == 0:
            # set the width of the table to be the size of its columns + the size of the separation between them
            self.table.setFixedWidth(sum(self.table.columnWidth(i) for i in range(self.table.columnCount())) + 2)
        else:
            # set the table to the width of the columns + the width of the separation between them
            # + some offset which I think comes from the row numbers
            self.table.setFixedWidth(sum(self.table.columnWidth(i) for i in range(self.table.columnCount())) + 2 + 11)

    def get_current_rectangle(self) -> patches.Rectangle:
        """
        Get the current rectangle object
        @return the rectangle patch
        """
        if not self.current_rectangle_index == -1:
            return self.rectangles[self.current_rectangle_index][1]

    def get_rectangles(self) -> list:
        """
        Get the rectangle patches currently held
        @return: the list of those rectangles
        """
        return [rectangle for _, rectangle in self.rectangles]

    def delete_current(self):
        """
        Delete the currently selected shape.
        """
        if self.current_rectangle_index == -1:
            logger.debug("No current controller, cannot delete it.")
            return

        controller, rectangle = self.rectangles.pop(self.current_rectangle_index)
        controller.remove_from(self.table)
        rectangle.remove()

        self.current_rectangle_index = -1
        self.manage_table_width()

    def clear(self):
        """
        Remove all currently shown shapes.
        """
        for controller, rectangle in self.rectangles:
            controller.remove_from(self.table)
            rectangle.remove()
        self.rectangles = []
        self.current_rectangle_index = -1
        self.manage_table_width()


class RectangleController:
    """
    A table containing data that represents one of the shown rectangles
    """

    NUMBER_OF_ROWS = 5

    def __init__(self, x0: float = 0, y0: float = 0, x1: float = 0, y1: float = 0):

        self.header = None
        self.set_header_items()

        self.fields = [DoubleProperty('x0', x0),
                       DoubleProperty('y0', y0),
                       DoubleProperty('x1', x1),
                       DoubleProperty('y1', y1)]

        self.peak_plot = None

    def set_header_items(self):
        """
        Set the items defining the header of the rectangle controller
        """
        parameter_item = QTableWidgetItem("Parameter")
        value_item = QTableWidgetItem("Value")

        # set the flags so that the header are not editable
        parameter_item.setFlags(parameter_item.flags() & (~Qt.ItemIsEditable))
        value_item.setFlags(value_item.flags() & (~Qt.ItemIsEditable))

        self.header = [parameter_item, value_item]

    def insert_in(self, table: QTableWidget):
        """
        Insert the controller at the end of the provided table
        @param table: the table in which to insert the controller
        """
        # add a header to separate the data from the previous one
        table.insertRow(table.rowCount())

        for col_index, item in enumerate(self.header):
            table.setItem(table.rowCount() - 1, col_index, item)

        # add the data
        for field in self.fields:
            table.insertRow(table.rowCount())

            item = field.get_name_item()
            table.setItem(table.rowCount() - 1, 0, item)

            item = field.get_value_item()
            table.setItem(table.rowCount() - 1, 1, item)

    def remove_from(self, table: QTableWidget):
        """
        Remove the rectangle controller from the table.
        @param table: the table from which to remove the rectangle.
        """
        table.removeRow(self.header[0].row())

        for field in self.fields:
            table.removeRow(field.get_name_item().row())

        self.set_peak_plot(None)

    def update_values(self, new_x0, new_y0, new_x1, new_y1):
        """
        Update fields values to the new ones
        @param new_x0: the x coordinate of a new corner of the rectangle
        @param new_y0: the y coordinate of that corner
        @param new_x1: the x coordinate of the new opposing corner
        @param new_y1: the y coordinate of that opposing corner
        """
        self.fields[0].value = new_x0
        self.fields[1].value = new_y0
        self.fields[2].value = new_x1
        self.fields[3].value = new_y1

    def get_values(self):
        return (field.value for field in self.fields)

    def save_old_values(self):
        """
        Save values from all the fields
        """
        for field in self.fields:
            field.save_old_value()

    def set_peak_plot(self, peak_plot):
        """
        Store the peak plot object and remove the previous one from the figure if it exists
        @param peak_plot: the new plot to store
        """
        if self.peak_plot:
            self.peak_plot.remove()
        self.peak_plot = peak_plot

    def __eq__(self, other):
        for field_1, field_2 in zip(self.fields, other.fields):
            if field_1 != field_2:
                return False
        return True


class DoubleProperty:
    """
    A property which value is a double
    """

    def __init__(self, name: str = "", value: float = 0):
        self._name = QTableWidgetItem(name)
        # set the flags so the name of the parameter is not editable by the user
        self._name.setFlags(self._name.flags() & (~Qt.ItemIsEditable))

        self._value = QTableWidgetItem(self.value_as_string(value))
        self._old_value: float = value

    @property
    def name(self) -> str:
        return self._name.text()

    def get_name_item(self) -> QTableWidgetItem:
        return self._name

    @property
    def value(self) -> float:
        try:
            return float(self._value.text())
        except ValueError:
            self._value.setText(str(self._old_value))
            return self._old_value

    @value.setter
    def value(self, new_value: float):
        self._value.setText(self.value_as_string(new_value))

    def get_value_item(self) -> QTableWidgetItem:
        return self._value

    @staticmethod
    def value_as_string(value: float, precision: int = 5) -> str:
        """
        Return the given value as a string with a given number of decimal places
        @param value: the value to format
        @param precision: the number of decimal places requested
        @return the value with that precision, as a string
        """
        formatting = "{: ." + str(precision) + "f}"
        return formatting.format(value)

    def __eq__(self, other):
        return self.name == other.name and self.value == other.value

    def save_old_value(self):
        """
        Store the current value of the cell in case it needs to be reused later.
        """
        self._old_value = self.value


def get_rectangle_corners(rectangle: patches.Rectangle):
    """
    From a patch, get the corners coordinates
    @param rectangle: the rectangle from which to get the coordinates
    @return x0, y0, x1, y1 the coordinates of opposing corners
    """
    x0, y0 = rectangle.get_xy()
    return x0, y0, x0 + rectangle.get_width(), y0 + rectangle.get_height()
