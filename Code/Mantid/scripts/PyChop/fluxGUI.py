import sys
from PyChop_LET_UI import Ui_MainWindow #import line for the UI python class
from PyQt4 import QtCore, QtGui #import others if needed
import math

from mantidplot import *
from mantid import *
import numpy as np
from scipy import interpolate
import array as  array




class MainWindow(QtGui.QMainWindow):

    def __init__(self, parent=None):
        QtGui.QMainWindow.__init__(self,parent)
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        QtCore.QObject.connect( self.ui.calcButton, QtCore.SIGNAL("clicked()"), self.calculate )
        QtCore.QObject.connect( self.ui.Plot, QtCore.SIGNAL("clicked()"), self.plotData )
        QtCore.QObject.connect( self.ui.OverPlot, QtCore.SIGNAL("clicked()"), self.addPlot)
        QtCore.QObject.connect( self.ui.ClearFlux, QtCore.SIGNAL("clicked()"), self.clearFluxAndRes)

        QtCore.QObject.connect(self.ui.actionLET,QtCore.SIGNAL("triggered()"), self.letSelected)
        QtCore.QObject.connect(self.ui.actionMARI,QtCore.SIGNAL("triggered()"), lambda  : self.otherInstrumentSelected('MAR'))
        QtCore.QObject.connect(self.ui.actionMERLIN,QtCore.SIGNAL("triggered()"),lambda : self.otherInstrumentSelected('MER'))
        QtCore.QObject.connect(self.ui.actionMAPS,QtCore.SIGNAL("triggered()"), lambda :  self.otherInstrumentSelected('MAP'))


        self.graph=None

        self.loadData()

        tab_name = "Fllux And Resolution Table"
        self.t= newTable(tab_name, 28, 3) #table in which are insered each time the value for the plot
        #self.t.setColName(1, "Frequency")
        #self.t.setColName(2, "Flux")
        #self.t.setColName(3, "Resolution")
        for j in xrange(0,len(self.frequencies)):
            self.t.setCell(1, j+1, self.frequencies[j])

    def loadData(self):
        """ Loads and preprocess all data used for interpolation
        """

        self.flux_matrix = np.multiply(self.loadMatrix("PyChop/small_matrix(flux).txt"),0.021)
                            # *0.021 -- this gives n/cm^2/s. 0.021= (40/60)*0.5*(1/16)
                            #40/60 is the fact that simulation is for 60uamps and not 40uamps beam power.
                            #(1/16) accounts for neutron on a 4x4cm vanadium sample i.e. 16cm^2.
                            #0.5 accounts for the factor of 2 difference between simulation and experimental
        self.flux_energies = array.array( 'f', [0.1, 0.2, 0.5, 0.8, 1.0, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 13.0, 13.1, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0, 25.0, 26.0, 27.0, 28.0, 29.0, 30.0, 31.0, 32.0, 35.0, 38.0, 40.0] )

        self.res_matrix  = self.loadMatrix("PyChop/small_matrix(res).txt")
        self.res_energies = array.array( 'f', [0.2, 0.4, 0.6, 0.8, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40] )
        self.frequencies = np.arange(30,310,10)

        self.ei=""
        self.ei_min = 0.2 # max(min(self.flux_energies),min(self.res_energies))
        self.ei_max = 40. #min(max(self.flux_energies),max(self.res_energies))

    def loadMatrix(self,fileName):
        """ Opens specified ASCII file in proper format and loads it into a matrix.
            The interpolation is based on these data

            return the Numpy matrix read from the fileName provided
        """

        thisDir=  config.getString('mantidqt.python_interfaces_directory')
        input_file = open(thisDir + fileName, 'r')
        input_lines = []
        data_matrix = []

        #fix the data formata
        def convert_mantid_string(mantid_str):
            mantid_str = mantid_str.replace('.', '')
            mantid_str = mantid_str.replace(',', '.')
            return mantid_str

        for line in iter(input_file.readline, ''):
            input_lines.append(convert_mantid_string(line))

        for line in input_lines:
            matrix_row = []
            line_elems = line.split()
            for n in line_elems:
                matrix_row.append(float(n))
            data_matrix.append(matrix_row)

        return data_matrix #python matrix.

    def interpolate_data(self,data_matrix,energies):
        """ interpolation based on the small matrices matrix_data
        """
        #create the correct form for the array z values
        #the values of energy are related to the values on which the interpolation has been made.
        r = len(data_matrix)
        c = len(self.frequencies)
        data_matrix = np.reshape(data_matrix, (r, c))
        val_flux = np.transpose(data_matrix)
        self.ei = self.ui.incidentEnergyValue.text()
        self.ei= float(self.ei)
        ei = [self.ei]
        #linear interpolation
        self.interpolation = interpolate.RectBivariateSpline(self.frequencies, energies, val_flux,kx=1, ky=1) #spline interpolation over the values of energy given.
        result = self.interpolation(self.frequencies, ei)#returns the value for the specific energy required ei.
        return result

    def letSelected(self):
        QtGui.QMessageBox.warning(self, "Currently You have to switch gui to select another instrument")
        self.ui.actionLET.setChecked(True)
    def otherInstrumentSelected(self,INAME):
        reply = QtGui.QMessageBox.question(self, 'Selecting : '+INAME,
        "Do you want to switch GUI?", QtGui.QMessageBox.Yes, QtGui.QMessageBox.No)
        if reply == QtGui.QMessageBox.Yes:
            config['default.instrument'] = INAME
            self.deleteLater()
        else:
            pass
        #QtGui.QMessageBox.setText(self,'Currently You have to switch GUI to have other instrument selected')

    def calculate(self):
        """returns the results of interpolation and prints them in the window.
        """

        self.ei = self.ui.incidentEnergyValue.text()

        if self.ei == "":
            QtGui.QMessageBox.warning(self, "LETFlux", "No Ei entered. Please,insert energy value in the range from "+str(self.ei_min)+" to "+str(self.ei_max)+" meV \n")
            return self
        else:
            self.ei = float (self.ei)

        if self.ei< self.ei_min or self.ei > self.ei_max:
            self.ei=""
            QtGui.QMessageBox.warning(self, "LETFlux", "Energy out of range.\n Please, Provide value between "+str(self.ei_min)+" and "+str(self.ei_max)+" meV \n")
        else:
            self.setUpTable()
            #self.ui.list.clear()
            string = 'Energy selected:' +'  '+ str(self.ei)
            self.ui.list.insertItem(0,string)
            string_title = 'Frequency(Hz)    Flux(n/s/cm^2)           Resolution[ueV]'
            self.ui.list.insertItem(1,string_title)
            t=self.t
            for i in xrange(0, len(self.frequencies)):
                string1 = str(self.frequencies[i])+'                    '+ "%e" % t.cell(2,i+1)+'              '+ "%e"% t.cell(3,i+1)
                self.ui.list.insertItem(2+i,string1)
        return self

    def clearFluxAndRes(self):
        """ Clear flux and resolution text printed in flux and resolution window
        """
        self.ui.list.clear()

    def setUpTable(self):
        flux =  self.interpolate_data(self.flux_matrix,self.flux_energies)
        res  =  self.interpolate_data(self.res_matrix,self.res_energies)

        for i in range(0, len(flux)):
            self.t.setCell(2, i+1, flux[i])
            self.t.setCell(3, i+1, res[i])
        return self


    def plotData(self):
        """ Plot flux and resoulution in the new window
        """
        if self.ei=="" :
            return self

        self.graph = newGraph("Flux and Resolution",2,1,2)

        l1 = self.graph.layer(1)
        l1.setAntialiasing()
        l1.setTitle("Flux")
        l1.setAxisTitle(Layer.Bottom, "Frequency [Hz]")
        l1.setAxisTitle(Layer.Left, "Flux [n/s/cm^2]")
        l1.showGrid()
