import os

from mantid.api import *
from mantid.simpleapi import *

def getDetIDsForBank(bank):
    """ Returns detector IDs corresponding to the specified EnginX bank number
    """
    groupingFilePath = os.path.join(config.getInstrumentDirectory(), 'Grouping', 'ENGINX_Grouping.xml')

    alg = AlgorithmManager.create('LoadDetectorsGroupingFile')
    alg.setProperty('InputFile', groupingFilePath)
    alg.setProperty('OutputWorkspace', '__EnginXGrouping')
    alg.execute()

    grouping = mtd['__EnginXGrouping']

    detIDs = set()

    for i in range(grouping.getNumberHistograms()):
        if grouping.readY(i)[0] == bank:
            detIDs.add(grouping.getDetector(i).getID())

    DeleteWorkspace(grouping)

    if len(detIDs) == 0:
        raise Exception('Unknown bank')

    return detIDs

def getWsIndicesForBank(bank, ws):
    """ Get a list of workspace indices which correspond to the specified bank
    """
    detIDs = getDetIDsForBank(bank)

    def isIndexInBank(index):
        try:
            det = ws.getDetector(index)
            return (det.getID() in detIDs)
        except:
            return False

    return filter(isIndexInBank, range(0, ws.getNumberHistograms()))


