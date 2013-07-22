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


class PoldiProjectRun(PythonAlgorithm):
    
    def category(self):
        """ Mantid required
        """
        return "Poldi"

    def name(self):
        """ Mantid required
        """
        return "PoldiProjectRun"

    def PyInit(self):
        """ Mantid required
        """
        self.declareProperty(ITableWorkspaceProperty("InputWorkspace", "PoldiAnalysis", direction=Direction.Input), 
                             "Poldi analysis main worksheet")

        self.declareProperty("wlenmin", 1.1, 
                             doc = 'minimal wavelength considered' , 
                             direction = Direction.Input)
        self.declareProperty("wlenmax", 5.0, 
                             doc = 'maximal wavelength considered' , 
                             direction = Direction.Input)
        
        
        self.declareProperty("BadWiresThreshold", 0.8, 
                             doc = 'Bad wires threshold parameter', 
                             direction = Direction.Input)
        
        self.declareProperty("PeakDetectionThreshold", 0.2, 
                             doc = 'Peak detection threshold parameter', 
                             direction = Direction.Input)
        
    
    
    
    
    
    def path_leaf(path):
        head, tail = ntpath.split(path)
        return tail
    
    def PyExec(self):
        """ Mantid required
        """
        self.log().debug('Poldi Data Analysis ---- start')
        
        sample_info_ws = self.getProperty("InputWorkspace").value
        
        wlen_min  = self.getProperty("wlenmin").value
        wlen_max  = self.getProperty("wlenmax").value
        
        bad_wires_threshold  = self.getProperty("BadWiresThreshold").value
        peak_detect_threshold  = self.getProperty("PeakDetectionThreshold").value
        
        self.log().error('Poldi run with parameters')
        self.log().error('      -  wlen_min : %s' %(wlen_min))
        self.log().error('      -  wlen_max : %s' %(wlen_max))
        self.log().error('      -  bad_wires_threshold   : %s' %(bad_wires_threshold))
        self.log().error('      -  peak_detect_threshold : %s' %(peak_detect_threshold))
        
        dictsearch='/home/christophe/poldi/dev/mantid-2.3.2-Source/PSIScripts'+"/mantidpoldi.dic"

        
        firstOne=""               
                
        self.log().debug('Poldi - load data')
        nb_of_sample = sample_info_ws.rowCount()
        self.log().error('Poldi -  %d samples listed' %(nb_of_sample))
                
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
        
            PoldiAutoCorrelation(InputWorkspace=mtd[sampleName], 
                                 PoldiSampleLogs=sampleNameLog, 
                                 PoldiDeadWires=mtd["PoldiDeadWires"], 
                                 PoldiChopperSlits=mtd["PoldiChopperSlits"], 
                                 PoldiSpectra=mtd["PoldiSpectra"], 
                                 PoldiIPP=mtd["PoldiIPP"], 
                                 OutputWorkspace=sampleNameCorr)
            
            
            sampleNamePeak = sample_info_ws.column("spl peak" )[sample]
        
            PoldiPeakDetection(InputWorkspace=sampleNameCorr,
                               PeakDetectionThreshold=peak_detect_threshold,
                               OutputWorkspace=sampleNamePeak)
            
            


registerAlgorithm(PoldiProjectRun())
