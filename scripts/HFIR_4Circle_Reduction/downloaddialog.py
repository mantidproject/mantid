##########
# Dialog to set up HTTP data downloading server and download HB3A data to local
##########
from PyQt4 import QtCore
from PyQt4 import QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

# TODO/TODO/NOW/NOW - Make this one work!

class DataDownloadDialog(QtGui.QDialog):
    """
    """
    def __init__(self, parent):
        """
        """

        # set up widgets
        # lineEdit_url
        # pushButton_testURLs 
        # lineEdit_localSrcDir
        # pushButton_browseLocalCache 
        # comboBox_mode
        # pushButton_downloadExpData
        # lineEdit_downloadScans
        # pushButton_ListScans
