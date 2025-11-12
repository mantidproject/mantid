# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

from mantid.api import AnalysisDataService as ADS
from mantidqt.plotting.sample_shape_ads_observer import SampleShapePlotADSObserver
from workbench.plotting.globalfiguremanager import FigureAction, GlobalFigureManager
from workbench.plotting.toolbar import ToolbarStateManager
from mantidqt.utils.qt.qappthreadcall import run_on_qapp_thread


@run_on_qapp_thread()
class SampleShapePlot:
    def __init__(
        self,
    ):
        self.ads_observer = None
        self.figure = None
        self.workspace_name = None
        self.plot_visible = None
        self.GFM_plot_number = None
        self.toolbar_manager = None
        self.beam_direction = None
        self.overall_limits = None

        self.Local_GlobalFigureManager = GlobalFigureManager
        # Register with CurrentFigure that we want to know of any
        # changes to the list of plots
        self.Local_GlobalFigureManager.add_observer(self)

    def reset_class(self):
        self.ads_observer = None
        self.figure = None
        self.workspace_name = None
        self.plot_visible = None
        self.GFM_plot_number = None
        self.Local_GlobalFigureManager = None
        self.toolbar_manager = None
        self.beam_direction = None
        self.overall_limits = None

    def notify(self, action, plot_number):
        """
        This is called by GlobalFigureManager when plots are created
        or destroyed, renamed or the active order is changed. This is used to
        track the plot_number and its visibility to correctly update
        sample shape plots as workspaces are updated.

        :param: action: A FigureAction corresponding to the event
        :param: plot_number: The unique number in GlobalFigureManager
        """
        if action == FigureAction.New:
            self.GFM_plot_number = plot_number
        if action == FigureAction.VisibilityChanged:
            self.plot_visible = is_visible(plot_number)

    def create_plot(self, workspace_name, figure=None, test=None, alpha=1.0, custom_color=None):
        self.workspace_name = workspace_name
        workspace = ADS.retrieve(workspace_name)
        sample_mesh = get_valid_sample_mesh_from_workspace(workspace)
        container_mesh = get_valid_container_mesh_from_workspace(workspace)
        component_meshes = get_valid_component_meshes_from_workspace(workspace)
        if sample_mesh is None and container_mesh is None and not component_meshes:
            raise Exception("Workspace must have attached Sample Shape or Environment")
        if figure:
            axes = figure.gca()
        else:
            self.plot_visible = True
            figure, axes = plt.subplots(subplot_kw={"projection": "mantid3d", "proj_type": "ortho"})

        sample_plotted, container_plotted, components_plotted = try_to_plot_all_sample_shapes(
            figure, sample_mesh, container_mesh, component_meshes, alpha, custom_color
        )
        if int(sample_plotted) + int(container_plotted) + int(components_plotted) == 0:
            raise Exception("Workspace has no valid Sample, Container or Component Shapes to plot")

        add_title(sample_plotted, container_plotted, components_plotted, axes, workspace_name)
        set_axes_to_largest_mesh(axes, sample_mesh, container_mesh, component_meshes)
        self.beam_direction = set_perspective(axes, workspace=workspace)
        set_axes_labels(axes)
        add_beam_arrow(axes, workspace)
        if workspace.sample().hasOrientedLattice():
            plot_lattice_vectors(axes, workspace)

        self.ads_observer = SampleShapePlotADSObserver(
            self.on_replace_workspace, self.on_rename_workspace, self.on_clear, self.on_delete_workspace
        )
        self.set_and_save_figure(workspace_name, figure)
        self.connect_to_home_toolbar_button()
        if test:
            return self
        return figure

    def set_and_save_figure(self, workspace_name, figure):
        if self.plot_visible:
            show_the_figure(figure)
        figure.canvas.draw()
        self.figure = figure
        set_figure_window_title(self.GFM_plot_number, workspace_name)

    def on_replace_workspace(self, workspace_name):
        self.do_not_replace_plot_if_closed()

        if self.workspace_name == workspace_name:
            self.figure.gca().clear()
            self.create_plot(workspace_name=self.workspace_name, figure=self.figure)

    def do_not_replace_plot_if_closed(self):
        plot_is_open = False
        if self.figure:
            for manager in GlobalFigureManager.figs.values():
                if manager.canvas.figure == self.figure:
                    plot_is_open = True

        if not plot_is_open:
            self.reset_class()

    def on_rename_workspace(self, old_workspace_name, new_workspace_name):
        if self.workspace_name == old_workspace_name:
            self.workspace_name = new_workspace_name
        # on_replace_workspace is also triggered by a workspace rename

    def on_delete_workspace(self, workspace_name):
        if self.workspace_name == workspace_name:
            self.close_this_plot()

    def on_clear(self):
        self.close_this_plot()

    def close_this_plot(self):
        GlobalFigureManager.destroy_fig(self.figure)
        self.reset_class()

    def connect_to_home_toolbar_button(self):
        if self.figure.canvas.toolbar:
            self.toolbar_manager = ToolbarStateManager(self.figure.canvas.toolbar)
            self.toolbar_manager.home_button_connect(self.on_home_clicked)

    def on_home_clicked(self):
        current_axes = self.figure.gca()
        if self.beam_direction:
            set_perspective(current_axes, beam_direction=self.beam_direction)
        current_axes.set_box_aspect((1, 1, 1))  # when upgraded to matplotlib 3.6 use current_axes.set_aspect('equal')
        self.figure.canvas.draw()


