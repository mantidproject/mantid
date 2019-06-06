# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from matplotlib.axes import ErrorbarContainer
from qtpy.QtCore import Qt

from mantidqt.widgets.plotconfigdialog.curvestabwidget.errorbarstabwidget import ErrorbarProperties, errorbars_hidden
from mantidqt.widgets.plotconfigdialog.curvestabwidget.linetabwidget import LineProperties
from mantidqt.widgets.plotconfigdialog.curvestabwidget.markertabwidget import MarkerProperties


def curve_hidden(curve):
    if isinstance(curve, ErrorbarContainer):
        return errorbars_hidden(curve)
    else:
        return not curve.get_visible()


def set_attrs_from_dict(obj, attr_dict):
    for prop, value in attr_dict.items():
        setattr(obj, prop, value)


class CurveProperties:

    def __init__(self, props):
        set_attrs_from_dict(self, props)

    @classmethod
    def from_view(cls, view):
        props = dict()
        props['label'] = view.get_curve_label()
        props['hide_curve'] = (view.get_hide_curve() == Qt.Checked)
        props['line'] = LineProperties.from_view(view)
        props['marker'] = MarkerProperties.from_view(view)
        props['errorbars'] = ErrorbarProperties.from_view(view)
        return cls(props)

    @classmethod
    def from_curve(cls, curve):
        props = dict()
        props['label'] = curve.get_label()
        props['hide_curve'] = curve_hidden(curve)
        if isinstance(curve, ErrorbarContainer):
            props['errorbars'] = ErrorbarProperties.from_container(curve)
            curve = curve[0]
        else:
            props['errorbars'] = None
        props['line'] = LineProperties.from_line(curve)
        props['marker'] = MarkerProperties.from_line(curve)
        return cls(props)
