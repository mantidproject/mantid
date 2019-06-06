# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from matplotlib.axes import ErrorbarContainer
from qtpy.QtCore import Qt

from mantidqt.widgets.plotconfigdialog.curvestabwidget.errorbarstabwidget import ErrorbarsProperties, errorbars_hidden
from mantidqt.widgets.plotconfigdialog.curvestabwidget.linetabwidget import LineProperties
from mantidqt.widgets.plotconfigdialog.curvestabwidget.markertabwidget import MarkerProperties


def curve_hidden(curve):
    if isinstance(curve, ErrorbarContainer):
        return errorbars_hidden(curve)
    else:
        return not curve.get_visible()


def hide_curve(curve, hide, hide_bars=False):
    if isinstance(curve, ErrorbarContainer):
        if curve[0]:
            curve[0].set_visible(not hide)
        if hide_bars:
            hide_errorbars(curve, hide)
    else:
        curve.set_visible(not hide)


def hide_errorbars(container, hide):
    if container[1]:
        for caps in container[1]:
            caps.set_visible(not hide)
    if container[2]:
        for bars in container[2]:
            bars.set_visible(not hide)


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
        props['hide'] = (view.get_hide_curve() == Qt.Checked)
        return cls(props)

    @classmethod
    def from_curve(cls, curve):
        props = dict()
        props['label'] = curve.get_label()
        props['hide'] = curve_hidden(curve)
        return cls(props)
