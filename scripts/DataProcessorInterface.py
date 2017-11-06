#pylint: disable=invalid-name
"""
    Script used to start the Test Interface from MantidPlot
"""
from ui.dataprocessorinterface import data_processor_gui

ui = data_processor_gui.DataProcessorGui()
if ui.setup_layout():
    ui.show()
