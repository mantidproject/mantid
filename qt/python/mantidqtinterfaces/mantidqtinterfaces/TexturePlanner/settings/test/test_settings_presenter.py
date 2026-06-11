# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import patch, MagicMock

from mantidqtinterfaces.TexturePlanner.settings.settings_presenter import TexturePlannerSettingsPresenter

file_path = "mantidqtinterfaces.TexturePlanner.settings.settings_presenter"


SETTINGS_FIXTURE = {
    "directions": True,
    "goniometers": False,
    "incident": True,
    "ks": False,
    "scattered": True,
    "stl_scale": "mm",
    "stl_x_degrees": 1.0,
    "stl_y_degrees": 2.0,
    "stl_z_degrees": 3.0,
    "stl_translation_vector": "1,2,3",
    "orientation_axes": "XYZ",
    "orientation_senses": "1,-1,1",
    "mc_events_per_point": 100,
    "mc_max_scatter_attempts": 5000,
    "mc_simulate_in": "Container",
    "mc_resimulate": True,
    "att_point": 2.5,
    "att_unit": "Wavelength",
    "att_use_data_range": True,
}


def _make_texture_model():
    m = MagicMock()
    m.plotter.vis_settings = {"directions": False, "goniometers": True, "incident": False, "ks": True, "scattered": False}
    m.workspaces.stl_kwargs = {"Scale": "cm", "XDegrees": 0.0, "YDegrees": 0.0, "ZDegrees": "0", "TranslationVector": "0,0,0"}
    m.orientations.orientation_kwargs = {"Axes": "YXY", "Senses": "-1,-1,-1"}
    m.absorption.mc_kwargs = {
        "EventsPerPoint": 50,
        "MaxScatterPtAttempts": 10000,
        "SimulateScatteringPointIn": "SampleOnly",
        "ResimulateTracksForDifferentWavelengths": False,
    }
    m.workspaces.attenuation_kwargs = {"point": 1.5, "unit": "dSpacing"}
    m.plotter.transmission_use_data_range = False
    return m


def _make_view_returning(settings):
    view = MagicMock()
    view.get_show_directions.return_value = settings["directions"]
    view.get_show_goniometers.return_value = settings["goniometers"]
    view.get_show_incident_beam.return_value = settings["incident"]
    view.get_show_ks.return_value = settings["ks"]
    view.get_show_scattered_beam.return_value = settings["scattered"]
    view.get_stl_scale.return_value = settings["stl_scale"]
    view.get_stl_x_deg.return_value = settings["stl_x_degrees"]
    view.get_stl_y_deg.return_value = settings["stl_y_degrees"]
    view.get_stl_z_deg.return_value = settings["stl_z_degrees"]
    view.get_stl_translation.return_value = settings["stl_translation_vector"]
    view.get_orient_axes.return_value = settings["orientation_axes"]
    view.get_orient_senses.return_value = settings["orientation_senses"]
    view.get_mc_events.return_value = settings["mc_events_per_point"]
    view.get_mc_max_scatter.return_value = settings["mc_max_scatter_attempts"]
    view.get_mc_simulate_in.return_value = settings["mc_simulate_in"]
    view.get_mc_resimulate.return_value = settings["mc_resimulate"]
    view.get_att_point.return_value = settings["att_point"]
    view.get_att_unit.return_value = settings["att_unit"]
    view.get_att_use_data_range.return_value = settings["att_use_data_range"]
    return view


@patch(file_path + ".TexturePlannerSettingsModel")
class TestTexturePlannerSettingsPresenter_Init(unittest.TestCase):
    def test_wires_view_handlers_to_presenter_methods(self, mock_model_cls):
        texture_model = _make_texture_model()
        view = MagicMock()

        presenter = TexturePlannerSettingsPresenter(texture_model, view)

        view.set_on_ok_clicked.assert_called_once_with(presenter.save_and_close)
        view.set_on_cancel_clicked.assert_called_once_with(presenter.close)
        view.set_on_apply_clicked.assert_called_once_with(presenter.save_settings)

    def test_creates_owns_settings_model_instance(self, mock_model_cls):
        presenter = TexturePlannerSettingsPresenter(_make_texture_model(), MagicMock())

        mock_model_cls.assert_called_once_with()
        self.assertIs(presenter.settings_model, mock_model_cls.return_value)


@patch(file_path + ".TexturePlannerSettingsModel")
class TestTexturePlannerSettingsPresenter_LoadFromFileOrDefault(unittest.TestCase):
    def test_reads_settings_dict_from_model_and_applies_to_texture_model(self, mock_model_cls):
        texture_model = _make_texture_model()
        mock_model_cls.return_value.get_settings_dict.return_value = SETTINGS_FIXTURE.copy()
        presenter = TexturePlannerSettingsPresenter(texture_model, MagicMock())

        presenter.load_settings_from_file_or_default()

        mock_model_cls.return_value.get_settings_dict.assert_called_once_with()
        # vis_settings updated from fixture
        self.assertEqual(texture_model.plotter.vis_settings["directions"], True)
        self.assertEqual(texture_model.plotter.vis_settings["goniometers"], False)
        # stl_kwargs updated
        self.assertEqual(texture_model.workspaces.stl_kwargs["Scale"], "mm")
        self.assertEqual(texture_model.workspaces.stl_kwargs["XDegrees"], 1.0)
        # orientation_kwargs updated
        self.assertEqual(texture_model.orientations.orientation_kwargs["Axes"], "XYZ")
        # mc_kwargs updated (note key remapping)
        self.assertEqual(texture_model.absorption.mc_kwargs["EventsPerPoint"], 100)
        self.assertEqual(texture_model.absorption.mc_kwargs["MaxScatterPtAttempts"], 5000)
        self.assertEqual(texture_model.absorption.mc_kwargs["SimulateScatteringPointIn"], "Container")
        self.assertIs(texture_model.absorption.mc_kwargs["ResimulateTracksForDifferentWavelengths"], True)
        # attenuation point/unit updated (material is no longer a persistent setting)
        self.assertEqual(texture_model.workspaces.attenuation_kwargs["point"], 2.5)
        self.assertEqual(texture_model.workspaces.attenuation_kwargs["unit"], "Wavelength")


