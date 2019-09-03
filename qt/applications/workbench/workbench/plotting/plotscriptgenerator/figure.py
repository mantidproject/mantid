# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from matplotlib import rcParams
from numpy import isclose

from mantidqt.widgets.plotconfigdialog.colorselector import convert_color_to_hex
from workbench.plotting.plotscriptgenerator.utils import convert_args_to_string

BASE_SUBPLOTS_COMMAND = "plt.subplots({})"

default_kwargs = {
    'dpi': rcParams['figure.dpi'],
    'edgecolor': convert_color_to_hex(rcParams['figure.edgecolor']),
    'facecolor': convert_color_to_hex(rcParams['figure.facecolor']),
    'figsize': rcParams['figure.figsize'],
    'frameon': rcParams['figure.frameon'],
    'ncols': 1,
    'nrows': 1,
    'num': ''
}


def get_subplots_command_kwargs(fig):
    ax = fig.get_axes()[0]
    kwargs = {
        'dpi': fig.dpi,
        'edgecolor': convert_color_to_hex(fig.get_edgecolor()),
        'facecolor': convert_color_to_hex(fig.get_facecolor()),
        'figsize': [fig.get_figwidth(), fig.get_figheight()],
        'frameon': fig.frameon,
        'ncols': ax.numCols,
        'nrows': ax.numRows,
        'num': fig.get_label(),
        'subplot_kw': {
            'projection': 'mantid'
        },
    }
    return kwargs


def generate_subplots_command(fig):
    kwargs = get_subplots_command_kwargs(fig)
    kwargs = _remove_kwargs_if_default(kwargs)
    return BASE_SUBPLOTS_COMMAND.format(convert_args_to_string(None, kwargs))


def _remove_kwargs_if_default(kwargs):
    for kwarg, default_value in default_kwargs.items():
        try:
            if kwarg == 'figsize' and isclose(kwargs[kwarg], default_value, rtol=0.05).all():
                kwargs.pop(kwarg)
            elif kwargs[kwarg] == default_value:
                kwargs.pop(kwarg)
        except KeyError:
            pass
    return kwargs
