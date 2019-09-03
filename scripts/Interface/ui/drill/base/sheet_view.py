# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantidqt.widgets import jobtreeview
from qtpy.QtWidgets import (QWidget, QSizePolicy)


class SheetView(QWidget):

    job_tree_view = None

    def __init__(self):
        QWidget.__init__(self)
        self.job_tree_view = jobtreeview.JobTreeView(
            ["Sample", "Transmission", "Absorber", "Beam", "Flux", "Container", "ContainerTransmission",
             "Mask", "TransmissionAbsorber", "TransmissionBeam", "Sensitivity"],
             self.cell_style(""), self)

        self.job_tree_view.setRootIsDecorated(False)
        self.add_row([''] * 11)
        size_policy = QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setSizePolicy(size_policy)
        size_policy2 = QSizePolicy(QSizePolicy.Maximum, QSizePolicy.Maximum)
        self.job_tree_view.setSizePolicy(size_policy2)
        # table_signals = jobtreeview.JobTreeViewSignalAdapter(self.job_tree_view, self)

    def add_row(self, value):
        value = [self.cell_style(x) for x in value]
        self.job_tree_view.appendChildRowOf(self.row([]), value)

    def row(self, path):
        return jobtreeview.RowLocation(path)

    def cell_style(self, text):
        background_color = 'white'
        border_thickness = 1
        border_color = 'black'
        border_opacity = 255
        is_editable = True
        return jobtreeview.Cell(text, background_color, border_thickness,
                                border_color, border_opacity, is_editable)


