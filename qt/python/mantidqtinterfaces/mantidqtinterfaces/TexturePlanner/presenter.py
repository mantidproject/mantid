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

        self.view.set_on_update_dirs(self.on_directions_updated)
        self.view.set_on_gonio_vec_updated(self.on_goniometer_updated)
        self.view.set_on_gonio_sense_updated(self.on_goniometer_updated)
        self.view.set_on_gonio_angle_updated(self.on_goniometer_updated)
        self.view.set_on_num_gonio_updated(self.on_num_gonio_updated)
        self.view.set_on_step_updated(self.update_angle_steps)
        self.view.set_on_group_changed(self.on_group_changed)
        self.view.set_on_add_orientation_clicked(self.add_orientation)
        self.view.set_on_current_index_changed(self.on_index_changed)

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
        self.view.spnIndex.setValue(new_index)

    def setup_group_options(self):
        self.view.setup_group_options(self.model.supported_groups)

    def on_index_changed(self):
        # need to save the state of the old orientation index, so we rely on the value that exists on the model
        self.model.update_gonio_string(self.get_vecs(), self.get_senses(), self.get_angles(), self.model.get_orientation_index())
        # we then update this value to match the new current index, so it is ready for the next call of this function
        updated_index = self.view.get_current_index()
        self.model.set_orientation_index(updated_index)

        # we now update the goniometer fields with the saved values
        vecs, senses, angles = self.model.get_goniometer_values(updated_index)
        self.view.set_vecs(vecs)
        self.view.set_senses(senses)
        self.view.set_angles(angles)
