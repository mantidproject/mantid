# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import PyQt4.QtGui  as QtGui
import PyQt4.QtCore as QtCore


class View(QtGui.QDialog):
    # In PyQt signals are the first thing to be defined in a class:
    displaySignal = QtCore.pyqtSignal()
    btnSignal = QtCore.pyqtSignal()

    def __init__(self, parent=None):
        # Call QDialog's constructor
        super(View, self).__init__(parent)

        # Initialise the widgets for the view (this can also be done from Qt Creator
        self.table = QtGui.QTableWidget()
        self.table.setWindowTitle("MVP Demo")
        self.table.resize(400, 250)
        self.table.setRowCount(5)
        self.table.setColumnCount(2)
        self.table.setHorizontalHeaderLabels(QtCore.QString("name;value;").split(";"))

        # Set display values in the widgets
        keys = ['value 1', 'operation', 'value 2', 'display', 'result']
        self.combo = {}
        self.create_combo_table(1, 1, 'operations')
        self.create_combo_table(3, 1, 'display')
        for row in range(len(keys)):
            self.set_names(keys[row], row)

        # Initialise layout of the widget and add child widgets to it
        grid = QtGui.QGridLayout()
        grid.addWidget(self.table)

        self.button = QtGui.QPushButton('Calculate', self)
        self.button.setStyleSheet("background-color:lightgrey")
        grid.addWidget(self.button)

        # Connect button click handler method to the button's 'clicked' signal
        self.button.clicked.connect(self.btn_click)
        # connect method to handle combo box selection changing to the corresponding signal
        self.combo['display'].currentIndexChanged.connect(self.display_changed)

        # Set the layout for the view widget
        self.setLayout(grid)

    # The next two methods handle the signals connected to in the presenter
    # They emit custom signals that can be caught by a presenter
    def btn_click(self):
        self.btnSignal.emit()

    def display_changed(self):
        self.displaySignal.emit()

    # Populate view
    def create_combo_table(self, row, col, key):
        self.combo[key] = QtGui.QComboBox()
        options = ['test']
        self.combo[key].addItems(options)
        self.table.setCellWidget(row, col, self.combo[key])

    # The next 5 methods update the appearance of the view.

    def set_options(self, key, options):
        self.combo[key].clear()
        self.combo[key].addItems(options)

    def set_names(self, name, row):
        text = QtGui.QTableWidgetItem(name)
        text.setFlags(QtCore.Qt.ItemIsEnabled)
        self.table.setItem(row, 0, text)

    # Update the view with the result of a calculation
    def setResult(self, value):
        self.table.setItem(4, 1, QtGui.QTableWidgetItem(str(value)))

    def hide_display(self):
        self.table.setRowHidden(4, True)

    def show_display(self):
        self.table.setRowHidden(4, False)

    # Finally, we have the get methods to allow the presenter to read the user's input

    def get_value(self, row):
        return float(self.table.item(row, 1).text())

    def get_operation(self):
        return self.combo['operations'].currentText()

    def get_display(self):
        return self.combo["display"].currentText()
