# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import sys


from mantidqt.gui_helper import get_qapplication
from mantidqtinterfaces.TexturePlanner.presenter import TexturePlannerPresenter
from mantidqtinterfaces.TexturePlanner.model import TexturePlannerModel
from mantidqtinterfaces.TexturePlanner.view import TexturePlannerView

app, within_mantid = get_qapplication()
if "TexturePlanner" not in globals():
    model = TexturePlannerModel()
    view = TexturePlannerView()
    presenter = TexturePlannerPresenter(model, view)
else:
    presenter = globals()["TexturePlanner"]
    try:
        visible = presenter.isVisible()
    except RuntimeError:
        # when a scan explorer is closed, the python object can linger while the underlying Qt object has been
        # deleted. In this case, we create a new one from scratch
        visible = False
    if not visible:
        model = TexturePlannerModel()
        view = TexturePlannerView()
        presenter = TexturePlannerPresenter(model, view)
presenter.view.show()
if not within_mantid:
    sys.exit(app.exec_())
