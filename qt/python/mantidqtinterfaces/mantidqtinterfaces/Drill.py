# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import sys

from qtpy import PYQT5

from mantid.kernel import config, logger


if not PYQT5:
    logger.error("Drill interface is supported only in workbench.")
else:
    if config["default.facility"] != "ILL":
        logger.error("Drill is enabled only if the facility is set to ILL.")
    else:
        from mantidqt.gui_helper import get_qapplication
        from mantidqtinterfaces.drill.view.DrillView import DrillView

        app, within_mantid = get_qapplication()
        if "drillInterface" not in globals():
            drillInterface = DrillView()
        else:
            drillInterface = globals()["drillInterface"]
            try:
                visible = drillInterface.isVisible()
            except:
                # underlying Qt object has been deleted
                visible = False
            if not visible:
                drillInterface = DrillView()
        drillInterface.show()
        if not within_mantid:
            sys.exit(app.exec_())
