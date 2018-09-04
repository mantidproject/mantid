from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui

from Muon.GUI.MuonAnalysis.loadfile.load_file_model import BrowseFileWidgetModel
from Muon.GUI.MuonAnalysis.loadfile.load_file_view import BrowseFileWidgetView
from Muon.GUI.MuonAnalysis.loadfile.load_file_presenter import BrowseFileWidgetPresenter

from Muon.GUI.MuonAnalysis.loadrun.load_run_model import LoadRunWidgetModel
from Muon.GUI.MuonAnalysis.loadrun.load_run_view import LoadRunWidgetView
from Muon.GUI.MuonAnalysis.loadrun.load_run_presenter import LoadRunWidgetPresenter

from Muon.GUI.MuonAnalysis.loadwidget.load_widget_model import LoadWidgetModel
from Muon.GUI.MuonAnalysis.loadwidget.load_widget_view import LoadWidgetView
from Muon.GUI.MuonAnalysis.loadwidget.load_widget_presenter import LoadWidgetPresenter

from Muon.GUI.Common.muon_load_data import MuonLoadData


if __name__ == "__main__":
    import sys

    app = QtGui.QApplication(sys.argv)
    # ui = BrowseFileWidgetPresenter(BrowseFileWidgetView(), BrowseFileWidgetModel(MuonLoadData()))
    # ui = LoadRunWidgetPresenter(LoadRunWidgetView(), LoadRunWidgetModel(MuonLoadData()))

    data = MuonLoadData()
    load_file_view = BrowseFileWidgetView()
    load_run_view = LoadRunWidgetView()

    ui = LoadWidgetPresenter(LoadWidgetView(load_file_view=load_file_view, load_run_view=load_run_view),
                             LoadWidgetModel(data))
    ui.set_load_file_widget(BrowseFileWidgetPresenter(load_file_view, BrowseFileWidgetModel(data)))
    ui.set_load_run_widget(LoadRunWidgetPresenter(load_run_view, LoadRunWidgetModel(data)))

    ui.show()
    sys.exit(app.exec_())


