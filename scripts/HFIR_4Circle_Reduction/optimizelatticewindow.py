#pylint: disable=C0103
import sys
from PyQt4 import QtGui, QtCore

import ui_OptimizeLattice


class OptimizeLatticeWindow(QtGui.QMainWindow):
    """
    Main window widget to set up parameters to optimize
    """
    def __init__(self, parent=None):
        """
        Initialization
        :param parent:
        :return:
        """
        # init
        QtGui.QMainWindow.__init__(self, parent)

        self.ui = ui_OptimizeLattice.Ui_MainWindow()
        self.ui.setupUi(self)

        # initialize widgets
        self.ui.comboBox_unitCellTypes.addItems(['Cubic',
                                                 'Tetragonal',
                                                 'Orthorhombic',
                                                 'Hexagonal',
                                                 'Rhombohedral',
                                                 'Monoclinic',
                                                 'Triclinic'])

        self.ui.lineEdit_tolerance.setText()

        # define event handling
        blabla

        return

    def get_unit_cell_type(self):
        """

        :return:
        """
        return

    def get_tolerance(self):
        """

        :return:
        """
        return

    def get_ub_source(self):
        """

        :return:
        """
        return tabX