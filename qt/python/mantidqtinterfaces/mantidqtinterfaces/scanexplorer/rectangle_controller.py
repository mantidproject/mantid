from qtpy.QtWidgets import QTableWidget, QTableWidgetItem, QWidget, QVBoxLayout

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
                controller.remove_from(self.table, index)
                self.controllers.pop(index)
                return
        logger.debug('No controller found for deleted rectangle.')


class RectangleController:
    """
    A table containing data that represents one of the shown rectangles
    """

    def __init__(self, x0: float = 0, y0: float = 0, x1: float = 0, y1: float = 0):

        self.fields = [DoubleProperty('x0', x0),
                       DoubleProperty('y0', y0),
                       DoubleProperty('x1', x1),
                       DoubleProperty('y1', y1)]

    def insert_in(self, table: QTableWidget):
        """
        Insert the controller at the end of the provided table
        @param table: the table in which to insert the controller
        """
        # add a header to separate the data from the previous one
        table.insertRow(table.rowCount())
        table.setItem(table.rowCount() - 1, 0, QTableWidgetItem("Parameter"))
        table.setItem(table.rowCount() - 1, 1, QTableWidgetItem("Value"))

        # add the data
        for field in self.fields:
            table.insertRow(table.rowCount())
            item = QTableWidgetItem(field.name)
            table.setItem(table.rowCount() - 1, 0, item)
            item = QTableWidgetItem(field.value_as_string())
            table.setItem(table.rowCount() - 1, 1, item)

    def remove_from(self, table: QTableWidget, index: int):
        """
        Remove the rectangle controller from the table.
        @param table: the table from which to remove the rectangle.
        @param index: the index of the controller in the list of controllers.
        """
        for _ in range(len(self.fields) + 1):
            table.removeRow(index * len(self.fields))

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
        self._name = name
        self._value = value

    @property
    def name(self):
        return self._name

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, new_value: float):
        assert(type(new_value) == float)
        self._value = new_value

    def value_as_string(self, precision: int = 5) -> str:
        """
        Return the held value as a string with a given number of decimal places
        @param precision: the number of decimal places requested
        @return the value with that precision, as a string
        """
        formatting = "{: ." + str(precision) + "f}"
        return formatting.format(self.value)

    def __eq__(self, other):
        return self.name == other.name and self.value == other.value
