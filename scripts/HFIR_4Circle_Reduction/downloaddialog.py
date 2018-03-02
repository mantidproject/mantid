##########
# Dialog to set up HTTP data downloading server and download HB3A data to local
##########
import os
from PyQt4 import QtCore
from PyQt4 import QtGui
import HFIR_4Circle_Reduction.fourcircle_utility as hb3a_util
from HFIR_4Circle_Reduction import ui_httpserversetup as ui_http


try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s


class DataDownloadDialog(QtGui.QDialog):
    """ dialog for set up HTTP server and download files to local computer
    This feature will be valid until SNS disables the HTTP server for HFIR data
    """
    def __init__(self, parent):
        """
        initialization
        :param parent:
        """
        super(DataDownloadDialog, self).__init__(parent)

        # set up UI
        self.ui = ui_http.Ui_Dialog()
        self.ui.setupUi(self)

        # initialize widgets
        self._init_widgets()

        # define event handing
        self.connect(self.ui.pushButton_testURLs, QtCore.SIGNAL('clicked()'),
                     self.do_test_url)

        self.connect(self.ui.pushButton_downloadExpData, QtCore.SIGNAL('clicked()'),
                     self.do_download_spice_data)

        self.connect(self.ui.pushButton_ListScans, QtCore.SIGNAL('clicked()'),
                     self.do_list_scans)

        self.connect(self.ui.comboBox_mode, QtCore.SIGNAL('currentIndexChanged(int)'),
                     self.do_change_data_access_mode)

        # self.connect(self.ui.pushButton_useDefaultDir, QtCore.SIGNAL('clicked()'),
        #              self.do_setup_dir_default)
        self.connect(self.ui.pushButton_browseLocalCache, QtCore.SIGNAL('clicked()'),
                     self.do_browse_local_cache_dir)

        # Set the URL red as it is better not check at this stage. Leave it to user
        self.ui.lineEdit_url.setStyleSheet("color: black;")

        # define class variable
        self._homeSrcDir = os.getcwd()
        try:
            self._myControl = None
            self._myControl = parent.controller
        except AttributeError as att_err:
            print (att_err)

        # experiment number
        self._expNumber = None

        return

    def _init_widgets(self):
        """
        initialize widgets
        :return:
        """
        self.ui.lineEdit_url.setText('http://neutron.ornl.gov/user_data/hb3a/')

        return

    def do_browse_local_cache_dir(self):
        """ Browse local cache directory
        :return:
        """
        local_cache_dir = str(QtGui.QFileDialog.getExistingDirectory(self,
                                                                     'Get Local Cache Directory',
                                                                     self._homeSrcDir))

        # Set local directory to control
        status, error_message = self._myControl.set_local_data_dir(local_cache_dir)
        if status is False:
            self.pop_one_button_dialog(error_message)
            return

        # Synchronize to local data/spice directory and local cache directory
        # if str(self.ui.lineEdit_localSpiceDir.text()) != '':
        #     prev_dir = str(self.ui.lineEdit_localSrcDir.text())
        #     self.pop_one_button_dialog('Local data directory was set up as %s' %
        #                                prev_dir)
        self.ui.lineEdit_localSrcDir.setText(local_cache_dir)
        # self.ui.lineEdit_localSpiceDir.setText(local_cache_dir)

        return

    def do_change_data_access_mode(self):
        """ Change data access mode between downloading from server and local
        Event handling methods
        :return:
        """
        # TODO/FIXME/NOW - Find out whether these widgets are used in the dialog
        # new_mode = str(self.ui.comboBox_mode.currentText())
        # self._dataAccessMode = new_mode

        # if new_mode.startswith('Local') is True:
        #     self.ui.lineEdit_localSpiceDir.setEnabled(True)
        #     self.ui.pushButton_browseLocalDataDir.setEnabled(True)
        #     self.ui.lineEdit_url.setEnabled(False)
        #     self.ui.lineEdit_localSrcDir.setEnabled(False)
        #     self.ui.pushButton_browseLocalCache.setEnabled(False)
        #     self._allowDownload = False
        # else:
        #     self.ui.lineEdit_localSpiceDir.setEnabled(False)
        #     self.ui.pushButton_browseLocalDataDir.setEnabled(False)
        #     self.ui.lineEdit_url.setEnabled(True)
        #     self.ui.lineEdit_localSrcDir.setEnabled(True)
        #     self.ui.pushButton_browseLocalCache.setEnabled(True)
        #     self._allowDownload = True

        return

    def do_download_spice_data(self):
        """ Download SPICE data
        :return:
        """
        # get experiment number
        exp_no = self._expNumber
        assert isinstance(exp_no, int), 'Experiment number {0} must be an integer but not a {1}.' \
                                        ''.format(exp_no, type(exp_no))

        # Check scans to download
        scan_list_str = str(self.ui.lineEdit_downloadScans.text())
        if len(scan_list_str) > 0:
            # user specifies scans to download
            valid, scan_list = hb3a_util.parse_int_array(scan_list_str)
            if valid is False:
                error_message = scan_list
                self.pop_one_button_dialog(error_message)
        else:
            # Get all scans
            server_url = str(self.ui.lineEdit_url.text())
            scan_list = hb3a_util.get_scans_list(server_url, exp_no, return_list=True)
        self.pop_one_button_dialog('Going to download scans %s.' % str(scan_list))

        # Check location
        destination_dir = str(self.ui.lineEdit_localSrcDir.text())
        status, error_message = self._myControl.set_local_data_dir(destination_dir)
        if status is False:
            self.pop_one_button_dialog(error_message)
        else:
            self.pop_one_button_dialog('Spice files will be downloaded to %s.' % destination_dir)

        # Set up myControl for downloading data
        self._myControl.set_exp_number(exp_no)

        server_url = str(self.ui.lineEdit_url.text())
        status, error_message = self._myControl.set_server_url(server_url)
        if status is False:
            self.pop_one_button_dialog(error_message)
            return

        # Download
        self._myControl.download_data_set(scan_list)

        return

    def do_list_scans(self):
        """ List all scans available and show the information in a pop-up dialog
        :return:
        """
        # Experiment number
        exp_no = int(self.ui.lineEdit_exp.text())

        access_mode = str(self.ui.comboBox_mode.currentText())
        if access_mode == 'Local':
            spice_dir = str(self.ui.lineEdit_localSpiceDir.text())
            message = hb3a_util.get_scans_list_local_disk(spice_dir, exp_no)
        else:
            url = str(self.ui.lineEdit_url.text())
            message = hb3a_util.get_scans_list(url, exp_no)

        self.pop_one_button_dialog(message)

        return

    def do_test_url(self):
        """ Test whether the root URL provided specified is good
        """
        url = str(self.ui.lineEdit_url.text())

        url_is_good, err_msg = hb3a_util.check_url(url)
        if url_is_good is True:
            self.pop_one_button_dialog("URL %s is valid." % url)
            self.ui.lineEdit_url.setStyleSheet("color: green;")
        else:
            self.pop_one_button_dialog(err_msg)
            self.ui.lineEdit_url.setStyleSheet("color: read;")

        return url_is_good

    def pop_one_button_dialog(self, message):
        """ Pop up a one-button dialog
        :param message:
        :return:
        """
        assert isinstance(message, str), 'Input message %s must a string but not %s.' \
                                         '' % (str(message), type(message))
        QtGui.QMessageBox.information(self, '4-circle Data Reduction', message)

        return

    def set_experiment_number(self, exp_number):
        """set the experiment number
        :param exp_number:
        :return:
        """
        assert isinstance(exp_number, int), 'Experiment number {0} to set to download dialog must be an integer but ' \
                                            'not a {1}.'.format(exp_number, type(exp_number))

        self._expNumber = exp_number

        return