def is_visible(plot_number):
    """
    Determines if plot window is visible or hidden
    :return: True if plot visible (window open), false if hidden
    """
    figure_manager = GlobalFigureManager.figs.get(plot_number)
    if figure_manager is None:
        raise ValueError("Error in is_visible, could not find a plot with the number {}.".format(plot_number))

    return figure_manager.window.isVisible()


def set_figure_window_title(plot_number, workspace_name):
    """
    Renames a figure in the GlobalFigureManager
    :param plot_number: The unique number in GlobalFigureManager
    :param workspace_name: The workspace name to include in new title
    """
    figure_manager = GlobalFigureManager.figs.get(plot_number)
    # This can be triggered before the plot is added to the
    # GlobalFigureManager, so we silently ignore this case
    if figure_manager is not None:
        figure_manager.set_window_title(f"{workspace_name} Sample Shape")


def plot_sample_container_and_components(workspace_name, test=None, alpha=1.0, custom_color=None):
    new_plot = SampleShapePlot()
    return new_plot.create_plot(workspace_name, test=test, alpha=alpha, custom_color=custom_color)


def try_to_plot_all_sample_shapes(figure, sample_mesh, container_mesh, component_meshes, alpha, custom_color):
    container_plotted = components_plotted = False
    sample_plotted = plot_sample_only(figure, sample_mesh, alpha, custom_color)
    if container_mesh is not None:
        container_plotted = plot_container(figure, container_mesh)
    if component_meshes:
        components_plotted = plot_components(figure, component_meshes)
    return sample_plotted, container_plotted, components_plotted


def is_mesh_not_empty(mesh):
    if len(mesh) > 3:
        return True


def get_valid_sample_mesh_from_workspace(workspace):
    sample_shape = get_sample_shape_from_workspace(workspace)
    if sample_shape:
        mesh = sample_shape.getMesh()
        if is_mesh_not_empty(mesh):
            return mesh


def get_sample_shape_from_workspace(workspace_with_sample):
    if workspace_with_sample.sample():
        return workspace_with_sample.sample().getShape()


def get_valid_container_mesh_from_workspace(workspace):
    if workspace.sample().hasEnvironment():
        container_shape = get_container_shape_from_workspace(workspace)
        mesh = container_shape.getMesh()
        if is_mesh_not_empty(mesh):
            return mesh


def get_container_shape_from_workspace(workspace_with_container):
    if workspace_with_container.sample():
        return workspace_with_container.sample().getEnvironment().getContainer().getShape()


def get_valid_component_meshes_from_workspace(workspace):
    component_meshes = []
    if workspace.sample().hasEnvironment():
        number_of_components = workspace.sample().getEnvironment().nelements()
        for component_index in range(1, number_of_components):
            component_shape = get_component_shape_from_workspace(workspace, component_index)
            component_mesh = component_shape.getMesh()
            if is_mesh_not_empty(component_mesh):
                component_meshes.append(component_mesh)
    return component_meshes


def get_component_shape_from_workspace(workspace_with_components, component_index):
    if workspace_with_components.sample():
        return workspace_with_components.sample().getEnvironment().getComponent(component_index)


def plot_sample_only(figure, mesh, alpha=1.0, specified_face_color=None):
    axes = figure.gca()
    if mesh is not None:
        if len(mesh) > 13 and not specified_face_color:
            face_colors = [
                "purple",
                "mediumorchid",
                "royalblue",
                "b",
                "red",
                "firebrick",
                "green",
                "darkgreen",
                "grey",
                "black",
                "gold",
                "orange",
            ]
            mesh_polygon = Poly3DCollection(mesh, facecolors=face_colors, edgecolors="black", alpha=alpha, linewidths=0.1)
        elif specified_face_color:
            mesh_polygon = Poly3DCollection(mesh, facecolors=specified_face_color, edgecolors="black", alpha=alpha, linewidths=0.1)
        else:
            mesh_polygon = Poly3DCollection(mesh, facecolors="red", edgecolors="black", alpha=alpha, linewidths=0.1)
        axes.add_collection3d(mesh_polygon)
        return True
    else:
        return False


