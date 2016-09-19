from PyQt4 import QtGui, QtCore
from ui_scaninfotablesetupwindow import Ui_MainWindow


class ScanInfoTableSetupWindow(QtGui.QMainWindow):
    """
    window class for GUI to set up scan information table
    """
    def __init__(self, parent):
        """
        Initialization
        Args:
            parent:
        """
        # initialize from base class
        QtGui.QMainWindow.__init__(self, parent)

        # set up GUI layout
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        # set up table
        self.ui.tableWidget_setupTable.setup()

        return

    def add_log_names(self, spice_log_list, spice_col_list):
        """
        Add log names to table
        Args:
            spice_log_list: list of string for log names
            spice_col_list: list of string for spice table column names
        Returns:

        """
        # check
        assert isinstance(log_name_list, list), 'Input for log names must be a list but not %s.' \
                                                '' % str(type(log_name_list))

        # add
        for log_name in log_name_list:
            assert isinstance(log_name, str)
            self.ui.tableWidget_setupTable.add_item(log_name)
        # END-FOR (log_name)

        return

    def set_up_information(self, info_str):
        """
        Set up information for user to read
        Args:
            info_str:

        Returns:

        """
        # check
        assert isinstance(info_str, str), 'Information string must be a string but not of type %s.' \
                                          '' % str(type(info_str))

        # set
        self.ui.label_information.setText(info_str)
