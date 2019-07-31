# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)


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
        return cls(props)

    @classmethod
    def from_view(cls, view):
        props = dict()
        props['title'] = view.get_title()
        props['xlim'] = (view.get_xlower_limit(), view.get_xupper_limit())
        props['xlabel'] = view.get_xlabel()
        props['xscale'] = view.get_xscale()
        props['ylim'] = (view.get_ylower_limit(), view.get_yupper_limit())
        props['ylabel'] = view.get_ylabel()
        props['yscale'] = view.get_yscale()
        return cls(props)
