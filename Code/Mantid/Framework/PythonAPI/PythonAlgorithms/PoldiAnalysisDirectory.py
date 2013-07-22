"""*WIKI* 


*WIKI*"""
from mantid.api import PythonAlgorithm, registerAlgorithm, MatrixWorkspaceProperty, ITableWorkspaceProperty
from mantid.api import FileProperty, FileAction
from mantid.kernel import Direction
from mantid.simpleapi import *
from pylab import *
import math
import numpy as np
import os.path
from os import listdir
from os.path import isfile, join



PI=math.pi
TWOPI = 2*PI

CONVLAMV = 3956.034*1000.
CONVKV = CONVLAMV / TWOPI


class PoldiAnalysisDirectory(PythonAlgorithm):
    
    def category(self):
        """ Mantid required
        """
        return "Poldi\old"

    def name(self):
        """ Mantid required
        """
        return "PoldiAnalysisDirectory"

    def PyInit(self):
        """ Mantid required
        """
        self.declareProperty(FileProperty(name="Directory",defaultValue="",action=FileAction.Directory))
#         self.declareProperty(WorkspaceProperty("OutputWorkspaceName", "", direction=Direction.Output), "Name of the output Workspace")
#          self.declareProperty("OutputWorkspaceName", 'toto', doc="Name of the output Workspace")

        self.declareProperty("wlenmin", 1.1, doc = 'minimal wavelength considered' , direction = Direction.Input)
        self.declareProperty("wlenmax", 5.0, doc = 'maximal wavelength considered' , direction = Direction.Input)
        
        
        self.declareProperty("BadWiresThreshold", 0.8, doc = 'Bad wires threshold parameter' , direction = Direction.Input)
        
        self.declareProperty(ITableWorkspaceProperty("PoldiAnalysis", "PoldiAnalysis", direction=Direction.Output), "Poldi analysis main worksheet")
    
    
    
    
    
    def path_leaf(path):
        head, tail = ntpath.split(path)
        return tail
    
    def PyExec(self):
        """ Mantid required
        """
        self.log().debug('Poldi Data Analysis ---- start')
        
        sample_info_ws = WorkspaceFactory.createTable()
        sample_info_ws.addColumn("str","spl Name")
        sample_info_ws.addColumn("str","data file")
        sample_info_ws.addColumn("str","spl log")
        sample_info_ws.addColumn("str","spl corr")
        
        
        
        
        
        directory = self.getProperty("Directory").value
        wlen_min  = self.getProperty("wlenmin").value
        wlen_max  = self.getProperty("wlenmax").value
        
        bad_wires_threshold  = self.getProperty("BadWiresThreshold").value
        
        dictsearch='/home/christophe/poldi/dev/mantid-2.3.2-Source/PSIScripts'+"/mantidpoldi.dic"

        
        onlyfiles = [ f for f in listdir(directory) if isfile(join(directory,f)) ]
        
        firstOne=""               
                
        self.log().debug('Poldi - load data')
        for dataFile in onlyfiles:
            (sampleName, sampleExt) = os.path.splitext(dataFile)
            if("hdf" in sampleExt):
                filePath = join(directory,dataFile)
                sampleNameLog = "%sLog" %sampleName
                sampleNameCorr = "%sCorr" %sampleName
                
                sample_info_ws.addRow([sampleName, filePath, sampleNameLog, sampleNameCorr])  
        nb_of_sample = sample_info_ws.rowCount()
        self.log().error('Poldi -  %d samples added' %(nb_of_sample))
                
        for sample in range(nb_of_sample):
            sampleName    = sample_info_ws.column("spl Name")[sample]
            filePath      = sample_info_ws.column("data file")[sample]
            sampleNameLog = sample_info_ws.column("spl log")[sample]
                
            self.log().error('Poldi - sample %s' %(sampleName))
            LoadSINQFile(Instrument="POLDI", 
                         Filename=filePath, 
                         OutputWorkspace=sampleName)
            
            PoldiLoadLog(Filename=filePath, 
                         Dictionary=dictsearch, 
                         PoldiLog=sampleNameLog)
            
            
            
        firstSplName   = sample_info_ws.column("spl Name")[0]
        firstSplPath   = sample_info_ws.column("data file")[0]
        firstSampleLog = sample_info_ws.column("spl log")[0]
            
        ws = mtd[firstSplName]

        
        self.log().debug('Poldi - load detector')
        LoadInstrument(Workspace=ws, InstrumentName="Poldi", RewriteSpectraMap=True)
        
        
        self.log().debug('Poldi - dead wires')
        PoldiRemoveDeadWires(InputWorkspace=ws, 
                             RemoveExcludedWires=True, 
                             AutoRemoveBadWires=True, 
                             BadWiresThreshold=bad_wires_threshold, 
                             PoldiDeadWires="PoldiDeadWires")
        
        
        self.log().debug('Poldi - chopper slits')
        PoldiLoadChopperSlits(InputWorkspace=ws, 
                              PoldiChopperSlits="PoldiChopperSlits")
        
        
        self.log().debug('Poldi - spectra')
        PoldiLoadSpectra(InputWorkspace=ws, 
                         PoldiSpectra="PoldiSpectra")
        
        
        self.log().debug('Poldi - IPP')
        PoldiLoadIPP(InputWorkspace=ws, 
                     PoldiIPP="PoldiIPP")
        
        
        
        
        
        for sample in range(nb_of_sample):
            sampleName     = sample_info_ws.column("spl Name" )[sample]
            filePath       = sample_info_ws.column("data file")[sample]
            sampleNameLog  = sample_info_ws.column("spl log"  )[sample]
            sampleNameCorr = sample_info_ws.column("spl corr" )[sample]
        
            PoldiAutoCorrelation(Filename=filePath, 
                                 InputWorkspace=mtd[sampleName], 
                                 PoldiSampleLogs=sampleNameLog, 
                                 PoldiDeadWires=mtd["PoldiDeadWires"], 
                                 PoldiChopperSlits=mtd["PoldiChopperSlits"], 
                                 PoldiSpectra=mtd["PoldiSpectra"], 
                                 PoldiIPP=mtd["PoldiIPP"], 
                                 PoldiAutoCorrelation=sampleNameCorr)
        
#         self.log().debug('Poldi - analysis')
#         for dataFile in onlyfiles:
#             (sampleName, sampleExt) = os.path.splitext(dataFile)
#             if("hdf" in sampleExt):
#                 sampleNameCorr = "%sCorr" %sampleName
#                 sampleNameLog  = "%sLog"  %sampleName
#                 self.log().error('Poldi - sample corr %s' %(sampleNameCorr))
#                 filePath = join(directory,dataFile)
#                 PoldiAutoCorrelation(Filename=filePath, 
#                                      InputWorkspace=mtd[sampleName], 
#                                      PoldiSampleLogs=sampleNameLog, 
#                                      PoldiDeadWires=mtd["PoldiDeadWires"], 
#                                      PoldiChopperSlits=mtd["PoldiChopperSlits"], 
#                                      PoldiSpectra=mtd["PoldiSpectra"], 
#                                      PoldiIPP=mtd["PoldiIPP"], 
#                                      PoldiAutoCorrelation=sampleNameCorr)
        
        
        self.setProperty("PoldiAnalysis", sample_info_ws)
        



registerAlgorithm(PoldiAnalysisDirectory())
