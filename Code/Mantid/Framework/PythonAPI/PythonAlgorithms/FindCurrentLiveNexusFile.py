"""*WIKI* 


*WIKI*"""
from MantidFramework import *
from mantidsimple import *
import os

class FindCurrentLiveNexusFile(PythonAlgorithm):
    """ Algorithm to find current live Nexus file
    """
    def __init__(self):
        """ Init
        """
        PythonAlgorithm.__init__(self)

        return

    def category(self):
        return "DataHandling"

    def name(self):
        return "FindCurrentLiveNexusFile"

    def PyInit(self):
        """ Define parameters
        """
        instruments = ["PG3", "NOM", "VULCAN", "SNAP"]
        facilities = ["SNS"]

        # 1. Input
        self.declareProperty("Facility", "", Validator=ListValidator(facilities),
                Description="Faclity")
        self.declareProperty("Instrument", "", Validator=ListValidator(instruments),
                Description="SNS Instrument Name")
        # 2. Output 
        self.declareProperty("CurrentLiveNexusFile", "", Direction=Direction.Output,
                Description="Current Live Nexus File")

        return


    def PyExec(self):
        """ Find live Nexus file 
        """
        # 1. Check
        facility = self.getProperty("Facility").upper()
        instrument = self.getProperty("Instrument").upper()

        if facility != "SNS":
            raise NotImplementedError("Facility must be SNS but not %s" % (facility))

        # 2. Get files
        directory = "/%s/%s/shared/live/"% (facility, instrument)
        print directory

        filelist = []
        try:
            files = os.listdir(directory)
            for file in files:
                if file.endswith(".nxs"):
                    filelist.append(file)
        except OSError, err:
            errstr = str(err)
            raise NotImplementedError(errstr)

        if len(filelist) == 0:
            raise NotImplementedError("There is no file existing in live file directory %s" % (directory))

        # 3. Find the current file
        for filename in fileslist:
	    # Nexus file
	    if filename.startswith("NOM_geom_"):
		key = filename.split("NOM_geom_")[1] 
	    elif filename.startswith("NOM_"):
		key = filename.split("NOM_")[1]
	    else:
		key = ""
		raise NotImplementedError("File %s has a non-standard name" % (filename))
	    # END-IF-ELSE
	    candidates[key] = filename
        # ENDFOR
    
        # 4. Get the current file
        keys = sorted(candidates.keys())
        if len(keys) == 0:
            raise NotImplementedError("There is no recognizable .nxs file existing")
        lastfile = candidates[keys[-1]]

        self.setProperty("CurrentLiveNexusFile", lastfile)

        return

mtd.registerPyAlgorithm(FindCurrentLiveNexusFile())
