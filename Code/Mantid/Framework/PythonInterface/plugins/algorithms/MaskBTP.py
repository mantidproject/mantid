"""*WIKI* 
Algorithm to mask detectors in particular banks, tube, or pixels. It applies to the following instruments only: ARCS, CNCS, HYSPEC, SEQUOIA.
If one of Bank, Tube, Pixel entries is left blank, it will apply to all elements of that type. For example:

MaskBTP(w,Bank = "1") will completely mask all tubes and pixels in bank 1. 
MaskBTP(w,Pixel = "1,2") will mask all pixels 1 and 2, in all tubes, in all banks. 

The algorithm allows ranged inputs: Pixel = "1-8,121-128" is equivalent to Pixel = "1,2,3,4,5,6,7,8,121,122,123,124,125,126,127,128"

Either the input workspace or the instrument must be set

*WIKI*"""

import mantid.simpleapi  
import mantid.api
import mantid.kernel
import numpy

class MaskBTP(mantid.api.PythonAlgorithm):
    """ Class to generate grouping file
    """

    def category(self):
        """ Mantid required
        """
        return "PythonAlgorithms;Transforms\\Masking;Inelastic"

    def name(self):
        """ Mantid require
        """
        return "MaskBTP"
    
    
    def PyInit(self):
        self.declareProperty(mantid.api.WorkspaceProperty("Workspace", "",direction=mantid.kernel.Direction.InOut, optional = mantid.api.PropertyMode.Optional), "Input workspace (optional)")
        allowedInstrumentList=mantid.kernel.StringListValidator(["","ARCS","CNCS","HYSPEC","SEQUOIA"])
        self.declareProperty("Instrument","",validator=allowedInstrumentList,doc="One of the following instruments: ARCS, CNCS, HYSPEC, SEQUOIA")
        self.declareProperty("Bank","",doc="Bank(s) to be masked. If empty, will apply to all banks")
        self.declareProperty("Tube","",doc="Tube(s) to be masked. If empty, will apply to all tubes")
        self.declareProperty("Pixel","",doc="Pixel(s) to be masked. If empty, will apply to all pixels")          
        self.declareProperty(mantid.kernel.IntArrayProperty(name="MaskedDetectors", direction=mantid.kernel.Direction.Output), doc="List of  masked detectors") 
        

    def PyExec(self):
        ws = self.getProperty("Workspace").value
        self.instrument=None
        self.instname = self.getProperty("Instrument").value
        bankString = self.getProperty("Bank").value
        tubeString = self.getProperty("Tube").value
        pixelString = self.getProperty("Pixel").value         
        
        if self.instname == "" and ws == None:
            raise ValueError("No workspace or instrument were selected" )    

        if ws != None:
            self.instrument = ws.getInstrument()
            self.instname = self.instrument.getName()
                
        instrumentList=["ARCS","CNCS","HYSPEC","SEQUOIA"]
        try:
            instrumentList.index(self.instname)
        except:
            raise ValueError("Instrument "+self.instname+" not in the allowed list")
            
        if (self.instrument==None):
            IDF=mantid.api.ExperimentInfo.getInstrumentFilename(self.instname)
            ws=mantid.simpleapi.LoadEmptyInstrument(IDF,OutputWorkspace=self.instname+"MaskBTP")
            self.instrument=ws.getInstrument()
            
        if (bankString == ""):
            if (self.instname == "ARCS"):
                banks=numpy.arange(115)+1
            elif (self.instname == "CNCS"):
                banks=numpy.arange(50)+1
            elif (self.instname == "HYSPEC"):
                banks=numpy.arange(20)+1
            elif (self.instname == "SEQUOIA"):
                banks=numpy.arange(113)+38
        else:
            banks=self._parseBTPlist(bankString)
        
        if (tubeString == ""):
            tubes=numpy.arange(8)+1
        else:
            tubes=self._parseBTPlist(tubeString)
        
        if(pixelString == ""):
            pixels=numpy.arange(128)+1
        else:
            pixels=self._parseBTPlist(pixelString)

  
             
        detlist=[]
        for b in banks:
            ep=self._getEightPackHandle(b)
            for t in tubes:
                if ((t<1) or (t>8)):
                    raise ValueError("Out of range index for tube number")
                else:
                    for p in pixels:
                        if ((p<1) or (p>128)):
                            raise ValueError("Out of range index for pixel number")
                        else:
                            pid=ep[int(t-1)][int(p-1)].getID()
                            detlist.append(pid)
        if len(detlist)> 0:
            mantid.simpleapi.MaskDetectors(Workspace=ws,DetectorList=detlist)
        else:
            self.log().information("no detectors within this range")
        self.setProperty("MaskedDetectors", numpy.array(detlist))
            
    def _parseBTPlist(self,value):
        """
        Helper function to transform a string into a list of integers
        For example "1,2-4,8-10" will become [1,2,3,4,8,9,10]
        It will deal with lists as well, so range(1,4) will still be [1,2,3]
        """
        parsed = []
        #split the commas
        parts = str(value).strip(']').strip('[').split(',')
        #now deal with the hyphens
        for p in parts:
            if len(p) > 0:
                elem = p.split("-")
            if len(elem) == 1:
                parsed.append(int(elem[0]))
            if len(elem) == 2:
                startelem = int(elem[0])
                endelem = int(elem[1])
                if endelem < startelem:
                    raise ValueError("The element after the hyphen needs to be greater or equal than the first element")
                elemlist = range(startelem,endelem+1)
                parsed.extend(elemlist)
        return parsed
    
    def _getEightPackHandle(self,banknum):
        """
        Helper function to return the handle to a given eightpack
        """
        banknum=int(banknum)
        if self.instname=="ARCS":
            if (1<=banknum<= 38):
                return self.instrument[3][banknum-1][0]
            elif(39<=banknum<= 77):
                return self.instrument[4][banknum-39][0]
            elif(78<=banknum<=115):
                return self.instrument[5][banknum-78][0]
            else:
                raise ValueError("Out of range index for ARCS instrument bank numbers")
        elif self.instname=="CNCS":
            if (1<=banknum<= 50):
                return self.instrument[3][banknum-1][0]
            else:
                raise ValueError("Out of range index for CNCS instrument bank numbers")
        elif self.instname=="HYSPEC":
            if (1<=banknum<= 20):
                return self.instrument[3][banknum-1][0]
            else:
                raise ValueError("Out of range index for HYSPEC instrument bank numbers")
        elif self.instname=="SEQUOIA":
            if (38<=banknum<= 74):
                return self.instrument[3][banknum-38][0]
            elif(75<=banknum<= 113):
                return self.instrument[4][banknum-75][0]
            elif(114<=banknum<=150):
                return self.instrument[5][banknum-114][0]
            else:
                raise ValueError("Out of range index for SEQUOIA instrument bank numbers")    

mantid.api.AlgorithmFactory.subscribe(MaskBTP)
