from PyQt4 import QtCore, QtGui
import sys
import mantid
from ValidateOL import ValidateUB
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
        
    def rowCount(self, parent):
        return 3
        
    def columnCount(self, parent):
        return 3

    def flags(self, index):
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
            except:
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
        self.ol=ol
        self.beginResetModel()
        self.__lattice=self.ol
        self.__UB=self.ol.getUB().copy()
        self.endResetModel()        
        
class MatrixUBInputWidget(QtGui.QWidget):
    def __init__(self,ol,parent=None):
        super(MatrixUBInputWidget,self).__init__(parent)
        self.setLayout(QtGui.QVBoxLayout())
        self._tableView = QtGui.QTableView(self)
        self._tableView.horizontalHeader().hide()
        self._tableView.verticalHeader().hide()
        self._tableView.horizontalHeader().setResizeMode(QtGui.QHeaderView.Stretch)
        self._tableView.verticalHeader().setResizeMode(QtGui.QHeaderView.Stretch)
        self.LoadIsawUBButton=QtGui.QPushButton("LoadIsawUB")
        self.layout().addWidget(QtGui.QLabel('UB matrix'))
        self.layout().addWidget(self._tableView)
        self.layout().addWidget(self.LoadIsawUBButton)
        self.ol=ol
        self.UBmodel = UBTableModel(self.ol,self)
        self._tableView.setModel(self.UBmodel)
        self._tableView.update()
        
    def updateOL(self,ol):
        print "received"
        self.ol=ol
        self.UBmodel.beginResetModel()
        self.UBmodel.__lattice=self.ol
        self.UBmodel.__UB=self.ol.getUB().copy()
        self.UBmodel.endResetModel()
        self._tableView.update()
        
if __name__ == '__main__':
    
    app = QtGui.QApplication(sys.argv)        
    ol=mantid.geometry.OrientedLattice(2,3,4,90,90,90)

    mainForm=MatrixUBInputWidget(ol)
    mainForm.show()  
    sys.exit(app.exec_())     
    
    
    
    
