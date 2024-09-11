# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Containing a set of classes used for finding (calculating and refining) UB matrix
"""

import os
from . import guiutility
from qtpy.QtWidgets import QDialog, QFileDialog
from mantid.kernel import Logger

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("HFIR_4Circle_Reduction").information("Using legacy ui importer")
    from mantidplot import load_ui


class AddScansForUBDialog(QDialog):
    """
    Dialog class to add scans to UB scans' table for calculating and
    """

    def __init__(self, parent):
        """
        initialization
        :param parent: main GUI, reductionControl
        """
        super(AddScansForUBDialog, self).__init__(parent)
        self._myParent = parent

        # set up UI
        ui_path = "AddUBPeaksDialog.ui"
        self.ui = load_ui(__file__, ui_path, baseinstance=self)

        # initialize widgets
        self.ui.checkBox_loadHKLfromFile.setChecked(True)

        self.ui.pushButton_findPeak.clicked.connect(self.do_find_peak)
        self.ui.pushButton_addPeakToCalUB.clicked.connect(self.do_add_single_scan)

        self.ui.pushButton_loadScans.clicked.connect(self.do_load_scans)
        self.ui.pushButton_addScans.clicked.connect(self.do_add_scans)

        self.ui.pushButton_quit.clicked.connect(self.do_quit)

    def do_add_scans(self):
        """
        add all the scans list in the 'plainTextEdit_scanList'
        :return:
        """
        scans_str = str(self.ui.plainTextEdit_scanList.toPlainText())
        scan_list = guiutility.parse_integer_list(scans_str)
        self._myParent.add_scans_ub_table(scan_list)

    def do_add_single_scan(self):
        """
        add single scan to refine UB matrix
        :return:
        """
        scan_number = int(self.ui.lineEdit_scanNumber.text())
        self._myParent.add_scans_ub_table([scan_number])

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
                self.ui.lineEdit_H.setText("%.2f" % hkl[0])
                self.ui.lineEdit_K.setText("%.2f" % hkl[1])
                self.ui.lineEdit_L.setText("%.2f" % hkl[2])

            self.ui.lineEdit_sampleQx.setText("%.5E" % vec_q[0])
            self.ui.lineEdit_sampleQy.setText("%.5E" % vec_q[1])
            self.ui.lineEdit_sampleQz.setText("%.5E" % vec_q[2])
        # END-IF

    def do_load_scans(self):
        """
        load an ASCII file containing scan numbers,
        and the results are written to 'plainTextEdit_scanList'
        :return:
        """
        # get file name
        scan_file = str(self.ui.lineEdit_scansFile.text())
        if os.path.exists(scan_file) is False:
            raise RuntimeError("Scan file {0} does not exist.".format(scan_file))

        # parse file
        exp_number, scans_str = guiutility.import_scans_text_file(scan_file)

        self.ui.plainTextEdit_scanList.setPlainText(scans_str)

    def do_quit(self):
        """
        quit
        :return:
        """
        self.close()


class SelectUBMatrixScansDialog(QDialog):
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
        ui_path = "UBSelectPeaksDialog.ui"
        self.ui = load_ui(__file__, ui_path, baseinstance=self)

        # define event handling methods
        self.ui.pushButton_selectScans.clicked.connect(self.do_select_scans)
        self.ui.pushButton_revertCurrentSelection.clicked.connect(self.do_revert_selection)
        self.ui.pushButton_exportSelectedScans.clicked.connect(self.do_export_selected_scans)
        self.ui.pushButton_quit.clicked.connect(self.do_quit)

    def do_quit(self):
        """

        :return:
        """
        self.close()

    def do_export_selected_scans(self):
        """
        export selected scans to a file
        :return:
        """
        # get the scans
        scans_list = self._myParent.ub_matrix_processing_table.get_selected_scans()
        scans_list.sort()

        # form the output string
        output_str = "# Exp = {0}.\n".format(self._myParent.current_exp_number)
        for scan in scans_list:
            output_str += "{0}, ".format(scan)

        # trim the last
        output_str = output_str[:-2]

        # get the output file name
        file_filter = "Text Files (*.dat);;All Files (*.*)"
        file_name = QFileDialog.getSaveFileName(self, "File to export selected scans", self._myParent.working_directory, file_filter)
        if not file_name:
            return
        if isinstance(file_name, tuple):
            file_name = file_name[0]

        # write file
        out_file = open(file_name, "w")
        out_file.write(output_str)
        out_file.close()

    def do_revert_selection(self):
        """
        revert the current selection of the UB table
        :return:
        """
        self._myParent.ub_matrix_processing_table.revert_selection()

    def do_select_scans(self):
        """

        :return:
        """
        # get check box
        if self.ui.checkBox_selectAllPeaks.isChecked():
            self._myParent.select_ub_scans(select_all=True)

        else:
            select_args = dict()

            if self.ui.checkBox_selectNuclearPeaks.isChecked():
                status, ret_obj = guiutility.parse_float_editors([self.ui.lineEdit_nuclearPeaksTolerance], allow_blank=False)
                if not status:
                    raise RuntimeError(ret_obj)
                hkl_tol = ret_obj[0]
                select_args["nuclear_peaks"] = True
                select_args["hkl_tolerance"] = hkl_tol

            if self.ui.checkBox_wavelength.isChecked():
                # wave length selection
                status, ret_obj = guiutility.parse_float_editors(
                    [self.ui.lineEdit_wavelength, self.ui.lineEdit_wavelengthTolerance], allow_blank=False
                )
                if status:
                    wave_length, wave_length_tol = ret_obj
                    select_args["wavelength"] = wave_length
                    select_args["wavelength_tolerance"] = wave_length_tol
                else:
                    select_args["wavelength"] = None

            # select with filters
            self._myParent.ub_matrix_processing_table.select_scans(**select_args)
        # END-IF-ELSE
