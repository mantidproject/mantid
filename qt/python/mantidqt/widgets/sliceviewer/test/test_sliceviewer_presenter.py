# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)

import matplotlib
matplotlib.use('Agg') # noqa: E402
import unittest

import mantid.api
from mantid.py3compat import mock
from mantidqt.widgets.sliceviewer.model import SliceViewerModel, WS_TYPE
from mantidqt.widgets.sliceviewer.presenter import SliceViewer
from mantidqt.widgets.sliceviewer.view import SliceViewerView


class SliceViewerTest(unittest.TestCase):

    def setUp(self):
        self.view = mock.Mock(spec=SliceViewerView)
        self.view.dimensions = mock.Mock()
        self.view.norm_opts = mock.Mock()

        self.model = mock.Mock(spec=SliceViewerModel)
        self.model.get_ws = mock.Mock()
        self.model.get_data = mock.Mock()

    def test_sliceviewer_MDH(self):

        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MDH)

        presenter = SliceViewer(None, model=self.model, view=self.view)

        # setup calls
        self.assertEqual(self.model.get_dimensions_info.call_count, 0)
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.view.dimensions.get_slicepoint.call_count, 1)
        self.assertEqual(self.view.plot_MDH.call_count, 1)

        # new_plot
        self.model.reset_mock()
        self.view.reset_mock()
        presenter.new_plot()
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.view.dimensions.get_slicepoint.call_count, 1)
        self.assertEqual(self.view.plot_MDH.call_count, 1)

        # update_plot_data
        self.model.reset_mock()
        self.view.reset_mock()
        presenter.update_plot_data()
        self.assertEqual(self.model.get_data.call_count, 1)
        self.assertEqual(self.view.dimensions.get_slicepoint.call_count, 1)
        self.assertEqual(self.view.update_plot_data.call_count, 1)

    def test_sliceviewer_MDE(self):

        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MDE)

        presenter = SliceViewer(None, model=self.model, view=self.view)

        # setup calls
        self.assertEqual(self.model.get_dimensions_info.call_count, 0)
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.view.dimensions.get_slicepoint.call_count, 1)
        self.assertEqual(self.view.dimensions.get_bin_params.call_count, 1)
        self.assertEqual(self.view.plot_MDH.call_count, 1)

        # new_plot
        self.model.reset_mock()
        self.view.reset_mock()
        presenter.new_plot()
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.view.dimensions.get_slicepoint.call_count, 1)
        self.assertEqual(self.view.dimensions.get_bin_params.call_count, 1)
        self.assertEqual(self.view.plot_MDH.call_count, 1)

        # update_plot_data
        self.model.reset_mock()
        self.view.reset_mock()
        presenter.update_plot_data()
        self.assertEqual(self.model.get_data.call_count, 1)
        self.assertEqual(self.view.dimensions.get_slicepoint.call_count, 1)
        self.assertEqual(self.view.dimensions.get_bin_params.call_count, 1)
        self.assertEqual(self.view.update_plot_data.call_count, 1)

    def test_sliceviewer_matrix(self):

        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MATRIX)

        presenter = SliceViewer(None, model=self.model, view=self.view)

        # setup calls
        self.assertEqual(self.model.get_dimensions_info.call_count, 0)
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.view.dimensions.get_slicepoint.call_count, 0)
        self.assertEqual(self.view.plot_matrix.call_count, 1)

        # new_plot
        self.model.reset_mock()
        self.view.reset_mock()
        presenter.new_plot()
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.assertEqual(self.view.dimensions.get_slicepoint.call_count, 0)
        self.assertEqual(self.view.plot_matrix.call_count, 1)

    def test_normalization_change_set_correct_normalization(self):
        self.model.get_ws_type = mock.Mock(return_value=WS_TYPE.MATRIX)
        self.view.plot_matrix = mock.Mock()

        presenter = SliceViewer(None, model=self.model, view=self.view)
        presenter.normalization_changed("By bin width")
        self.view.plot_matrix.assert_called_with(self.model.get_ws(),
                                                 normalize=mantid.api.MDNormalization.VolumeNormalization)


if __name__ == '__main__':
    unittest.main()
