from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantid.kernel import logger
from mantidqt.plotting import sample_shape


class ShowSamplePresenter(object):
    def __init__(self, model, view, include_gauge_volume):
        self.model = model
        self.view = view
        self.include_gauge_volume = include_gauge_volume
        self.view.sig_view_shape_requested.connect(self._on_view_shape_clicked)

    def _on_view_shape_generic(self, ws_name, fix_axes_to_sample):
        try:
            fig = sample_shape.plot_sample_container_and_components(ws_name, alpha=0.5, custom_color="grey")
            ax_transform, ax_labels = output_settings.get_texture_axes_transform()
            self.model.plot_sample_directions(fig, ws_name, ax_transform, ax_labels, fix_axes_to_sample)
            if self.include_gauge_volume:
                self._add_gauge_vol_view(fig)
        except Exception as exception:
            logger.warning("Could not show sample shape for workspace '{}':\n{}\n".format(ws_name, exception))

    def _on_view_shape_clicked(self, ws_name):
        self._on_view_shape_generic(ws_name, True)

    def on_view_reference_shape_clicked(self):
        self._on_view_shape_generic(self.model.reference_ws, False)

    def _add_gauge_vol_view(self, fig):
        if self.view.include_absorption():
            self.model.plot_gauge_vol(self.view.get_shape_method(), self.view.get_custom_shape(), fig)
