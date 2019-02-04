# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
"""
    Script used to start the Test Interface from MantidPlot
"""
from ui.batchwidget import batch_widget_gui

ui = batch_widget_gui.DataProcessorGui()
if ui.setup_layout():
    ui.show()
