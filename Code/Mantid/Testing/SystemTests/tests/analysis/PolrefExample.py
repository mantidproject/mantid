#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *

class PolrefExample(stresstesting.MantidStressTest):
    ''' Sample script from Tim Charlton. Described as Mantid version of quick:lam

    Owen Arnold
    29/06/2012
    The analysis performed here is a subset of what is done in ReflectometryISIS.py.
    We may want to remove this test in the furture to avoid duplication. However,

    I'm leaving this in here for now because Tim Charlton suggests making the ReflectometryISIS.py
    test more generic for every reflectometry instrument.
    '''

    def runTest(self):
        LoadRaw(Filename="POLREF00003014.raw",OutputWorkspace="W",SpectrumMax="4",LoadMonitors="Separate")
        ConvertUnits(InputWorkspace="W_monitors",OutputWorkspace="M",Target="Wavelength",AlignBins="1")
        DeleteWorkspace(Workspace="W_monitors")
        CalculateFlatBackground(InputWorkspace="M",OutputWorkspace="M",WorkspaceIndexList="0,1,2",StartX="15",EndX="17")
        ConvertUnits(InputWorkspace="W",OutputWorkspace="D",Target="Wavelength",AlignBins="1")
        DeleteWorkspace(Workspace="W")
        OneMinusExponentialCor(InputWorkspace="D",OutputWorkspace="D",C="1.99012524619")
        ExponentialCorrection(InputWorkspace="D",OutputWorkspace="D",C1="0.0100836650034")
        PolynomialCorrection(InputWorkspace="D",OutputWorkspace="D",Coefficients="-1.3697,0.8602,-0.7839,0.2866,-0.0447,0.0025")
        ExponentialCorrection(InputWorkspace="M",OutputWorkspace="M",C1="0.42672",Operation="Multiply")
        CreateSingleValuedWorkspace(OutputWorkspace="shift",DataValue="3.16666666667")
        Plus(LHSWorkspace="M",RHSWorkspace="shift",OutputWorkspace="M")
        OneMinusExponentialCor(InputWorkspace="M",OutputWorkspace="M",C="0.42672")
        RebinToWorkspace(WorkspaceToRebin="M",WorkspaceToMatch="D",OutputWorkspace="M")
        CropWorkspace(InputWorkspace="M",OutputWorkspace="I0",StartWorkspaceIndex="2")
        DeleteWorkspace(Workspace="M")
        Divide(LHSWorkspace="D",RHSWorkspace="I0",OutputWorkspace="R")
        DeleteWorkspace(Workspace="D")
        DeleteWorkspace(Workspace="I0")

    def validate(self):
        # Need to disable checking of the Spectra-Detector map because it isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        return 'R_1','PolrefTest.nxs'
