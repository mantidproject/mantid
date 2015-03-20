#pylint: disable=invalid-name
import InstrumentSetupWidget,ClassicUBInputWidget,MatrixUBInputWidget,DimensionSelectorWidget
from PyQt4 import QtCore, QtGui
import sys
import mantid
from ValidateOL import ValidateOL

class DGSPlannerGUI(QtGui.QWidget):
    def __init__(self,ol=None,parent=None):
        # pylint: disable=unused-argument
        super(DGSPlannerGUI,self).__init__(parent)
        #OrientedLattice
        if ValidateOL(ol):
            self.ol=ol
        else:
            self.ol=mantid.geometry.OrientedLattice()
        self.instrumentWidget=InstrumentSetupWidget.InstrumentSetupWidget(self)
        self.setLayout(QtGui.QVBoxLayout())
        self.layout().addWidget(self.instrumentWidget)
        self.ublayout=QtGui.QHBoxLayout()
        self.classic=ClassicUBInputWidget.ClassicUBInputWidget(self.ol)
        self.ublayout.addWidget(self.classic)
        self.classic.changed.connect(self.printUB)
        self.matrix=MatrixUBInputWidget.MatrixUBInputWidget(self.ol)
        self.ublayout.addStretch(1)
        self.ublayout.addWidget(self.matrix)
        self.matrix.UBmodel.changed.connect(self.printUB)
        self.matrix.UBmodel.changed.connect(self.classic.updateOL)
        self.classic.changed.connect(self.matrix.UBmodel.updateOL)
        self.layout().addLayout(self.ublayout)
        self.dimensionWidget=DimensionSelectorWidget.DimensionSelectorWidget(self)
        self.layout().addWidget(self.dimensionWidget)

    @QtCore.pyqtSlot(mantid.geometry.OrientedLattice)
    def printUB(self,ol):
        print ol.getUB()

if __name__=='__main__':
    app=QtGui.QApplication(sys.argv)
    orl=mantid.geometry.OrientedLattice(2,3,4,90,90,90)
    mainForm=DGSPlannerGUI(orl)
    mainForm.show()
    sys.exit(app.exec_())
