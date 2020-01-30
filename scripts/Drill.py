# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from qtpy import PYQT5
if not PYQT5:
    raise RuntimeError('Drill is operational only in workbench.')
from mantid.simpleapi import config
if config['default.facility'] != 'ILL':
    raise RuntimeError('Set the facility to ILL to launch this interface.')
from mantidqt.gui_helper import get_qapplication
from Interface.ui.drill.main import (model, view, presenter)
app, within_mantid = get_qapplication()
window = view.DrillView()
model = model.DrillModel()
presenter = presenter.DrillPresenter(model, view)
window.show()
if not within_mantid:
    sys.exit(app.exec_())
