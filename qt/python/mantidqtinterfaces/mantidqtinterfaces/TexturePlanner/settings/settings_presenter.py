# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantidqtinterfaces.TexturePlanner.settings.settings_model import TexturePlannerSettingsModel


class TexturePlannerSettingsPresenter(object):
    def __init__(self, texture_model, view):
        self.texture_model = texture_model
        self.settings_model = TexturePlannerSettingsModel()
        self.view = view
        self._on_settings_applied = None

        self.view.set_on_ok_clicked(self.save_and_close)
        self.view.set_on_cancel_clicked(self.close)
        self.view.set_on_apply_clicked(self.save_settings)

    def set_on_settings_applied(self, slot):
        self._on_settings_applied = slot

    def load_settings_from_file_or_default(self):
        """Called on interface startup to restore previously saved settings into the model."""
        settings = self.settings_model.get_settings_dict()
        self._apply_settings_to_texture_model(settings)

    def show(self):
        self._populate_view_from_texture_model()
        self.view.show()

    def close(self):
        self.view.close()

    def save_and_close(self):
        self.save_settings()
        self.close()

    def save_settings(self):
        settings = self._collect_settings_from_view()
        self.settings_model.set_settings_dict(settings)
        self._apply_settings_to_texture_model(settings)
        if self._on_settings_applied is not None:
            self._on_settings_applied()

    # ========================
    # Internal helpers
    # ========================

    def _collect_settings_from_view(self):
        return {
            "directions": self.view.get_show_directions(),
            "goniometers": self.view.get_show_goniometers(),
            "incident": self.view.get_show_incident_beam(),
            "ks": self.view.get_show_ks(),
            "scattered": self.view.get_scattered_beam(),
            "stl_scale": self.view.get_stl_scale(),
            "stl_x_degrees": self.view.get_stl_x_deg(),
            "stl_y_degrees": self.view.get_stl_y_deg(),
            "stl_z_degrees": self.view.get_stl_z_deg(),
            "stl_translation_vector": self.view.get_stl_translation(),
            "orientation_axes": self.view.get_orient_axes(),
            "orientation_senses": self.view.get_orient_senses(),
            "mc_events_per_point": self.view.get_mc_events(),
            "mc_max_scatter_attempts": self.view.get_mc_max_scatter(),
            "mc_simulate_in": self.view.get_mc_simulate_in(),
            "mc_resimulate": self.view.get_mc_resimulate(),
            "att_point": self.view.get_att_point(),
            "att_unit": self.view.get_att_unit(),
            "att_material": self.view.get_att_material(),
        }

    def _apply_settings_to_texture_model(self, settings):
        self.texture_model.vis_settings["directions"] = settings["directions"]
        self.texture_model.vis_settings["goniometers"] = settings["goniometers"]
        self.texture_model.vis_settings["incident"] = settings["incident"]
        self.texture_model.vis_settings["ks"] = settings["ks"]
        self.texture_model.vis_settings["scattered"] = settings["scattered"]

        self.texture_model.stl_kwargs["Scale"] = settings["stl_scale"]
        self.texture_model.stl_kwargs["XDegrees"] = settings["stl_x_degrees"]
        self.texture_model.stl_kwargs["YDegrees"] = settings["stl_y_degrees"]
        self.texture_model.stl_kwargs["ZDegrees"] = settings["stl_z_degrees"]
        self.texture_model.stl_kwargs["TranslationVector"] = settings["stl_translation_vector"]

        self.texture_model.orientation_kwargs["Axes"] = settings["orientation_axes"]
        self.texture_model.orientation_kwargs["Senses"] = settings["orientation_senses"]

        self.texture_model.mc_kwargs["EventsPerPoint"] = settings["mc_events_per_point"]
        self.texture_model.mc_kwargs["MaxScatterPtAttempts"] = settings["mc_max_scatter_attempts"]
        self.texture_model.mc_kwargs["SimulateScatteringPointIn"] = settings["mc_simulate_in"]
        self.texture_model.mc_kwargs["ResimulateTracksForDifferentWavelengths"] = settings["mc_resimulate"]

        self.texture_model.attenuation_kwargs["point"] = settings["att_point"]
        self.texture_model.attenuation_kwargs["unit"] = settings["att_unit"]
        self.texture_model.set_material_string(settings["att_material"])

    def _populate_view_from_texture_model(self):
        """Populate the settings dialog fields from the current state of the texture model."""
        vis = self.texture_model.vis_settings
        self.view.set_stl_scale(vis["directions"])
        self.view.set_stl_x_deg(vis["goniometers"])
        self.view.set_stl_y_deg(vis["incident"])
        self.view.set_stl_z_deg(vis["ks"])
        self.view.set_stl_translation(vis["scattered"])

        stl = self.texture_model.stl_kwargs
        self.view.set_stl_scale(stl["Scale"])
        self.view.set_stl_x_deg(stl["XDegrees"])
        self.view.set_stl_y_deg(stl["YDegrees"])
        self.view.set_stl_z_deg(stl["ZDegrees"])
        self.view.set_stl_translation(stl["TranslationVector"])

        orient = self.texture_model.orientation_kwargs
        self.view.set_orient_axes(orient["Axes"])
        self.view.set_orient_senses(orient["Senses"])

        mc = self.texture_model.mc_kwargs
        self.view.set_mc_events(mc["EventsPerPoint"])
        self.view.set_mc_max_scatter(mc["MaxScatterPtAttempts"])
        self.view.set_mc_simulate_in(mc["SimulateScatteringPointIn"])
        self.view.set_mc_resimulate(mc["ResimulateTracksForDifferentWavelengths"])

        att = self.texture_model.attenuation_kwargs
        self.view.set_att_point(att["point"])
        self.view.set_att_unit(att["unit"])
        self.view.set_att_material(att["material"])
