#pylint: disable=C0103
from PyQt4 import QtGui, QtCore

import ui_OptimizeLattice


class OptimizeLatticeWindow(QtGui.QMainWindow):
    """
    Main window widget to set up parameters to optimize
    """

    # establish signal for communicating from App2 to App1 - must be defined before the constructor
    mySignal = QtCore.pyqtSignal(int)

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

        self.ui.comboBox_ubSource.addItems(['Tab - Calculate UB Matrix', 'Tab - Accepted UB Matrix'])

        self.ui.lineEdit_tolerance.setText('0.12')

        # define event handling
        self.connect(self.ui.pushButton_Ok, QtCore.SIGNAL('clicked()'),
                     self.do_ok)

        self.connect(self.ui.pushButton_cancel, QtCore.SIGNAL('clicked()'),
                     self.do_quit)

        if parent is not None:
            # connect to the method to refine UB matrix by constraining lattice parameters
            self.mySignal.connect(parent.refine_ub_lattice)

        # flag to trace back its previous step
        self._prevIndexByFFT = False

        return

    def do_ok(self):
        """
        User decide to go on and then send a signal to parent
        :return:
        """

        tolerance = self.get_tolerance()
        if tolerance is None:
            raise RuntimeError('Tolerance cannot be left blank!')

        # set up a hand-shaking signal
        signal_value = 1000
        self.mySignal.emit(signal_value)

        # quit
        self.do_quit()

        return

    def do_quit(self):
        """
        Quit the window
        :return:
        """
        self.close()

        return

    def get_unit_cell_type(self):
        """
        Get the tolerance
        :return:
        """
        unit_cell_type = str(self.ui.comboBox_unitCellTypes.currentText())

        return unit_cell_type

    def get_tolerance(self):
        """
        Get the tolerance for refining UB matrix with unit cell type.
        :return:
        """
        tol_str = str(self.ui.lineEdit_tolerance.text()).strip()

        if len(tol_str) == 0:
            # blank: return None
            tol = None
        else:
            tol = float(tol_str)

        return tol

    def get_ub_source(self):
        """
        Get the index of the tab where the UB matrix comes from
        :return:
        """
        source = str(self.ui.comboBox_ubSource.currentText())

        if source == 'Tab - Calculate UB Matrix':
            tab_index = 3
        else:
            tab_index = 4

        return tab_index

    def set_prev_ub_refine_method(self, use_fft=False):
        """

        :param use_fft:
        :return:
        """
        self._prevIndexByFFT = use_fft

        return
