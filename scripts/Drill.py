# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantidqt.gui_helper import get_qapplication
from Interface.ui.drill.main import (model, view, presenter)
from qtpy import PYQT5
if not PYQT5:
    raise RuntimeError('Drill is operational only on workbench.')
from mantid.simpleapi import config
if config['default.facility'] != 'ILL':
    raise RuntimeError('Set the facility to ILL to launch this interface.')
app, within_mantid = get_qapplication()
instrument = config['default.instrument']
window = view.DrillView(instrument)
model = model.DrillModel(instrument)
presenter = presenter.DrillPresenter(model, view)
window.show()
if not within_mantid:
    sys.exit(app.exec_())
