from qtpy.QtWidgets import QTableWidget, QTableWidgetItem, QWidget, QVBoxLayout


class RectanglesManager(QWidget):
    """
    A widget holding all the rectangle controllers currently active
    """
    def __init__(self, parent=None):
        super(RectanglesManager, self).__init__(parent=parent)
        self.controllers = []
        self.setLayout(QVBoxLayout())

    def add_controller(self, x0, y0, x1, y1):
        """
        Add a rectangle controller with given initial values
        @param x0: the x coordinate of a corner of the rectangle
        @param y0: the y coordinate of that corner
        @param x1: the x coordinate of the opposing corner
        @param y1: the y coordinate of that opposing corner
        """
        controller = RectangleController(self, x0, y0, x1, y1)
        self.layout().addWidget(controller)
        self.controllers.append(controller)


class RectangleController(QTableWidget):
    """
    A table containing data that represents one of the shown rectangles
    """

    def __init__(self, parent=None, x0=0, y0=0, x1=0, y1=0):
        super(QTableWidget, self).__init__(parent=parent)
        self.setRowCount(4)
        self.setColumnCount(2)

        self.verticalHeader().hide()
        self.horizontalHeader().hide()

        self.fields = [DoubleProperty('x0', x0),
                       DoubleProperty('y0', y0),
                       DoubleProperty('x1', x1),
                       DoubleProperty('y1', y1)]

        for row_index, field in enumerate(self.fields):
            item = QTableWidgetItem(field.name)
            self.setItem(row_index, 0, item)
            item = QTableWidgetItem(field.value_as_string())
            self.setItem(row_index, 1, item)


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
