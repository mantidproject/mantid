# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import sys


from mantidqt.gui_helper import get_qapplication
from mantidqtinterfaces.TexturePlanner.presenter import TexturePlannerPresenter
from mantidqtinterfaces.TexturePlanner.model import TexturePlannerModel
from mantidqtinterfaces.TexturePlanner.view import TexturePlannerView

app, within_mantid = get_qapplication()
# The variable name must match the globals() key so workbench can find and reuse the existing
# interface on relaunch (see e.g. DrILL.py); otherwise a fresh window is created every time.
if "TexturePlanner" not in globals():
    TexturePlanner = TexturePlannerPresenter(TexturePlannerModel(), TexturePlannerView())
else:
    TexturePlanner = globals()["TexturePlanner"]
    try:
        # the presenter is not a widget, so visibility is queried on its view
        visible = TexturePlanner.view.isVisible()
    except (RuntimeError, AttributeError):
        # when the window is closed, the python object can linger while the underlying Qt object has
        # been deleted. In this case, we create a new one from scratch
        visible = False
    if not visible:
        TexturePlanner = TexturePlannerPresenter(TexturePlannerModel(), TexturePlannerView())
TexturePlanner.view.show()
if not within_mantid:
    sys.exit(app.exec_())
