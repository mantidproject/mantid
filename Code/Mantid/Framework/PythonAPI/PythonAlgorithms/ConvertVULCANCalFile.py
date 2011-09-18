"""*WIKI* 


*WIKI*"""
################################################################################
#
#  Convert Vulcan's Calibration File To Mantid Standard
#
#  Author:  W. Zhou
#
################################################################################
from MantidFramework import *
from mantidsimple import *

class ConvertVULCANCalFile(PythonAlgorithm):
    """ Class to convert VULCAN's calibration file
    """
    def __init__(self):
        """ Initialization
        """
        PythonAlgorithm.__init__(self)

        self._instrument="Vulcan"

        return

    def category(self):
        """ Mantid required
        """
        return "Algorithm"

    def name(self):
        """ Mantid require
        """
        return "ConvertVULCANCalFile"

    def PyInit(self):
        """ Python initialization:  Define input parameters
        """
        numbankslist = ["1", "6"]
        filetypes = ["VULCAN-ASCII"]

        self.declareProperty("NumberBanks", "6", Validator=ListValidator(numbankslist),
                Description="Number of banks required")
        self.declareFileProperty("VulcanIDLCalFilename", "", FileAction.Load, ['.dat'], 
                Description="VULCAN's calibration file for IDL program (the file to convert from)")
        self.declareFileProperty("VulcanMantidCalFilename", "", FileAction.Save, ['.cal'],
                Description="VULCAN's calibration file for Mantid (the file to convert to)")
        self.declareProperty("FileType", "VULCAN-ASCII", Validator=ListValidator(filetypes),
                Description="Input File Type")

        return

    def PyExec(self):
        """ Main execution body
        """
        # 1. Get input
        numbanks = int(self.getProperty("NumberBanks"))
        idlcalfilename = self.getProperty("VulcanIDLCalFilename")
        mantidcalfilename = self.getProperty("VulcanMantidCalFilename")
        filetype = self.getProperty("FileType")

        # 2. Temporary workspace
        offsetwsname = "OffsetTemp"
        groupwsname = "GroupTemp"

        if numbanks == 1:
            # 1-bank
            groupname = "VULCAN"        
        elif numbanks == 6:
            # 6-bank
            groupname = "bank21,bank22,bank23,bank26,bank27,bank28"
        else:
            raise NotImplementedError("%d-Bank Case Is Not Supported" % (numbanks))

        # 3. Create
        LoadDspacemap(Filename=str(idlcalfilename), InstrumentName=self._instrument, 
                FileType=filetype, OutputWorkspace=offsetwsname)

        CreateGroupingWorkspace(InstrumentName=self._instrument, GroupNames=groupname, OutputWorkspace=groupwsname)

        SaveCalFile(Groupingworkspace=groupwsname, OffsetsWorkspace=offsetwsname, Filename=str(mantidcalfilename))

        # 4. Clean
        print "Should Implement Something To Clean Temporary Workspace"

        return

mtd.registerPyAlgorithm(ConvertVULCANCalFile())

# offsetwsname = "OffsetTemp"
# groupwsname = "GroupTemp"
# dspacefilename = "/home/wzz/Projects/Mantid-Project/Tests/Vulcan-01/pid_offset_vulcan_new.dat"
# 
# groupname = "bank21,bank22,bank23,bank26,bank27,bank28" # 6-bank
# groupname = "VULCAN"        # 1-bank
# 
# LoadDspacemap(Filename=dspacefilename, InstrumentName=instrument, FileType=filetype, OutputWorkspace=offsetwsname)
# CreateGroupingWorkspace(InstrumentName="Vulcan", GroupNames=groupname, OutputWorkspace=groupwsname)
# SaveCalFile(Groupingworkspace=groupwsname, OffsetsWorkspace=offsetwsname, 
# Filename="/home/wzz/Projects/Mantid-Project/Tests/Vulcan-01/ee.cal")
