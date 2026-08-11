# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX-License-Identifier: GPL-3.0+
import unittest

from mantid.api import FrameworkManager
from mantidqt.utils.qt.testing import start_qapplication
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_view import SettingsView


@start_qapplication
class SettingsViewTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # the C++ FileFinderWidgets in the dialog cannot be constructed before the framework is up
        FrameworkManager.Instance()

    def test_dialog_is_modal(self):
        # SettingsPresenter defers re-reading settings for a new RB number until show(), which is only
        # safe because the dialog is modal - nothing can change the RB while the dialog is up. If this
        # test fails because modality was deliberately dropped, SettingsPresenter.show() and
        # SettingsPresenter.set_rb_num() need revisiting rather than this assertion relaxing.
        view = SettingsView()

        self.assertTrue(view.isModal())


if __name__ == "__main__":
    unittest.main()
