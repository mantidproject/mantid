# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import absolute_import

import os
import subprocess
import sys

from mantid.kernel import logger


class ShowInExplorer(object):
    @staticmethod
    def open_directory(path):
        if sys.platform == "win32":
            os.startfile(os.path.dirname(path))
        elif sys.platform == 'darwin':
            subprocess.check_call(['open', '--', path])
        elif sys.platform == 'linux2':
            call_params = ['xdg-open', path]
            try:
                subprocess.check_call(call_params)
            except subprocess.CalledProcessError as ex:
                logger.notice("Could not manage to open the folder in explorer.")
                logger.debug("Error encountered: {}".format(ex))


class CodeEditorTabPresenter(object):
    def __init__(self, view=None):
        self.view = view

    def action_show_in_explorer(self):
        widget = self.view.widget(self.view.last_tab_clicked)
        if not widget:
            # the widget has been deleted without updating the last tab clicked
            return

        dirname = widget.editor.fileName()
        if "" != dirname:
            ShowInExplorer.open_directory(dirname)
        else:
            logger.notice("Selected tab is not saved to a file, and cannot be shown in explorer.")
