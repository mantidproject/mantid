# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_model import ShowSampleModel


class ShowSamplePresenter(object):
    def __init__(self, model, view, include_gauge_volume):
        self.tab_model = model
        self.sample_model = ShowSampleModel(inc_gauge_vol=include_gauge_volume)
        self.view = view
        self.include_gauge_volume = include_gauge_volume
        self.view.set_on_view_shape_requested(self._on_view_shape_clicked)

    def _view_shape(self, ws_name, fix_axes):
        self.sample_model.set_ws_name(ws_name)
        self.sample_model.set_fix_axes_to_sample(fix_axes)
        self._set_gauge_vol_str()
        ax_transform, ax_labels = output_settings.get_texture_axes_transform()
        self.sample_model.show_shape_plot(ax_transform, ax_labels)

    def _on_view_shape_clicked(self, ws_name):
        self._view_shape(ws_name, True)

    def on_view_reference_shape_clicked(self):
        self._view_shape(self.tab_model.get_reference_ws(), False)

    def _set_gauge_vol_str(self):
        if self.include_gauge_volume:
            self.sample_model.set_gauge_vol_str(
                self.tab_model.get_gauge_vol_str(self.view.get_shape_method(), self.view.get_custom_shape())
            )
