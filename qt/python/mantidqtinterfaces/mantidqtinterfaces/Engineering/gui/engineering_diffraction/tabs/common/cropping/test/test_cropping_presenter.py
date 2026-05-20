# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from enum import Enum
from unittest import mock

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.cropping import (
    cropping_model,
    cropping_presenter,
    cropping_view,
)

dir_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.cropping"


class GROUP(Enum):
    CUSTOM = 1
    NORTH = 2
    SOUTH = 3
    CROPPED = 4


class CroppingPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(cropping_view.CroppingView, instance=True)
        self.view.finder_custom = mock.MagicMock()
        self.view.finder_custom.isValid.return_value = True
        self.view.get_custom_spectra_text.return_value = "1400"

        self.parent = mock.MagicMock()

        self.model = mock.create_autospec(cropping_model.CroppingModel, instance=True)

        # Dynamic options which might be returned by the model: (GROUP, description, file input?, spectra input?)
        self.options = [
            (GROUP.CUSTOM, "Custom grouping file", True, False),
            (GROUP.NORTH, "North bank", False, False),
            (GROUP.SOUTH, "South bank", False, False),
            (GROUP.CROPPED, "Cropped spectra", False, True),
        ]
        self.model.get_cropping_options.return_value = self.options
        self.model.validate_and_clean_spectrum_numbers.return_value = ("", "1400")

        # __init__ calls on_combo_changed(0), so ensure options are ready before creating presenter
        self.presenter = cropping_presenter.CroppingPresenter(self.parent, self.view, self.model)

    def test_init_connects_view_signals_and_sets_default_combo_index_0(self):
        self.view.set_on_combo_changed.assert_called_once()
        self.view.set_on_custom_groupingfile_changed.assert_called_once()
        self.view.set_on_custom_spectra_changed.assert_called_once()

        # Since __init__ triggers on_combo_changed(0), index 0 should be applied
        self.assertEqual(self.presenter.group, GROUP.CUSTOM)
        self.assertTrue(self.presenter.custom_groupingfile_enabled)
        self.assertFalse(self.presenter.custom_spectra_enabled)

    def test_combo_changed_index_0_custom_groupingfile(self):
        self.presenter.on_combo_changed(0)

        self.assertEqual(self.presenter.group, GROUP.CUSTOM)
        self.assertTrue(self.presenter.custom_groupingfile_enabled)
        self.assertFalse(self.presenter.custom_spectra_enabled)

        self.view.set_custom_groupingfile_widget_visible.assert_called()
        self.view.set_custom_spectra_widget_hidden.assert_called()

    def test_combo_changed_index_1_no_custom_widgets(self):
        self.presenter.on_combo_changed(1)

        self.assertEqual(self.presenter.group, GROUP.NORTH)
        self.assertFalse(self.presenter.custom_groupingfile_enabled)
        self.assertFalse(self.presenter.custom_spectra_enabled)

        self.view.set_custom_groupingfile_widget_hidden.assert_called()
        self.view.set_custom_spectra_widget_hidden.assert_called()

    def test_combo_changed_index_2_no_custom_widgets(self):
        self.presenter.on_combo_changed(2)

        self.assertEqual(self.presenter.group, GROUP.SOUTH)
        self.assertFalse(self.presenter.custom_groupingfile_enabled)
        self.assertFalse(self.presenter.custom_spectra_enabled)

        self.view.set_custom_groupingfile_widget_hidden.assert_called()
        self.view.set_custom_spectra_widget_hidden.assert_called()

    def test_combo_changed_index_3_custom_spectra(self):
        self.model.validate_and_clean_spectrum_numbers.return_value = ("", "1400")
        self.view.get_custom_spectra_text.return_value = "1400"

        self.presenter.on_combo_changed(3)

        self.assertEqual(self.presenter.group, GROUP.CROPPED)
        self.assertFalse(self.presenter.custom_groupingfile_enabled)
        self.assertTrue(self.presenter.custom_spectra_enabled)

        self.view.set_custom_groupingfile_widget_hidden.assert_called()
        self.view.set_custom_spectra_widget_visible.assert_called()

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

    def test_custom_groupingfile_changed_valid(self):
        self.view.get_custom_groupingfile.return_value = "stuff/mygroupfile.cal"
        self.view.finder_custom.isValid.return_value = True

        self.presenter.on_groupingfile_changed()

        self.assertEqual("stuff/mygroupfile.cal", self.presenter.custom_groupingfile)
        self.assertTrue(self.presenter.groupingfile_valid)

    def test_custom_groupingfile_changed_invalid(self):
        self.view.get_custom_groupingfile.return_value = "badstuff/notacalfile.jpg"
        self.view.finder_custom.isValid.return_value = False

        self.presenter.on_groupingfile_changed()

        self.assertIsNone(self.presenter.custom_groupingfile)
        self.assertFalse(self.presenter.groupingfile_valid)

    def test_set_custom_widgets_visibility_none_visible_hides_both_and_sets_valid(self):
        # Force invalid then check it becomes valid when widgets are not used
        self.presenter.groupingfile_valid = False
        self.presenter.spectra_valid = False

        self.presenter.set_custom_widgets_visibility(custom_visible=False, crop_visible=False)

        self.view.set_custom_groupingfile_widget_hidden.assert_called()
        self.view.set_custom_spectra_widget_hidden.assert_called()
        self.assertTrue(self.presenter.groupingfile_valid)
        self.assertTrue(self.presenter.spectra_valid)

    def test_set_custom_widgets_visibility_custom_visible_calls_groupingfile_validation_and_shows_widget(self):
        self.view.finder_custom.isValid.return_value = True
        self.view.get_custom_groupingfile.return_value = "stuff/custom.xml"

        self.presenter.set_custom_widgets_visibility(custom_visible=True, crop_visible=False)

        self.view.set_custom_spectra_widget_hidden.assert_called()
        self.view.set_custom_groupingfile_widget_visible.assert_called()
        self.assertTrue(self.presenter.spectra_valid)
        self.assertTrue(self.presenter.groupingfile_valid)
        self.assertEqual(self.presenter.custom_groupingfile, "stuff/custom.xml")

    def test_set_custom_widgets_visibility_crop_visible_validates_spectra_and_shows_widget(self):
        self.model.validate_and_clean_spectrum_numbers.return_value = ("", "1400")
        self.view.get_custom_spectra_text.return_value = "1400"

        self.presenter.set_custom_widgets_visibility(custom_visible=False, crop_visible=True)

        self.view.set_custom_groupingfile_widget_hidden.assert_called()
        self.view.set_custom_spectra_widget_visible.assert_called()
        self.assertTrue(self.presenter.groupingfile_valid)
        self.assertTrue(self.presenter.spectra_valid)
        self.assertEqual(self.presenter.custom_spectra, "1400")

    def test_set_cropping_options_sets_combo_descriptions_from_model(self):
        self.presenter.set_cropping_options()

        expected = [opt[1] for opt in self.options]
        self.view.set_combo_options.assert_called_with(expected)

    def test_set_instrument_override_updates_instrument_refreshes_options_and_resets_index_0(self):
        # New instrument has different options, prove combo gets refreshed
        new_options = [
            (GROUP.NORTH, "Only option now", False, False),
        ]
        self.model.get_cropping_options.return_value = new_options

        # Clear calls from init
        self.view.reset_mock()
        self.model.reset_mock()

        # INSTRUMENT_DICT assumed to be {0: ENGINX, 1: IMAT}
        self.presenter.set_instrument_override(1)

        self.assertEqual(self.presenter.instrument, "IMAT")
        self.view.set_combo_options.assert_called_with(["Only option now"])
        self.assertEqual(self.presenter.group, GROUP.NORTH)


if __name__ == "__main__":
    unittest.main()
