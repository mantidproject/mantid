# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from qtpy.QtCore import Qt
from qtpy.QtWidgets import qApp

from mantidqt.utils.qt import load_ui

form, base = load_ui(__file__, "about.ui")


class AboutView(base, form):
    def __init__(self, parent, presenter, version_text, date_text = None):
        super(AboutView, self).__init__(parent)
        self.setupUi(self)
        self.set_layout(version_text, date_text)
        self.presenter = presenter

    def set_layout(self, version_text, date_text):
        self.setWindowTitle(self.windowTitle() + " "
                            + version_text)
        version_label = version_text
        # add a date if it is an official release
        if date_text and len(version_text) < 10:
            # strip off the first few characters that will be "day, "
            version_label += " ({0})".format(date_text[5:])
        self.lblVersion.setText(self.lblVersion.text() + version_label)

        self.setAttribute(Qt.WA_DeleteOnClose, True)

        stlyeName = qApp.style().metaObject().className()
        if stlyeName in ["QCDEStyle", "QMotifStyle"]:
            # add stylesheet formatting for other environments
            ss = self.styleSheet()
            ss += "\n"
            "QDialog#FirstTimeSetup QCommandLinkButton {"
            " background-color: rgba(255, 255, 255, 0);"
            "  border-radius: 15px;"
            "}"
            "\n"
            "QDialog#FirstTimeSetup QCommandLinkButton:hover {"
            "  background-color: rgba(255, 255, 255, 128);"
            "}"
            self.setStyleSheet(ss)

    def closeEvent(self, event):
        self.presenter.action_close()
        self.deleteLater()
        super(AboutView, self).closeEvent(event)
