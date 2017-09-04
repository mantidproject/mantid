#pylint: disable=invalid-name
"""
    Script used to start the ISIS Reflectomery GUI from MantidPlot
"""
from __future__ import (absolute_import, division, print_function)
from ui.reflectometer import refl_gui

ui = refl_gui.ReflGui()
if ui.setup_layout():
    ui.show()
