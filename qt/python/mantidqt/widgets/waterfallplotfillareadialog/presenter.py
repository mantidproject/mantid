# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantidqt.widgets.waterfallplotfillareadialog.view import WaterfallPlotFillAreaDialogView


class WaterfallPlotFillAreaDialogPresenter:

    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        if view:
            self.view = view
        else:
            self.view = WaterfallPlotFillAreaDialogView(parent)

        self.view.show()

        self.view.close_push_button.clicked.connect(self.view.close)