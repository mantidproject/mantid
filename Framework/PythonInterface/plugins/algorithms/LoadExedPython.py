from mantid.kernel import *
from mantid.api import *

import os
import struct
import numpy as np
import copy
import types



class LoadEXED(PythonAlgorithm):
    __doc__ = """This is the EXED data loader written in Python.
    Original code was written by Wolf-Dieter Stein.
    Modifications have been made by Maciej Bartkowiak and Garrett Granroth (ORNL) in 2017,
    in order to fully replace the C++ version of LoadEXED,
    improve the treatment of the monitor spectra and Bring Exed into the Mantid Distribution.
    """

    def category(self):
        return "AtHZB;PythonAlgorithms"

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
                             doc="Instrument definition file. By default taken to have the same name as the data file, but with XML extension.")

        self.declareProperty(MatrixWorkspaceProperty(name="OutputWorkspace",
                                                     defaultValue = "",
                                                     direction=Direction.Output),
                             doc="Mantid workspace containing the measured data.")


    def PyExec(self):
        from mantid.simpleapi import LoadInstrument
        from mantid.simpleapi import LoadExedHelper
        from mantid.simpleapi import AddSampleLog
        from mantid.simpleapi import SetGoniometer
        from mantid.simpleapi import ExtractSpectra
        from mantid.simpleapi import MaskDetectors
        from mantid.simpleapi import RemoveMaskedSpectra

        #self.LoadRaw()

        fn = self.getPropertyValue("Filename")
        wsn = self.getPropertyValue("OutputWorkspace")
        print fn, wsn

        self.fxml = self.getPropertyValue("InstrumentXML")
        if self.fxml == "":
            self.fxml = '.'.join(fn.split('.')[:-1]+['xml'])

        ws = LoadExedHelper(Filename = fn, XMLFilename = self.fxml, OutputWorkspace = wsn)


        if (self.fxml == ""):
            self.fxml = fn[:fn.find('.')] + ".xml"

        print ws, wsn


        LoadInstrument(Workspace=ws, Filename = self.fxml, RewriteSpectraMap= True)

        #Sample_logs

        fin = open(fn,'rb')
        header = True
        parms_dict={}
        parms_dict['omega']= '0.0'
        parms_dict['chi']='0.0'
        while (header):
            b_line = fin.readline()
            line=b_line.decode('ascii')
            if line.find("Following are binary data")>0:
                header=False
            if line.find("=")>0:
                line_lst=line.split('=')
                if len(line_lst)==2:
                  parm_val=line_lst[1].strip()
                  if (parm_val.isdigit())&(line_lst[0].find('Run_Number')<0):
                        parm_val=eval(parm_val)
                  parms_dict[line_lst[0].strip()]=parm_val
                if line.find("Comment")>-1:
                   parms_dict['Comment']=line.strip('Comment =')
        fin.close()
        parms_dict['phi']=copy.deepcopy(parms_dict['CAR_OMEGA_MAG'])
                #print line
        nrows=int(parms_dict['NDET'])

        for sl in parms_dict.keys():
            #print (sl.encode('ascii','ignore'), parms_dict[sl].encode('ascii','ignore'))
            key_in=sl.encode('ascii','ignore')
            if isinstance(parms_dict[sl],types.UnicodeType):
                AddSampleLog(Workspace=ws, LogName=key_in, LogText=parms_dict[sl].encode('ascii','ignore'))#, LogType='Number')
            else:
                AddSampleLog(Workspace=ws, LogName=key_in, LogText=str(parms_dict[sl])), #LogType='Number')

        SetGoniometer(Workspace=ws, Goniometers='Universal', Axis0='phi,0,1,0,1')
        # ExtractSingleSpectrum(InputWorkspace = wsn, WorkspaceIndex = nrows-2, OutputWorkspace = wsn + '_Monitors')
        ExtractSpectra(InputWorkspace = wsn, WorkspaceIndexList = ','.join([str(s) for s in range(nrows-2, nrows)]), OutputWorkspace = wsn + '_Monitors')
        MaskDetectors(Workspace = wsn, WorkspaceIndexList = ','.join([str(s) for s in range(nrows-2, nrows)]))
        ws = RemoveMaskedSpectra(InputWorkspace = wsn, OutputWorkspace = wsn)

        self.setProperty("OutputWorkspace", ws)


# Register algorthm with Mantid.
AlgorithmFactory.subscribe(LoadEXED)
