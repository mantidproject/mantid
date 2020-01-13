# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat import mock
from Engineering.gui.engineering_diffraction.tabs.common.cropping import cropping_model, cropping_view, cropping_presenter

dir_path = "Engineering.gui.engineering_diffraction.tabs.common.cropping"


class CroppingPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(cropping_view.CroppingView)
        self.model = mock.create_autospec(cropping_model.CroppingModel)
        self.presenter=  cropping_presenter.CroppingPresenter(self.model, self.view)

    def test_combo_changed_index_bank_1(self):
        self.presenter.on_combo_changed(0)

        self.assertEqual(self.presenter.bank, 1)
        self.assertFalse(self.presenter.custom_spectra_enabled)
        self.view.set_custom_spectra_entry_hidden.assert_called_with()

    def test_combo_changed_index_bank_2(self):
        self.presenter.on_combo_changed(1)

        self.assertEqual(self.presenter.bank, 2)
        self.assertFalse(self.presenter.custom_spectra_enabled)
        self.view.set_custom_spectra_entry_hidden.assert_called_with()

    def test_combo_changed_index_custom(self):
        self.presenter.on_combo_changed(2)

        self.assertEqual(self.presenter.bank, 0)
        self.assertTrue(self.presenter.custom_spectra_enabled)
        self.view.set_custom_spectra_entry_visible.assert_called_with()

    def test_custom_spectra_changed_valid(self):
        self.model.validate_and_clean_spectrum_numbers.return_value = ("", "1-5, 3-45")

        self.presenter.on_spectra_changed("1-5, 45-3")

        self.assertEqual(self.presenter.custom_spectra, "1-5, 3-45")
        self.assertTrue(self.presenter.custom_valid)
        self.view.set_invalid_indicator_hidden.assert_called_with()

    def test_custom_spectra_changed_invalid(self):
        self.model.validate_and_clean_spectrum_numbers.return_value = ("Error lol", "")

        self.presenter.on_spectra_changed("1-5, 45-3")

        self.assertEqual(self.presenter.custom_spectra, "")
        self.assertFalse(self.presenter.custom_valid)
        self.view.set_invalid_indicator_visible.assert_called_with("Error lol")


if __name__ == '__main__':
    unittest.main()
