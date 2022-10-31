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


def is_shape_valid(shape):
    if is_mesh_not_empty(shape.getMesh()):
        return True


def is_mesh_not_empty(mesh):
    if len(mesh) > 3:
        return True


def get_valid_sample_shape_from_workspace(workspace):
    sample_shape = get_sample_shape_from_workspace(workspace)
    if is_shape_valid(sample_shape):
        return sample_shape


def get_sample_shape_from_workspace(workspace_with_sample):
    if workspace_with_sample.sample():
        return workspace_with_sample.sample().getShape()


def get_valid_container_shape_from_workspace(workspace):
    container_shape = get_container_shape_from_workspace(workspace)
    if is_shape_valid(container_shape):
        return container_shape


def get_container_shape_from_workspace(workspace_with_container):
    if workspace_with_container.sample():
        return workspace_with_container.sample().getEnvironment().getContainer().getShape()


def get_valid_component_shape_from_workspace(workspace, component_index):
    component_shape = get_component_shape_from_workspace(workspace, component_index)
    if is_shape_valid(component_shape):
        return component_shape


def get_component_shape_from_workspace(workspace_with_components, component_index):
    if workspace_with_components.sample():
        return workspace_with_components.sample().getEnvironment().getComponent(component_index)


def plot_sample_container_and_components(workspace_name):
    sample_plotted = container_plotted = components_plotted = None
    workspace = ADS.retrieve(workspace_name)
    figure, axes = plt.subplots(subplot_kw={'projection': 'mantid3d'})
    if workspace.sample():
        sample_plotted = plot_sample_only(workspace, figure)
    if workspace.sample().hasEnvironment():
        container_plotted = plot_container(workspace, figure)
        number_of_components = workspace.sample().getEnvironment().nelements()
        if number_of_components > 1:
            components_plotted = plot_components(workspace, figure)

    add_title(sample_plotted, container_plotted, components_plotted, axes, workspace_name)
    set_axes_to_largest_mesh(axes, workspace)
    set_perspective(axes)
    set_axes_labels(axes)
    add_beam_arrow(axes, workspace)
    if workspace.sample().hasOrientedLattice():
        plot_lattice_vectors(axes, workspace)
    show_the_figure(figure)
    if sample_plotted or container_plotted or components_plotted:
        return figure


def plot_sample_only(workspace, figure):
    axes = figure.gca()
    # get shape and mesh vertices
    shape = get_valid_sample_shape_from_workspace(workspace)
    if shape:
        mesh = shape.getMesh()
        # Create 3D Polygon and set facecolor
        mesh_polygon = Poly3DCollection(mesh, edgecolors=['b'], alpha=0.5, linewidths=0.1)
        mesh_polygon.set_facecolor((0, 1, 0, 0.5))
        axes.add_collection3d(mesh_polygon)
        return True


def plot_container(workspace, figure):
    axes = figure.gca()
    container_shape = get_valid_container_shape_from_workspace(workspace)
    container_mesh = container_shape.getMesh()
    mesh_polygon = Poly3DCollection(container_mesh, edgecolors='red', alpha=0.1, linewidths=0.05, zorder=0.5)
    mesh_polygon.set_facecolor((0, 1, 0, 0.5))
    axes.add_collection3d(mesh_polygon)
    return True


def plot_components(workspace, figure):
    axes = figure.gca()
    number_of_components = workspace.sample().getEnvironment().nelements()
    for component_index in range(1, number_of_components):
        component_shape = get_valid_component_shape_from_workspace(workspace, component_index)
        component_mesh = component_shape.getMesh()
        mesh_polygon_loop = Poly3DCollection(component_mesh, edgecolors='red', alpha=0.1, linewidths=0.05, zorder=0.5)
        mesh_polygon_loop.set_facecolor((0, 1, 0, 0.5))
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
    plot_axes.set_xlabel('X / m')
    plot_axes.set_ylabel('Y / m')
    plot_axes.set_zlabel('Z / m')


