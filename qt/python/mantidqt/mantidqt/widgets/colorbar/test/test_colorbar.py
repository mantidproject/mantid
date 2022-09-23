# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import matplotlib.pyplot as plt
import numpy as np

from unittest import TestCase

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.colorbar.colorbar import ColorbarWidget, NORM_OPTS
from matplotlib.colors import LogNorm, Normalize, SymLogNorm


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
        plt.close('all')
        self.fig, self.widget = None, None

    def test_that_an_integer_vmax_will_not_cause_an_error_when_the_clim_gets_updated(self):
        image = plt.imshow(self.data, cmap="plasma", norm=LogNorm(vmin=0.01, vmax=1))

        # The clim gets updated here
        self.widget.set_mappable(image)

    def test_that_masked_data_in_log_mode_will_cause_a_switch_back_to_linear_normalisation(self):
        image = plt.imshow(self.data, cmap="plasma", norm=LogNorm(vmin=0.01, vmax=1.0))
        masked_image = plt.imshow(self.masked_data, cmap="plasma", norm=LogNorm(vmin=0.01, vmax=1.0))

        self.widget.set_mappable(image)
        self.widget.norm.setCurrentIndex(NORM_OPTS.index("Log"))
        self.widget.set_mappable(masked_image)

        self.assertEqual(self.widget.norm.currentIndex(), NORM_OPTS.index("Linear"))
        self.assertTrue(isinstance(masked_image.norm, Normalize))

    def test_that_masked_data_in_symmetric_log_mode_will_cause_a_switch_back_to_linear_normalisation(self):
        image = plt.imshow(self.data, cmap="plasma", norm=SymLogNorm(1e-8, vmin=0.01, vmax=1.0))
        masked_image = plt.imshow(self.masked_data, cmap="plasma", norm=SymLogNorm(1e-8, vmin=0.01, vmax=1.0))

        self.widget.set_mappable(image)
        self.widget.norm.setCurrentIndex(NORM_OPTS.index("SymmetricLog10"))
        self.widget.set_mappable(masked_image)

        self.assertEqual(self.widget.norm.currentIndex(), NORM_OPTS.index("Linear"))
        self.assertTrue(isinstance(masked_image.norm, Normalize))

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

    def test_that_all_zero_slice_with_log_normalisation_gives_valid_clim(self):
        image = plt.imshow(self.data*0, cmap="plasma", norm=Normalize(vmin=None, vmax=None))

        self.widget.set_mappable(image)
        self.widget.autoscale.setChecked(True)

        self.widget.norm.setCurrentIndex(NORM_OPTS.index("Log"))
        c_min, c_max = self.widget._calculate_clim()

        self.assertEqual(c_min, 0.1)
        self.assertEqual(c_max, 1)

    def test_that_all_nan_slice_with_log_normalisation_gives_valid_clim(self):
        image = plt.imshow(self.data*np.nan, cmap="plasma", norm=Normalize(vmin=None, vmax=None))

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
