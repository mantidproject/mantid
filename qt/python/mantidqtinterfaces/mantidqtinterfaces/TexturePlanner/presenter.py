# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import annotations
from typing import TYPE_CHECKING, List, Tuple
from qtpy.QtCore import QTimer

from mantid.api import AlgorithmObserver
from mantidqt.interfacemanager import InterfaceManager
from mantidqt.widgets.tutorial.launcher import run_tutorial, should_show_on_startup

from mantidqtinterfaces.TexturePlanner.tutorial.chapters import CHAPTERS
from mantidqtinterfaces.TexturePlanner.settings.settings_view import TexturePlannerSettingsView
from mantidqtinterfaces.TexturePlanner.settings.settings_presenter import TexturePlannerSettingsPresenter
from mantidqtinterfaces.TexturePlanner.helpers.instrument import CUSTOM_GROUP
from mantidqtinterfaces.TexturePlanner.view import (
    EXPORT_SSCANSS,
    EXPORT_EULER,
    EXPORT_MATRIX,
    EXPORT_REFERENCE_WS,
    EXPORT_TRANSMISSION_WEIGHTING,
    GAUGE_VOL_CUSTOM_SHAPE,
    GAUGE_VOL_NONE,
)

if TYPE_CHECKING:
    from mantidqtinterfaces.TexturePlanner.model import TexturePlannerModel, FlatArrayTuple
    from mantidqtinterfaces.TexturePlanner.view import TexturePlannerView
    from numpy import ndarray

# Settings key the tutorial's "already shown once" flag is stored under, beside this interface's
# other settings.
TUTORIAL_SETTINGS_KEY = "TexturePlanner"

# Title of the tutorial window, which frames a throwaway copy of this interface.
TUTORIAL_WINDOW_TITLE = "Texture Planner — Tutorial"