@patch(file_path + ".TexturePlannerSettingsModel")
class TestTexturePlannerSettingsPresenter_Show(unittest.TestCase):
    def test_populates_view_from_texture_model_then_shows(self, mock_model_cls):
        texture_model = _make_texture_model()
        view = MagicMock()
        presenter = TexturePlannerSettingsPresenter(texture_model, view)

        presenter.show()

        view.set_show_directions.assert_called_once_with(False)
        view.set_show_goniometers.assert_called_once_with(True)
        view.set_show_incident_beam.assert_called_once_with(False)
        view.set_show_ks.assert_called_once_with(True)
        view.set_show_scattered_beam.assert_called_once_with(False)

        view.set_stl_scale.assert_called_once_with("cm")
        view.set_stl_x_deg.assert_called_once_with(0.0)
        view.set_stl_y_deg.assert_called_once_with(0.0)
        view.set_stl_z_deg.assert_called_once_with("0")
        view.set_stl_translation.assert_called_once_with("0,0,0")

        view.set_orient_axes.assert_called_once_with("YXY")
        view.set_orient_senses.assert_called_once_with("-1,-1,-1")

        view.set_mc_events.assert_called_once_with(50)
        view.set_mc_max_scatter.assert_called_once_with(10000)
        view.set_mc_simulate_in.assert_called_once_with("SampleOnly")
        view.set_mc_resimulate.assert_called_once_with(False)

        view.set_att_point.assert_called_once_with(1.5)
        view.set_att_unit.assert_called_once_with("dSpacing")
        view.set_att_use_data_range.assert_called_once_with(False)

        view.show.assert_called_once_with()


@patch(file_path + ".TexturePlannerSettingsModel")
class TestTexturePlannerSettingsPresenter_Close(unittest.TestCase):
    def test_close_delegates_to_view(self, mock_model_cls):
        view = MagicMock()
        presenter = TexturePlannerSettingsPresenter(_make_texture_model(), view)

        presenter.close()

        view.close.assert_called_once_with()


@patch(file_path + ".TexturePlannerSettingsModel")
class TestTexturePlannerSettingsPresenter_SaveSettings(unittest.TestCase):
    def test_collects_from_view_and_persists_via_settings_model(self, mock_model_cls):
        texture_model = _make_texture_model()
        view = _make_view_returning(SETTINGS_FIXTURE)
        presenter = TexturePlannerSettingsPresenter(texture_model, view)

        presenter.save_settings()

        mock_model_cls.return_value.set_settings_dict.assert_called_once_with(SETTINGS_FIXTURE)

    def test_applies_collected_settings_to_texture_model(self, mock_model_cls):
        texture_model = _make_texture_model()
        view = _make_view_returning(SETTINGS_FIXTURE)
        presenter = TexturePlannerSettingsPresenter(texture_model, view)

        presenter.save_settings()

        self.assertEqual(texture_model.plotter.vis_settings["directions"], True)
        self.assertEqual(texture_model.workspaces.stl_kwargs["TranslationVector"], "1,2,3")
        self.assertEqual(texture_model.orientations.orientation_kwargs["Senses"], "1,-1,1")
        self.assertIs(texture_model.absorption.mc_kwargs["ResimulateTracksForDifferentWavelengths"], True)

    def test_invokes_on_settings_applied_callback_if_registered(self, mock_model_cls):
        view = _make_view_returning(SETTINGS_FIXTURE)
        presenter = TexturePlannerSettingsPresenter(_make_texture_model(), view)
        callback = MagicMock()
        presenter.set_on_settings_applied(callback)

        presenter.save_settings()

        callback.assert_called_once_with()

    def test_no_error_when_callback_unset(self, mock_model_cls):
        view = _make_view_returning(SETTINGS_FIXTURE)
        presenter = TexturePlannerSettingsPresenter(_make_texture_model(), view)

        # should not raise
        presenter.save_settings()


@patch(file_path + ".TexturePlannerSettingsModel")
class TestTexturePlannerSettingsPresenter_SaveAndClose(unittest.TestCase):
    def test_calls_save_settings_then_close(self, mock_model_cls):
        view = _make_view_returning(SETTINGS_FIXTURE)
        presenter = TexturePlannerSettingsPresenter(_make_texture_model(), view)
        presenter.save_settings = MagicMock()
        presenter.close = MagicMock()

        presenter.save_and_close()

        presenter.save_settings.assert_called_once_with()
        presenter.close.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()
