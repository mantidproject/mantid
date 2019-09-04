# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantidqt.widgets.jobtreeview import *


class SansJobTreeView(JobTreeView):

    def __init__(self):
        columns = ["Sample", "Transmission", "Absorber", "Beam", "Flux", "Container", "ContainerTransmission",
                   "Mask", "TransmissionAbsorber", "TransmissionBeam", "Sensitivity", "CustomOptions"]

        JobTreeView.__init__(self, columns,self.cell_style(''))

        self.setRootIsDecorated(False)
        self.add_row([''] * len(columns))

    def add_row(self, value):
        value = [self.cell_style(x) for x in value]
        self.appendChildRowOf(self.row([]), value)

    def row(self, path):
        return RowLocation(path)

    def cell_style(self, text):
        background_color = 'white'
        border_thickness = 1
        border_color = 'black'
        border_opacity = 255
        is_editable = True
        return Cell(text, background_color, border_thickness,
                    border_color, border_opacity, is_editable)