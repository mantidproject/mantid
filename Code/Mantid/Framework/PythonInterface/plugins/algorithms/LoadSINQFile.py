#pylint: disable=no-init,invalid-name
#--------------------------------------------------------------
# Algorithm which loads a SINQ file. It matches the instrument
# and the right dictionary file and then goes away and calls
# LoadFlexiNexus and others to load the file.
#
# Mark Koennecke, November 2012
#--------------------------------------------------------------
from mantid.api import AlgorithmFactory
from mantid.api import PythonAlgorithm, FileProperty, FileAction, WorkspaceProperty
from mantid.kernel import Direction, StringListValidator
import mantid.simpleapi
from mantid import config
import os.path
import numpy as np
import re

#--------- place to look for dictionary files


class LoadSINQFile(PythonAlgorithm):
    def category(self):
        return "DataHandling;PythonAlgorithms"

    def summary(self):
        return "Load a SINQ file with the right dictionary."

    def PyInit(self):
        global dictsearch
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
            "POLDI_legacy":"poldi_legacy.dic",
            "POLDI":"poldi.dic",
            "RITA-2":"rita.dic",
            "SANS":"sans.dic",
            "SANS2":"sans.dic",
            "TRICS":"trics.dic"\
        }

        lookupInstrumentName = inst
        if inst == 'POLDI':
            lookupInstrumentName = self._getPoldiLookupName(fname, lookupInstrumentName)

        dictsearch = os.path.join(config['instrumentDefinition.directory'],"nexusdictionaries")
        dicname = os.path.join(dictsearch, diclookup[lookupInstrumentName])
        wname = "__tmp"
        ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,OutputWorkspace=wname)

        if inst == "POLDI":
            if ws.getNumberHistograms() == 800:
                ws.maskDetectors(SpectraList=range(0,800)[::2])

                config.appendDataSearchDir(config['groupingFiles.directory'])
                grp_file = "POLDI_Grouping_800to400.xml"
                ws = mantid.simpleapi.GroupDetectors(InputWorkspace=ws,
                                                     OutputWorkspace=wname,
                                                     MapFile=grp_file, Behaviour="Sum")

            # Reverse direction of POLDI data so that low index corresponds to low 2theta.
            histogramCount = ws.getNumberHistograms()
            oldYData = []
            for i in range(histogramCount):
                oldYData.append([x for x in ws.readY(i)])

            for i in range(histogramCount):
                ws.setY(i, np.array(oldYData[histogramCount - 1 - i]))

        elif inst == "TRICS":
            ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,OutputWorkspace=wname)
            ws = mantid.simpleapi.SINQTranspose3D(ws,OutputWorkspace=wname)

        # Attach workspace to the algorithm property
        self.setProperty("OutputWorkspace", ws)
        # delete temporary reference
        mantid.simpleapi.DeleteWorkspace(wname,EnableLogging=False)

    def _getPoldiLookupName(self, fname, lookupInstrumentName):
        year = self._extractYearFromFileName(fname)
        if year < 2015:
            lookupInstrumentName += '_legacy'
        return lookupInstrumentName

    def _extractYearFromFileName(self, filename):
        pureFileName = os.path.basename(filename)
        pattern = re.compile("\w+(\d{4})n[\w\d\.]+")
        matches = re.match(pattern, pureFileName)

        return int(matches.group(1))

#---------- register with Mantid
AlgorithmFactory.subscribe(LoadSINQFile)
