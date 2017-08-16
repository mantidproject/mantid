from __future__ import (absolute_import, division, print_function)
from six import iteritems
import mantid.simpleapi as mantid


class MaxEntPresenter(object):

    def __init__(self,view):
        self.view=view
        # set data
        self.getWorkspaceNames()
        #connect
        #self.view.tableClickSignal.connect(self.tableClicked)
        self.view.maxEntButtonSignal.connect(self.handleMaxEntButton)

    # only get ws that are groups or pairs
    # ignore raw
    def getWorkspaceNames(self):
        options=mantid.AnalysisDataService.getObjectNames()
        options=[item.replace(" ","") for item in options]
        final_options=[]
        for pick in options:
            if ";" in pick and "Raw" not in pick:
                final_options.append(pick)
        self.view.addItems(final_options)

#    #functions
#    def tableClicked(self,row,col):
#        if row == self.view.getImBoxRow() and col ==1:
#            self.view.changedHideUnTick(self.view.getImBox(),self.view.getImBoxRow()+1)
#        elif  row == self.view.getShiftBoxRow() and col ==1:
#            self.view.changed(self.view.getShiftBox(),self.view.getShiftBoxRow()+1)
#
    def handleMaxEntButton(self):
        inputs = self.get_MaxEnt_input()
        alg=mantid.AlgorithmManager.create("MaxEnt")
        alg.initialize()
        alg.setChild(True)
        for name,value in iteritems(inputs):
            mantid.logger.warning(name+"  "+str(value))
            alg.setProperty(name,value)
        alg.execute() 
        wsChi=  mantid.AnalysisDataService.addOrReplace( inputs["EvolChi"],alg.getProperty("EvolChi").value)
        wsAngle=mantid.AnalysisDataService.addOrReplace( inputs["EvolAngle"],alg.getProperty("EvolAngle").value)
        wsImage=mantid.AnalysisDataService.addOrReplace( inputs["ReconstructedImage"],alg.getProperty("ReconstructedImage").value)
        wsData= mantid.AnalysisDataService.addOrReplace( inputs["ReconstructedData"],alg.getProperty("ReconstructedData").value)

        if mantid.AnalysisDataService.doesExist("EvolChiMuon"):
            EvolChiMuon=mantid.AnalysisDataService.retrieve("EvolChiMuon")
            EvolChiMuon.add(inputs["EvolChi"])
        else:
            mantid.GroupWorkspaces(InputWorkspaces=inputs["EvolChi"],OutputWorkspace="EvolChiMuon")
        
        if mantid.AnalysisDataService.doesExist("EvolAngleMuon"):
            EvolAngleMuon=mantid.AnalysisDataService.retrieve("EvolAngleMuon")
            EvolAngleMuon.add(inputs["EvolAngle"])
        else:
            mantid.GroupWorkspaces(InputWorkspaces=inputs["EvolAngle"],OutputWorkspace="EvolAngleMuon")
 
        if mantid.AnalysisDataService.doesExist("ReconstructedImageMuon"):
            ReconstructedImageMuon=mantid.AnalysisDataService.retrieve("ReconstructedImageMuon")
            ReconstructedImageMuon.add(inputs["ReconstructedImage"])
        else:
            mantid.GroupWorkspaces(InputWorkspaces=inputs["ReconstructedImage"],OutputWorkspace="ReconstructedImageMuon")
 
        if mantid.AnalysisDataService.doesExist("ReconstructedDataMuon"):
            ReconstructedMuon=mantid.AnalysisDataService.retrieve("ReconstructedDataMuon")
            ReconstructedMuon.add(inputs["ReconstructedData"])
        else:
            mantid.GroupWorkspaces(InputWorkspaces=inputs["ReconstructedData"],OutputWorkspace="ReconstructedDataMuon")


    def get_MaxEnt_input(self):
        inputs=self.view.initMaxEntInput()
        if self.view.isRaw():
            self.view.addRaw(inputs,"InputWorkspace")
            self.view.addRaw(inputs,"EvolChi")
            self.view.addRaw(inputs,"ReconstructedImage")
            self.view.addRaw(inputs,"ReconstructedData")
        return inputs