#       self.graphFlux.insertCurve(self.t, "Flux/Frequency_2", Layer.Scatter)
#       legend = self.graphFlux.newLegend(str(self.ei))
        l2 = self.graph.layer(2)
        l2.setAntialiasing()
        l2.setTitle("Resolution")
        l2.setAxisTitle(Layer.Bottom, "Frequency [Hz]")
        l2.setAxisTitle(Layer.Left, "Resolution [ueV]")
#       legend = self.graphRes.newLegend(str(self.ei))
#       self.graphRes.insertCurve(self.t, "Resolution/Frequency_2", Layer.Scatter)
        l2.showGrid()

        self.addPlot()

        self.raise_()
        self.show()

        return self

    def addPlot(self):
        """ Adds plot to an existing graph
        """
        if self.graph==None:
            self.plotData()
        else:
            self.calculate()
            if self.ei == "":
                return
            try:
                l1 = self.graph.layer(1) # <- this is activeLayer()
            except AttributeError:
                QtGui.QMessageBox.warning(self, "LETFlux", "Plot has been deleted, Can not overplot. Plot graph first\n")
                return
            l1.addCurves(self.t, (1,2), Layer.Line, 1, 4)
#           l1.newLegend(str(self.ei))
            l2 = self.graph.layer(2) # <- this is  activeLayer()
            l2.addCurves(self.t, (1,3), Layer.Line, 1, 4)

        return

