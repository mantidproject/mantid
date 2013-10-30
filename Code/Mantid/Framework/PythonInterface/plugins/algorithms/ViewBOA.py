#--------------------------------------------------------------------
# Algorithm which loads a BOA file and creates the 3 BOA plots
# of Uwe Filges desire.
#
# Mark Koennecke, July 2013
#---------------------------------------------------------------------

from mantid.api import AlgorithmFactory
from mantid.api import PythonAlgorithm, WorkspaceFactory, FileProperty, FileAction, WorkspaceProperty
from mantid.kernel import Direction, StringListValidator, ConfigServiceImpl
import mantid.simpleapi
from mantid.simpleapi import mtd
import datetime
import math
import os.path

class ViewBOA(PythonAlgorithm):
    def category(self):
        return 'PythonAlgorithms;SINQ'

    def PyInit(self):
        now = datetime.datetime.now()
        self.declareProperty("Year",now.year,"Choose year",direction=Direction.Input)
        self.declareProperty('Numor',0,'Choose file number',direction=Direction.Input)
        self.declareProperty('CD-Distance',6.000,'Chopper Detector distance in meter',direction=Direction.Input)

    def PyExec(self):
        year=self.getProperty('Year').value
        num=self.getProperty('Numor').value
        CD = self.getProperty('CD-Distance').value
        self.log().error('Running LoadBOA for file number ' + str(num))
        rawfile = 'tmp' + str(num)
        mantid.simpleapi.LoadSINQ('BOA',year,num,rawfile)
        raw = mtd[rawfile]
        ntimebin = raw.getDimension(0).getNBins()
        self.log().error(rawfile + ' has ' + str(ntimebin) + ' time bins')
        psdsum = 'psdsum' + str(num)
        exec(psdsum + ' = mantid.simpleapi.ProjectMD(rawfile,\'X\',0,ntimebin)')
        ysum = 'ysum' + str(num)
        nx = raw.getDimension(1).getNBins()
        exec(ysum + ' = mantid.simpleapi.ProjectMD(rawfile,\'Y\',0,nx)')
        ny = raw.getDimension(2).getNBins()
        exec('tmp2' + ' = mantid.simpleapi.InvertMDDim(ysum)')
        hist = 'histogram' + str(num)
        exec('tmp3' + ' = mantid.simpleapi.MDHistoToWorkspace2D(\'tmp2\')')
        exec(hist + ' = mantid.simpleapi.GroupDetectors(\'tmp3\',\'\',\'\',\'0-' + str(ny) +
             '\',\'\',False,\'Sum\',False)')
        self.TOFToLambda(hist,CD)
        mantid.simpleapi.DeleteWorkspace(rawfile)
        mantid.simpleapi.DeleteWorkspace('tmp2')
        mantid.simpleapi.DeleteWorkspace('tmp3')

    def TOFToLambda(self, wsname, CD):
        ws2d = mtd[wsname]
        tofdata = ws2d.dataX(0)
        for i in range(len(tofdata)):
            tofdata[i] = (3.9560346E-7*(tofdata[i]*1.E-7/CD))*1.E10
        

AlgorithmFactory.subscribe(ViewBOA)
