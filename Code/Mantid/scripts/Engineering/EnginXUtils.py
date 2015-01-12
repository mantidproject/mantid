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

def getExpectedPeakDefaults():
    ExpectPeakDefault = [3.1243, 2.7057, 1.9132,1.6316, 1.5621, 1.3529, 1.2415,1.2100, 1.1046, 1.0414, 0.9566, 0.9147, 0.9019, 0.8556, 0.8252, 0.8158, 0.7811]
    return ExpectPeakDefault