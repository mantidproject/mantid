# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
import unittest
from unittest import mock
from numpy import eye, c_, array, tile, array_equal, arange

from mantidqt.widgets.sliceviewer.cutviewer.presenter import CutViewerPresenter
from mantidqt.widgets.sliceviewer.cutviewer.view import CutViewerView
from mantidqt.widgets.sliceviewer.cutviewer.model import CutViewerModel
from mantidqt.widgets.sliceviewer.presenters.presenter import SliceViewer


class TestCutViewerModel(unittest.TestCase):
    def setUp(self):
        # load empty instrument so can create a peak table
        mock_view = mock.create_autospec(CutViewerView)
        mock_model = mock.create_autospec(CutViewerModel)
        self.mock_sv_presenter = mock.create_autospec(SliceViewer)
        self.presenter = CutViewerPresenter(self.mock_sv_presenter, mock_model, mock_view)

    def test_on_cut_done(self):
        self.presenter.on_cut_done("wsname")
        self.presenter.view.plot_cut_ws.assert_called_once_with("wsname")

    @mock.patch("mantidqt.widgets.sliceviewer.cutviewer.presenter.CutViewerPresenter.get_cut_representation_parameters")
    @mock.patch("mantidqt.widgets.sliceviewer.cutviewer.presenter.CutViewerPresenter.update_cut")
    def test_show_view(self, mock_update_cut, mock_get_cut_rep):
        mock_get_cut_rep.return_value = 6 * [None]  # xmin, xmax, ymin, ymax, thickness, axes_transform
        in_vecs = eye(3)
        in_extents = arange(6).reshape(2, 3)
        in_nbins = array([10, 1, 1])
        self.presenter.model.get_default_bin_params.return_value = in_vecs, in_extents, in_nbins  # vecs, extents, nbins

        self.presenter.show_view()

        self.presenter.view.show.assert_called_once()
        self.presenter.view.set_bin_params.assert_called_once_with(in_vecs, in_extents, in_nbins)
        mock_update_cut.assert_called_once()
        mock_get_cut_rep.assert_called_once()
        args = mock_get_cut_rep.call_args[0]  # for some reason assert_called_once_with struggles wth np arrays here
        self.assertTrue(array_equal(in_vecs[:-1, :], args[0]))  # only has 2D slice
        self.assertTrue(array_equal(in_extents[:, :-1], args[1]))
        self.assertTrue(array_equal(in_nbins[:-1], args[2]))

    def test_update_cut_with_valid_bin_params(self):
        in_vecs = eye(3)
        in_extents = tile(c_[[0.0, 1.0]], (1, 3))
        in_nbins = array([10, 1, 1])
        self.presenter.view.get_vector.side_effect = lambda irow: in_vecs[irow, :]
        self.presenter.view.get_extents.side_effect = lambda irow: in_extents[:, irow]
        self.presenter.view.get_nbin.side_effect = lambda irow: in_nbins[irow]
        self.presenter.model.valid_bin_params.return_value = True

        self.presenter.update_cut()

        out_vecs, out_extents, out_nbins = self.mock_sv_presenter.perform_non_axis_aligned_cut.call_args[0]
        self.assertTrue(array_equal(in_vecs, out_vecs))
        self.assertTrue(array_equal(in_extents.flatten(order="F"), out_extents))
        self.assertTrue(array_equal(in_nbins, out_nbins))

    @mock.patch("mantidqt.widgets.sliceviewer.cutviewer.presenter.CutViewerPresenter.get_bin_params_from_view")
    def test_update_cut_with_invalid_bin_params(self, mock_get_bin_params):
        mock_get_bin_params.return_value = 3 * [None]
        self.presenter.model.valid_bin_params.return_value = False

        self.presenter.update_cut()

        self.mock_sv_presenter.perform_non_axis_aligned_cut.assert_not_called()

    @mock.patch("mantidqt.widgets.sliceviewer.cutviewer.presenter.CutViewerPresenter.update_cut")
    def test_update_bin_params_from_cut_representation(self, mock_update_cut):
        xmin, xmax, ymin, ymax, thickness = 0, 1, 0, 1, 0.1
        self.presenter.model.calc_bin_params_from_cut_representation.return_value = 3 * [None]  # vecs, extents, nbins
        self.presenter.model.calc_cut_representation_parameters.return_value = xmin, xmax, ymin, ymax, thickness
        in_vecs = eye(3)
        self.presenter.view.get_vector.side_effect = lambda irow: in_vecs[irow, :]
        self.presenter.view.get_extents.return_value = [-1, 1]  # ignored
        self.presenter.view.get_nbin.return_value = 2  # ignored

        self.presenter.update_bin_params_from_cut_representation(xmin, xmax, ymin, ymax, thickness)

        mock_update_cut.assert_called_once()
        # check last vector passed as out of plane vector
        out_plane_vec = self.presenter.model.calc_bin_params_from_cut_representation.call_args[0][-1]
        self.assertTrue(array_equal(out_plane_vec, array([0, 0, 1])))

    @mock.patch("mantidqt.widgets.sliceviewer.cutviewer.presenter.CutViewerPresenter.update_cut")
    def test_on_slicepoint_changed(self, mock_update_cut):
        self.mock_sv_presenter.get_sliceinfo.return_value.z_value = 1.0
        self.presenter.model.get_slicewidth.return_value = 0.1

        self.presenter.on_slicepoint_changed()

        self.presenter.view.set_slicepoint.assert_called_with(1.0, 0.1)
        mock_update_cut.assert_called_once()


if __name__ == "__main__":
    unittest.main()
