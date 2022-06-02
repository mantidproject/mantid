from qtpy.QtWidgets import QTableWidget, QTableWidgetItem, QWidget, QVBoxLayout
from qtpy.QtCore import Qt

from mantid.kernel import logger


class RectanglesManager(QWidget):
    """
    A widget holding all the rectangle controllers currently active
    """
    def __init__(self, parent=None):
        super(RectanglesManager, self).__init__(parent=parent)
        self.controllers = []  # list of all the controllers currently managed
        self.table = QTableWidget()
        self.table.setColumnCount(2)
        self.setLayout(QVBoxLayout())
        self.layout().addWidget(self.table)

    def add_controller(self, x0: float, y0: float, x1: float, y1: float):
        """
        Add a rectangle controller with given initial values
        @param x0: the x coordinate of a corner of the rectangle
        @param y0: the y coordinate of that corner
        @param x1: the x coordinate of the opposing corner
        @param y1: the y coordinate of that opposing corner
        """
        controller = RectangleController(x0, y0, x1, y1)
        controller.insert_in(self.table)
        self.controllers.append(controller)

    def remove_controller(self, x0: float, y0: float, x1: float, y1: float):
        """
        Remove controller corresponding to the rectangle at those coordinates
        @param x0: the x coordinate of a corner of the rectangle
        @param y0: the y coordinate of that corner
        @param x1: the x coordinate of the opposing corner
        @param y1: the y coordinate of that opposing corner
        """
        expected_fields = RectangleController(x0, y0, x1, y1)
        for index, controller in enumerate(self.controllers):
            if controller == expected_fields:
                controller.remove_from(self.table)
                self.controllers.pop(index)
                return
        logger.debug('No controller found for deleted rectangle.')


class RectangleController:
    """
    A table containing data that represents one of the shown rectangles
    """

    def __init__(self, x0: float = 0, y0: float = 0, x1: float = 0, y1: float = 0):

        self.header = None
        self.set_header_items()

        self.fields = [DoubleProperty('x0', x0),
                       DoubleProperty('y0', y0),
                       DoubleProperty('x1', x1),
                       DoubleProperty('y1', y1)]

    def set_header_items(self):
        """
        Set the items defining the header of the rectangle controller
        """
        parameter_item = QTableWidgetItem("Parameter")
        parameter_item.setFlags(parameter_item.flags() & (~Qt.ItemIsEditable))

        value_item = QTableWidgetItem("Value")
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

    def __eq__(self, other):
        for field_1, field_2 in zip(self.fields, other.fields):
            if field_1 != field_2:
                return False
        return True

    def update_values(self, new_x0, new_y0, new_x1, new_y1):
        pass


class DoubleProperty:
    """
    A property which value is a double
    """

    def __init__(self, name: str = "", value: float = 0):
        self._name = QTableWidgetItem(name)
        self._name.setFlags(self._name.flags() & (~Qt.ItemIsEditable))

        self._value = QTableWidgetItem(self.value_as_string(value))

    @property
    def name(self) -> str:
        return self._name.text()

    def get_name_item(self) -> QTableWidgetItem:
        return self._name

    @property
    def value(self) -> float:
        return float(self._value.text())

    @value.setter
    def value(self, new_value: float):
        assert(type(new_value) == float)
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
