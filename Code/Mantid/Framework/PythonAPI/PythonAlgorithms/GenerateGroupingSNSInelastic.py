"""*WIKI* 
Generate grouping files for ARCS, CNCS, HYSPEC, and SEQUOIA, by grouping py pixels along a tube and px tubes. 
py is 1, 2, 4, 8, 16, 32, 64, or 128. 
px is 1, 2, 4, or 8.

Author:  A. Savici

*WIKI*"""

import mantid
import mantid.api
import mantid.simpleapi  
import mantid.kernel
from numpy import arange

 
class GenerateGroupingSNSInelastic(mantid.api.PythonAlgorithm):
    """ Class to generate grouping file
    """

    def category(self):
        """ Mantid required
        """
        return "Inelastic;PythonAlgorithms;Transforms\\Grouping"

    def name(self):
        """ Mantid require
        """
        return "GenerateGroupingSNSInelastic"

    def PyInit(self):
        """ Python initialization:  Define input parameters
        """
        py = ["1", "2", "4","8","16","32","64","128"]
        px = ["1", "2", "4","8"]
        instrument = ["ARCS","CNCS","HYSPEC","SEQUOIA"]

        self.declareProperty("AlongTubes", "1",mantid.kernel.StringListValidator(py), "Number of pixels across tubes to be grouped")
        self.declareProperty("AcrossTubes", "1", mantid.kernel.StringListValidator(px), "Number of pixels across tubes to be grouped")
        self.declareProperty("Instrument", instrument[0], mantid.kernel.StringListValidator(instrument), "The instrument for wich to create grouping")
        f=mantid.api.FileProperty("Filename","",mantid.api.FileAction.Save,".xml")

        self.declareProperty(f,"Output filename.")

        return

    def PyExec(self):
        """ Main execution body
        """
        # 1. Get input
        pixelsy = int(self.getProperty("AlongTubes").value)
        pixelsx = int(self.getProperty("AcrossTubes").value)
        instrument = self.getProperty("Instrument").value
        filename = self.getProperty("Filename").value
        
        path=mantid.config["instrumentDefinition.directory"]
        w = mantid.simpleapi.LoadEmptyInstrument(Filename=path+'/'+instrument+"_Definition.xml")

        i=0
        while(w.getDetector(i).isMonitor()):
            i += 1
        #i is the index of the first true detector
        #now, crop the workspace of the monitors
        w = mantid.simpleapi.CropWorkspace(w,StartWorkspaceIndex=i)
        
        #get number of detectors (not including monitors)        
        y=w.extractY()
        numdet=(y[y==1]).size

        spectra = arange(numdet).reshape(-1,8,128)

        banks = numdet/8/128

        
        f = open(filename,'w')

        f.write('<?xml version="1.0" encoding="UTF-8" ?>\n<detector-grouping>\n')


        groupnum = 0
        for i in arange(banks):
            for j in arange(8/pixelsx)*pixelsx:
                for k in arange(128/pixelsy)*pixelsy:

                    groupname = str(groupnum)
                    ids = spectra[i, j:j+pixelsx, k:k+pixelsy].reshape(-1)              
                    detids = []
                    for l in ids:
                        detids.append(w.getDetector(int(l)).getID())

                    detids = str(detids).replace("[","").replace("]","")

                    f.write('<group name="'+groupname+'">\n   <detids val="'+detids+'"/> \n</group>\n')
                    groupnum += 1
        f.write('</detector-grouping>')
        f.close()
        mantid.simpleapi.DeleteWorkspace('w')
        return
        
mantid.api.registerAlgorithm(GenerateGroupingSNSInelastic)

