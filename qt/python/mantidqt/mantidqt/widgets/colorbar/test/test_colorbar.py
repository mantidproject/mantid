# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import matplotlib.pyplot as plt
import numpy as np

from unittest import mock, TestCase

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.colorbar.colorbar import ColorbarWidget


@start_qapplication
class ColorbarWidgetTest(TestCase):

    @classmethod
    def setUpClass(cls) -> None:
        cls.data = np.arange(100.0).reshape((10, 10))
        cls.masked_data = np.ma.masked_array(cls.data, mask=[list(range(1, 11))]*10)

    def setUp(self) -> None:
        self.fig = plt.figure(figsize=(1, 1))
        self.image = plt.imshow(self.data, cmap="plasma")
        self.masked_image = plt.imshow(self.masked_data, cmap="plasma")

        self.widget = ColorbarWidget()
        self.widget.cmin_value = 0
        self.widget.set_mappable(self.image)

    def tearDown(self) -> None:
        plt.close('all')
        self.fig, self.image, self.masked_image = None, None, None
        self.widget = None

    def test_that_masked_data_will_not_allow_negative_cmin_for_log_normalisation(self):
        normalisation_index = 1  # Log

        self.widget.norm.setCurrentIndex(normalisation_index)
        self.widget.set_mappable(self.masked_image)

        self.assertGreaterEqual(self.widget.cmin_value, 0.0)

    def test_that_masked_data_will_not_cause_error_when_switching_to_symmetric_log_normalisation(self):
        normalisation_index = 2  # SymmetricLog10

        self.widget.norm.setCurrentIndex(normalisation_index)
        self.widget.set_mappable(self.masked_image)

        with mock.patch('qtpy.QtWidgets.QComboBox.currentIndex') as patch:
            patch.return_value = normalisation_index

            self.widget.norm_changed()
