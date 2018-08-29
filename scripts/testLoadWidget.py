from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets
from Muon.GUI.MuonAnalysis.loadfile.load_file_model import BrowseFileWidgetModel
from Muon.GUI.MuonAnalysis.loadfile.load_file_view import BrowseFileWidgetView
from Muon.GUI.MuonAnalysis.loadfile.load_file_presenter import BrowseFileWidgetPresenter

from Muon.GUI.Common.muon_load_data import MuonLoadData

if __name__ == "__main__":
    import sys

    app = QtWidgets.QApplication(sys.argv)
    ui = BrowseFileWidgetPresenter(BrowseFileWidgetView(), BrowseFileWidgetModel(MuonLoadData()))

    ui.show()
    sys.exit(app.exec_())
