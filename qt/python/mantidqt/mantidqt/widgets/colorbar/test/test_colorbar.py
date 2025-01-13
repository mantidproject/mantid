# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from unittest.mock import patch

import matplotlib.pyplot as plt
import numpy as np

from unittest import TestCase, mock

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.colorbar.colorbar import ColorbarWidget, NORM_OPTS
from matplotlib.colors import LogNorm, Normalize, SymLogNorm, ListedColormap


@start_qapplication
class ColorbarWidgetTest(TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.data = np.arange(100.0).reshape((10, 10))
        cls.masked_data = np.ma.masked_array(cls.data, mask=True)

    def setUp(self) -> None:
        self.fig = plt.figure(figsize=(1, 1))

        self.widget = ColorbarWidget()
        self.widget.cmin_value = 0

    def tearDown(self) -> None:
        plt.close("all")
        self.fig, self.widget = None, None

    def test_that_an_integer_vmax_will_not_cause_an_error_when_the_clim_gets_updated(self):
        image = plt.imshow(self.data, cmap="plasma", norm=LogNorm(vmin=0.01, vmax=1))

        # The clim gets updated here
        self.widget.set_mappable(image)

    def test_that_switching_normalisation_will_autoscale_the_colorbar_limits_appropriately(self):
        image = plt.imshow(self.data, cmap="plasma", norm=SymLogNorm(1e-8, vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)
        c_min, c_max = self.widget._calculate_clim()

        for normalisation_index in range(len(NORM_OPTS)):
            self.widget.norm.setCurrentIndex(normalisation_index)
            expected_c_min = c_min if normalisation_index != NORM_OPTS.index("Log") else max(c_min, 1)

            self.assertEqual(self.widget.cmin_value, expected_c_min)
            self.assertEqual(self.widget.cmax_value, c_max)

    @patch("mantidqt.widgets.colorbar.colorbar.get_current_cmap")
    def test_cmap_values_are_replaced_correctly(self, mocked_cmap):
        image = plt.imshow(self.data, cmap="plasma", norm=Normalize(vmin=None, vmax=None))
        mock_cmap = mock.MagicMock()
        mock_cmap.name = "gist_rainbow"
        mocked_cmap.return_value = mock_cmap
        self.widget.set_mappable(image)

    def test_that_all_zero_slice_with_log_normalisation_gives_valid_clim(self):
        image = plt.imshow(self.data * 0, cmap="plasma", norm=Normalize(vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)

        self.widget.norm.setCurrentIndex(NORM_OPTS.index("Log"))
        c_min, c_max = self.widget._calculate_clim()

        self.assertEqual(c_min, 0.1)
        self.assertEqual(c_max, 1)

    def test_that_all_nan_slice_with_log_normalisation_gives_valid_clim(self):
        image = plt.imshow(self.data * np.nan, cmap="plasma", norm=Normalize(vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)

        for normalisation_index in range(len(NORM_OPTS)):
            self.widget.norm.setCurrentIndex(normalisation_index)
            c_min, c_max = self.widget._calculate_clim()

            self.assertEqual(c_min, 0.1)
            self.assertEqual(c_max, 1)

    def test_mixed_slice_gives_valid_clim(self):
        image = plt.imshow(self.data, cmap="plasma", norm=Normalize(vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)

        for normalisation_index in range(len(NORM_OPTS)):
            self.widget.norm.setCurrentIndex(normalisation_index)
            c_min, c_max = self.widget._calculate_clim()
            expected_c_min = 0 if normalisation_index != NORM_OPTS.index("Log") else 1

            self.assertEqual(c_min, expected_c_min)
            self.assertEqual(c_max, 99)

    def test_colorbar_limits_min_max(self):
        image = plt.imshow(self.data * np.nan, cmap="plasma", norm=Normalize(vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)
        self.widget.autotype.setCurrentIndex(0)

        vmin, vmax = self.widget._calculate_auto_color_limits(self.data)

        self.assertEqual(vmin, 0.0)
        self.assertEqual(vmax, 99.0)

    def test_colorbar_limits_3sigma(self):
        image = plt.imshow(self.data * np.nan, cmap="plasma", norm=Normalize(vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)
        self.widget.autotype.setCurrentIndex(1)

        vmin, vmax = self.widget._calculate_auto_color_limits(self.data)

        self.assertEqual(vmin, 0.0)
        self.assertEqual(vmax, 99.0)

    def test_colorbar_limits_15interquartile(self):
        image = plt.imshow(self.data * np.nan, cmap="plasma", norm=Normalize(vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)
        self.widget.autotype.setCurrentIndex(2)

        vmin, vmax = self.widget._calculate_auto_color_limits(self.data)

        self.assertEqual(vmin, 0.0)
        self.assertEqual(vmax, 99.0)

    def test_colorbar_limits_15median(self):
        image = plt.imshow(self.data * np.nan, cmap="plasma", norm=Normalize(vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)
        self.widget.autotype.setCurrentIndex(3)

        vmin, vmax = self.widget._calculate_auto_color_limits(self.data)

        self.assertEqual(vmin, 12.0)
        self.assertEqual(vmax, 87.0)

    def test_colorbar_limits_min_max_lognorm(self):
        image = plt.imshow(self.data * np.nan, cmap="plasma", norm=Normalize(vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)
        self.widget.autotype.setCurrentIndex(0)
        self.widget.norm.setCurrentIndex(1)

        vmin, vmax = self.widget._calculate_auto_color_limits(self.data)

        self.assertEqual(vmin, 0.0099)
        self.assertEqual(vmax, 99.0)

    def test_colorbar_limits_min_max_symmetric_log10(self):
        image = plt.imshow(self.data * np.nan, cmap="plasma", norm=Normalize(vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)
        self.widget.autotype.setCurrentIndex(0)
        self.widget.norm.setCurrentIndex(2)

        vmin, vmax = self.widget._calculate_auto_color_limits(self.data)

        self.assertEqual(vmin, 0.0)
        self.assertEqual(vmax, 99.0)

    def test_colorbar_limits_min_max_power(self):
        image = plt.imshow(self.data * np.nan, cmap="plasma", norm=Normalize(vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)
        self.widget.autotype.setCurrentIndex(0)
        self.widget.norm.setCurrentIndex(3)

        vmin, vmax = self.widget._calculate_auto_color_limits(self.data)

        self.assertEqual(vmin, 0.0)
        self.assertEqual(vmax, 99.0)

    def test_invalid_cmax_range_is_reset(self):
        image = plt.imshow(self.data, cmap="plasma", norm=SymLogNorm(1e-8, vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)

        self.widget.cmax_value = 10
        # less than cmin_value therefore invalid
        self.widget.cmax.setText("-10")
        self.widget.clim_changed()

        self.assertEqual("10.0", self.widget.cmax.text())

    def test_invalid_cmin_range_is_reset(self):
        image = plt.imshow(self.data, cmap="plasma", norm=SymLogNorm(1e-8, vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)

        self.widget.cmax_value = 10
        self.widget.cmin_value = 0
        # greater than cmax_value therefore invalid
        self.widget.cmin.setText("20")
        self.widget.clim_changed()

        self.assertEqual("0.0", self.widget.cmin.text())

    def test_invalid_cmax_syntax_is_reset(self):
        image = plt.imshow(self.data, cmap="plasma", norm=SymLogNorm(1e-8, vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)
        self.widget.cmax_value = 10

        for value in ("0,1", "0,001"):
            self.widget.cmax.setText(value)
            self.widget.clim_changed()

            self.assertEqual("10.0", self.widget.cmax.text())

    def test_invalid_cmin_syntax_is_reset(self):
        image = plt.imshow(self.data, cmap="plasma", norm=SymLogNorm(1e-8, vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)
        self.widget.cmin_value = 0

        for value in ("0,1", "0,001"):
            self.widget.cmin.setText(value)
            self.widget.clim_changed()

            self.assertEqual("0.0", self.widget.cmin.text())

    def test_custom_colormaps(self):
        # test that users can register custom colormaps and have it as an option in the colorbar
        cmap_name = "custom_cmap"
        self.assertFalse(cmap_name in plt.colormaps)
        self.assertTrue(self.widget.cmap.findText(cmap_name) == -1)

        plt.colormaps.register(cmap=ListedColormap([[0, 0, 0], [0, 0, 1]]), name=cmap_name)
        self.assertTrue(cmap_name in plt.colormaps)

        # now when you create a colorbar widget, the custom colormap should now be available
        cb = ColorbarWidget()
        self.assertTrue(cb.cmap.findText(cmap_name) != -1)

        plt.colormaps.unregister(cmap_name)