def set_perspective(plot_axes):
    plot_axes.view_init(elev=15, azim=-135)


def call_set_mesh_axes_equal(axes, mesh):
    # not able to directly mock set_mesh_axes_equal() as part of MantidAxes3D, which inherits from matplotlib code
    axes.set_mesh_axes_equal(mesh)


def set_axes_to_largest_mesh(axes, workspace):
    overall_limits = overall_limits_for_all_meshes(workspace)
    call_set_mesh_axes_equal(axes, np.array(overall_limits))


def overall_limits_for_all_meshes(workspace, include_components=True):
    sample_shape = get_valid_sample_shape_from_workspace(workspace)
    if sample_shape:
        overall_limits = overall_limits_for_every_axis(sample_shape.getMesh())
    if workspace.sample():
        if include_components and workspace.sample().hasEnvironment():
            environment = workspace.sample().getEnvironment()
            number_of_components = environment.nelements()
            for component_index in range(number_of_components):
                if component_index == 0:  # Container
                    loop_component_mesh = get_valid_container_shape_from_workspace(workspace).getMesh()
                else:
                    loop_component_mesh = get_valid_component_shape_from_workspace(workspace, component_index).getMesh()
                current_mesh_limits = overall_limits_for_every_axis(loop_component_mesh)
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
    if source and sample:
        beam_origin = plot_axes.get_xlim3d()[0], plot_axes.get_ylim3d()[0], plot_axes.get_zlim3d()[0]
        beam_direction = calculate_beam_direction(source, sample)
        add_arrow(plot_axes, beam_direction, origin=beam_origin)


def add_arrow(ax, vector, origin=None, factor=None, color='black', linestyle='-'):
    # Add arrows for Beam or Crystal lattice
    if origin is None:
        origin = (ax.get_xlim3d()[1], ax.get_ylim3d()[1], ax.get_zlim3d()[1])
    if factor is None:
        lims = ax.get_xlim3d()
        factor = (lims[1]-lims[0]) / 3.0
    vector_norm = vector / np.linalg.norm(vector)
    ax.quiver(
        origin[0], origin[1], origin[2],
        vector_norm[0]*factor, vector_norm[1]*factor, vector_norm[2]*factor,
        color=color,
        linestyle=linestyle
    )


def plot_lattice_vectors(plot_axes, workspace):
    """Add arrows for real and reciprocal lattice vectors"""
    real_lattice_vectors, reciprocal_lattice_vectors = calculate_lattice_vectors(workspace)
    colors = ['r', 'g', 'b']
    plot_real_lattice_vectors(plot_axes, real_lattice_vectors, colors)
    plot_reciprocal_lattice_vectors(plot_axes, reciprocal_lattice_vectors, colors)


def plot_reciprocal_lattice_vectors(plot_axes, reciprocal_lattice, colors):
    for i in range(3):  # plot reciprocal_lattice with '--' dashed linestyle
        add_arrow(plot_axes, reciprocal_lattice[:, i], color=colors[i], linestyle='--')


def plot_real_lattice_vectors(plot_axes, real_lattice, colors):
    for i in range(3):  # plot real_lattice with '-' solid linestyle
        add_arrow(plot_axes, real_lattice[:, i], color=colors[i])


def calculate_lattice_vectors(workspace):
    ub_matrix = np.array(workspace.sample().getOrientedLattice().getUB())
    hkl = np.array([[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]])
    q_sample = np.matmul(ub_matrix, hkl)
    goniometer = workspace.getRun().getGoniometer().getR()
    reciprocal_lattice_vectors = np.matmul(goniometer, q_sample)  # QLab
    real_lattice_vectors = (2.0*np.pi)*np.linalg.inv(np.transpose(reciprocal_lattice_vectors))

    return real_lattice_vectors, reciprocal_lattice_vectors
