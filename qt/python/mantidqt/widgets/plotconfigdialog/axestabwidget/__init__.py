# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from matplotlib.ticker import NullLocator
from mpl_toolkits.mplot3d import Axes3D


class AxProperties(dict):
    """
    An object to store the properties that can be set in the Axes
    Tab. It can be constructed from a view or an Axes object.
    """

    def __init__(self, props):
        self.update(props)

    def __getattr__(self, item):
        return self[item]

    @classmethod
    def from_ax_object(cls, ax):
        props = dict()
        props['title'] = ax.get_title()
        props['xlim'] = ax.get_xlim()
        props['xlabel'] = ax.get_xlabel()
        props['xscale'] = ax.get_xscale().title()
        props['ylim'] = ax.get_ylim()
        props['ylabel'] = ax.get_ylabel()
        props['yscale'] = ax.get_yscale().title()
        props['canvas_color'] = ax.get_facecolor()

        if isinstance(ax, Axes3D):
            props['zlim'] = ax.get_zlim()
            props['zlabel'] = ax.get_zlabel()
            props['zscale'] = ax.get_zscale().title()
        else:
            props['minor_ticks'] = not isinstance(ax.xaxis.minor.locator, NullLocator)
            props['minor_gridlines'] = ax.show_minor_gridlines if hasattr(ax, 'show_minor_gridlines') else False

        return cls(props)

    @classmethod
    def from_view(cls, view):
        props = dict()
        props['title'] = view.get_title()
        props['minor_ticks'] = view.get_show_minor_ticks()
        props['minor_gridlines'] = view.get_show_minor_gridlines()
        props['canvas_color'] = view.get_canvas_color()

        ax = view.get_axis()
        props[f'{ax}lim'] = (view.get_lower_limit(), view.get_upper_limit())
        props[f'{ax}label'] = view.get_label()
        props[f'{ax}scale'] = view.get_scale()

        return cls(props)
