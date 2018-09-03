from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets

from Muon.GUI.MuonAnalysis.loadrun.load_run_model import LoadRunWidgetModel
from Muon.GUI.MuonAnalysis.loadrun.load_run_view import LoadRunWidgetView
from Muon.GUI.MuonAnalysis.loadrun.load_run_presenter import LoadRunWidgetPresenter

from Muon.GUI.Common.muon_load_data import MuonLoadData

if __name__ == "__main__":
    import sys

    app = QtWidgets.QApplication(sys.argv)
    ui = LoadRunWidgetPresenter(LoadRunWidgetView(), LoadRunWidgetModel(MuonLoadData()))

    ui.show()
    sys.exit(app.exec_())
