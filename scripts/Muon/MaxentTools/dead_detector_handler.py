
from __future__ import (absolute_import, division, print_function)
import numpy as np
import mantid.simpleapi as mantid


def removeDeadDetectors(ws):
    deadDetectors = []
    for j in range(0, ws.getNumberHistograms()):
        if np.count_nonzero(ws.readY(j)) == 0:
            deadDetectors.append(j + 1)
    if len(deadDetectors)==0:
        return ws,deadDetectors
    maskAlg = mantid.AlgorithmManager.create("MaskDetectors")
    maskAlg.initialize()
    maskAlg.setChild(True)
    maskAlg.setProperty("Workspace", ws)
    maskAlg.setProperty("SpectraList", deadDetectors)
    maskAlg.execute()

    rmAlg = mantid.AlgorithmManager.create("ExtractUnmaskedSpectra")
    rmAlg.initialize()
    rmAlg.setChild(True)
    rmAlg.setProperty("InputWorkspace", ws)
    rmAlg.setProperty("OutputWorkspace", "out")
    rmAlg.execute()

    new = rmAlg.getProperty("OutputWorkspace").value

    return new, deadDetectors
