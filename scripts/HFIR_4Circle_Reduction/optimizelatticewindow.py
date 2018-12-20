# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=C0103
from __future__ import (absolute_import, division, print_function)
from qtpy.QtWidgets import (QMainWindow)
from qtpy.QtCore import Signal as pyqtSignal
from mantid.kernel import Logger
try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("HFIR_4Circle_Reduction").information('Using legacy ui importer')
    from mantidplot import load_ui


class OptimizeLatticeWindow(QMainWindow):
    """
    Main window widget to set up parameters to optimize
    """

    # establish signal for communicating from App2 to App1 - must be defined before the constructor
    mySignal = pyqtSignal(int)

    def __init__(self, parent=None):
        """
        Initialization
        :param parent:
        :return:
        """
        # init
        QMainWindow.__init__(self, parent)

        ui_path = "OptimizeLattice.ui"
        self.ui = load_ui(__file__, ui_path, baseinstance=self)

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
        self.ui.pushButton_Ok.clicked.connect(self.do_ok)
        self.ui.pushButton_cancel.clicked.connect(self.do_quit)

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
