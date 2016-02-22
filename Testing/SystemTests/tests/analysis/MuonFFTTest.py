#pylint: disable=no-init,attribute-defined-outside-init
import stresstesting
from mantid.simpleapi import *
from math import pi

class MuonFFTTest(stresstesting.MantidStressTest):
    '''Tests the FFT algorithm on a MUSR workspace, to check it can cope with rounding errors in X'''

    def runTest(self):
        Load(Filename='MUSR00022725.nxs', OutputWorkspace='MUSR00022725')
        CropWorkspace(InputWorkspace='MUSR00022725', OutputWorkspace='MUSR00022725', XMin=0, XMax=4, EndWorkspaceIndex=63)

        # create a PhaseTable with detector information
        tab = CreateEmptyTableWorkspace()
        tab.addColumn('int', 'DetID')
        tab.addColumn('double', 'Phase')
        tab.addColumn('double', 'Asym')
        for i in range(0,32):
            phi = 2*pi*i/32.
            tab.addRow([i + 1, 0.2, phi])
        for i in range(0,32):
            phi = 2*pi*i/32.
            tab.addRow([i + 33, 0.2, phi])
        ows = PhaseQuad(InputWorkspace='MUSR00022725', PhaseTable='tab')

        FFT(ows, Real=0, Imaginary=1, AcceptXRoundingErrors=True, OutputWorkspace='MuonFFTResults')

    def validate(self):
        self.tolerance = 1E-1
        return ('MuonFFTResults','MuonFFTMUSR00022725.nxs')
