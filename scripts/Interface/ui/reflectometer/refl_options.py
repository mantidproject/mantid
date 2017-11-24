# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from PyQt4 import QtCore, QtGui
from ui.reflectometer.ui_refl_options_window import Ui_OptionsDialog

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s


class ReflOptions(QtGui.QDialog, Ui_OptionsDialog):
    """
    Member variables
    """
    __frequency = 0
    __method = 0
    __method_list = ["Add", "Replace", "Append"]
    __icat_download = False

    def __init__(self, def_method, def_freq, def_alg_use, def_icat_download, def_group_tof_workspaces, def_stitch_right):
        """
        Initialise the interface
        """
        super(QtGui.QDialog, self).__init__()

        # Initialize member variables
        self.__alg_use = def_alg_use
        self.__method = def_method
        self.__frequency = def_freq
        self.__icat_download = def_icat_download
        self.__group_tof_workspaces = def_group_tof_workspaces
        self.__stitch_right = def_stitch_right

        self.setupUi(self)

        # Setup UI controls
        self.comboAccMethod.addItems(self.__method_list)
        if def_method in self.__method_list:
            self.comboAccMethod.setCurrentIndex(self.__method_list.index(def_method))
        else:
            self.comboAccMethod.setCurrentIndex(0)

        self.dspinFrequency.setValue(def_freq)
        self.checkAlg.setChecked(def_alg_use)
        self.checkICATDownload.setChecked(def_icat_download)
        self.checkGroupTOFWorkspaces.setChecked(def_group_tof_workspaces)
        self.checkScaleRight.setChecked(def_stitch_right)

        # connect update signals to functions
        self.dspinFrequency.valueChanged.connect(self.__update_frequency)
        self.comboAccMethod.activated.connect(self.__update_method)
        self.checkAlg.clicked.connect(self.__update_Alg_use)
        self.checkICATDownload.clicked.connect(self.__update_download_method)
        self.checkGroupTOFWorkspaces.clicked.connect(self.__update_groupTOF_method)
        self.checkScaleRight.clicked.connect(self.__update_stitch_right)

    def __update_Alg_use(self, checked):
        self.__alg_use = checked

    def __update_frequency(self, freq):
        self.__frequency = freq

    def __update_method(self, meth):
        self.__method = meth

    def __update_download_method(self, checked):
        self.__icat_download = checked

    def __update_groupTOF_method(self, checked):
        self.__group_tof_workspaces = checked

    def __update_stitch_right(self, checked):
        self.__stitch_right = checked

    def icatDownload(self):
        return self.__icat_download

    def groupTOFWorkspaces(self):
        return self.__group_tof_workspaces

    def frequency(self):
        return self.__frequency

    def useAlg(self):
        return self.__alg_use

    def method(self):
        return self.__method

    def stitchRight(self):
        return self.__stitch_right
