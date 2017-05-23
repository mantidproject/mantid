from __future__ import (absolute_import, division, print_function)
from mantid.kernel import *
from mantid.api import *

import os
import struct
import numpy as np
import copy
import types

def read_file(fn):
    """
    function to read header and return a dictionary of the parameters parms_dict
    the read the data and return it in det_udet, det_count, det_tbc, and data
    """
    fin = open(fn,'rb')
    header = True
    parms_dict={}
    parms_dict['omega']= '0.0'
    parms_dict['chi']='0.0'
    while (header):
        b_line = fin.readline()
        line=b_line.decode('ascii')
        if line.find("=")>0:
            line_lst=line.split('=')
            if len(line_lst)==2:
              parm_val=line_lst[1].strip()
              if (parm_val.isdigit())&(line_lst[0].find('Run_Number')<0):
                    parm_val=eval(parm_val)
              parms_dict[line_lst[0].strip()]=parm_val
            if line.find("Comment")>-1:
               parms_dict['Comment']=line.strip('Comment =')
        if line.find("Following are binary data")>0:
            header=False
            while line.find("DATA=")<0:
                b_line = fin.readline()
                line=b_line.decode('ascii')
                #print line
    nrows=int(parms_dict['NDET'])
    nbins=int(parms_dict['NTC'])
    print ("read UDET")
    det_udet = struct_data_read(fin,nrows)

    print ("read Counter")
    det_count = struct_data_read(fin,nrows)

    print ("read TimeBinBoundaries")
    det_tbc = struct_data_read(fin,nbins+1,'f')

    print ("read Data")
    data = np.fromfile(fin, np.uint32, nrows*nbins, '')

    fin.close()
    parms_dict['phi']=copy.deepcopy(parms_dict['CAR_OMEGA_MAG'])
    return parms_dict, det_udet, det_count, det_tbc, data

def struct_data_read(fin,nrows,data_type='i',byte_size=4):
    """
    helper function to read binary data_type
    requires the file handle, number of rows and data_type
    """
    tmp_lst=[]
    for i in range(nrows):
        data = struct.unpack(data_type,fin.read(byte_size))[0]
        tmp_lst.append(data)
    return tmp_lst

class LoadEXED(PythonAlgorithm):
    __doc__ = """This is the EXED data loader written in Python.
    Based off code was written by Wolf-Dieter Stein and  Maciej Bartkowiak.
    Garrett Granroth (ORNL)
    """

    def category(self):
        return "Inelastic;Diffraction;PythonAlgorithms"

    def name(self):
        return "LoadEXED"

    def version(self):
        return 3

    def summary(self):
        return """Modifications have been made by Maciej Bartkowiak and Garrett Granroth (ORNL) in 2017,
        in order to fully replace the C++ version of LoadEXED,
        improve the treatment of the monitor spectra and Bring Exed into the Mantid Distribution."""


    def PyInit(self):
        self.declareProperty(FileProperty(name="Filename",defaultValue = "",
                                          action=FileAction.Load,
                                          extensions = ["raw"]),
                             doc="Data file produced by egraph.")

        self.declareProperty(FileProperty(name="InstrumentXML",defaultValue = "",
                                          action=FileAction.OptionalLoad,
                                          extensions = ["xml"]),
                             doc="Instrument definition file. If no file is specified, the default idf is used.")

        self.declareProperty(MatrixWorkspaceProperty(name="OutputWorkspace",
                                                     defaultValue = "",
                                                     direction=Direction.Output),
                             doc="Mantid workspace containing the measured data.")


    def PyExec(self):
        from mantid.simpleapi import LoadInstrument
        #from mantid.simpleapi import LoadExedHelper
        from mantid.simpleapi import CreateWorkspace
        from mantid.simpleapi import AddSampleLog
        from mantid.simpleapi import SetGoniometer
        from mantid.simpleapi import ExtractSpectra
        from mantid.simpleapi import MaskDetectors
        from mantid.simpleapi import RemoveMaskedSpectra
        from mantid.simpleapi import mtd

        #self.LoadRaw()

        fn = self.getPropertyValue("Filename")
        wsn = self.getPropertyValue("OutputWorkspace")
        #print (fn, wsn)

        self.fxml = self.getPropertyValue("InstrumentXML")

        #load data

        parms_dict, det_udet, det_count, det_tbc, data = read_file(fn)
        nrows=int(parms_dict['NDET'])
        nbins=int(parms_dict['NTC'])
        xdata = np.array(det_tbc)
        xdata_mon = np.linspace(xdata[0],xdata[-1], len(xdata))
        ydata=data.astype(np.float)
        edata=np.sqrt(ydata)
        CreateWorkspace(OutputWorkspace=wsn,DataX=xdata,DataY=ydata,DataE=edata,
        NSpec=nrows,UnitX='TOF',WorkspaceTitle='Data',YUnitLabel='Counts')

        #self.setProperty("OutputWorkspace", wsn)
        print ("ws:", wsn)
        ws=mtd[wsn]

        # fix the x values for the monitor
        for i in range(nrows-2,nrows):
            ws.setX(i,xdata_mon)
        print ("set detector IDs")
        #set detetector IDs
        for i in range(nrows):
    		s = ws.getSpectrum(i).setDetectorID(det_udet[i])
            
        #load idf

        if (self.fxml == ""):
            LoadInstrument(Workspace=wsn, InstrumentName = "Exed", RewriteSpectraMap= True)
        else:
            LoadInstrument(Workspace=wsn, Filename = self.fxml, RewriteSpectraMap= True)

        #Sample_logs the header values are written into the sample logs

        for sl in parms_dict.keys():
            #print (sl.encode('ascii','ignore'), parms_dict[sl].encode('ascii','ignore'))
            key_in=sl.encode('ascii','ignore')
            if isinstance(parms_dict[sl],types.UnicodeType):
                AddSampleLog(Workspace=wsn, LogName=key_in, LogText=parms_dict[sl].encode('ascii','ignore'))#, LogType='Number')
            else:
                AddSampleLog(Workspace=wsn, LogName=key_in, LogText=str(parms_dict[sl]))#, LogType='Number')

        SetGoniometer(Workspace=wsn, Goniometers='Universal', Axis0='phi,0,1,0,1')
        # Separate monitors into seperate workspace
        ExtractSpectra(InputWorkspace = wsn, WorkspaceIndexList = ','.join([str(s) for s in range(nrows-2, nrows)]), OutputWorkspace = wsn + '_Monitors')
        MaskDetectors(Workspace = wsn, WorkspaceIndexList = ','.join([str(s) for s in range(nrows-2, nrows)]))
        RemoveMaskedSpectra(InputWorkspace = wsn, OutputWorkspace = wsn)

        self.setProperty("OutputWorkspace", wsn)

# Register algorthm with Mantid.
AlgorithmFactory.subscribe(LoadEXED)
