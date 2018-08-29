from __future__ import (absolute_import, division, print_function)

# import sys
# from PyQt5 import QtCore, QtGui, QtWidgets
# from Muon.GUI.MuonAnalysis.LoadWidget.LoadWidget import LoadWidgetView
# from Muon.GUI.MuonAnalysis.LoadWidget.LoadWidgetModel import LoadWidgetModel
# from Muon.GUI.MuonAnalysis.LoadWidget.LoadWidgetPresenter import LoadWidgetPresenter
#
# if __name__ == "__main__":
#
#     app = QtWidgets.QApplication(sys.argv)
#     ui = LoadWidgetPresenter(LoadWidgetView(),LoadWidgetModel())
#     ui.view.show()
#     sys.exit(app.exec_())

from qtpy import QtGui, QtCore, QtWidgets
from mantid.simpleapi import *
from Muon.GUI.MuonAnalysis.loadfile.load_file_model import BrowseFileWidgetModel
from Muon.GUI.MuonAnalysis.loadfile.load_file_view import BrowseFileWidgetView
from Muon.GUI.MuonAnalysis.loadfile.load_file_presenter import BrowseFileWidgetPresenter

# from Muon.GUI.MuonAnalysis.loadrun.load_run_model import LoadRunWidgetModel
# from Muon.GUI.MuonAnalysis.loadrun.load_run_view import LoadRunWidgetView
# from Muon.GUI.MuonAnalysis.loadrun.load_run_presenter import LoadRunWidgetPresenter
#
# from Muon.GUI.MuonAnalysis.loadwidget.load_widget_model import LoadWidgetModel
# from Muon.GUI.MuonAnalysis.loadwidget.load_widget_view import LoadWidgetView
# from Muon.GUI.MuonAnalysis.loadwidget.load_widget_presenter import LoadWidgetPresenter

from Muon.GUI.Common.muon_load_data import MuonLoadData


if __name__ == "__main__":
    import sys

    app = QtWidgets.QApplication(sys.argv)
    ui = BrowseFileWidgetPresenter(BrowseFileWidgetView(), BrowseFileWidgetModel(MuonLoadData()))
    # ui = LoadRunWidgetPresenter(LoadRunWidgetView(), LoadRunWidgetModel(MuonLoadData()))
    #
    # data = MuonLoadData()
    # load_file_view = BrowseFileWidgetView()
    # load_run_view = LoadRunWidgetView()
    #
    # ui = LoadWidgetPresenter(LoadWidgetView(load_file_view=load_file_view, load_run_view=load_run_view), LoadWidgetModel(data))
    # ui.set_load_file_widget(BrowseFileWidgetPresenter(load_file_view, BrowseFileWidgetModel(data)))
    # ui.set_load_run_widget(LoadRunWidgetPresenter(load_run_view, LoadRunWidgetModel(data)))

    ui.show()
    sys.exit(app.exec_())


