# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import patch, MagicMock

from mantidqtinterfaces.TexturePlanner.presenter import TexturePlannerPresenter

file_path = "mantidqtinterfaces.TexturePlanner.presenter"


def _make_view():
    view = MagicMock()
    view.gonio_axes = ["g0", "g1", "g2", "g3", "g4", "g5"]
    view.get_num_gonios.return_value = 1
    view.get_current_index.return_value = 0
    view.get_rd_dir.return_value = "1,0,0"
    view.get_nd_dir.return_value = "0,1,0"
    view.get_td_dir.return_value = "0,0,1"
    view.get_rd_name.return_value = "RD"
    view.get_nd_name.return_value = "ND"
    view.get_td_name.return_value = "TD"
    view.get_group.return_value = "banks"
    view.get_vecs.return_value = ["vecs"]
    view.get_senses.return_value = ["senses"]
    view.get_angles.return_value = ["angles"]
    return view


def _make_model():
    model = MagicMock()
    model.gonio_index = 0
    model.plot_transmission = False
    model.instrument.supported_groups = ("banks",)
    model.instrument.get_supported_instruments.return_value = ("ENGINX", "IMAT")
    model.get_default_texture_directions.return_value = (
        ("RD", "ND", "TD"),
        ((1, 0, 0), (0, 1, 0), (0, 0, 1)),
    )
    model.orientations.get_num_orientations.return_value = 0
    model.orientations.get_vecs.side_effect = lambda v, n: v
    model.orientations.get_senses.side_effect = lambda s, n: s
    model.orientations.get_angles.side_effect = lambda a, n: a
    return model


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
def _make_presenter(mock_settings_view, mock_settings_presenter, model=None, view=None):
    """Construct a presenter with the settings collaborators patched out."""
    if model is None:
        model = _make_model()
    if view is None:
        view = _make_view()
    presenter = TexturePlannerPresenter(model, view)
    return presenter, model, view


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_Init(unittest.TestCase):
    def test_stores_model_and_view(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()

        presenter = TexturePlannerPresenter(model, view)

        self.assertIs(presenter.model, model)
        self.assertIs(presenter.view, view)

    def test_constructs_settings_presenter_with_settings_view(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()

        TexturePlannerPresenter(model, view)

        mock_settings_view.assert_called_once_with(parent=view)
        mock_settings_presenter.assert_called_once_with(model, mock_settings_view.return_value)

    def test_loads_settings_from_file_or_default(self, mock_settings_view, mock_settings_presenter):
        TexturePlannerPresenter(_make_model(), _make_view())

        mock_settings_presenter.return_value.load_settings_from_file_or_default.assert_called_once_with()

    def test_pushes_instrument_options_to_view(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()

        TexturePlannerPresenter(model, view)

        view.set_instrument_options.assert_called_once_with(("ENGINX", "IMAT"))

    def test_initialises_view_with_default_texture_directions(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()

        TexturePlannerPresenter(model, view)

        view.set_rd_name.assert_called_with("RD")
        view.set_nd_name.assert_called_with("ND")
        view.set_td_name.assert_called_with("TD")
        view.set_rd_dir.assert_called_with((1, 0, 0))
        view.set_nd_dir.assert_called_with((0, 1, 0))
        view.set_td_dir.assert_called_with((0, 0, 1))

    def test_sets_default_step_size(self, mock_settings_view, mock_settings_presenter):
        view = _make_view()

        TexturePlannerPresenter(_make_model(), view)

        view.set_step_size.assert_called_once_with(15)

    def test_pushes_supported_groups_to_view(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        model.instrument.supported_groups = ("Texture20", "banks")
        view = _make_view()

        TexturePlannerPresenter(model, view)

        view.setup_group_options.assert_called_once_with(("Texture20", "banks"))


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_GonioEnable(unittest.TestCase):
    def test_enables_first_n_axes_and_disables_the_rest(self, mock_settings_view, mock_settings_presenter):
        view = _make_view()
        presenter = TexturePlannerPresenter(_make_model(), view)

        view.set_gonio_axis_enabled.reset_mock()
        presenter.update_enabled_gonios(3)

        calls = view.set_gonio_axis_enabled.call_args_list
        self.assertEqual(calls[0].args, ("g0", True))
        self.assertEqual(calls[1].args, ("g1", True))
        self.assertEqual(calls[2].args, ("g2", True))
        self.assertEqual(calls[3].args, ("g3", False))
        self.assertEqual(calls[4].args, ("g4", False))
        self.assertEqual(calls[5].args, ("g5", False))


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_ModelGettersFromView(unittest.TestCase):
    def test_get_vecs_delegates_to_orientations(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_vecs.return_value = "raw_vecs"
        view.get_num_gonios.return_value = 2
        presenter = TexturePlannerPresenter(model, view)

        result = presenter.get_vecs()

        model.orientations.get_vecs.assert_called_with("raw_vecs", 2)
        self.assertEqual(result, "raw_vecs")

    def test_get_senses_delegates_to_orientations(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_senses.return_value = "raw_senses"
        view.get_num_gonios.return_value = 2
        presenter = TexturePlannerPresenter(model, view)

        result = presenter.get_senses()

        model.orientations.get_senses.assert_called_with("raw_senses", 2)
        self.assertEqual(result, "raw_senses")

    def test_get_angles_delegates_to_orientations(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_angles.return_value = "raw_angles"
        view.get_num_gonios.return_value = 2
        presenter = TexturePlannerPresenter(model, view)

        result = presenter.get_angles()

        model.orientations.get_angles.assert_called_with("raw_angles", 2)
        self.assertEqual(result, "raw_angles")


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_GonioUpdated(unittest.TestCase):
    def test_sets_gonio_index_on_model(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_current_index.return_value = 4
        presenter = TexturePlannerPresenter(model, view)

        model.set_gonio_index.reset_mock()
        presenter.on_goniometer_updated(2)

        model.set_gonio_index.assert_called_once_with(2)

    def test_updates_orientation_gRs_and_gonio_string(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_current_index.return_value = 4
        presenter = TexturePlannerPresenter(model, view)

        model.orientations.update_gRs.reset_mock()
        model.orientations.update_gonio_string.reset_mock()

        presenter.on_goniometer_updated(0)

        model.orientations.update_gRs.assert_called_once_with(["vecs"], ["senses"], ["angles"], 4)
        model.orientations.update_gonio_string.assert_called_once_with(["vecs"], ["senses"], ["angles"], 4)

    def test_applies_orientation_R_to_workspace_and_refreshes_geometry(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_current_index.return_value = 2
        orientation = MagicMock()
        orientation.R.as_matrix.return_value = "R_matrix"
        model.orientations.__getitem__.return_value = orientation
        presenter = TexturePlannerPresenter(model, view)

        presenter.on_goniometer_updated(0)

        model.workspaces.ws.run.return_value.getGoniometer.return_value.setR.assert_called_with("R_matrix")
        model.geometry.recompute_scattering_geometry.assert_called()
        model.update_projected_data.assert_called_with(2)


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_NumGonioUpdated(unittest.TestCase):
    def test_propagates_num_gonios_to_model_and_view(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_num_gonios.return_value = 3
        model.update_gonio_index.return_value = 0
        presenter = TexturePlannerPresenter(model, view)
        presenter.on_goniometer_updated = MagicMock()

        presenter.on_num_gonio_updated()

        model.orientations.set_n_gonio.assert_called_with(3)
        model.update_gonio_index.assert_called_with(3)
        presenter.on_goniometer_updated.assert_called_once_with(0)
        view.hide_axis_columns.assert_called_with()


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_DirectionsUpdated(unittest.TestCase):
    def test_pushes_dirs_to_model_then_recomputes_and_refreshes(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        presenter = TexturePlannerPresenter(model, view)
        presenter.update_plots = MagicMock()

        model.set_ax_transform.reset_mock()
        model.set_dir_names.reset_mock()
        model.geometry.recompute.reset_mock()
        model.update_all_projected_data.reset_mock()

        presenter.on_directions_updated()

        model.set_ax_transform.assert_called_once_with("1,0,0", "0,1,0", "0,0,1")
        model.set_dir_names.assert_called_once_with("RD", "ND", "TD")
        model.geometry.recompute.assert_called_once_with()
        model.update_all_projected_data.assert_called_once_with()
        presenter.update_plots.assert_called_once_with()


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_GroupAndInstrumentChanged(unittest.TestCase):
    def test_on_group_changed_sets_group_and_refreshes(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        presenter = TexturePlannerPresenter(model, view)
        presenter.update_plots = MagicMock()
        model.instrument.set_group.reset_mock()
        model.geometry.recompute.reset_mock()
        model.update_all_projected_data.reset_mock()

        presenter.on_group_changed()

        model.instrument.set_group.assert_called_once_with("banks")
        model.geometry.recompute.assert_called_once_with()
        model.update_all_projected_data.assert_called_once_with()
        presenter.update_plots.assert_called_once_with()

    def test_on_instrument_changed_updates_instrument_and_groups(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_instrument.return_value = "IMAT"
        presenter = TexturePlannerPresenter(model, view)
        presenter.update_plots = MagicMock()
        presenter.on_group_changed = MagicMock()
        view.setup_group_options.reset_mock()
        model.instrument.update_instrument.reset_mock()

        presenter.on_instrument_changed()

        model.instrument.update_instrument.assert_called_once_with("IMAT")
        view.setup_group_options.assert_called_once_with(model.instrument.supported_groups)
        presenter.on_group_changed.assert_called_once_with()
        presenter.update_plots.assert_called_once_with()


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_OnSettingsApplied(unittest.TestCase):
    def test_skips_reprojection_when_transmission_off(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        model.plot_transmission = False
        presenter = TexturePlannerPresenter(model, _make_view())
        presenter.update_plots = MagicMock()
        model.update_all_projected_data.reset_mock()

        presenter.on_settings_applied()

        model.update_all_projected_data.assert_not_called()
        presenter.update_plots.assert_called_once_with()

    def test_reprojects_when_transmission_on(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        model.plot_transmission = True
        presenter = TexturePlannerPresenter(model, _make_view())
        presenter.update_plots = MagicMock()
        model.update_all_projected_data.reset_mock()

        presenter.on_settings_applied()

        model.update_all_projected_data.assert_called_once_with()
        presenter.update_plots.assert_called_once_with()


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_AddOrientation(unittest.TestCase):
    def test_adds_and_selects_new_orientation(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        presenter = TexturePlannerPresenter(model, view)
        presenter.on_goniometer_updated = MagicMock()

        model.orientations.get_num_orientations.return_value = 3

        presenter.add_orientation()

        model.orientations.add_orientation.assert_called_once_with()
        view.spnIndex.setMaximum.assert_called_with(3)
        view.set_current_index.assert_called_with(2)
        presenter.on_goniometer_updated.assert_called_once_with(model.gonio_index)


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_OnIndexChanged(unittest.TestCase):
    def test_pushes_index_and_refreshes_gonio_fields(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_current_index.return_value = 2
        model.orientations.get_goniometer_values.return_value = ("v", "s", "a")
        presenter = TexturePlannerPresenter(model, view)

        presenter.on_index_changed()

        model.orientations.set_orientation_index.assert_called_with(2)
        model.orientations.get_goniometer_values.assert_called_with(2)
        view.set_vecs.assert_called_with("v")
        view.set_senses.assert_called_with("s")
        view.set_angles.assert_called_with("a")


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_EnableLoaders(unittest.TestCase):
    def test_stl_enabled_when_path_nonempty(self, mock_settings_view, mock_settings_presenter):
        view = _make_view()
        view.get_stl_string.return_value = "file.stl"
        presenter = TexturePlannerPresenter(_make_model(), view)

        presenter.enable_load_stl()

        view.set_load_stl_enabled.assert_called_with(True)

    def test_stl_disabled_when_path_empty(self, mock_settings_view, mock_settings_presenter):
        view = _make_view()
        view.get_stl_string.return_value = ""
        presenter = TexturePlannerPresenter(_make_model(), view)

        presenter.enable_load_stl()

        view.set_load_stl_enabled.assert_called_with(False)

    def test_xml_enabled_when_path_nonempty(self, mock_settings_view, mock_settings_presenter):
        view = _make_view()
        view.get_xml_string.return_value = "file.xml"
        presenter = TexturePlannerPresenter(_make_model(), view)

        presenter.enable_load_xml()

        view.set_load_xml_enabled.assert_called_with(True)

    def test_orient_enabled_when_path_nonempty(self, mock_settings_view, mock_settings_presenter):
        view = _make_view()
        view.get_orientation_file.return_value = "orient.txt"
        presenter = TexturePlannerPresenter(_make_model(), view)

        presenter.enable_load_orient()

        view.set_load_orientation_enabled.assert_called_with(True)

    def test_outputs_enabled_only_when_both_dir_and_file_nonempty(self, mock_settings_view, mock_settings_presenter):
        view = _make_view()
        presenter = TexturePlannerPresenter(_make_model(), view)

        view.get_save_dir.return_value = "/dir"
        view.get_save_filename.return_value = "name"
        presenter.enable_outputs()
        view.set_outputs_enabled.assert_called_with(True)

        view.get_save_filename.return_value = ""
        presenter.enable_outputs()
        view.set_outputs_enabled.assert_called_with(False)


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_LoadFiles(unittest.TestCase):
    def test_load_stl_delegates_to_workspace_and_sets_initial_shape(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_stl_string.return_value = "file.stl"
        presenter = TexturePlannerPresenter(model, view)
        presenter.set_initial_shape = MagicMock()

        presenter.load_stl()

        model.workspaces.load_stl.assert_called_once_with("file.stl")
        presenter.set_initial_shape.assert_called_once_with()

    def test_load_xml_delegates_to_workspace_and_sets_initial_shape(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_xml_string.return_value = "file.xml"
        presenter = TexturePlannerPresenter(model, view)
        presenter.set_initial_shape = MagicMock()

        presenter.load_xml()

        model.workspaces.load_xml.assert_called_once_with("file.xml")
        presenter.set_initial_shape.assert_called_once_with()

    def test_load_orientation_file_propagates_num_gonios_and_refreshes(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_orientation_file.return_value = "orient.txt"
        model.orientations.load_orientation_file.return_value = 3
        model.orientations.get_num_orientations.return_value = 5
        presenter = TexturePlannerPresenter(model, view)
        presenter.update_table = MagicMock()
        presenter.update_plots = MagicMock()

        presenter.load_orientation_file()

        model.orientations.load_orientation_file.assert_called_once_with("orient.txt")
        view.set_num_gonios.assert_called_with(3)
        view.spnIndex.setMaximum.assert_called_with(5)
        # last set_current_index reflects update_orientation_selector's call with the new count
        view.set_current_index.assert_called_with(5)
        model.update_all_projected_data.assert_called_with()
        presenter.update_table.assert_called_with()
        presenter.update_plots.assert_called_with()


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_SelectionAndDeletion(unittest.TestCase):
    def test_update_selected_pushes_view_selection(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_select_indices.return_value = [0, 1]
        presenter = TexturePlannerPresenter(model, view)

        presenter.update_selected()

        model.orientations.update_selected.assert_called_with([0, 1])

    def test_update_included_pushes_view_inclusion_and_refreshes_plots(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_include_indices.return_value = [2]
        presenter = TexturePlannerPresenter(model, view)
        presenter.update_plots = MagicMock()

        presenter.update_included()

        model.orientations.update_included.assert_called_with([2])
        presenter.update_plots.assert_called_with()

    def test_select_all_then_updates_table(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        presenter = TexturePlannerPresenter(model, _make_view())
        presenter.update_table = MagicMock()

        presenter.select_all()

        model.orientations.select_all.assert_called_with()
        presenter.update_table.assert_called_with()

    def test_deselect_all_then_updates_table(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        presenter = TexturePlannerPresenter(model, _make_view())
        presenter.update_table = MagicMock()

        presenter.deselect_all()

        model.orientations.deselect_all.assert_called_with()
        presenter.update_table.assert_called_with()

    def test_delete_selected_resyncs_index_and_refreshes(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        model.orientations.get_orientation_index.return_value = 1
        presenter = TexturePlannerPresenter(model, view)
        presenter.on_index_changed = MagicMock()
        presenter.update_table = MagicMock()
        presenter.update_plots = MagicMock()

        presenter.delete_selected()

        model.orientations.delete_selected.assert_called_with()
        view.set_current_index.assert_called_with(1)
        presenter.on_index_changed.assert_called_with()
        model.update_all_projected_data.assert_called_with()
        presenter.update_table.assert_called_with()
        presenter.update_plots.assert_called_with()


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_Exports(unittest.TestCase):
    def test_to_sscanss_delegates_to_exporter(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_save_dir.return_value = "/out"
        view.get_save_filename.return_value = "name"
        presenter = TexturePlannerPresenter(model, view)

        presenter.to_sscanss()

        model.exporter.output_as_sscanss.assert_called_with("/out", "name")

    def test_to_euler_delegates_to_exporter(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_save_dir.return_value = "/out"
        view.get_save_filename.return_value = "name"
        presenter = TexturePlannerPresenter(model, view)

        presenter.to_euler()

        model.exporter.output_as_euler.assert_called_with("/out", "name")

    def test_to_matrix_delegates_to_exporter(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_save_dir.return_value = "/out"
        view.get_save_filename.return_value = "name"
        presenter = TexturePlannerPresenter(model, view)

        presenter.to_matrix()

        model.exporter.output_as_matrix.assert_called_with("/out", "name")

    def test_to_reference_workspace_delegates_to_exporter(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_save_dir.return_value = "/out"
        view.get_save_filename.return_value = "name"
        presenter = TexturePlannerPresenter(model, view)

        presenter.to_reference_workspace()

        model.exporter.output_as_reference_workspace.assert_called_with("/out", "name")


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_TransmissionAndShape(unittest.TestCase):
    def test_set_show_transmission_pushes_to_model_and_refreshes(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_show_transmission.return_value = True
        presenter = TexturePlannerPresenter(model, view)
        presenter.update_plots = MagicMock()
        presenter.update_custom_shape_finder_enabled = MagicMock()
        presenter.update_set_gauge_vol_enabled = MagicMock()

        presenter.set_show_transmission()

        model.set_plot_transmission.assert_called_with(True)
        model.update_all_projected_data.assert_called_with()
        presenter.update_plots.assert_called_with()
        presenter.update_custom_shape_finder_enabled.assert_called_with()
        presenter.update_set_gauge_vol_enabled.assert_called_with()

    def test_set_initial_shape_pushes_dimensions_and_refreshes(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_init_x.return_value = 1.0
        view.get_init_y.return_value = 2.0
        view.get_init_z.return_value = 3.0
        view.get_init_px.return_value = 0.1
        view.get_init_py.return_value = 0.2
        view.get_init_pz.return_value = 0.3
        presenter = TexturePlannerPresenter(model, view)
        presenter.update_plots = MagicMock()

        presenter.set_initial_shape()

        model.workspaces.update_initial_shape.assert_called_with(1.0, 2.0, 3.0, 0.1, 0.2, 0.3)
        model.update_all_projected_data.assert_called_with()
        presenter.update_plots.assert_called_with()


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_GaugeVolume(unittest.TestCase):
    def test_set_gauge_volume_passes_shape_method_and_custom_shape(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_shape_method.return_value = "Custom Shape"
        view.get_custom_shape.return_value = "<xml/>"
        presenter = TexturePlannerPresenter(model, view)
        presenter.update_plots = MagicMock()

        presenter.set_gauge_volume()

        model.workspaces.set_gauge_volume_str.assert_called_with("Custom Shape", "<xml/>")
        model.update_all_projected_data.assert_called_with()
        presenter.update_plots.assert_called_with()

    def test_clear_gauge_volume_sets_none(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        presenter = TexturePlannerPresenter(model, _make_view())
        presenter.update_plots = MagicMock()

        presenter.clear_gauge_volume()

        model.workspaces.set_gauge_volume_str.assert_called_with("No Gauge Volume", None)
        model.update_all_projected_data.assert_called_with()
        presenter.update_plots.assert_called_with()

    def test_custom_shape_finder_visible_only_for_custom_shape(self, mock_settings_view, mock_settings_presenter):
        view = _make_view()
        presenter = TexturePlannerPresenter(_make_model(), view)

        view.get_shape_method.return_value = "Custom Shape"
        presenter.update_custom_shape_finder_enabled()
        view.set_finder_gauge_vol_visible.assert_called_with(True)

        view.get_shape_method.return_value = "Preset"
        presenter.update_custom_shape_finder_enabled()
        view.set_finder_gauge_vol_visible.assert_called_with(False)

    def test_set_gauge_vol_enabled_requires_custom_shape_when_custom(self, mock_settings_view, mock_settings_presenter):
        view = _make_view()
        presenter = TexturePlannerPresenter(_make_model(), view)

        view.get_shape_method.return_value = "Custom Shape"
        view.get_custom_shape.return_value = None
        presenter.update_set_gauge_vol_enabled()
        view.set_set_gauge_vol_enabled.assert_called_with(False)

        view.get_custom_shape.return_value = "<xml/>"
        presenter.update_set_gauge_vol_enabled()
        view.set_set_gauge_vol_enabled.assert_called_with(True)

    def test_set_gauge_vol_enabled_unconditionally_for_non_custom(self, mock_settings_view, mock_settings_presenter):
        view = _make_view()
        view.get_shape_method.return_value = "Preset"
        presenter = TexturePlannerPresenter(_make_model(), view)

        view.set_set_gauge_vol_enabled.reset_mock()
        presenter.update_set_gauge_vol_enabled()

        view.set_set_gauge_vol_enabled.assert_called_once_with(True)


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_Settings(unittest.TestCase):
    def test_open_settings_shows_settings_presenter(self, mock_settings_view, mock_settings_presenter):
        presenter = TexturePlannerPresenter(_make_model(), _make_view())

        presenter.open_settings()

        mock_settings_presenter.return_value.show.assert_called_with()


@patch(file_path + ".TexturePlannerSettingsPresenter")
@patch(file_path + ".TexturePlannerSettingsView")
class TestTexturePlannerPresenter_UpdatePlotsAndTable(unittest.TestCase):
    def test_update_plots_forwards_view_handles_and_index(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        view.get_current_index.return_value = 1
        presenter = TexturePlannerPresenter(model, view)

        model.plotter.update_plot.reset_mock()
        presenter.update_plots()

        model.plotter.update_plot.assert_called_once_with(
            ["vecs"],
            ["senses"],
            ["angles"],
            view.get_lab_figure.return_value,
            view.get_lab_ax.return_value,
            view.get_pf_ax.return_value,
            1,
        )

    def test_update_table_sets_row_count_and_adds_rows(self, mock_settings_view, mock_settings_presenter):
        model = _make_model()
        view = _make_view()
        presenter = TexturePlannerPresenter(model, view)

        # reset to ignore the implicit update_table() that runs during __init__
        view.tableWidget.setRowCount.reset_mock()
        view.add_table_row.reset_mock()
        model.orientations.get_num_orientations.return_value = 2
        model.orientations.get_table_info.return_value = [("a", "b"), ("c", "d")]

        presenter.update_table()

        view.tableWidget.setRowCount.assert_called_with(2)
        self.assertEqual(
            [c.args for c in view.add_table_row.call_args_list],
            [(0, "a", "b"), (1, "c", "d")],
        )


if __name__ == "__main__":
    unittest.main()
