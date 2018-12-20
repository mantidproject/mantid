from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui
import Muon.GUI.Common.message_box as message_box

# determine whether the interface is opened from within Mantid or not
# (outside of Mantid we cannot use the "Manage user directories" functionality)
STANDALONE_EXEC = True
try:
    from mantidqtpython import MantidQt
except:
    STANDALONE_EXEC = False


class HelpWidgetView(QtGui.QWidget):

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super(HelpWidgetView, self).__init__(parent)

        self.setup_interface_layout()

    def setup_interface_layout(self):
        self.setObjectName("HelpWidget")
        self.resize(500, 100)

        self.help_label = QtGui.QLabel(self)
        self.help_label.setObjectName("helpLabel")
        self.help_label.setText("Help : ")

        self.help_button = QtGui.QToolButton(self)
        self.help_button.setObjectName("rebinHelpButton")
        self.help_button.setText("?")

        self.manage_user_dir_button = QtGui.QPushButton(self)
        self.manage_user_dir_button.setObjectName("manageUserDirectoriesButton")
        self.manage_user_dir_button.setText("Manage User Directories")

        self.horizontal_layout = QtGui.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.help_label)
        self.horizontal_layout.addWidget(self.help_button)
        self.horizontal_layout.addStretch(0)
        self.horizontal_layout.addWidget(self.manage_user_dir_button)

        self.setLayout(self.horizontal_layout)

    def getLayout(self):
        return self.horizontal_layout

    def on_manage_user_directories_clicked(self, slot):
        self.manage_user_dir_button.clicked.connect(slot)

    def on_help_button_clicked(self, slot):
        self.help_button.clicked.connect(slot)

    def show_directory_manager(self):
        if STANDALONE_EXEC:
            MantidQt.API.ManageUserDirectories.openUserDirsDialog(self)
        else:
            self.warning_popup("Cannot open user directories dailog outside MantidPlot.")
