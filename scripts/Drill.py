# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from qtpy import PYQT5
from mantid.kernel import logger
if not PYQT5:
    logger.error('Drill interface is supported only in workbench.')
else:
    from mantid.simpleapi import config
    if config['default.facility'] != 'ILL':
        logger.error('Drill is enabled only if the facility is set to ILL.')
    else:
        from mantidqt.gui_helper import get_qapplication
        from Interface.ui.drill.main import (model, view, presenter)
        app, within_mantid = get_qapplication()
        window = view.DrillView()
        model = model.DrillModel()
        presenter = presenter.DrillPresenter(model, view)
        window.show()
        if not within_mantid:
            sys.exit(app.exec_())
