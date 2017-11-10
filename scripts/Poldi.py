#pylint: disable=invalid-name
"""
    Script used to start the Test Interface from MantidPlot
"""
from ui.poldi import poldi_gui

ui = poldi_gui.PoldiGui()
if ui.setup_layout():
    ui.show()
