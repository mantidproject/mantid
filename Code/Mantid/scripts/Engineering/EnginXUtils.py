#pylint: disable=invalid-name
import os

from mantid.api import *
import mantid.simpleapi as sapi

def getDetIDsForBank(bank):
    """ Returns detector IDs corresponding to the specified EnginX bank number
    """
    groupingFilePath = os.path.join(sapi.config.getInstrumentDirectory(),
                                    'Grouping', 'ENGINX_Grouping.xml')

    alg = AlgorithmManager.create('LoadDetectorsGroupingFile')
    alg.setProperty('InputFile', groupingFilePath)
    alg.setProperty('OutputWorkspace', '__EnginXGrouping')
    alg.execute()

    grouping = mtd['__EnginXGrouping']

    detIDs = set()

    for i in range(grouping.getNumberHistograms()):
        if grouping.readY(i)[0] == bank:
            detIDs.add(grouping.getDetector(i).getID())

    sapi.DeleteWorkspace(grouping)

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
            return det.getID() in detIDs
        except RuntimeError:
            return False

    return [i for i in range(0, ws.getNumberHistograms()) if isIndexInBank(i)]

def  generateOutputParTable(self, name, difc, zero):
    """
    Produces a table workspace with the two fitted calibration parameters

    @param name :: the name to use for the table workspace that is created here
        """
    tbl = sapi.CreateEmptyTableWorkspace(OutputWorkspace=name)
    tbl.addColumn('double', 'difc')
    tbl.addColumn('double', 'zero')
    tbl.addRow([float(difc), float(zero)])
