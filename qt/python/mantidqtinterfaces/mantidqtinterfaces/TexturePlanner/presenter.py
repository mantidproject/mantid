# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class TexturePlannerPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view

        self.set_view_with_default_texture_directions()
        self.update_enabled_gonios(self.view.get_num_gonios())
        self.set_max_orientation_index()
        self.setup_group_options()

        self.set_model_texture_directions()
        self.set_model_group()
        self.on_goniometer_updated(0)

        # connect slots
        self.view.set_on_stl_file_changed(self.enable_load_stl)
        self.view.set_on_xml_file_changed(self.enable_load_xml)
        self.view.set_on_orient_file_changed(self.enable_load_orient)
        self.view.set_on_load_stl_clicked(self.load_stl)
        self.view.set_on_load_xml_clicked(self.load_xml)
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
        self.view.set_on_group_changed(self.on_group_changed)
        self.view.set_on_add_orientation_clicked(self.add_orientation)
        self.view.set_on_current_index_changed(self.on_index_changed)
        self.view.sig_select_state_changed.connect(self.update_selected)
        self.view.sig_include_state_changed.connect(self.update_included)
        self.view.set_on_select_all_clicked(self.select_all)
        self.view.set_on_deselect_all_clicked(self.deselect_all)
        self.view.set_on_delete_selected_clicked(self.delete_selected)
        self.view.set_on_output_sscanss_clicked(self.to_sscanss)
        self.view.set_on_output_euler_clicked(self.to_euler)
        self.view.set_on_output_matrix_clicked(self.to_matrix)
        self.view.set_on_save_dir_changed(self.enable_outputs)
        self.view.set_on_save_file_changed(self.enable_outputs)
        self.view.set_on_show_mu_toggled(self.set_show_mu)
        self.view.set_on_material_changed(self.set_material)
        self.view.set_on_gauge_vol_state_changed(self.update_gauge_volume_state)
        self.view.set_on_gauge_vol_file_changed(self.update_set_gauge_vol_enabled)
        self.view.set_on_set_gauge_volume_clicked(self.set_gauge_volume)

    def set_view_texture_directions(self, names, vecs):
        self.view.set_rd_name(names[0])
        self.view.set_nd_name(names[1])
        self.view.set_td_name(names[2])
        self.view.set_rd_dir(vecs[0])
        self.view.set_nd_dir(vecs[1])
        self.view.set_td_dir(vecs[2])

    def set_model_texture_directions(self):
        self.model.set_ax_transform(self.view.get_rd_dir(), self.view.get_nd_dir(), self.view.get_td_dir())
        self.model.set_dir_names(self.view.get_rd_name(), self.view.get_nd_name(), self.view.get_td_name())

    def set_model_group(self):
        self.model.set_group(self.view.get_group())
        self.model.get_detQ_lab()

    def set_view_with_default_texture_directions(self):
        names, vecs = self.model.get_default_texture_directions()
        self.set_view_texture_directions(names, vecs)

    def update_enabled_gonios(self, num_gonios):
        [self.view.set_gonio_axis_enabled(gon, True) for gon in self.view.gonio_axes[:num_gonios]]
        [self.view.set_gonio_axis_enabled(gon, False) for gon in self.view.gonio_axes[num_gonios:]]

    def update_angle_steps(self):
        self.view.set_angle_steps()

    def set_max_orientation_index(self):
        num_orientations = self.model.get_num_orientations()
        self.view.set_max_ind(num_orientations)

    def get_vecs(self):
        return self.model.get_vecs(self.view.get_vecs(), self.view.get_num_gonios())

    def get_senses(self):
        return self.model.get_senses(self.view.get_senses(), self.view.get_num_gonios())

    def get_angles(self):
        return self.model.get_angles(self.view.get_angles(), self.view.get_num_gonios())

    def on_goniometer_updated(self, goniometer_index, *args):
        self.model.set_gonio_index(goniometer_index)
        orientation_index = self.view.get_current_index()
        self.model.update_gRs(self.get_vecs(), self.get_senses(), self.get_angles(), orientation_index)
        self.model.update_projected_data(orientation_index)
        self.model.update_gonio_string(self.get_vecs(), self.get_senses(), self.get_angles(), orientation_index)
        self.update_plots()
        self.update_table()

    def on_num_gonio_updated(self):
        num_gonios = self.view.get_num_gonios()
        self.model.set_n_gonio(num_gonios)
        self.update_enabled_gonios(num_gonios)
        self.on_goniometer_updated(self.model.update_gonio_index(num_gonios))
        self.view.hide_axis_columns()

    def on_directions_updated(self):
        self.set_model_texture_directions()
        self.model.get_detQ_lab()
        self.model.update_all_projected_data()
        self.update_plots()

    def on_group_changed(self):
        self.set_model_group()
        self.model.update_all_projected_data()
        self.update_plots()

    def update_plots(self):
        self.model.update_plot(
            self.get_vecs(),
            self.get_senses(),
            self.get_angles(),
            self.view.get_lab_figure(),
            self.view.get_lab_ax(),
            self.view.get_pf_ax(),
            self.view.get_current_index(),
        )

    def update_table(self):
        self.view.tableWidget.setRowCount(self.model.get_num_orientations())
        for row_index, row_info in enumerate(self.model.get_table_info()):
            self.view.add_table_row(row_index, *row_info)

    def add_orientation(self):
        # add an orientation to the dictionary
        self.model.add_orientation()
        new_index = self.model.get_num_orientations()

        # update the orientation selector
        self.update_orientation_selector(new_index)
        # update the goniometer bookkeeping
        self.on_goniometer_updated(new_index)

    def update_orientation_selector(self, new_index):
        self.view.spnIndex.setMaximum(self.model.get_num_orientations())
        self.view.set_current_index(new_index)

    def setup_group_options(self):
        self.view.setup_group_options(self.model.supported_groups)

    def update_goniometer_values_from_index(self, index):
        vecs, senses, angles = self.model.get_goniometer_values(index)
        self.view.set_vecs(vecs)
        self.view.set_senses(senses)
        self.view.set_angles(angles)

    def on_index_changed(self):
        updated_index = self.view.get_current_index()
        self.model.set_orientation_index(updated_index)

        # we now update the goniometer fields with the saved values
        self.update_goniometer_values_from_index(updated_index)

    def enable_load_stl(self):
        self.view.set_load_stl_enabled(self.view.get_stl_string() != "")

    def enable_load_xml(self):
        self.view.set_load_xml_enabled(self.view.get_xml_string() != "")

    def enable_load_orient(self):
        self.view.set_load_orientation_enabled(self.view.get_orientation_file() != "")

    def enable_outputs(self):
        self.view.set_outputs_enabled(self.view.get_save_dir() != "" and self.view.get_save_filename() != "")

    def load_stl(self):
        self.model.load_stl(self.view.get_stl_string())
        self.update_plots()

    def load_xml(self):
        self.model.load_xml(self.view.get_xml_string())
        self.update_plots()

    def load_orientation_file(self):
        num_gonios = self.model.load_orientation_file(self.view.get_orientation_file())
        # update selected number of goniometers to match
        self.view.set_num_gonios(num_gonios)
        # update the orientation selector
        self.view.set_current_index(self.model.get_num_orientations() - 1)
        self.update_orientation_selector(self.model.get_num_orientations())
        # update table and plot
        self.model.update_all_projected_data()
        self.update_table()
        self.update_plots()

    def update_selected(self):
        self.model.update_selected(self.view.get_select_indices())

    def update_included(self):
        self.model.update_included(self.view.get_include_indices())
        self.update_plots()

    def select_all(self):
        self.model.select_all()
        self.update_table()

    def deselect_all(self):
        self.model.deselect_all()
        self.update_table()

    def delete_selected(self):
        self.model.delete_selected()
        self.view.set_current_index(self.model.get_orientation_index())
        # call on index change, just in case the index happens to remain, we still need the same update
        self.on_index_changed()
        self.update_orientation_selector(self.model.get_orientation_index())
        # update table and plot
        self.model.update_all_projected_data()
        self.update_table()
        self.update_plots()

    def to_sscanss(self):
        self.model.output_as_sscanss(self.view.get_save_dir(), self.view.get_save_filename())

    def to_euler(self):
        self.model.output_as_euler(self.view.get_save_dir(), self.view.get_save_filename())

    def to_matrix(self):
        self.model.output_as_matrix(self.view.get_save_dir(), self.view.get_save_filename())

    def set_show_mu(self):
        self.update_custom_shape_finder_enabled()
        self.update_set_gauge_vol_enabled()
        self.view.set_material_visible(self.view.get_show_mu())
        self.model.set_plot_attenuation(self.view.get_show_mu())
        self.model.update_all_projected_data()
        self.update_plots()

    def set_material(self):
        self.model.set_material_string(self.view.get_material())
        self.model.update_all_projected_data()
        self.update_plots()

    def set_initial_shape(self):
        self.model.update_initial_shape(
            self.view.get_init_x(),
            self.view.get_init_y(),
            self.view.get_init_z(),
            self.view.get_init_px(),
            self.view.get_init_py(),
            self.view.get_init_pz(),
        )
        self.model.update_all_projected_data()
        self.update_plots()

    def set_gauge_volume(self):
        self.model.set_gauge_volume_str(self.view.get_shape_method(), self.view.get_custom_shape())
        self.model.update_all_projected_data()
        self.update_plots()

    def update_gauge_volume_state(self):
        self.update_custom_shape_finder_enabled()
        self.update_set_gauge_vol_enabled()

    def update_custom_shape_finder_enabled(self):
        self.view.set_finder_gauge_vol_enabled(self.view.get_shape_method() == "Custom Shape")

    def update_set_gauge_vol_enabled(self):
        self.view.set_set_gauge_vol_enabled(True)
        if self.view.get_shape_method() == "Custom Shape":
            self.view.set_set_gauge_vol_enabled(self.view.get_custom_shape() is not None)
