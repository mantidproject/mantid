"""
Containing a set of classes used for finding (calculating and refining) UB matrix
"""
import os

import ui_AddUBPeaksDialog
import ui_UBSelectPeaksDialog
import guiutility

from PyQt4 import QtGui, QtCore


class AddScansForUBDialog(QtGui.QDialog):
    """
    Dialog class to add scans to UB scans' table for calculating and
    """
    def __init__(self, parent):
        """
        initialization
        :param parent: main GUI, reductionControl
        """
        super(AddScansForUBDialog, self).__init__(self)
        self._myParent = parent

        # set up UI
        self.ui = ui_AddUBPeaksDialog.Ui_Dialog()
        self.ui.setupUi(self)

        # initialize widgets
        self.ui.checkBox_loadHKLfromFile.setChecked(True)

        # define event handling
        self.connect(self.ui.pushButton_findPeak, QtCore.SIGNAL('clicked()'),
                     self.do_find_peak)
        self.connect(self.ui.pushButton_addPeakToCalUB, QtCore.SIGNAL('clicked()'),
                     self.do_add_single_scan)

        self.connect(self.ui.pushButton_loadScans, QtCore.SIGNAL('clicked()'),
                     self.do_load_scans)
        self.connect(self.ui.pushButton_addScans, QtCore.SIGNAL('clicked()'),
                     self.do_add_scans)

        self.connect(self.ui.pushButton_quit, QtCore.SIGNAL('clicked()'),
                     self.do_quit)

        return

    def do_add_scans(self):
        """
        add all the scans list in the 'plainTextEdit_scanList'
        :return:
        """
        scans_str = str(self.ui.plainTextEdit_scanList.getPaintContext())
        scan_list = guiutility.parse_integer_list(scans_str)

        XXX
        self._myParent.add_scans_ub_table(scan_list)

        return

    def do_add_single_scan(self):
        """

        :return:
        """
        # TODO/ISSUE/NOW - Implement!

        return

    def do_find_peak(self):
        """
        find the peak(s) in a merged scan
        :return:
        """
        # get scan number
        status, ret_obj = guiutility.parse_integers_editors([self.ui.lineEdit_scanNumber])
        if status:
            scan_number = ret_obj[0]
        else:
            # pop error message
            self._myParent.pop_one_button_dialog(ret_obj)
            return

        # load HKL from SPICE?
        hkl_from_spice = self.ui.checkBox_loadHKLfromFile.isChecked()

        # find peak
        status, ret_obj = self._myParent.find_peak_in_scan(scan_number, hkl_from_spice)

        # set the result
        if status:
            hkl, vec_q = ret_obj
            if len(hkl) > 0:
                self.ui.lineEdit_H.setText('%.2f' % hkl[0])
                self.ui.lineEdit_K.setText('%.2f' % hkl[1])
                self.ui.lineEdit_L.setText('%.2f' % hkl[2])

            self.ui.lineEdit_sampleQx.setText('%.5E' % vec_q[0])
            self.ui.lineEdit_sampleQy.setText('%.5E' % vec_q[1])
            self.ui.lineEdit_sampleQz.setText('%.5E' % vec_q[2])
        # END-IF

        return

    def do_load_scans(self):
        """
        load an ASCII file containing scan numbers,
        and the results are written to 'plainTextEdit_scanList'
        :return:
        """
        # get file name
        scan_file = str(self.ui.lineEdit_scansFile.text())
        if os.path.exists(scan_file) is False:
            raise RuntimeError('Scan file {0} does not exist.'.format(scan_file))

        # parse file
        exp_number, scans_str = guiutility.import_scans_text_file(scan_file)

        self.ui.plainTextEdit_scanList.setPlainText(scans_str)

        return

    def do_quit(self):
        """
        quit
        :return:
        """
        self.close()

        return


class SelectUBMatrixScansDialog(QtGui.QDialog):
    """
    Dialog to select scans for processing UB matrix
    """
    def __init__(self, parent):
        """
        initialization
        :param parent:
        """
        super(SelectUBMatrixScansDialog, self).__init__(parent)
        self._myParent = parent

        # set ui
        self.ui = ui_UBSelectPeaksDialog.Ui_Dialog()
        self.ui.setupUi(self)

        # define event handling methods
        self.connect(self.ui.pushButton_selectScans, QtCore.SIGNAL('clicked()'),
                     self.do_select_scans)


        self.connect(self.ui.pushButton_quit, QtCore.SIGNAL('clicked()'),
                     self.do_quit)

        return

    def do_quit(self):
        """

        :return:
        """
        self.close()

        return

    def do_select_scans(self):
        """

        :return:
        """
        # get check box
        if self.ui.checkBox_selectAllPeaks.isChecked():
            self._myParent.select_ub_scans(select_all=True)

        else:
            if self.ui.checkBox_selectNuclearPeaks.isChecked():
                status, ret_obj = guiutility.parse_float_editors([self.ui.lineEdit_nuclearPeaksTolerance],
                                                                 allow_bank=False)
                if not status:
                    raise RuntimeError(ret_obj)
                hkl_tol = ret_obj[0]
                self._myParent.select_ub_scans(hkl_tol)