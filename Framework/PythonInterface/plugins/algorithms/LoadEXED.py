from __future__ import (absolute_import, division, print_function)
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import LoadInstrument, AddSampleLogMultiple, SetGoniometer
from mantid.simpleapi import ExtractSpectra, MaskDetectors, RemoveMaskedSpectra
from mantid.simpleapi import RotateInstrumentComponent
import struct
import numpy as np
import copy
import types


class LoadEXED(PythonAlgorithm):
    __doc__ = """This algorithm reads Exed raw files and creates two workspaces.
    One for the detectors and another for the monitor.
    """

    def category(self):
        return "Inelastic;Diffraction;PythonAlgorithms"

    def name(self):
        return "LoadEXED"

    def version(self):
        return 1

    def summary(self):
        return """This algorithm reads Exed raw files and creates two workspaces.
        One for the detectors and another for the monitor."""

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
        fn = self.getPropertyValue("Filename")
        wsn = self.getPropertyValue("OutputWorkspace")
        #print (fn, wsn)

        self.fxml = self.getPropertyValue("InstrumentXML")

        #load data

        parms_dict, det_udet, det_count, det_tbc, data = self.read_file(fn)
        nrows=int(parms_dict['NDET'])
        #nbins=int(parms_dict['NTC'])
        xdata = np.array(det_tbc)
        xdata_mon = np.linspace(xdata[0],xdata[-1], len(xdata))
        ydata=data.astype(np.float)
        ydata=ydata.reshape(nrows,-1)
        edata=np.sqrt(ydata)
        #CreateWorkspace(OutputWorkspace=wsn,DataX=xdata,DataY=ydata,DataE=edata,
        #                NSpec=nrows,UnitX='TOF',WorkspaceTitle='Data',YUnitLabel='Counts')
        nr,nc=ydata.shape
        ws = WorkspaceFactory.create("Workspace2D", NVectors=nr,
                                     XLength=nc+1, YLength=nc)
        for i in range(nrows):
            ws.setX(i, xdata)
            ws.setY(i, ydata[i])
            ws.setE(i, edata[i])
        ws.getAxis(0).setUnit('tof')
        AnalysisDataService.addOrReplace(wsn,ws)

        #self.setProperty("OutputWorkspace", wsn)
        #print ("ws:", wsn)
        #ws=mtd[wsn]

        # fix the x values for the monitor
        for i in range(nrows-2,nrows):
            ws.setX(i,xdata_mon)
        self.log().information("set detector IDs")
        #set detetector IDs
        for i in range(nrows):
            ws.getSpectrum(i).setDetectorID(det_udet[i])
        #Sample_logs the header values are written into the sample logs
        log_names=[sl.encode('ascii','ignore') for sl in parms_dict.keys()]
        log_values=[sl.encode('ascii','ignore') if isinstance(sl,types.UnicodeType) else str(sl) for sl in parms_dict.values()]
        AddSampleLogMultiple(Workspace=wsn, LogNames=log_names,LogValues=log_values)
        SetGoniometer(Workspace=wsn, Goniometers='Universal')
        if (self.fxml == ""):
            LoadInstrument(Workspace=wsn, InstrumentName = "Exed", RewriteSpectraMap= True)
        else:
            LoadInstrument(Workspace=wsn, Filename = self.fxml, RewriteSpectraMap= True)
        RotateInstrumentComponent(Workspace=wsn,ComponentName='Tank',Y=1,
                                  Angle=-float(parms_dict['phi'].encode('ascii','ignore')), RelativeRotation=False)
        # Separate monitors into seperate workspace
        ExtractSpectra(InputWorkspace = wsn, WorkspaceIndexList = ','.join([str(s) for s in range(nrows-2, nrows)]),
                       OutputWorkspace = wsn + '_Monitors')
        MaskDetectors(Workspace = wsn, WorkspaceIndexList = ','.join([str(s) for s in range(nrows-2, nrows)]))
        RemoveMaskedSpectra(InputWorkspace = wsn, OutputWorkspace = wsn)

        self.setProperty("OutputWorkspace", wsn)

    def read_file(self,fn):
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
            self.log().information("read UDET")
            #print ("read UDET")
            det_udet = self.struct_data_read(fin,nrows)

            self.log().information("read Counter")
            det_count = self.struct_data_read(fin,nrows)

            self.log().information("read TimeBinBoundaries")
            det_tbc = self.struct_data_read(fin,nbins+1,'f')

            self.log().information("read Data")
            data = np.fromfile(fin, np.uint32, nrows*nbins, '')

            fin.close()
            parms_dict['phi']=copy.deepcopy(parms_dict['CAR_OMEGA_MAG'])
            return parms_dict, det_udet, det_count, det_tbc, data

    def struct_data_read(self,fin,nrows,data_type='i',byte_size=4):
                """
                helper function to read binary data_type
                requires the file handle, number of rows and data_type
                """
                self.log().debug("nrows %d" %nrows)
                tmp_lst=[struct.unpack(data_type,fin.read(byte_size))[0] for i in range(nrows)]
                #for i in range(nrows):
                #    data = struct.unpack(data_type,fin.read(byte_size))[0]
                #    tmp_lst.append(data)
                #print(tmp_lst)
                return tmp_lst


# Register algorthm with Mantid.
AlgorithmFactory.subscribe(LoadEXED)
