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
            self.ui.tableWidget_spreadsheet.add_scan_information(scan_number, spice_hkl, calculated_hkl,
                                                                 mask_name, intensity1, error1, intensity2, error2,
                                                                 intensity3, erro3, lorentz_factor, estimated_bkgd,
                                                                 gauss_bkgd, gauss_sigma, gauss_a, motor_name,
                                                                 motor_step, k_shift, absorption_correction)
        # END-FOR

        return
