# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
from numpy import zeros
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from mantidqt.widgets.sliceviewer.cutviewer.view import CutViewerView
    from mantidqt.widgets.sliceviewer.cutviewer.model import CutViewerModel
    from mantidqt.widgets.sliceviewer.presenters.presenter import SliceViewer


class CutViewerPresenter:
    def __init__(self, sliceviewer_presenter: "SliceViewer", model: "CutViewerModel", view: "CutViewerView"):
        """
        :param painter: An object responsible for drawing the representation of the cut
        :param sliceinfo_provider: An object responsible for providing access to current slice information
        :param parent: An optional parent widget
        """
        self.view = view
        self.model = model
        self._sliceview_presenter = sliceviewer_presenter
        self.view.subscribe_presenter(self)

    def show_view(self):
        self.view.show()
        self.reset_view_table()
        self.update_cut()

    def hide_view(self):
        self.view.hide()

    def get_view(self):
        return self.view

    def reset_view_table(self):
        vectors, extents, nbins = self.model.get_default_bin_params(
            self._sliceview_presenter.get_dimensions(),
            self._sliceview_presenter.get_data_limits_to_fill_current_axes(),
            self._sliceview_presenter.get_sliceinfo().z_value,
        )
        self.view.set_bin_params(vectors, extents, nbins)
        self.view.plot_cut_representation(*self.get_cut_representation_parameters(vectors[:-1, :], extents[:, :-1], nbins[:-1]))

    def validate_bin_params(self, irow, icol):
        iunchanged = int(not bool(irow))  # index of u1 or u2 - which ever not changed (3rd row not editable)
        vectors, extents, nbins = self.get_bin_params_from_view()
        if icol < 3:
            vectors = self.model.validate_vectors(irow, iunchanged, vectors)
        elif icol == 5:
            nbins = self.model.validate_nbins(irow, iunchanged, nbins)
        elif icol == 6:
            nbins, extents = self.model.validate_step(irow, iunchanged, nbins, extents, self.view.get_step(irow))
        else:
            extents = self.model.validate_extents(irow, extents)
        return vectors, extents, nbins

    def get_bin_params_from_view(self):
        vectors = zeros((3, 3), dtype=float)
        extents = zeros((2, 3), dtype=float)
        nbins = zeros(3, dtype=int)
        for ivec in range(vectors.shape[0]):
            vectors[ivec, :] = self.view.get_vector(ivec)
            extents[:, ivec] = self.view.get_extents(ivec)
            nbins[ivec] = self.view.get_nbin(ivec)
        return vectors, extents, nbins

    def update_cut(self):
        vectors, extents, nbins = self.get_bin_params_from_view()
        if self.model.valid_bin_params(vectors, extents, nbins):
            self._sliceview_presenter.perform_non_axis_aligned_cut(vectors, extents.flatten(order="F"), nbins)

    def get_cut_representation_parameters(self, vectors_in_plane, extents_in_plane, nbins_in_plane):
        xmin, xmax, ymin, ymax, thickness = self.model.calc_cut_representation_parameters(
            vectors_in_plane, extents_in_plane, nbins_in_plane, self._sliceview_presenter.get_dimensions().get_states()
        )
        axes_transform = self._sliceview_presenter.get_sliceinfo().get_northogonal_transform()
        return xmin, xmax, ymin, ymax, thickness, axes_transform

    def update_bin_params_from_cut_representation(self, xmin, xmax, ymin, ymax, thickness):
        out_plane_vector = self.view.get_vector(2)  # integrated dimension (last row in table)
        vectors_in_plane, extents_in_plane, nbins_in_plane = self.model.calc_bin_params_from_cut_representation(
            xmin, xmax, ymin, ymax, thickness, out_plane_vector
        )
        self.view.set_bin_params(vectors_in_plane, extents_in_plane, nbins_in_plane)
        self.view.plot_cut_representation(*self.get_cut_representation_parameters(vectors_in_plane, extents_in_plane, nbins_in_plane))
        self.update_cut()

    # signals
    def on_dimension_changed(self):
        self.reset_view_table()
        self.update_cut()

    def on_slicepoint_changed(self):
        slicept = self._sliceview_presenter.get_sliceinfo().z_value
        width = self.model.get_slicewidth(self._sliceview_presenter.get_dimensions())
        self.view.set_slicepoint(slicept, width)
        self.update_cut()

    def on_cut_done(self, wsname):
        self.view.plot_cut_ws(wsname)

    def handle_cell_changed(self, irow: int, icol: int):
        vectors, extents, nbins = self.validate_bin_params(irow, icol)
        self.view.set_bin_params(vectors, extents, nbins)
        self.update_cut()
        self.view.plot_cut_representation(*self.get_cut_representation_parameters(vectors[:-1, :], extents[:, :-1], nbins[:-1]))
