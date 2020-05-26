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
        from Interface.ui.drill.presenter.DrillPresenter import DrillPresenter
        from Interface.ui.drill.model.DrillModel import DrillModel

        app, within_mantid = get_qapplication()
        if 'drillInterface' not in globals():
            drillInterface = DrillPresenter(DrillModel(), DrillView())
        drillInterface.show()
        if not within_mantid:
            sys.exit(app.exec_())
