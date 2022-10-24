# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#

import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

from mantid.api import AnalysisDataService as ADS


def get_sample_shape_from_workspace(workspace_name):
    selected_workspace = ADS.retrieve(workspace_name)
    if selected_workspace.sample():
        return selected_workspace.sample().getShape()


def is_sample_shape_not_empty(shape):
    mesh = shape.getMesh()
    if len(mesh) > 3:
        return True


def get_valid_sample_shape_from_workspace(workspace_name):
    sample_shape = get_sample_shape_from_workspace(workspace_name)
    if is_sample_shape_not_empty(sample_shape):
        return sample_shape


def plot_sample_shape(workspace_name):
    sample_shape_figure = plot_sample_only(workspace_name)
    workspace = ADS.retrieve(workspace_name)
    if workspace.sample().hasEnvironment():
        plot_container_and_components(workspace_name, sample_shape_figure)
    return sample_shape_figure


def plot_sample_only(workspace_name):
    workspace = ADS.retrieve(workspace_name)

    # get shape and mesh vertices
    sample = workspace.sample()
    shape = sample.getShape()
    mesh = shape.getMesh()

    # Create 3D Polygon and set facecolor
    mesh_polygon = Poly3DCollection(mesh, facecolors=['g'], edgecolors=['b'], alpha=0.5, linewidths=0.1)
    mesh_polygon.set_facecolor((0, 1, 0, 0.5))
    fig, axes = plt.subplots(subplot_kw={'projection': 'mantid3d'})
    axes.add_collection3d(mesh_polygon)

    axes.set_mesh_axes_equal(mesh)
    axes.view_init(elev=10, azim=-150)
    axes.set_title(f'Sample Shape: {workspace_name}')
    axes.set_xlabel('X / m')
    axes.set_ylabel('Y / m')
    axes.set_zlabel('Z / m')

    fig.show()
    return fig


def plot_container_and_components(workspace_name, figure):
    axes = figure.gca()
    workspace = ADS.retrieve(workspace_name)
    environment = workspace.sample().getEnvironment()

    number_of_components = environment.nelements()
    for component_index in range(number_of_components):
        mesh_loop = None
        if component_index == 0:  # Container
            mesh_loop = environment.getComponent(component_index).getShape().getMesh()
        else:
            mesh_loop = environment.getComponent(component_index).getMesh()
        mesh_polygon_loop = Poly3DCollection(mesh_loop, edgecolors='red', alpha=0.1, linewidths=0.05, zorder=0.5)
        mesh_polygon_loop.set_facecolor((0, 1, 0, 0.5))
        axes.add_collection3d(mesh_polygon_loop)

    axes.set_title('Sample and Components')
    figure.show()
    return True

#
# def arrow(ax, vector, origin = None, factor = None, color = 'black',linestyle = '-'):
#     if origin == None:
#         origin = (ax.get_xlim3d()[1],ax.get_ylim3d()[1],ax.get_zlim3d()[1])
#     if factor == None:
#         lims = ax.get_xlim3d()
#         factor = (lims[1]-lims[0]) / 3.0
#     vector_norm = vector / np.linalg.norm(vector)
#     ax.quiver(
#         origin[0], origin[1], origin[2],
#         vector_norm[0]*factor, vector_norm[1]*factor, vector_norm[2]*factor,
#         color = color,
#         linestyle = linestyle
#     )
# # Add arrow along beam direction
# source = ws.getInstrument().getSource().getPos()
# sample = ws.getInstrument().getSample().getPos() - source
# arrow(axes, sample, origin=(0,0,-0.04))
#
# plt.show()
#
#
#
# Add arrows for Beam or Crystal lattice
#
# def arrow(ax, vector, origin = None, factor = None, color = 'black',linestyle = '-'):
#     if origin == None:
#         origin = (ax.get_xlim3d()[1],ax.get_ylim3d()[1],ax.get_zlim3d()[1])
#     if factor == None:
#         lims = ax.get_xlim3d()
#         factor = (lims[1]-lims[0]) / 3.0
#     vector_norm = vector / np.linalg.norm(vector)
#     ax.quiver(
#         origin[0], origin[1], origin[2],
#         vector_norm[0]*factor, vector_norm[1]*factor, vector_norm[2]*factor,
#         color = color,
#         linestyle = linestyle
#     )
#
#     # Create ws and plot sample shape as previously described
#
# '''Add arrow along beam direction'''
# source = ws.getInstrument().getSource().getPos()
# sample = ws.getInstrument().getSample().getPos() - source
# arrow(axes, sample, origin=(0,0,-0.04))
#
# '''Calculate Lattice Vectors'''
# SetUB(ws, a=1, b=1, c=2, alpha=90, beta=90, gamma=60)
# if not sample.hasOrientedLattice():
#     raise Exception("There is no valid lattice")
# UB = np.array(ws.sample().getOrientedLattice().getUB())
# hkl = np.array([[1.0,0.0,0.0],[0.0,1.0,0.0],[0.0,0.0,1.0]])
# QSample = np.matmul(UB,hkl)
# Goniometer = ws.getRun().getGoniometer().getR()
# reciprocal_lattice = np.matmul(Goniometer,QSample)#QLab
# real_lattice = (2.0*np.pi)*np.linalg.inv(np.transpose(reciprocal_lattice))
#
# '''Add arrows for real and reciprocal lattice vectors'''
# colors = ['r','g','b']
# for i in range(3): # plot real_lattice with '-' solid linestyle
#     arrow(axes, real_lattice[:,i], color = colors[i])
# for i in range(3): # plot reciprocal_lattice with '--' dashed linestyle
#     arrow(axes, reciprocal_lattice[:,i], color = colors[i], linestyle = '--')
#
#
#
#
# set mesh axes equal
#
#
# mesh = shape.getMesh()
# mesh_polygon = Poly3DCollection(mesh, facecolors = facecolors, linewidths=0.1)
# fig, axes = plt.subplots(subplot_kw={'projection':'mantid3d'})
# axes.add_collection3d(mesh_polygon)
#
# axes.set_mesh_axes_equal(mesh)
# # then add arrows as desired
#
# '''
