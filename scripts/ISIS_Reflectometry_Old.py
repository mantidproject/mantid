# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
"""
    Script used to start the ISIS Reflectomery GUI from MantidPlot
"""
from __future__ import (absolute_import, division, print_function)
from ui.reflectometer import refl_gui

ui = refl_gui.ReflGui()
if ui.setup_layout():
    ui.show()
