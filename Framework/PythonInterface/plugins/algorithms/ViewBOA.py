#pylint: disable=no-init,invalid-name
from mantid.api import AlgorithmFactory
from mantid.api import PythonAlgorithm
from mantid.kernel import Direction
from mantid.simpleapi import *
import datetime

class ViewBOA(PythonAlgorithm):
    def category(self):
        return 'PythonAlgorithms;SINQ'

    def summary(self):
        return "Load a BOA file and create the 3 BOA plots."

    def PyInit(self):
        now = datetime.datetime.now()
        self.declareProperty("Year",now.year,"Choose year",direction=Direction.Input)
        self.declareProperty('Numor',0,'Choose file number',direction=Direction.Input)
        self.declareProperty('CD-Distance',6.000,'Chopper Detector distance in metres',direction=Direction.Input)

    def PyExec(self):
        year=self.getProperty('Year').value
        num=self.getProperty('Numor').value
        CD = self.getProperty('CD-Distance').value
        self.log().error('Running LoadBOA for file number ' + str(num))
        rawfile = 'tmp' + str(num)
        LoadSINQ('BOA',year,num, OutputWorkspace=rawfile)
        raw = mtd[rawfile]
        ntimebin = raw.getDimension(0).getNBins()
        self.log().error(rawfile + ' has ' + str(ntimebin) + ' time bins')

        psdsum = 'psdsum' + str(num)
        ProjectMD(rawfile,'X',0,ntimebin, OutputWorkspace=psdsum)

        ysum = 'ysum' + str(num)
        nx = raw.getDimension(1).getNBins()
        ProjectMD(rawfile,'Y',0,nx, OutputWorkspace=ysum)

        ny = raw.getDimension(2).getNBins()
        tmp2 = InvertMDDim(ysum)
        tmp3 = MDHistoToWorkspace2D(tmp2)

        hist = 'histogram' + str(num)
        GroupDetectors(InputWorkspace='tmp3', OutputWorkspace=hist, DetectorList='0-' + str(ny), PreserveEvents=False)

        self.TOFToLambda(hist,CD)
        DeleteWorkspace(rawfile)
        DeleteWorkspace(tmp2)
        DeleteWorkspace(tmp3)

    def TOFToLambda(self, wsname, CD):
        ws2d = mtd[wsname]
        tofdata = ws2d.dataX(0)
        for i in range(len(tofdata)):
            tofdata[i] = (3.9560346E-7*(tofdata[i]*1.E-7/CD))*1.E10


AlgorithmFactory.subscribe(ViewBOA)
