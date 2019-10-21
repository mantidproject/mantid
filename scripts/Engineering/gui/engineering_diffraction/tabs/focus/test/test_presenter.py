# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat import mock
from mantid.py3compat.mock import patch
from Engineering.gui.engineering_diffraction.tabs.focus import model, view, presenter

tab_path = "Engineering.gui.engineering_diffraction.tabs.focus"


class FocusPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(view.FocusView)
        self.model = mock.create_autospec(model.FocusModel)
        self.presenter = presenter.FocusPresenter(self.model, self.view)

    @patch(tab_path + ".presenter.FocusPresenter.start_focus_worker")
    def test_worker_started_with_correct_params(self, worker):
        self.view.get_focus_filename.return_value = "305738"
        self.view.get_north_bank.return_value = False
        self.view.get_south_bank.return_value = True
        self.view.get_plot_output.return_value = True

        self.presenter.on_focus_clicked()
        worker.assert_called_with("305738", ["South"], True)
