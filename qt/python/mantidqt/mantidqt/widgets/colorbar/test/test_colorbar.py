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
from mantidqt.widgets.colorbar.colorbar import ColorbarWidget
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
        normalisation_index = 1  # Log
        image = plt.imshow(self.data, cmap="plasma", norm=LogNorm(vmin=0.01, vmax=1.0))
        masked_image = plt.imshow(self.masked_data, cmap="plasma", norm=LogNorm(vmin=0.01, vmax=1.0))

        self.widget.set_mappable(image)
        self.widget.norm.setCurrentIndex(normalisation_index)
        self.widget.set_mappable(masked_image)

        self.assertEqual(self.widget.norm.currentIndex(), 0)  # Linear
        self.assertTrue(isinstance(masked_image.norm, Normalize))

    def test_that_masked_data_in_symmetric_log_mode_will_cause_a_switch_back_to_linear_normalisation(self):
        normalisation_index = 2  # SymmetricLog10
        image = plt.imshow(self.data, cmap="plasma", norm=SymLogNorm(1e-8, vmin=0.01, vmax=1.0))
        masked_image = plt.imshow(self.masked_data, cmap="plasma", norm=SymLogNorm(1e-8, vmin=0.01, vmax=1.0))

        self.widget.set_mappable(image)
        self.widget.norm.setCurrentIndex(normalisation_index)
        self.widget.set_mappable(masked_image)

        self.assertEqual(self.widget.norm.currentIndex(), 0)  # Linear
        self.assertTrue(isinstance(masked_image.norm, Normalize))
