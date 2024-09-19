# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
"""
Script used to start the Test Interface from MantidPlot
"""

import sys
from qtpy import QtCore
from SANS.sans.common.enums import SANSFacility
from mantidqtinterfaces.sans_isis.gui_logic.models.run_tab_model import RunTabModel
from mantidqtinterfaces.sans_isis.gui_logic.presenter.run_tab_presenter import RunTabPresenter
from mantidqtinterfaces.sans_isis.views import sans_data_processor_gui

if "workbench" in sys.modules:
    from workbench.config import get_window_config

    parent, flags = get_window_config()
else:
    parent, flags = None, None

if "isis_sans_reduction_view" in globals():
    isis_sans_reduction_view = globals()["isis_sans_reduction_view"]
    if not isis_sans_reduction_view.isHidden():
        isis_sans_reduction_view.setWindowState(
            isis_sans_reduction_view.windowState() & ~QtCore.Qt.WindowMinimized | QtCore.Qt.WindowActive
        )
        isis_sans_reduction_view.activateWindow()
    else:
        isis_sans_reduction_view = sans_data_processor_gui.SANSDataProcessorGui(parent, flags)
        run_tab_presenter = RunTabPresenter(SANSFacility.ISIS, run_tab_model=RunTabModel(), view=isis_sans_reduction_view)
        isis_sans_reduction_view.show()
else:
    isis_sans_reduction_view = sans_data_processor_gui.SANSDataProcessorGui(parent, flags)
    run_tab_presenter = RunTabPresenter(SANSFacility.ISIS, run_tab_model=RunTabModel(), view=isis_sans_reduction_view)
    isis_sans_reduction_view.show()
