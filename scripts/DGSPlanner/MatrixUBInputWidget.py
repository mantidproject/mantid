#pylint: disable=invalid-name,no-name-in-module,too-many-public-methods
from __future__ import (absolute_import, division, print_function)
from PyQt4 import QtCore, QtGui
import sys
import mantid
from DGSPlanner.ValidateOL import ValidateUB
from LoadNexusUB import LoadNexusUB
try:
    from PyQt4.QtCore import QString
except ImportError:
    QString = type("")


class UBTableModel(QtCore.QAbstractTableModel):
    changed=QtCore.pyqtSignal(mantid.geometry.OrientedLattice)

    def __init__(self, lattice,  parent = None):
        QtCore.QAbstractTableModel.__init__(self, parent)
        self.__lattice = lattice
        self.__UB=self.__lattice.getUB().copy()
        self.sendSignal()

    def rowCount(self, dummy_parent):
        return 3

    def columnCount(self, dummy_parent):
        return 3

    def flags(self, dummy_index):
        return QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable

    def data(self, index, role):
        if role == QtCore.Qt.EditRole:
            row = index.row()
            column = index.column()
            return QString(format(self.__UB[row][column],'.4f'))
        elif role == QtCore.Qt.DisplayRole:
            row = index.row()
            column = index.column()
            value = QString(format(self.__UB[row][column],'.4f'))
            return value
        elif role == QtCore.Qt.BackgroundRole:
            if ValidateUB(self.__UB):
                return QtGui.QBrush(QtCore.Qt.white)
            else:
                return QtGui.QBrush(QtCore.Qt.red)

    def setData(self, index, value, role = QtCore.Qt.EditRole):
        if role == QtCore.Qt.EditRole:
            row = index.row()
            column = index.column()
            try:
                val=value.toFloat()[0] #QVariant
            except AttributeError:
                val=float(value) #string
            self.__UB[row][column]=val
            self.dataChanged.emit(index, index)
            if ValidateUB(self.__UB):
                self.__lattice.setUB(self.__UB)
                self.sendSignal()
                return True
        return False

    def sendSignal(self):
        self.changed.emit(self.__lattice)

    def updateOL(self,ol):
        self.beginResetModel()
        self.__lattice=ol
        self.__UB=self.__lattice.getUB().copy()
        self.endResetModel()


class MatrixUBInputWidget(QtGui.QWidget):
    # pylint: disable=too-few-public-methods
    def __init__(self,ol,parent=None):
        # pylint: disable=unused-argument,super-on-old-class
        super(MatrixUBInputWidget,self).__init__(parent)
        self.setLayout(QtGui.QVBoxLayout())
        self._tableView = QtGui.QTableView(self)
        self._tableView.horizontalHeader().hide()
        self._tableView.verticalHeader().hide()
        self._tableView.horizontalHeader().setResizeMode(QtGui.QHeaderView.Stretch)
        self._tableView.verticalHeader().setResizeMode(QtGui.QHeaderView.Stretch)
        self.LoadIsawUBButton=QtGui.QPushButton("LoadIsawUB")
        self.LoadNexusUBButton=QtGui.QPushButton("LoadNexusUB")
        self.layout().addWidget(QtGui.QLabel('UB matrix'))
        self.layout().addWidget(self._tableView)
        self.hbox = QtGui.QHBoxLayout()
        self.hbox.addStretch(1)
        self.hbox.addWidget(self.LoadIsawUBButton)
        self.hbox.addWidget(self.LoadNexusUBButton)
        self.layout().addLayout(self.hbox)
        self.ol=ol
        self.UBmodel = UBTableModel(self.ol,self)
        self._tableView.setModel(self.UBmodel)
        self._tableView.update()
        self._tableView.setMinimumSize(self._tableView.sizeHintForColumn(0)*6, self._tableView.sizeHintForRow(0)*4)
        self._tableView.setMaximumSize(self._tableView.sizeHintForColumn(0)*6, self._tableView.sizeHintForRow(0)*4)
        self.LoadIsawUBButton.clicked.connect(self.loadIsawUBDialog)
        self.LoadNexusUBButton.clicked.connect(self.loadNexusUBDialog)
        self.layout().addStretch(1)

    def loadIsawUBDialog(self):
        # pylint: disable=bare-except
        try:
            fname = QtGui.QFileDialog.getOpenFileName(self, 'Open ISAW UB file',filter=QString('Mat file (*.mat);;All Files (*)'))
            __tempws=mantid.simpleapi.CreateSingleValuedWorkspace(0.)
            mantid.simpleapi.LoadIsawUB(__tempws,str(fname))
            ol=mantid.geometry.OrientedLattice(__tempws.sample().getOrientedLattice())
            ol.setU(__tempws.sample().getOrientedLattice().getU())
            self.UBmodel.updateOL(ol)
            self.UBmodel.sendSignal()
            mantid.simpleapi.DeleteWorkspace(__tempws)
        except:
            mantid.logger.error("Could not open the file, or not a valid UB matrix")

    def loadNexusUBDialog(self):
        # pylint: disable=bare-except
        try:
            fname = QtGui.QFileDialog.getOpenFileName(\
                    self, 'Open Nexus file to extract UB matrix',\
                    filter=QString('Nexus file (*.nxs.h5);;All Files (*)'))
            this_here_UB = LoadNexusUB(str(fname))
            UB00 = self.UBmodel.createIndex(0,0)
            UB01 = self.UBmodel.createIndex(0,1)
            UB02 = self.UBmodel.createIndex(0,2)
            UB10 = self.UBmodel.createIndex(1,0)
            UB11 = self.UBmodel.createIndex(1,1)
            UB12 = self.UBmodel.createIndex(1,2)
            UB20 = self.UBmodel.createIndex(2,0)
            UB21 = self.UBmodel.createIndex(2,1)
            UB22 = self.UBmodel.createIndex(2,2)
            self.UBmodel.setData(UB00, this_here_UB[0,0])
            self.UBmodel.setData(UB01, this_here_UB[0,1])
            self.UBmodel.setData(UB02, this_here_UB[0,2])
            self.UBmodel.setData(UB10, this_here_UB[1,0])
            self.UBmodel.setData(UB11, this_here_UB[1,1])
            self.UBmodel.setData(UB12, this_here_UB[1,2])
            self.UBmodel.setData(UB20, this_here_UB[2,0])
            self.UBmodel.setData(UB21, this_here_UB[2,1])
            self.UBmodel.setData(UB22, this_here_UB[2,2])
            self.UBmodel.sendSignal()
        except:
            mantid.logger.error("Could not open the Nexus file, or could not find UB matrix")

if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    inputol=mantid.geometry.OrientedLattice(2,3,4,90,90,90)

    mainForm=MatrixUBInputWidget(inputol)
    mainForm.show()
    sys.exit(app.exec_())
