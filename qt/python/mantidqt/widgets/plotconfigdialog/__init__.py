# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from matplotlib.axes import ErrorbarContainer
from matplotlib.collections import QuadMesh


def generate_ax_name(ax):
    """
    Generate a name for the given axes. This will come from the
    title of the axes (if there is one) and the position of the axes
    on the figure.
    """
    title = ax.get_title().split('\n')[0].strip()
    position = "({}, {})".format(ax.rowNum, ax.colNum)
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
        return ValueError("Only one of 'curves_only' and 'images_only' may be "
                          "True.")
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
    if len(ax.images) > 0 or any(isinstance(collection, QuadMesh) for collection in ax.collections):
        return True


def image_in_figure(fig):
    for ax in fig.get_axes():
        if image_in_ax(ax):
            return True


def curve_in_ax(ax):
    """Return True if there is an ErrobarContainer or Line2D in axes"""
    if line_in_ax(ax) or errorbars_in_ax(ax):
        return True
    return False


def line_in_ax(ax):
    return len(ax.get_lines()) > 0


def errorbars_in_ax(ax):
    return any(isinstance(c, ErrorbarContainer) for c in ax.containers)


def get_images_from_fig(fig):
    colorbar_images = get_colorbars_from_fig(fig)
    images = []
    for ax in fig.get_axes():
        ax_imgs = get_images_from_ax(ax)
        images += [img for img in ax_imgs if img not in colorbar_images]
    return images


def get_colorbars_from_fig(fig):
    colorbar_imgs = []
    for ax in fig.get_axes():
        for img in get_images_from_ax(ax):
            try:
                colorbar_imgs.append(img.colorbar.solids)
            except AttributeError:
                pass
    return colorbar_imgs


def get_images_from_ax(ax):
    return ax.images + [col for col in ax.collections if isinstance(col, QuadMesh)]
