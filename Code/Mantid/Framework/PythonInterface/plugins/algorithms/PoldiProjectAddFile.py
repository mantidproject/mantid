"""*WIKI* 


*WIKI*"""
from mantid.api import PythonAlgorithm, AlgorithmFactory
from mantid.api import registerAlgorithm, MatrixWorkspaceProperty
from mantid.api import ITableWorkspaceProperty
from mantid.api import FileProperty, FileAction


# from MantidFramework import *
# from mantidsimple import *

from mantid.kernel import Direction
from mantid.simpleapi import *


#from pylab import *
import math
import numpy as np
import os.path
from os import listdir
from os.path import isfile, join
import re



PI=math.pi
TWOPI = 2*PI

CONVLAMV = 3956.034*1000.
CONVKV = CONVLAMV / TWOPI


class PoldiProjectAddFile(PythonAlgorithm):
    
    def category(self):
        """ Mantid required
        """
        return "Poldi"

    def name(self):
        """ Mantid required
        """
        return "PoldiProjectAddDir"

    def PyInit(self):
        """ Mantid required
        """
        self.declareProperty(FileProperty(name="File",defaultValue="",action=FileAction.Load))

        self.declareProperty(ITableWorkspaceProperty("OutputWorkspace", "PoldiAnalysis", direction=Direction.Output),
                              "Poldi analysis main worksheet")
    
#         self.declareProperty(self, "RunTheAnalysis", False, 
#                              doc="If True, the PoldiProjectRun algo is called. Default: False")
    
    
    
    
    def path_leaf(path):
        head, tail = ntpath.split(path)
        return tail
    
    
    def interpreteName(self, name):
        patern="(.*[ a-zA-Z]*/*\*)*poldi(?P<year>[0-9]*)n(?P<numero>[0-9]*)"
        regex  = re.match(patern, name, re.M|re.I)
        year   = int(regex.group("year"))
        numero = int(regex.group("numero"))
        return (year, numero)
    
    
    
    
    
    
    def PyExec(self):
        """ Mantid required
        """
        self.log().debug('Poldi Data Analysis ---- start')
        load_data_at_the_end = False
        
        try:
            sample_info_ws_name = self.getProperty("OutputWorkspace").value
            if(sample_info_ws_name == ""):
                sample_info_ws_name = "PoldiAnalysis"
            self.log().debug('Poldi Data Analysis ---- %s'%(sample_info_ws_name))
            sample_info_ws = mtd["PoldiAnalysis"]
            self.log().debug('                    ---- workspace loaded')
        except:
            self.log().debug('                    ---- workspace created')
            sample_info_ws = WorkspaceFactory.createTable()
            sample_info_ws.addColumn("str","spl Name")
            sample_info_ws.addColumn("int","year")
            sample_info_ws.addColumn("int","number")
            sample_info_ws.addColumn("str","data file")
            sample_info_ws.addColumn("str","spl log")
            sample_info_ws.addColumn("str","spl corr")
            sample_info_ws.addColumn("str","spl dead wires")
            sample_info_ws.addColumn("str","spl peak")
            load_data_at_the_end = True
        
        
        
        
        
        dataFile = self.getProperty("File").value
        
        self.log().debug('Poldi - load data - %s'%(dataFile))
        (sample_root, sample_name) = os.path.split(dataFile)
        (sample_name, sampleExt) = os.path.splitext(sample_name)
        self.log().error('Poldi -  samples : %s' %(sample_root))
        self.log().error('Poldi -          : %s' %(sample_name))
        self.log().error('Poldi -          : %s' %(sampleExt))
        
        if("hdf" in sampleExt):
            self.log().error('Poldi -  samples : %s' %(sample_name))
            file_path = dataFile
            sample_name_log = "%sLog" %sample_name
            sample_name_corr = "%sCorr" %sample_name
            sample_name_deadw = "%sDeadWires" %sample_name
            (sample_year, sample_numero) = self.interpreteName(sample_name)
            sample_name_peak = "%sPeak" %sample_name
            
            sample_info_ws.addRow([sample_name, sample_year, sample_numero, file_path, 
                                   sample_name_log, 
                                   sample_name_corr,
                                   sample_name_deadw,
                                   sample_name_peak])  
        nb_of_sample = sample_info_ws.rowCount()
        self.log().error('Poldi -   1 samples added')
        self.log().error('      -  %d samples in total' %(nb_of_sample))
                
                
        if(load_data_at_the_end):
            self.setProperty("OutputWorkspace", sample_info_ws)
        
        
#         if(self.getProperty("RunTheAnalysis").value):
#             PoldiProjectRun(InputWorkspace=sample_info_ws)



AlgorithmFactory.subscribe(PoldiProjectAddFile)
