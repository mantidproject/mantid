# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.cropping import (
    cropping_model,
    cropping_view,
    cropping_presenter,
)
from Engineering.EnggUtils import GROUP

dir_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.cropping"


class CroppingPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(cropping_view.CroppingView, instance=True)
        self.view.finder_custom = mock.MagicMock()
        self.view.finder_custom.isValid.return_value = True

        self.parent = mock.MagicMock()
        self.model = mock.create_autospec(cropping_model.CroppingModel, instance=True)
        self.presenter = cropping_presenter.CroppingPresenter(self.parent, self.view, self.model)

    def test_combo_changed_index_customcal(self):
        self.presenter.on_combo_changed(0)

        self.assertEqual(self.presenter.group, GROUP.CUSTOM)
        self.assertTrue(self.presenter.custom_calfile_enabled)
        self.assertFalse(self.presenter.custom_spectra_enabled)
        self.view.set_custom_calfile_widget_visible.assert_called_with()
        self.view.set_custom_spectra_widget_hidden.assert_called_with()

    def test_combo_changed_index_bank_1(self):
        self.presenter.on_combo_changed(1)

        self.assertEqual(self.presenter.group, GROUP.NORTH)
        self.assertFalse(self.presenter.custom_calfile_enabled)
        self.assertFalse(self.presenter.custom_spectra_enabled)
        self.view.set_custom_calfile_widget_hidden.assert_called_with()
        self.view.set_custom_spectra_widget_hidden.assert_called_with()

    def test_combo_changed_index_bank_2(self):
        self.presenter.on_combo_changed(2)

        self.assertEqual(self.presenter.group, GROUP.SOUTH)
        self.assertFalse(self.presenter.custom_calfile_enabled)
        self.assertFalse(self.presenter.custom_spectra_enabled)
        self.view.set_custom_calfile_widget_hidden.assert_called_with()
        self.view.set_custom_spectra_widget_hidden.assert_called_with()

    def test_combo_changed_index_custom(self):
        self.model.validate_and_clean_spectrum_numbers.return_value = ("", "1400")
        self.presenter.on_combo_changed(3)

        self.assertEqual(self.presenter.group, GROUP.CROPPED)
        self.assertFalse(self.presenter.custom_calfile_enabled)
        self.assertTrue(self.presenter.custom_spectra_enabled)
        self.view.set_custom_calfile_widget_hidden.assert_called_with()
        self.view.set_custom_spectra_widget_visible.assert_called_with()

    def test_custom_spectra_changed_valid(self):
        self.model.validate_and_clean_spectrum_numbers.return_value = ("", "1-5, 3-45")

        self.presenter.on_spectra_changed("1-5, 45-3")

        self.assertEqual("1-5, 3-45", self.presenter.custom_spectra)
        self.assertTrue(self.presenter.spectra_valid)
        self.view.set_crop_invalid_indicator_hidden.assert_called_with()

    def test_custom_spectra_changed_invalid(self):
        self.model.validate_and_clean_spectrum_numbers.return_value = ("Error lol", "")

        self.presenter.on_spectra_changed("1-5, 45-3")

        self.assertIsNone(self.presenter.custom_spectra)
        self.assertFalse(self.presenter.spectra_valid)
        self.view.set_crop_invalid_indicator_visible.assert_called_with("Error lol")

    def test_custom_calfile_changed_valid(self):
        self.view.get_custom_calfile.return_value = "stuff/mycalfile.cal"
        self.presenter.on_calfile_changed()

        self.assertEqual("stuff/mycalfile.cal", self.presenter.custom_calfile)
        self.assertTrue(self.presenter.calfile_valid)

    def test_custom_calfile_changed_invalid(self):
        self.view.get_custom_calfile.return_value = "badstuff/notacalfile.jpg"
        self.view.finder_custom.isValid.return_value = None
        self.presenter.on_calfile_changed()

        self.assertIsNone(self.presenter.custom_calfile)
        self.assertFalse(self.presenter.calfile_valid)


if __name__ == "__main__":
    unittest.main()