def plot_container(figure, container_mesh):
    axes = figure.gca()
    mesh_polygon = Poly3DCollection(container_mesh, edgecolors="black", alpha=0.1, linewidths=0.05, zorder=0.5)
    mesh_polygon.set_facecolor((0, 1, 0, 0.5))
    axes.add_collection3d(mesh_polygon)
    return True


def plot_components(figure, component_meshes):
    axes = figure.gca()
    for component_mesh in component_meshes:
        mesh_polygon_loop = Poly3DCollection(component_mesh, edgecolors="black", alpha=0.1, linewidths=0.05, zorder=0.5)
        mesh_polygon_loop.set_facecolor((0, 0, 1, 0.5))
        axes.add_collection3d(mesh_polygon_loop)
    return True


def add_title(sample_plotted, container_plotted, components_plotted, plot_axes, name_of_workspace):
    title = construct_title(sample_plotted, container_plotted, components_plotted, name_of_workspace)
    plot_axes.set_title(title)


def construct_title(sample_plotted, container_plotted, components_plotted, name_of_workspace):
    title_string = ""
    if sample_plotted:
        title_string += "Sample"
    if container_plotted:
        if title_string:
            if components_plotted:
                title_string += ", "
            else:
                title_string += " and "
        title_string += "Container"
    if components_plotted:
        if title_string:
            title_string += " and "
        title_string += "Components"
    title_string += f": {name_of_workspace}"
    return title_string


def show_the_figure(plot_figure):
    plot_figure.show()


def set_axes_labels(plot_axes):
    plot_axes.set_xlabel("X / m")
    plot_axes.set_ylabel("Y / m")
    plot_axes.set_zlabel("Z / m")


def direction_not_valid(direction):
    if not direction:
        return True
    if direction != "X" and direction != "Z":
        return True
    else:
        return False


def set_perspective(plot_axes, workspace=None, beam_direction=None):
    # assume beam_direction Z (X) means pointingUp direction Y (Z)
    if direction_not_valid(beam_direction) and not workspace:
        # guess the original beam_direction and raise warning
        plot_axes.view_init(vertical_axis="y", elev=30, azim=-135)
        raise Exception("set_perspective must be called with either beam_direction or a workspace")
    if direction_not_valid(beam_direction):
        frame = workspace.getInstrument().getReferenceFrame()
        if frame.pointingUpAxis() == "Y" and frame.pointingAlongBeamAxis() == "Z":
            # almost all instruments have this instrument reference frame
            beam_direction = "Z"
        else:  # e.g. POLREF has beam along X, pointing up Z
            beam_direction = "X"

    if beam_direction == "Z":
        plot_axes.view_init(vertical_axis="y", elev=30, azim=-135)
    else:
        plot_axes.view_init(vertical_axis="z", elev=30, azim=-135)

    return beam_direction


def call_set_mesh_axes_equal(axes, mesh):
    # not able to directly mock set_mesh_axes_equal() as part of MantidAxes3D, which inherits from matplotlib code
    axes.set_mesh_axes_equal(mesh)


def set_axes_to_largest_mesh(axes, sample_mesh, container_mesh=None, component_meshes=[]):
    overall_limits = overall_limits_for_all_meshes(sample_mesh, container_mesh, component_meshes)
    call_set_mesh_axes_equal(axes, np.array(overall_limits))
    axes.set_box_aspect((1, 1, 1))  # when upgraded to matplotlib 3.6 use axes.set_aspect('equal')


def overall_limits_for_all_meshes(sample_mesh, container_mesh=None, component_meshes=[]):
    overall_limits = None
    if sample_mesh is not None:
        overall_limits = overall_limits_for_every_axis(sample_mesh)
    if container_mesh is not None:
        current_mesh_limits = overall_limits_for_every_axis(container_mesh)
        overall_limits = greater_limits(current_mesh_limits, overall_limits)
    for component_mesh in component_meshes:
        current_mesh_limits = overall_limits_for_every_axis(component_mesh)
        overall_limits = greater_limits(current_mesh_limits, overall_limits)
    return overall_limits


