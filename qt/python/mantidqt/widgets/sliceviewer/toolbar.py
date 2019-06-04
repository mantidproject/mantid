# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#

from __future__ import (absolute_import, division, print_function,
                        unicode_literals)
from mantidqt.MPLwidgets import NavigationToolbar2QT
from mantidqt.icons import get_icon
from qtpy.QtCore import Signal, Qt, QSize
from qtpy.QtWidgets import QLabel, QSizePolicy


class SliceViewerNavigationToolbar(NavigationToolbar2QT):

    gridClicked = Signal()
    linePlotsClicked = Signal(bool)

    toolitems = (
        ('Home', 'Reset original view', 'mdi.home', 'home', None),
        ('Pan', 'Pan axes with left mouse, zoom with right', 'mdi.arrow-all', 'pan', False),
        ('Zoom', 'Zoom to rectangle', 'mdi.magnify-plus-outline', 'zoom', False),
        (None, None, None, None, None),
        ('Grid', 'Toggle grid on/off', 'mdi.grid', 'gridClicked', None),
        ('LinePlots', 'Toggle lineplots on/off', 'mdi.chart-bell-curve', 'linePlotsClicked', False),
        ('Save', 'Save the figure', 'mdi.content-save', 'save_figure', None),
        (None, None, None, None, None),
        ('Customize', 'Configure plot options', 'mdi.settings', 'edit_parameters', None),
    )

    def _init_toolbar(self):
        for text, tooltip_text, fa_icon, callback, checked in self.toolitems:
            if text is None:
                self.addSeparator()
            else:
                if fa_icon:
                    a = self.addAction(get_icon(fa_icon),
                                       text, getattr(self, callback))
                else:
                    a = self.addAction(text, getattr(self, callback))
                self._actions[callback] = a
                if checked is not None:
                    a.setCheckable(True)
                    a.setChecked(checked)
                if tooltip_text is not None:
                    a.setToolTip(tooltip_text)

        self.buttons = {}

        # Add the x,y location widget at the right side of the toolbar
        # The stretch factor is 1 which means any resizing of the toolbar
        # will resize this label instead of the buttons.
        if self.coordinates:
            self.locLabel = QLabel("", self)
            self.locLabel.setAlignment(Qt.AlignRight | Qt.AlignTop)
            self.locLabel.setSizePolicy(
                QSizePolicy(QSizePolicy.Expanding,
                            QSizePolicy.Ignored))
            labelAction = self.addWidget(self.locLabel)
            labelAction.setVisible(True)

        # Adjust icon size or they are too small in PyQt5 by default
        self.setIconSize(QSize(24, 24))
