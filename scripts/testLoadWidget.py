from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets

from Muon.GUI.Common.load_run_widget.model import LoadRunWidgetModel
from Muon.GUI.Common.load_run_widget.view import LoadRunWidgetView
from Muon.GUI.Common.load_run_widget.presenter import LoadRunWidgetPresenter

from Muon.GUI.Common.muon_load_data import MuonLoadData

if __name__ == "__main__":
    import sys

    app = QtWidgets.QApplication(sys.argv)
    ui = LoadRunWidgetPresenter(LoadRunWidgetView(), LoadRunWidgetModel(MuonLoadData()))

    ui.show()
    sys.exit(app.exec_())
