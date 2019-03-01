# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import absolute_import

import os

from mantid.kernel import logger
from mantidqt.utils.show_in_explorer import ShowInExplorer


class CodeEditorTabPresenter(object):
    def __init__(self, view=None):
        self.view = view

    def action_show_in_explorer(self):
        widget = self.view.widget(self.view.last_tab_clicked)
        if not widget:
            # the widget has been deleted without updating the last tab clicked
            return

        filepath = widget.editor.fileName()
        if "" != filepath:
            ShowInExplorer.open_directory(os.path.dirname(filepath))
        else:
            logger.notice("Selected tab is not saved to a file, and cannot be shown in explorer.")
