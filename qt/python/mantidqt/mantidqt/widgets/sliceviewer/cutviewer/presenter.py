# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
from mantidqt.widgets.sliceviewer.cutviewer.view import CutViewerView
from mantidqt.widgets.sliceviewer.cutviewer.model import CutViewerModel


class CutViewerPresenter:
    def __init__(self, sliceviewer_presenter, canvas):
        """
        :param painter: An object responsible for drawing the representation of the cut
        :param sliceinfo_provider: An object responsible for providing access to current slice information
        :param parent: An optional parent widget
        """
        self.view = CutViewerView(self, canvas, sliceviewer_presenter.get_frame())
        self.model = CutViewerModel(sliceviewer_presenter.get_proj_matrix())
        self._sliceview_presenter = sliceviewer_presenter

    def show_view(self):
        self.view.show()
        self.reset_view_table()
        self.update_cut()

    def hide_view(self):
        self.view.hide()

    def get_view(self):
        return self.view

    def reset_view_table(self):
        self.view.set_bin_params(
            *self.model.get_default_bin_params(
                self._sliceview_presenter.get_dimensions(),
                self._sliceview_presenter.get_data_limits_to_fill_current_axes(),
                self._sliceview_presenter.get_sliceinfo().z_value,
            )
        )

    def validate_bin_params(self, irow, icol):
        iunchanged = int(not bool(irow))  # index of u1 or u2 - which ever not changed (3rd row not editable)
        vectors, extents, nbins = self.view.get_bin_params()
        if icol < 3:
            vectors = self.model.validate_vectors(irow, iunchanged, vectors)
        elif icol == 5:
            nbins = self.model.validate_nbins(irow, iunchanged, nbins)
        elif icol == 6:
            nbins, extents = self.model.validate_step(irow, iunchanged, nbins, extents, self.view.get_step(irow))
        else:
            extents = self.model.validate_extents(irow, extents)
        return vectors, extents, nbins

    def update_cut(self):
        vectors, extents, nbins = self.view.get_bin_params()
        if self.model.valid_bin_params(vectors, extents, nbins):
            self._sliceview_presenter.perform_non_axis_aligned_cut(vectors, extents.flatten(order="F"), nbins)

    def get_cut_representation_parameters(self):
        cut_rep_params = self.model.calc_cut_representation_parameters(
            *self.view.get_bin_params(), self._sliceview_presenter.get_dimensions().get_states()
        )
        return *cut_rep_params, self._sliceview_presenter.get_sliceinfo().get_northogonal_transform()

    def update_bin_params_from_cut_representation(self, xmin, xmax, ymin, ymax, thickness):
        vectors, _, _ = self.view.get_bin_params()
        self.view.set_bin_params(*self.model.calc_bin_params_from_cut_representation(xmin, xmax, ymin, ymax, thickness, vectors[-1, :]))
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
