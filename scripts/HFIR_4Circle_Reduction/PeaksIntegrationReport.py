import os

from PyQt4 import QtGui, QtCore
import ui_PeakIntegrationSpreadSheet


class PeaksIntegrationReportDialog(QtGui.QDialog):
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
        self.ui = ui_PeakIntegrationSpreadSheet.Ui_Dialog()
        self.ui.setupUi(self)

        # initialize widget
        self.ui.tableWidget_spreadsheet.setup()

        # set up handlers
        self.connect(self.ui.pushButton_exportTable, QtCore.SIGNAL('clicked()'),
                     self.do_export_table)

        self.connect(self.ui.pushButton_quit, QtCore.SIGNAL('clicked()'),
                     self.do_quit)

        return

    def do_export_table(self):
        """

        :return:
        """
        default_dir = os.getcwd()
        output_file = str(QtGui.QFileDialog.getSaveFileName(self, 'Export table to csv file', default_dir,
                                                            'Data Files (*.dat);;All  Files (*.*)'))

        # return if cancelled
        if len(output_file) == 0:
            return

        # write
        self.ui.tableWidget_spreadsheet.export_table_csv(output_file)

        return

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
            print '[WARNING] There is no peak integration summary given for the report.'
            return

        scan_number_list = sorted(peak_integration_summary.keys())
        for scan_number in scan_number_list:
            print '[DB...BAT] Scan {0} Peak integration report keys: {1}' \
                  ''.format(scan_number, peak_integration_summary[scan_number].keys())

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
        # END-FOR

        return
