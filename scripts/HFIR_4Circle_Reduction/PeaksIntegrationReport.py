# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import os

from qtpy.QtWidgets import (QDialog, QFileDialog)  # noqa
from mantid.kernel import Logger
try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("HFIR_4Circle_Reduction").information('Using legacy ui importer')
    from mantidplot import load_ui
from qtpy.QtWidgets import (QVBoxLayout)
from HFIR_4Circle_Reduction.hfctables import PeaksIntegrationSpreadSheet


class PeaksIntegrationReportDialog(QDialog):
    """
    Dialog to report the details of peaks integration
    """
    def __init__(self, parent):
        """
        initialization
        :param parent:
        """
        super(PeaksIntegrationReportDialog, self).__init__(parent)

        # set up UI
        ui_path = "PeakIntegrationSpreadSheet.ui"
        self.ui = load_ui(__file__, ui_path, baseinstance=self)
        self._promote_widgets()

        # initialize widget
        self.ui.tableWidget_spreadsheet.setup()

        # set up handlers
        self.ui.pushButton_exportTable.clicked.connect(self.do_export_table)
        self.ui.pushButton_quit.clicked.connect(self.do_quit)

        return

    def _promote_widgets(self):
        tableWidget_spreadsheet_layout = QVBoxLayout()
        self.ui.frame_tableWidget_spreadsheet.setLayout(tableWidget_spreadsheet_layout)
        self.ui.tableWidget_spreadsheet = PeaksIntegrationSpreadSheet(self)
        tableWidget_spreadsheet_layout.addWidget(self.ui.tableWidget_spreadsheet)

        return

    def do_export_table(self):
        """
        export table to a file
        :return:
        """
        default_dir = os.getcwd()
        output_file = QFileDialog.getSaveFileName(self, 'Export table to csv file', default_dir,
                                                  'Data Files (*.dat);;All  Files (*.*)')
        if not output_file:
            return
        if isinstance(output_file, tuple):
            output_file = output_file[0]

        # write
        self.ui.tableWidget_spreadsheet.export_table_csv(output_file)

    def do_quit(self):
        """

        :return:
        """
        self.close()

        return

    def set_report(self, peak_integration_summary):
        """

        :param peak_integration_summary: dictionary of dictionary; key is scan number
        :return:
        """
        # check input
        assert isinstance(peak_integration_summary, dict)

        if len(peak_integration_summary) == 0:
            print('[WARNING] There is no peak integration summary given for the report.')
            return

        scan_number_list = sorted(peak_integration_summary.keys())
        for scan_number in scan_number_list:
            try:
                spice_hkl = peak_integration_summary[scan_number]['SPICE HKL']
                calculated_hkl = peak_integration_summary[scan_number]['Mantid HKL']
                mask_name = peak_integration_summary[scan_number]['Mask']
                intensity1 = peak_integration_summary[scan_number]['Raw Intensity']
                error1 = peak_integration_summary[scan_number]['Raw Intensity Error']
                intensity2 = peak_integration_summary[scan_number]['Intensity 2']
                error2 = peak_integration_summary[scan_number]['Intensity 2 Error']
                intensity3 = peak_integration_summary[scan_number]['Gauss Intensity']
                error3 = peak_integration_summary[scan_number]['Gauss Error']
                lorentz_factor = peak_integration_summary[scan_number]['Lorentz']
                estimated_bkgd = peak_integration_summary[scan_number]['Estimated Background']
                gauss_bkgd = peak_integration_summary[scan_number]['Fitted Background']
                gauss_a = peak_integration_summary[scan_number]['Fitted A']
                gauss_sigma = peak_integration_summary[scan_number]['Fitted Sigma']
                motor_name = peak_integration_summary[scan_number]['Motor']
                motor_step = peak_integration_summary[scan_number]['Motor Step']
                k_shift = peak_integration_summary[scan_number]['K-vector']
                absorption_correction = peak_integration_summary[scan_number]['Absorption Correction']

                self.ui.tableWidget_spreadsheet.add_scan_information(scan_number, spice_hkl, calculated_hkl,
                                                                     mask_name, intensity1, error1, intensity2, error2,
                                                                     intensity3, error3, lorentz_factor, estimated_bkgd,
                                                                     gauss_bkgd, gauss_sigma, gauss_a, motor_name,
                                                                     motor_step, k_shift, absorption_correction)
            except KeyError as key_err:
                print ('ERROR: Unable to add scan {0} to report due to {1}'.format(scan_number, key_err))

        # END-FOR

        return
