"""*WIKI*
Extracts the IPTS number from a run using FileFinde,findRuns. It returns a string the full path to the IPTS shared folder to allow for saving of files in accessible user folders
*WIKI*"""

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
import os


class GetIPTS(PythonAlgorithm):
    def getValidInstruments(self):
        instruments = ['']

        for name in ['SNS', 'HFIR']:
            facility = ConfigService.getFacility(name)
            facility = [item.shortName() for item in facility.instruments()]
            facility = [item for item in facility if item != 'DAS']
            facility.sort()
            instruments.extend(facility)

        return instruments

    def findFile(self, instrument, runnumber):
        # start with run and check the five before it
        runIds = range(runnumber, runnumber-6, -1)
        # check for one after as well
        runIds.append(runnumber + 1)

        runIds = [str(id) for id in runIds if id > 0]

        # prepend non-empty instrument name
        if len(instrument) > 0:
            runIds = ['%s_%s' % (instrument, id) for id in runIds]

        # look for a file
        for runId in runIds:
            self.log().information("Looking for '%s'" % runId)
            try:
                return FileFinder.findRuns(runId)[0]
            except RuntimeError:
                pass  # just keep looking

        # failed to find any is an error
        raise RuntimeError("Cannot find IPTS directory for '%s'"
                           % runnumber)

    def getIPTSLocal(self, instrument, runnumber):
        filename = self.findFile(instrument, runnumber)

        # convert to the path to the proposal
        location = filename.find('IPTS')
        location = filename.find('/', location)
        direc = filename[0:location+1]
        return direc

    def PyInit(self):
        self.declareProperty('RunNumber', defaultValue=0,
                             direction=Direction.Input,
                             validator=IntBoundedValidator(lower=0),
                             doc="Extracts the IPTS number for a run")

        instruments = self.getValidInstruments()
        self.declareProperty('Instrument', '',
                             StringListValidator(instruments),
                             "Empty uses default instrument")

        self.declareProperty('Directory', '',
                             direction=Direction.Output)

    def PyExec(self):
        instrument = self.getProperty('Instrument').value
        runnumber = self.getProperty('RunNumber').value

        direc = self.getIPTSLocal(instrument, runnumber)
        self.setPropertyValue('Directory', direc)
        self.log().notice('IPTS directory is: %s' % direc)

AlgorithmFactory.subscribe(GetIPTS)
