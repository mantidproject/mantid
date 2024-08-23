# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from matplotlib.axes import Axes
from matplotlib.collections import QuadMesh
from matplotlib.colors import LogNorm
from mpl_toolkits.mplot3d.art3d import Poly3DCollection


class ImageProperties(dict):
    def __init__(self, props):
        self.update(props)

    def __getattr__(self, item):
        return self[item]

    @classmethod
    def from_image(cls, image):
        if isinstance(image, list):
            image = image[0]
        props = dict()

        props["label"] = ""
        for ax in image.figure.axes:
            if type(ax) is Axes:
                props["label"] = ax.yaxis.label.get_text()

        cmap_name = image.cmap.name if hasattr(image, "cmap") else image.get_cmap().name
        props["colormap"] = cmap_name
        props["reverse_colormap"] = False
        if props["colormap"].endswith("_r"):
            props["colormap"] = props["colormap"][:-2]
            props["reverse_colormap"] = True
        props["vmin"], props["vmax"] = image.get_clim()

        if isinstance(image, QuadMesh) or isinstance(image, Poly3DCollection):
            props["interpolation"] = None
        else:
            props["interpolation"] = image.get_interpolation()
        if type(image.norm) is LogNorm:
            props["scale"] = "Logarithmic"
        else:
            props["scale"] = "Linear"
        return cls(props)

    @classmethod
    def from_view(cls, view):
        props = dict()
        props["label"] = view.get_label()
        props["colormap"] = view.get_colormap()
        props["reverse_colormap"] = view.get_reverse_colormap()
        if props["reverse_colormap"]:
            props["colormap"] += "_r"
        props["vmin"] = view.get_min_value()
        props["vmax"] = view.get_max_value()
        if view.interpolation_enabled():
            props["interpolation"] = view.get_interpolation().lower()
        else:
            props["interpolation"] = None
        props["scale"] = view.get_scale()
        return cls(props)
