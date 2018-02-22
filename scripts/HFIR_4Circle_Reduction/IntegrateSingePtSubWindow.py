#pylint: disable=C0103
from __future__ import (absolute_import, division, print_function)
from PyQt4 import QtGui, QtCore

from . import ui_OptimizeLattice


class IntegrateSinglePtIntensityWindow(QtGui.QMainWindow):
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

    # TODO FIXME NOW NOW2 - From here
    def integrate_roi_linear(self, exp_number, scan_number, pt_number, output_dir):
        """
        integrate the 2D data inside region of interest along both axis-0 and axis-1 individually.
        and the result (as 1D data) will be saved to ascii file.
        the X values will be the corresponding pixel index either along axis-0 or axis-1
        :return:
        """
        def save_to_file(base_file_name, axis, array1d, start_index):
            """
            save the result (1D data) to an ASCII file
            :param base_file_name:
            :param axis:
            :param array1d:
            :param start_index:
            :return:
            """
            file_name = '{0}_axis_{1}.dat'.format(base_file_name, axis)

            wbuf = ''
            vec_x = np.arange(len(array1d)) + start_index
            for x, d in zip(vec_x, array1d):
                wbuf += '{0} \t{1}\n'.format(x, d)

            ofile = open(file_name, 'w')
            ofile.write(wbuf)
            ofile.close()

            return

        matrix = self.array2d
        assert isinstance(matrix, np.ndarray), 'A matrix must be an ndarray but not {0}.'.format(type(matrix))

        # get region of interest
        if self._roiStart is None:
            self._roiStart = (0, 0)
        if self._roiEnd is None:
        

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
