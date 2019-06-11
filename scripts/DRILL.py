# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantidqt.gui_helper import get_qapplication
from mantidqt.widgets import jobtreeview
from qtpy.QtWidgets import QMainWindow, QWidget

from qtpy import PYQT5
if not PYQT5:
    raise RuntimeError('DRILL interface is supported only in workbench!')

from Interface.ui.ill.main.ui_main import Ui_MainWindow

class MyApp(QMainWindow, Ui_MainWindow):
    def __init__(self):
        QMainWindow.__init__(self)
        Ui_MainWindow.__init__(self)
        self.setupUi(self)

        table = jobtreeview.JobTreeView(
            ["Sample", "Transmissioni", "Absorber", "Beam", "Container", "Reference"],
             self.cell(""), self.centralwidget)

    def cell(self, text):
        background_color = 'white'
        border_thickness = 1
        border_color = "black"
        border_opacity = 255
        is_editable = True
        return jobtreeview.Cell(text, background_color, border_thickness,
                                border_color, border_opacity, is_editable)

app, within_mantid = get_qapplication()
main_window = Ui_MainWindow()
window = MyApp()
window.show()
if not within_mantid:
    sys.exit(app.exec_())