class TexturePlannerPresenter(AlgorithmObserver):
    def __init__(self, model: TexturePlannerModel, view: TexturePlannerView, offer_tutorial: bool = True):
        """:param offer_tutorial: whether this planner may show the guided tutorial on a first ever
        open, and offer it from the toolbar. False for the throwaway planner the tutorial itself
        drives, which must not offer a tutorial inside a tutorial."""
        super().__init__()
        self.model = model
        self.view = view
        # cached result of the (expensive) "does the custom grouping file fit the instrument" check,
        # so the Update Instrument enable rule stays cheap and is not run on every name keystroke.
        # True for preset groups, which never need the check.
        self._grouping_applicable = True

        self.settings_presenter = TexturePlannerSettingsPresenter(model, TexturePlannerSettingsView(parent=view))
        self.settings_presenter.load_settings_from_file_or_default()
        self.settings_presenter.set_on_settings_applied(self.on_settings_applied)
        self.view.set_on_settings_clicked(self.open_settings)
        self.view.set_on_close(self.on_close)
        self._offer_tutorial = offer_tutorial
        # kept alive for as long as the tour runs: the session owns the sandbox planner it drives
        self._tutorial_session = None
        if offer_tutorial:
            self.view.set_on_tutorial_clicked(self.open_tutorial)
        else:
            # the tutorial's own planner: the button would start a tutorial inside the tutorial
            self.view.set_tutorial_button_visible(False)

        self.set_instrument_options()
        self.set_view_with_default_texture_directions()
        self.update_enabled_gonios(self.view.get_num_gonios())
        self.set_max_orientation_index()
        self.setup_group_options()
        self.view.set_step_size(15)

        self.set_model_texture_directions()
        self.set_model_group()
        self.on_goniometer_updated(0)

        # connect slots
        self.view.set_on_stl_file_changed(self.enable_load_stl)
        self.view.set_on_xml_file_changed(self.enable_load_xml)
        self.view.set_on_orient_file_changed(self.enable_load_orient)
        self.view.set_on_load_stl_clicked(self.load_stl)
        self.view.set_on_load_xml_clicked(self.load_xml)
        self.view.set_on_set_material_clicked(self.open_set_material_dialog)
        self.view.set_on_material_set(self.on_material_set)
        self.view.set_on_load_orient_clicked(self.load_orientation_file)
        self.view.set_on_init_x_changed(self.set_initial_shape)
        self.view.set_on_init_y_changed(self.set_initial_shape)
        self.view.set_on_init_z_changed(self.set_initial_shape)
        self.view.set_on_init_px_changed(self.set_initial_shape)
        self.view.set_on_init_py_changed(self.set_initial_shape)
        self.view.set_on_init_pz_changed(self.set_initial_shape)
        self.view.set_on_update_dirs(self.on_directions_updated)
        self.view.set_on_gonio_vec_updated(self.on_goniometer_updated)
        self.view.set_on_gonio_sense_updated(self.on_goniometer_updated)
        self.view.set_on_gonio_angle_updated(self.on_goniometer_updated)
        self.view.set_on_num_gonio_updated(self.on_num_gonio_updated)
        self.view.set_on_step_updated(self.update_angle_steps)
        self.view.set_on_add_orientation_clicked(self.add_orientation)
        self.view.set_on_current_index_changed(self.on_index_changed)
        self.view.sig_select_state_changed.connect(self.update_selected)
        self.view.sig_include_state_changed.connect(self.update_included)
        self.view.set_on_select_all_clicked(self.select_all)
        self.view.set_on_deselect_all_clicked(self.deselect_all)
        self.view.set_on_delete_selected_clicked(self.delete_selected)
        self.view.set_on_export_clicked(self.on_export_clicked)
        self.view.set_on_save_dir_changed(self.enable_outputs)
        self.view.set_on_save_file_changed(self.enable_outputs)
        self.view.set_on_show_transmission_toggled(self.set_show_transmission)
        self.view.set_on_transform_dirs_toggled(self.set_transform_dirs)
        self.view.set_on_lab_dirs_toggled(self.set_lab_dirs)
        self.view.set_on_gauge_vol_state_changed(self.update_gauge_volume_state)
        self.view.set_on_gauge_vol_file_changed(self.update_set_gauge_vol_enabled)
        self.view.set_on_set_gauge_volume_clicked(self.set_gauge_volume)
        self.view.set_on_clear_gauge_volume_clicked(self.clear_gauge_volume)
        self.view.set_on_gauge_vol_group_toggled(self.update_custom_shape_finder_enabled)
        self.update_custom_shape_finder_enabled()
        self.view.set_texture_directions_enabled(not self.view.get_lab_dirs())
        self.view.set_on_instrument_changed(self.on_instrument_changed)
        self.view.set_on_group_changed(self.on_group_selection_changed)
        self.view.set_on_custom_instrument_name_changed(self.on_custom_instrument_name_changed)
        self.view.set_on_custom_instrument_name_committed(self.on_custom_instrument_name_committed)
        self.view.set_on_grouping_file_changed(self.on_grouping_file_changed)
        self.view.set_on_update_instrument_clicked(self.update_instrument_and_group)
        self.update_custom_widgets_visibility()
        self.refresh_update_instrument_enabled()
        self.update_material_display()

        self._offer_tutorial_on_first_open()

    def on_close(self) -> None:
        # the tutorial drives a second planner of its own; closing this one must take that with it,
        # or its window would be left behind with nothing to return to
        if self._tutorial_session is not None:
            self._tutorial_session.close()
        # remove this instance's workspaces from the ADS so they don't linger after the window closes
        self.model.workspaces.cleanup()

    def open_settings(self) -> None:
        self.settings_presenter.show()

    # ----------------------------------------------------------------------------------
    # Guided tutorial. The tour runs against a separate, throwaway planner (see
    # TexturePlanner/tutorial/sandbox.py) so it can load a sample, add orientations and
    # export a file without any of it reaching the window the user is working in.
    # ----------------------------------------------------------------------------------

    def open_tutorial(self, mark_as_seen: bool = False) -> None:
        # imported here rather than at module scope: the sandbox builds a presenter, so importing
        # it from the presenter's own module would be a cycle
        from mantidqtinterfaces.TexturePlanner.tutorial.sandbox import make_sandbox_factory

        if self._tutorial_session is not None:
            # already running; bring its window forward rather than starting a second one
            window = self._tutorial_session.window
            if window is not None:
                window.raise_()
                window.activateWindow()
                return
        self._tutorial_session = run_tutorial(
            sandbox_factory=make_sandbox_factory(parent=self.view),
            chapters=CHAPTERS,
            parent=self.view,
            settings_key=TUTORIAL_SETTINGS_KEY,
            mark_as_seen=mark_as_seen,
            title=TUTORIAL_WINDOW_TITLE,
        )
        self._tutorial_session.finished.connect(self._on_tutorial_finished)

    def _on_tutorial_finished(self) -> None:
        self._tutorial_session = None

    def _offer_tutorial_on_first_open(self) -> None:
        """Show the tutorial the first time this interface is ever opened.

        Deferred to the event loop rather than run here: this is still the presenter's constructor,
        so the window it would appear over has not been shown or laid out yet, and the tutorial
        would have nothing to position itself against.
        """
        if not self._offer_tutorial or not should_show_on_startup(TUTORIAL_SETTINGS_KEY):
            return
        QTimer.singleShot(0, lambda: self.open_tutorial(mark_as_seen=True))

    def set_view_texture_directions(self, names: Tuple[str, str, str], vecs: FlatArrayTuple | ndarray) -> None:
        self.view.set_rd_name(names[0])
        self.view.set_nd_name(names[1])
        self.view.set_td_name(names[2])
        self.view.set_rd_dir(vecs[0])
        self.view.set_nd_dir(vecs[1])
        self.view.set_td_dir(vecs[2])

    def set_model_texture_directions(self) -> None:
        self.model.set_dir_names(self.view.get_rd_name(), self.view.get_nd_name(), self.view.get_td_name())
        # if the view is showing the lab directions - only allow updating the sample names
        if not self.view.get_lab_dirs():
            self.model.set_ax_transform(self.view.get_rd_dir(), self.view.get_nd_dir(), self.view.get_td_dir())

    def set_model_group(self) -> None:
        self.model.instrument.set_group(self.view.get_group())
        self.model.geometry.recompute()

    def set_instrument_options(self) -> None:
        self.view.set_instrument_options(self.model.instrument.get_supported_instruments())

    def set_view_with_default_texture_directions(self) -> None:
        names, vecs = self.model.get_default_texture_directions()
        self.set_view_texture_directions(names, vecs)

    def set_view_with_texture_directions(self) -> None:
        names, vecs = self.model.get_texture_directions(self.view.get_lab_dirs())
        self.set_view_texture_directions(names, vecs)

    def update_enabled_gonios(self, num_gonios: int) -> None:
        for gon in self.view.gonio_axes[:num_gonios]:
            self.view.set_gonio_axis_enabled(gon, True)
        for gon in self.view.gonio_axes[num_gonios:]:
            self.view.set_gonio_axis_enabled(gon, False)

    def update_angle_steps(self) -> None:
        self.view.set_angle_steps()

    def set_max_orientation_index(self) -> None:
        num_orientations = self.model.orientations.get_num_orientations()
        self.view.set_max_ind(num_orientations)

    def get_vecs(self) -> List[ndarray]:
        return self.model.orientations.get_vecs(self.view.get_vecs(), self.view.get_num_gonios())

    def get_senses(self) -> List[float]:
        return self.model.orientations.get_senses(self.view.get_senses(), self.view.get_num_gonios())

    def get_angles(self) -> List[float]:
        return self.model.orientations.get_angles(self.view.get_angles(), self.view.get_num_gonios())

    def on_goniometer_updated(self, goniometer_index: int, *args) -> None:
        self.model.set_gonio_index(goniometer_index)
        orientation_index = self.view.get_current_index()
        self.model.orientations.update_gRs(self.get_vecs(), self.get_senses(), self.get_angles(), orientation_index)
        self.refresh_scattering_geometry()
        self.model.update_projected_data(orientation_index)
        self.model.orientations.update_gonio_string(self.get_vecs(), self.get_senses(), self.get_angles(), orientation_index)
        self.update_plots()
        self.update_table()

    def refresh_scattering_geometry(self) -> None:
        """Recompute the lab-frame detector directions and Qs against the current scattering centre.

        The centre depends on how the displayed orientation of the sample sits in the gauge
        volume, so that orientation's R is applied to the workspace first. Must be called whenever
        the sample is moved, the gauge volume changes, or the displayed orientation changes."""
        R = self.model.orientations[self.view.get_current_index()].R
        self.model.workspaces.ws.run().getGoniometer().setR(R.as_matrix())
        self.model.geometry.recompute_scattering_geometry()

    def on_num_gonio_updated(self) -> None:
        num_gonios = self.view.get_num_gonios()
        self.model.orientations.set_n_gonio(num_gonios)
        self.update_enabled_gonios(num_gonios)
        self.on_goniometer_updated(self.model.clamp_gonio_index(num_gonios))
        self.view.hide_axis_columns()

    def on_directions_updated(self) -> None:
        self.set_model_texture_directions()
        self.model.geometry.recompute()
        self.model.update_all_projected_data()
        self.update_plots()

    # ----------------------------------------------------------------------------------
    # Instrument / group selection.
    #
    # Changing the instrument or group combos (and the custom name / grouping file fields)
    # only updates widget visibility and the Update Instrument button's enabled state - none
    # of it touches the model. The model is rebuilt in one place, update_instrument_and_group,
    # when the user clicks Update Instrument (only enabled for a valid, complete selection).
    # This keeps the interface out of half-applied states (e.g. a custom instrument with no
    # grouping file yet, or an unrecognised instrument name).
    # ----------------------------------------------------------------------------------

    def on_instrument_changed(self) -> None:
        if self.view.is_custom_instrument():
            # a custom instrument has no presets, so its group is fixed to the custom file
            self.view.setup_group_options((CUSTOM_GROUP,))
            self.view.set_group_enabled(False)
        else:
            self.view.setup_group_options(self.model.instrument.groups_for_instrument(self.view.get_instrument()))
            self.view.set_group_enabled(True)
        # switching instrument invalidates any previously chosen grouping file (it belongs to a
        # different detector layout); make the user pick a fresh one before they can apply
        self.view.clear_grouping_file()
        self.update_custom_widgets_visibility()
        self.revalidate_grouping()

    def on_group_selection_changed(self) -> None:
        self.update_custom_widgets_visibility()
        self.revalidate_grouping()

    def on_custom_instrument_name_changed(self) -> None:
        # per-keystroke feedback: while being changed assume invalid: red border + button state
        # reset the cached instrument state to false
        self.model.instrument.set_is_valid_instrument(False)
        self.view.set_custom_instrument_valid(False)
        if self.view.is_custom_instrument() and self._group_is_custom():
            self._grouping_applicable = False
            self.view.set_grouping_file_problem("")
        self.refresh_update_instrument_enabled()

    def on_custom_instrument_name_committed(self) -> None:
        # name seems finalised (box focus lost or enter pressed): now run the applicability check
        valid = self.model.instrument.check_is_valid_instrument(self.view.get_custom_instrument_name())
        self.view.set_custom_instrument_valid(valid)
        self.revalidate_grouping()

    def on_grouping_file_changed(self) -> None:
        self.revalidate_grouping()

    def update_instrument_and_group(self) -> None:
        """Apply the current instrument + group selection to the model and recompute (button slot).

        The button is only enabled for a valid, complete and applicable selection"""
        self.model.instrument.update_instrument(self._selected_instrument_name())
        if self._group_is_custom():
            self.model.instrument.set_custom_grouping_file(self.view.get_grouping_file())
        self.set_model_group()
        self.model.update_all_projected_data()
        self.update_plots()

    def revalidate_grouping(self) -> None:
        """Recompute (and cache) whether the chosen custom grouping file fits the selected
        instrument, surface a problem on the finder when it does not, and refresh the button.

        Called only on the infrequent triggers (file chosen, instrument changed, group changed,
        custom name committed) because the check builds a
        throwaway instrument workspace."""
        applicable = True
        if self._group_is_custom():
            name = self._selected_instrument_name()
            grouping_file = self.view.get_grouping_file()
            name_ok = not self.view.is_custom_instrument() or self._cached_instrument_name_valid()
            applicable = bool(grouping_file) and name_ok and self.model.instrument.is_grouping_file_applicable(name, grouping_file)
            unfit = bool(grouping_file) and name_ok and not applicable
            self.view.set_grouping_file_problem("Grouping file is not applicable to this instrument" if unfit else "")
        self._grouping_applicable = applicable
        self.refresh_update_instrument_enabled()

    def refresh_update_instrument_enabled(self) -> None:
        self.view.set_update_instrument_enabled(self._selection_is_applicable())

    def _selection_is_applicable(self) -> bool:
        """The Update Instrument button is enabled only for a valid, complete selection:
        a known instrument (or valid custom IDF name) paired with an available group (a preset,
        or - for the custom group - a chosen grouping file confirmed applicable to the instrument)."""
        instrument_ok = not self.view.is_custom_instrument() or self._cached_instrument_name_valid()
        group_ok = not self._group_is_custom() or self._grouping_applicable
        return instrument_ok and group_ok

    def _cached_instrument_name_valid(self) -> bool:
        return self.model.instrument.get_is_valid_instrument()

    def _selected_instrument_name(self) -> str:
        if self.view.is_custom_instrument():
            return self.view.get_custom_instrument_name()
        return self.view.get_instrument()

    def update_custom_widgets_visibility(self) -> None:
        self.view.set_custom_instrument_name_visible(self.view.is_custom_instrument())
        self.view.set_grouping_finder_visible(self._group_is_custom())

    def _group_is_custom(self) -> bool:
        return self.view.get_group() == CUSTOM_GROUP

    def on_settings_applied(self) -> None:
        if self.model.plot_transmission:
            self.model.update_all_projected_data()
        self.update_plots()

    def update_plots(self) -> None:
        self.model.plotter.update_plot(
            self.get_vecs(),
            self.get_senses(),
            self.get_angles(),
            self.view.get_lab_figure(),
            self.view.get_lab_ax(),
            self.view.get_pf_ax(),
            self.view.get_current_index(),
        )

    def update_table(self) -> None:
        self.view.tableWidget.setRowCount(self.model.orientations.get_num_orientations())
        for row_index, row_info in enumerate(self.model.orientations.get_table_info()):
            self.view.add_table_row(row_index, *row_info)

    def add_orientation(self) -> None:
        # add an orientation to the dictionary
        self.model.orientations.add_orientation()
        new_orientation_index = self.model.orientations.get_num_orientations() - 1

        # update the orientation selector
        self.update_orientation_selector(new_orientation_index)
        # refresh goniometer bookkeeping for the newly-selected orientation
        self.on_goniometer_updated(self.model.gonio_index)

    def update_orientation_selector(self, new_index: int) -> None:
        self.view.spnIndex.setMaximum(self.model.orientations.get_num_orientations())
        self.view.set_current_index(new_index)

    def setup_group_options(self) -> None:
        self.view.setup_group_options(self.model.instrument.supported_groups)

    def update_goniometer_values_from_index(self, index: int) -> None:
        vecs, senses, angles = self.model.orientations.get_goniometer_values(index)
        self.view.set_goniometer_state(vecs, senses, angles)

    def on_index_changed(self) -> None:
        updated_index = self.view.get_current_index()
        self.model.orientations.set_orientation_index(updated_index)

        # Reflect the saved values into the goniometer fields without firing their change signals:
        # displaying an orientation must not rewrite it at widget precision (the fields hold
        # rounded display values, whereas e.g. an orientation loaded from a matrix file is exact).
        self.update_goniometer_values_from_index(updated_index)

        # refresh the displays for the newly selected orientation, reusing any cached results
        # (they were computed for this orientation's R, which has not changed)
        self.refresh_scattering_geometry()
        orientation = self.model.orientations[updated_index]
        if orientation.pf_points is None or (self.model.plot_transmission and orientation.transmission is None):
            self.model.update_projected_data(updated_index)
        self.update_plots()

    def enable_load_stl(self) -> None:
        self.view.set_load_stl_enabled(self.view.get_stl_string() != "")

    def enable_load_xml(self) -> None:
        self.view.set_load_xml_enabled(self.view.get_xml_string() != "")

    def enable_load_orient(self) -> None:
        self.view.set_load_orientation_enabled(self.view.get_orientation_file() != "")

    def enable_outputs(self) -> None:
        self.view.set_outputs_enabled(self.view.get_save_dir() != "" and self.view.get_save_filename() != "")

    def load_stl(self) -> None:
        self.model.workspaces.load_stl(self.view.get_stl_string())
        self.set_initial_shape()
        self.update_material_display()

    def load_xml(self) -> None:
        self.model.workspaces.load_xml(self.view.get_xml_string())
        self.set_initial_shape()
        self.update_material_display()

    def update_material_display(self) -> None:
        self.view.set_current_material(self.model.workspaces.get_material_name())

    def open_set_material_dialog(self) -> None:
        """Open the standard SetSampleMaterial dialog against the (hidden) raw mesh workspace.

        InputWorkspace is preset and therefore locked by the dialog, so the user never has to pick
        one of the planner's internal workspaces"""
        manager = InterfaceManager()
        preset = {"InputWorkspace": self.model.workspaces.WS_MESH_RAW}
        dialog = manager.createDialogFromName("SetSampleMaterial", -1, self.view, False, preset, "", (), ("InputWorkspace",))
        dialog.addAlgorithmObserver(self)
        dialog.setModal(True)
        dialog.show()

    def finishHandle(self) -> None:
        # AlgorithmObserver callback: fires on the algorithm worker thread when the SetSampleMaterial
        # dialog's algorithm finishes. Hop to the GUI thread via a Qt signal before touching
        # workspaces / redrawing plots.
        self.view.signal_material_set()

    def on_material_set(self) -> None:
        # the dialog only set the material on WS_MESH_RAW; share it with the other workspaces, then
        # recompute transmission (if shown) and refresh the plots
        self.model.workspaces.propagate_material()
        self.update_material_display()
        self.on_settings_applied()

    def load_orientation_file(self) -> None:
        num_gonios = self.model.orientations.load_orientation_file(self.view.get_orientation_file())
        # update selected number of goniometers to match
        self.view.set_num_gonios(num_gonios)
        # update the orientation selector
        self.update_orientation_selector(self.model.orientations.get_num_orientations() - 1)
        # update table and plot
        self.model.update_all_projected_data()
        self.update_table()
        self.update_plots()

    def update_selected(self) -> None:
        self.model.orientations.update_selected(self.view.get_select_indices())

    def update_included(self) -> None:
        self.model.orientations.update_included(self.view.get_include_indices())
        self.update_plots()

    def select_all(self) -> None:
        self.model.orientations.select_all()
        self.update_table()

    def deselect_all(self) -> None:
        self.model.orientations.deselect_all()
        self.update_table()

    def delete_selected(self) -> None:
        self.model.orientations.delete_selected()
        self.view.set_current_index(self.model.orientations.get_orientation_index())
        # call on index change, just in case the index happens to remain, we still need the same update
        self.on_index_changed()
        self.update_orientation_selector(self.model.orientations.get_orientation_index())
        # update table and plot
        self.model.update_all_projected_data()
        self.update_table()
        self.update_plots()

    def on_export_clicked(self) -> None:
        exporter = self.model.exporter
        handlers = {
            EXPORT_SSCANSS: exporter.output_as_sscanss,
            EXPORT_EULER: exporter.output_as_euler,
            EXPORT_MATRIX: exporter.output_as_matrix,
            EXPORT_REFERENCE_WS: exporter.output_as_reference_workspace,
            EXPORT_TRANSMISSION_WEIGHTING: exporter.output_transmission_weighting,
        }
        handlers[self.view.get_export_format()](self.view.get_save_dir(), self.view.get_save_filename())

    def set_show_transmission(self) -> None:
        self.update_custom_shape_finder_enabled()
        self.update_set_gauge_vol_enabled()
        show_transmission = self.view.get_show_transmission()
        self.model.set_plot_transmission(show_transmission)
        # the transmission weighting export is only offered once transmission has been estimated
        self.view.set_transmission_weighting_available(show_transmission)
        self.model.update_all_projected_data()
        self.update_plots()

    def set_transform_dirs(self) -> None:
        self.model.set_transform_dirs(self.view.get_transform_dirs())
        if self.view.get_lab_dirs():
            self.set_view_with_texture_directions()
        self.model.update_all_projected_data()
        self.update_plots()

    def set_lab_dirs(self) -> None:
        self.set_view_with_texture_directions()
        # the lab-frame values are derived, so they are shown read-only
        self.view.set_texture_directions_enabled(not self.view.get_lab_dirs())

    def set_initial_shape(self) -> None:
        self.model.workspaces.update_initial_shape(
            self.view.get_init_x(),
            self.view.get_init_y(),
            self.view.get_init_z(),
            self.view.get_init_px(),
            self.view.get_init_py(),
            self.view.get_init_pz(),
        )
        # moving the shape moves the scattering centre, so the detector geometry must follow
        self.refresh_scattering_geometry()
        self.model.update_all_projected_data()
        self.update_plots()
        if self.view.get_lab_dirs():
            # the initial rotation is what maps the sample frame onto the lab frame, so the
            # displayed lab directions are stale until they are re-derived
            self.set_view_with_texture_directions()

    def set_gauge_volume(self) -> None:
        self.model.workspaces.set_gauge_volume_str(self.view.get_shape_method(), self.view.get_custom_shape())
        # the gauge volume changes which part of the sample scatters, moving the scattering centre
        self.refresh_scattering_geometry()
        self.model.update_all_projected_data()
        self.update_plots()

    def clear_gauge_volume(self) -> None:
        self.model.workspaces.set_gauge_volume_str(GAUGE_VOL_NONE, None)
        self.refresh_scattering_geometry()
        self.model.update_all_projected_data()
        self.update_plots()

    def update_gauge_volume_state(self) -> None:
        self.update_custom_shape_finder_enabled()
        self.update_set_gauge_vol_enabled()

    def update_custom_shape_finder_enabled(self) -> None:
        self.view.set_finder_gauge_vol_visible(self.view.get_shape_method() == GAUGE_VOL_CUSTOM_SHAPE)

    def update_set_gauge_vol_enabled(self) -> None:
        self.view.set_set_gauge_vol_enabled(True)
        if self.view.get_shape_method() == GAUGE_VOL_CUSTOM_SHAPE:
            self.view.set_set_gauge_vol_enabled(self.view.get_custom_shape() is not None)
