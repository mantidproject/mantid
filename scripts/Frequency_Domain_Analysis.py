#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
import PyQt4.QtGui as QtGui
import sys

#from muon import FFT_view
#from muon import FFT_presenter
from Muon import view_constructor
from Muon import model_constructor
from Muon import transform_view
from Muon import transform_presenter

class frequencyDomainAnalysisGui(QtGui.QMainWindow):
    def __init__(self,parent=None):
        super(frequencyDomainAnalysisGui,self).__init__(parent)

        #      view =FFT_view.FFTView(self)
        #        self.presenter =FFT_presenter.FFTPresenter(view) #the main ui class in this file is called MainWindow
        #        self.setCentralWidget(view)
        #        self.setWindowTitle("Frequency Domain Analysis")
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
ex= frequencyDomainAanlysisGui()
ex.resize(700,700)
ex.show()
app.exec_()
