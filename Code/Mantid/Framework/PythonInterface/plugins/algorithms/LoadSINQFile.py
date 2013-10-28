"""*WIKI*
== DESCRIPTION ==

LoadSINQFile is a wrapper algorithm around LoadFlexiNexus. It locates a suitable dictionary file for the instrument in question and then goes away to call LoadFlexiNexus with the right arguments. It also performs any other magic which might be required to get the data in the right shape for further processing in Mantid. 

*WIKI*"""

#--------------------------------------------------------------
# Algorithm which loads a SINQ file. It matches the instrument 
# and the right dictionary file and then goes away and calls 
# LoadFlexiNexus and others to load the file.
#
# Mark Koennecke, November 2012
#--------------------------------------------------------------
from mantid.api import AlgorithmFactory
from mantid.api import PythonAlgorithm, WorkspaceFactory, FileProperty, FileAction, WorkspaceProperty, FrameworkManager
from mantid.kernel import Direction, StringListValidator, ConfigServiceImpl
import mantid.simpleapi
from mantid import config
import os.path

#--------- place to look for dictionary files


class LoadSINQFile(PythonAlgorithm):
    def category(self):
        return "DataHandling;PythonAlgorithms"

    def PyInit(self):
        global dictsearch
        self.setWikiSummary("Load a SINQ file with the right dictionary.")
        instruments=["AMOR","BOA","DMC","FOCUS","HRPT","MARSI","MARSE","POLDI",
                     "RITA-2","SANS","SANS2","TRICS"]
        self.declareProperty("Instrument","AMOR",
                             StringListValidator(instruments),
                             "Choose Instrument",direction=Direction.Input)
        self.declareProperty(FileProperty(name="Filename",defaultValue="",
                                          action=FileAction.Load, extensions=[".h5",".hdf"]))
        self.declareProperty(WorkspaceProperty("OutputWorkspace","",direction=Direction.Output))

    def PyExec(self):
        inst=self.getProperty('Instrument').value
        fname = self.getProperty('Filename').value

        diclookup = {\
            "AMOR":"amor.dic",
            "BOA":"boa.dic",
            "DMC":"dmc.dic",
            "FOCUS":"focus.dic",
            "HRPT":"hrpt.dic",
            "MARSI":"marsin.dic",
            "MARSE":"marse.dic",
            "POLDI":"poldi.dic",
            "RITA-2":"rita.dic",
            "SANS":"sans.dic",
            "SANS2":"sans.dic",
            "TRICS":"trics.dic"
        }
        dictsearch = os.path.join(config['instrumentDefinition.directory'],"nexusdictionaries")
        dicname = os.path.join(dictsearch, diclookup[inst])
        wname = "__tmp"
        ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,OutputWorkspace=wname)

        if inst == "POLDI":
            config.appendDataSearchDir(config['groupingFiles.directory'])
            grp_file = "POLDI_Grouping_800to400.xml"
            ws = mantid.simpleapi.GroupDetectors(InputWorkspace=ws,
                                                 OutputWorkspace=wname,
                                                 MapFile=grp_file, Behaviour="Sum")
        elif inst == "TRICS":
            ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,OutputWorkspace=wname)
            ws = mantid.simpleapi.SINQTranspose3D(ws,OutputWorkspace=wname)

        # Attach workspace to the algorithm property
        self.setProperty("OutputWorkspace", ws)
        # delete temporary reference
        mantid.simpleapi.DeleteWorkspace(wname,EnableLogging=False)

#---------- register with Mantid
AlgorithmFactory.subscribe(LoadSINQFile)
