from mantid.kernel import *
from mantid.api import *
import mantid.simpleapi as api

import os
import struct
import numpy as np



class LoadExedHelper(PythonAlgorithm):
	
    def category(self):
	return "AtHZB;PythonAlgorithms"

    def name(self):
	return "LoadExedHelper"

    def summary(self):
	return "Help File for Load EXED Raw File."
	
	
    def PyInit(self):
        self.declareProperty(FileProperty(name="Filename",defaultValue = "",
                                          action=FileAction.Load,
                                          extensions = ["raw"]),
                             doc="Data with reflection.")
			     
	self.declareProperty("StandardXML", False, "Use standard XML")
	
	self.declareProperty(FileProperty(name="XMLFilename",defaultValue = "",
                                          action=FileAction.Load,
                                          extensions = ["xml"]),
                             doc="XML Filename.")

        self.declareProperty(MatrixWorkspaceProperty(name="OutputWorkspace",
                                                     defaultValue = "",
                                                     direction=Direction.Output),
                             doc="Data with reflection.")


    def PyExec(self):
	
        from mantid.simpleapi import LoadInstrument
	from mantid.simpleapi import LoadEmptyInstrument
	from mantid.simpleapi import AddSampleLog
	from mantid.simpleapi import SetGoniometer

        nbins = 2
        nrows = 2
	
	

        fn = self.getPropertyValue("Filename")
        outname = self.getPropertyValue("OutputWorkspace")
	
	self.fxml = fn[:fn.find('.')] + ".xml"
	#ws=LoadEmptyInstrument(Filename = fxml)
	
        print fn
        fin = open(fn,'rb')
        header = True
        counter = 0
	
	sample_logs = []
	sample_logs.append(['omega', '0.0'])
	sample_logs.append(['chi', '0.0'])
        while (header):
            line = fin.readline()
            counter = counter +1
            #print line
            

            if (line[:4] == "NTC="):
                nbins = int(line[4:line.find('\n')])

            if (line[:5] == "NDET="):
                nrows = int(line[5:line.find('\n')])
                            
            if (line == "DATA=1\n"):
                header = False
  
	    if (line[:10] == "CAR_OMEGA="):
                omega = line[10:line.find('\n')]
		sample_logs.append(['phi',omega])
	    

	#monitors = [500001,500002]
	
        print "read UDET"
        det_udet = []
	#mon_udet = []
	#counter = 0
        for i in range(nrows):
            data = struct.unpack('i',fin.read(4))[0]
            #print data
	    #if data in monitors:
	    #mon_udet.append([counter,data])
	    #else:
	    #    det_udet.append([counter,data])
	    #counter = counter + 1
	    det_udet.append(data)

        print "read Counter"
        det_count = []
        for i in range(nrows):
            data = struct.unpack('i',fin.read(4))[0]
            #print data
            det_count.append(data)

        print "read TimeBinBoundaries"

        #tbc = np.fromfile(fin, np.float, nbins+1, '')
        det_tbc = []
        # mon_tbc = []
        for i in range(nbins+1):
            data = struct.unpack('f',fin.read(4))[0]
            #print data
            det_tbc.append(data)
	    
	
        print "read Data"

        data = np.fromfile(fin, np.uint32, nrows*nbins, '')
        fin.close()

        output_ws = WorkspaceFactory.create("Workspace2D", NVectors=nrows,
                                            XLength = nbins+1,
                                            YLength = nbins)
					    
	#output_monitors = WorkspaceFactory.create("Workspace2D", NVectors=nmons,
        #                                    XLength = nbins+1,
        #                                    YLength = nbins)
					    
	self.setProperty("OutputWorkspace", output_ws)
	
	print "ws:", output_ws
	
        output_ws.setYUnit("Counts")
        output_ws.setTitle("Data")
        unit = output_ws.getAxis(0).setUnit("TOF")
        #unit.setLabel("TOF","Milliseconds")

            
        data.shape = (nrows,nbins)

        xdata = np.array(det_tbc)
        xdata_mon = np.linspace(xdata[0],xdata[-1], len(xdata))

        for i in range(nrows-2):
            #print i
            tmp = data[i]
            ydata = tmp.astype(np.float)
            edata = np.sqrt(ydata)
            output_ws.setX(i,xdata)
            output_ws.setY(i,ydata)
            output_ws.setE(i,edata)  


        for i in range(nrows-2,nrows):
            #print i
            tmp = data[i]
            ydata = tmp.astype(np.float)
            edata = np.sqrt(ydata)
            output_ws.setX(i,xdata_mon)
            output_ws.setY(i,ydata)
            output_ws.setE(i,edata)  
            
        print nrows, nbins
	
	
	#output_ws.replaceSpectraMap(SpectraDetectorMap(det_count, det_udet, nrows)

        self.setProperty("OutputWorkspace", output_ws)
	

        fxml = fn[:fn.find('.')] + ".xml"
	
	print fxml

        wsn = self.getPropertyValue("OutputWorkspace")
	
	print wsn

        #det = LoadInstrument(Workspace="324", Filename = fxml, RewriteSpectraMap= True)
	
	#ws=LoadEmptyInstrument(Filename = fxml)
	
	
	print "set detector IDs"
	
	#set detetector IDs
	for i in range(nrows):
		s = output_ws.getSpectrum(i).setDetectorID(det_udet[i])
		
	#LoadInstrument(Workspace=ws, Filename = fxml, RewriteSpectraMap= True)
	
	print "add Sample Logs"
	print "ws:", output_ws
	
	#for sl in sample_logs:
	#	AddSampleLog(Workspace=output_ws, LogName=sl[0], LogText=sl[1], LogType='Number')
	#	
	#SetGoniometer(Workspace=output_ws, Goniometers='Universal', Axis0='phi,0,1,0,1')
		
	# api.ExtractSingleSpectrum(InputWorkspace = outname, WorkspaceIndex=nrows-2, OutputWorkspace = outname+'_Monitors')
	

	
# Register algorthm with Mantid.
AlgorithmFactory.subscribe(LoadExedHelper)