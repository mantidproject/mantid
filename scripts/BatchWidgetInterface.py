#pylint: disable=invalid-name
"""
    Script used to start the Test Interface from MantidPlot
"""
from ui.batchwidget import batch_widget_gui

ui = batch_widget_gui.DataProcessorGui()
if ui.setup_layout():
    ui.show()
