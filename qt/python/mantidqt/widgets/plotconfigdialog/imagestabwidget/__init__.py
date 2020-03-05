# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from matplotlib.collections import QuadMesh
from matplotlib.colors import LogNorm


class ImageProperties(dict):

    def __init__(self, props):
        self.update(props)

    def __getattr__(self, item):
        return self[item]

    @classmethod
    def from_image(cls, image):
        props = dict()
        props['label'] = image.get_label()
        cmap_name = image.cmap.name if hasattr(image,"cmap") else image.get_cmap().name
        props['colormap'] = cmap_name
        props['reverse_colormap'] = False
        if props['colormap'].endswith('_r'):
            props['colormap'] = props['colormap'][:-2]
            props['reverse_colormap'] = True
        props['vmin'], props['vmax'] = image.get_clim()

        if isinstance(image, QuadMesh):
            props['interpolation'] = None
        else:
            props['interpolation'] = image.get_interpolation()
        if type(image.norm) is LogNorm:
            props['scale'] = 'Logarithmic'
        else:
            props['scale'] = 'Linear'
        return cls(props)

    @classmethod
    def from_view(cls, view):
        props = dict()
        props['label'] = view.get_label()
        props['colormap'] = view.get_colormap()
        props['reverse_colormap'] = view.get_reverse_colormap()
        if props['reverse_colormap']:
            props['colormap'] += '_r'
        props['vmin'] = view.get_min_value()
        props['vmax'] = view.get_max_value()
        if view.interpolation_enabled():
            props['interpolation'] = view.get_interpolation().lower()
        else:
            props['interpolation'] = None
        props['scale'] = view.get_scale()
        return cls(props)
