# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from mantid.plots.utility import row_num, col_num
from matplotlib.container import ErrorbarContainer
from matplotlib.collections import QuadMesh
from mpl_toolkits.mplot3d.art3d import Poly3DCollection


def generate_ax_name(ax):
    """
    Generate a name for the given axes. This will come from the
    title of the axes (if there is one) and the position of the axes
    on the figure.
    """
    title = ax.get_title()
    position = "({}, {})".format(row_num(ax), col_num(ax))
    if title:
        return "{}: {}".format(title, position)
    return position


def get_axes_names_dict(fig, curves_only=False, images_only=False):
    """
    Return dictionary mapping axes names to the corresponding Axes
    object.
    :param fig: A matplotlib Figure object
    :param curves_only: Bool. If True only add axes to dict if it contains a curve
    :param images_only: Bool. If True only add axes to dict if it contains an image
    """
    if curves_only and images_only:
        return ValueError("Only one of 'curves_only' and 'images_only' may be " "True.")
    axes_names = {}
    for ax in fig.get_axes():
        if ax not in [img.axes for img in get_colorbars_from_fig(fig)]:
            if curves_only and curve_in_ax(ax):
                axes_names[generate_ax_name(ax)] = ax
            elif images_only and image_in_ax(ax):
                axes_names[generate_ax_name(ax)] = ax
            elif not curves_only and not images_only:
                axes_names[generate_ax_name(ax)] = ax
    return axes_names


def curve_in_figure(fig):
    """Return True if there is an ErrobarContainer or Line2D in fig"""
    for ax in fig.get_axes():
        if line_in_ax(ax) or errorbars_in_ax(ax):
            return True
    return False


def image_in_ax(ax):
    """Return True if there's an image in the Axes object"""
    if len(ax.images) > 0 or any(isinstance(collection, QuadMesh) for collection in ax.collections):
        return True
    return False


def image_in_figure(fig):
    """Return True if there's an image in the Figure object"""
    for ax in fig.get_axes():
        if image_in_ax(ax):
            return True
    return False


def legend_in_figure(fig):
    """Return True if there's a legend in the Figure object"""
    for ax in fig.get_axes():
        if ax.get_legend() and ax.get_legend().get_texts():
            return True
    return False


def curve_in_ax(ax):
    """Return True if there is an ErrobarContainer or Line2D in axes"""
    if line_in_ax(ax) or errorbars_in_ax(ax):
        return True
    return False


def line_in_ax(ax):
    """Return True if there are any lines in the Axes object"""
    return len(ax.get_lines()) > 0


def errorbars_in_ax(ax):
    """
    Return True if there are any ErrorbarContainers in the Axes object
    """
    return any(isinstance(c, ErrorbarContainer) for c in ax.containers)


def get_images_from_fig(fig):
    """
    Return the images in the given Figure.
    This is a list of list so ensure images that are a group of collections remain together
    """
    colorbar_images = get_colorbars_from_fig(fig)
    images = []
    for ax in fig.get_axes():
        ax_imgs = get_images_from_ax(ax)
        imgs = [img for img in ax_imgs if img not in colorbar_images]
        if imgs:
            images.append(imgs)
    return images


def get_colorbars_from_fig(fig):
    """Returns a list of the colorbar axes in the given Figure"""
    colorbar_imgs = []
    for ax in fig.get_axes():
        for img in get_images_from_ax(ax):
            try:
                colorbar_imgs.append(img.colorbar.solids)
            except AttributeError:
                pass
    return colorbar_imgs


def get_images_from_ax(ax):
    """Returns a list of image objects in the given Axes"""
    return ax.images + [col for col in ax.collections if isinstance(col, QuadMesh) or isinstance(col, Poly3DCollection)]
