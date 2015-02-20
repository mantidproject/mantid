#pylint: disable=no-init,invalid-name
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
        """ Mantid required
        """
        return "GenerateGroupingSNSInelastic"

    def summary(self):
        """ Mantid required
        """
        return "Generate grouping files for ARCS, CNCS, HYSPEC, and SEQUOIA."


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

        IDF=mantid.api.ExperimentInfo.getInstrumentFilename(instrument)
        __w = mantid.simpleapi.LoadEmptyInstrument(Filename=IDF)

        i=0
        while(__w.getDetector(i).isMonitor()):
            i += 1
        #i is the index of the first true detector
        #now, crop the workspace of the monitors
        __w = mantid.simpleapi.CropWorkspace(__w,StartWorkspaceIndex=i)

        #get number of detectors (not including monitors)
        y=__w.extractY()
        numdet=(y[y==1]).size

        spectra = arange(numdet).reshape(-1,8,128)

        banks = numdet/8/128


        f = open(filename,'w')

        f.write('<?xml version="1.0" encoding="UTF-8" ?>\n<detector-grouping instrument="'+instrument+'">\n')


        groupnum = 0
        for i in arange(banks):
            for j in arange(8/pixelsx)*pixelsx:
                for k in arange(128/pixelsy)*pixelsy:

                    groupname = str(groupnum)
                    ids = spectra[i, j:j+pixelsx, k:k+pixelsy].reshape(-1)
                    detids = []
                    for l in ids:
                        detids.append(__w.getDetector(int(l)).getID())

                    detids = str(detids).replace("[","").replace("]","")

                    f.write('<group name="'+groupname+'">\n   <detids val="'+detids+'"/> \n</group>\n')
                    groupnum += 1
        f.write('</detector-grouping>')
        f.close()
        mantid.simpleapi.DeleteWorkspace(__w.getName())
        return

mantid.api.AlgorithmFactory.subscribe(GenerateGroupingSNSInelastic)
