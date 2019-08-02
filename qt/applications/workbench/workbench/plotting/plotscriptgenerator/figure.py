# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from workbench.plotting.plotscriptgenerator.utils import convert_args_to_string

BASE_CREATE_FIG_COMMAND = "plt.figure({})"


def get_figure_command_kwargs(fig):
    kwargs = {
        'figsize': (fig.get_figwidth(), fig.get_figheight()),
        'dpi': fig.dpi
    }
    return kwargs


def generate_figure_command(fig):
    """Generate command to create figure"""
    kwargs = get_figure_command_kwargs(fig)
    return BASE_CREATE_FIG_COMMAND.format(convert_args_to_string(None, kwargs))
