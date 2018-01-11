from __future__ import absolute_import, print_function

import sys

from qtpy.QtWidgets import QApplication

from mantidqt.utils.qt.plugins import setup_library_paths
from mantidqt.widgets.workspacewidget.mantidtreemodel import MantidTreeModel
from mantidqt.widgets.workspacewidget.workspacetreewidget import WorkspaceTreeWidget

raw_input('Please attach the Debugger now if required. Press any key to continue')
setup_library_paths()
app = QApplication([""])


param = MantidTreeModel()
widget = WorkspaceTreeWidget(param)

widget.setWindowTitle("")
widget.show()

sys.exit(app.exec_())
