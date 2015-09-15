#pylint: disable=invalid-name
"""
    Script used to start the ISIS Reflectomery GUI from MantidPlot
"""
from ui.reflectometer import refl_gui

ui = refl_gui.ReflGui()
if ui.setup_layout():
    ui.show()
