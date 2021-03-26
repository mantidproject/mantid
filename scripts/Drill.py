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
    logger.error('Drill interface is supported only in workbench.')
else:
    if config['default.facility'] != 'ILL':
        logger.error('Drill is enabled only if the facility is set to ILL.')
    else:
        from mantidqt.gui_helper import get_qapplication
        from Interface.ui.drill.view.DrillView import DrillView

        app, within_mantid = get_qapplication()
        if 'workbench' in sys.modules:
            from workbench.config import get_window_config

            parent, flags = get_window_config()
        else:
            parent, flags = None, None

        if 'drillInterface' not in globals():
            drillInterface = DrillView(parent, flags)
        if 'drillInterface' in globals():
            # 'unresolved reference drillInterface' if we use an else statement.
            # This is necessary to ensure settings for 'On top'/'Floating' window behaviour are propagated to DRILL
            # if they are changed by the user after DRILL has been opened.
            drillInterface.setParent(parent)
            drillInterface.setWindowFlags(flags)
        drillInterface.show()
        if not within_mantid:
            sys.exit(app.exec_())