def overall_limits_for_every_axis(mesh):
    if is_mesh_not_empty(mesh):
        flattened_mesh = mesh.flatten()
        minimum_x = flattened_mesh[0::3].min()
        maximum_x = flattened_mesh[0::3].max()
        minimum_y = flattened_mesh[1::3].min()
        maximum_y = flattened_mesh[1::3].max()
        minimum_z = flattened_mesh[2::3].min()
        maximum_z = flattened_mesh[2::3].max()
        return [minimum_x, minimum_y, minimum_z, maximum_x, maximum_y, maximum_z]
    else:
        return None


def compare_and_replace_minimum(new_limit, old_limit):
    output_limit = new_limit
    if old_limit < new_limit:
        output_limit = old_limit
    return output_limit


def compare_and_replace_maximum(new_limit, old_limit):
    output_limit = new_limit
    if old_limit > new_limit:
        output_limit = old_limit
    return output_limit


def greater_limits(new_limits, old_limits):
    # Set limits to new limits, and replace with old limits that are wider
    if old_limits:
        min_x = compare_and_replace_minimum(new_limits[0], old_limits[0])
        min_y = compare_and_replace_minimum(new_limits[1], old_limits[1])
        min_z = compare_and_replace_minimum(new_limits[2], old_limits[2])
        max_x = compare_and_replace_maximum(new_limits[3], old_limits[3])
        max_y = compare_and_replace_maximum(new_limits[4], old_limits[4])
        max_z = compare_and_replace_maximum(new_limits[5], old_limits[5])
    else:
        [min_x, min_y, min_z, max_x, max_y, max_z] = new_limits
    return [min_x, min_y, min_z, max_x, max_y, max_z]


def calculate_beam_direction(source, sample):
    source_position = source.getPos()
    sample_position = sample.getPos()
    beam_vector = sample_position - source_position
    return beam_vector


def add_beam_arrow(plot_axes, workspace):
    # Add arrow along beam direction
    source = workspace.getInstrument().getSource()
    sample = workspace.getInstrument().getSample()
    if source is not None and sample is not None:
        beam_origin = plot_axes.get_xlim3d()[0], plot_axes.get_ylim3d()[0], plot_axes.get_zlim3d()[0]
        beam_direction = calculate_beam_direction(source, sample)
        add_arrow(plot_axes, beam_direction, origin=beam_origin)


def add_arrow(ax, vector, origin=None, relative_factor=None, color="black", linestyle="-"):
    # Add arrows for Beam or Crystal lattice
    if origin is None:
        origin = (ax.get_xlim3d()[1], ax.get_ylim3d()[1], ax.get_zlim3d()[0])
    lims = ax.get_xlim3d()
    factor = (lims[1] - lims[0]) / 3.0
    if relative_factor:
        factor *= relative_factor
    vector_norm = vector / np.linalg.norm(vector)
    ax.quiver(
        origin[0],
        origin[1],
        origin[2],
        vector_norm[0] * factor,
        vector_norm[1] * factor,
        vector_norm[2] * factor,
        color=color,
        linestyle=linestyle,
    )


def plot_lattice_vectors(plot_axes, workspace):
    """Add arrows for real and reciprocal lattice vectors"""
    real_lattice_vectors, reciprocal_lattice_vectors = calculate_lattice_vectors(workspace)
    colors = ["r", "g", "b"]
    plot_real_lattice_vectors(plot_axes, real_lattice_vectors, colors)
    plot_reciprocal_lattice_vectors(plot_axes, reciprocal_lattice_vectors, colors)


def plot_reciprocal_lattice_vectors(plot_axes, reciprocal_lattice, colors):
    for i in range(3):  # plot reciprocal_lattice with '--' dashed linestyle
        add_arrow(plot_axes, reciprocal_lattice[:, i], color=colors[i], linestyle="--")


def plot_real_lattice_vectors(plot_axes, real_lattice, colors):
    for i in range(3):  # plot real_lattice with '-' solid linestyle
        add_arrow(plot_axes, real_lattice[:, i], relative_factor=0.8, color=colors[i])


def calculate_lattice_vectors(workspace):
    ub_matrix = np.array(workspace.sample().getOrientedLattice().getUB())
    hkl = np.array([[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]])  # h, k and l are column vectors in this matrix
    q_sample = np.matmul(ub_matrix, hkl)
    goniometer = workspace.getRun().getGoniometer().getR()
    reciprocal_lattice_vectors = np.matmul(goniometer, q_sample)  # QLab
    real_lattice_vectors = (2.0 * np.pi) * np.linalg.inv(np.transpose(reciprocal_lattice_vectors))

    return real_lattice_vectors, reciprocal_lattice_vectors
