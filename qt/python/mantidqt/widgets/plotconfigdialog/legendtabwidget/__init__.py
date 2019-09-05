# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from matplotlib.colors import to_hex
from matplotlib.legend_handler import *
from matplotlib.collections import *

class LegendProperties(dict):
    def __init__(self, props):
        self.update(props)

    def __getattr__(self, item):
        return self[item]

    @classmethod
    def from_legend(cls, legend):
        props = dict()

        title = legend.get_title()
        if isinstance(title.get_text(), unicode):
            props['title'] = title.get_text()
        else:
            props['title'] = None

        props['title_font'] = title.get_fontname()
        props['title_size'] = title.get_fontsize()
        props['title_color'] = to_hex(title.get_color())

        box = legend.get_frame()
        props['background_color'] = to_hex(box.get_facecolor())
        props['edge_color'] = to_hex(box.get_edgecolor())
        props['transparency'] = 100 - (box.get_alpha()*100)

        text = legend.get_texts()[0]
        props['entries_font'] = text.get_fontname()
        props['entries_size'] = text.get_fontsize()
        props['entries_color'] = to_hex(text.get_color())

        return cls(props)

    @classmethod
    def from_view(cls, view):
        props = dict()
        props['title'] = view.get_title()
        props['background_color'] = view.get_background_color()
        props['edge_color'] = view.get_edge_color()
        props['transparency'] = (100-float(view.get_transparency_spin_box_value()))/100
        props['entries_font'] = view.get_entries_font()
        props['entries_size'] = view.get_entries_size()
        props['entries_color'] = view.get_entries_color()
        props['title_font'] = view.get_title_font()
        props['title_size'] = view.get_title_size()
        props['title_color'] = view.get_title_color()
        props['marker_size'] = view.get_marker_size()

        return cls(props)

