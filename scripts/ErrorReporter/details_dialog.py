# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import qtpy
import platform
if qtpy.PYQT5:  # noqa
    from ErrorReporter import resources_qt5  # noqa
elif qtpy.PYQT4:  # noqa
    from ErrorReporter import resources_qt4  # noqa
else:  # noqa
    raise RuntimeError("Unknown QT version: {}".format(qtpy.QT_VERSION))  # noqa

from qtpy import QtCore
from mantidqt.utils.qt import load_ui
from mantid.kernel import version_str as mantid_version_str

DEFAULT_STACKTRACE_TEXT = "Stacktrace unavailable"

moreDetailsUIBase, moreDetailsUI = load_ui(__file__, 'moredetails.ui')


class MoreDetailsDialog(moreDetailsUIBase, moreDetailsUI):

    def __init__(self, parent=None):
        super(self.__class__, self).__init__(parent)
        self.setupUi(self)
        self.pushButtonOk.clicked.connect(self.hide)
        self.setWindowFlags(QtCore.Qt.Dialog | QtCore.Qt.Window | QtCore.Qt.WindowTitleHint
                            | QtCore.Qt.CustomizeWindowHint)

    def set_version_text(self, mantid_application):
        environment_text = f"Using {mantid_application + mantid_version_str()} on {platform.platform()}"
        self.version_text.setText(environment_text)

    def set_stacktrace_text(self, text):
        text = text if text else DEFAULT_STACKTRACE_TEXT
        self.stacktrace_text.setText(text)
