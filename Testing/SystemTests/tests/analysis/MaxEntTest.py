#pylint: disable=no-init,attribute-defined-outside-init
import stresstesting
from mantid.simpleapi import *


class MaxEntTest(stresstesting.MantidStressTest):
    '''Tests the MaxEnt algorithm on a MUSR workspace'''

    def runTest(self):
        Load(Filename='MUSR00022725.nxs', OutputWorkspace='MUSR00022725')
        CropWorkspace(InputWorkspace='MUSR00022725', OutputWorkspace='MUSR00022725', XMin=0.11, XMax=4, EndWorkspaceIndex=0)
        RemoveExpDecay(InputWorkspace='MUSR00022725', OutputWorkspace='MUSR00022725')
        Rebin(InputWorkspace='MUSR00022725', OutputWorkspace='MUSR00022725', Params='0.016')
        _evolChi, _evolAngle, _image, _data = MaxEnt(InputWorkspace='MUSR00022725', A=0.005, ChiTarget=200)
        GroupWorkspaces(InputWorkspaces='_evolAngle,_image,_data',
                        OutputWorkspace='MaxEntResults')

    def validate(self):
        self.tolerance = 5E-2
        return ('MaxEntResults','MaxEntMUSR00022725.nxs')
