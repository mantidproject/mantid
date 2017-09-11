#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from PyQt4.QtGui import *
import sys

from Muon import view_constructor
from Muon import model_constructor
from Muon import transform_view
from Muon import transform_presenter

class frequencyDomainAnalysisGui(QMainWindow):
    def __init__(self,parent=None):
        super(frequencyDomainAnalysisGui,self).__init__(parent)
        groupedViews = view_constructor.viewConstructor(True,self)
        groupedModels = model_constructor.modelConstructor(True)
        view =transform_view.transformView(groupedViews,self)
        self.presenter =transform_presenter.transformPresenter(view,groupedModels)
        self.setCentralWidget(view)
        self.setWindowTitle("Frequency Domain Analysis")


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app

app = qapp()
ex= frequencyDomainAanalysisGui()
ex.resize(700,700)
ex.show()
app.exec_()
