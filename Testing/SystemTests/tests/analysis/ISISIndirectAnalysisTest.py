#pylint: disable=no-init,attribute-defined-outside-init
import stresstesting
from mantid.simpleapi import *


class ElasticWindowMultipleTest(stresstesting.MantidStressTest):

    def runTest(self):
        Load(Filename='osi92762_graphite002_red.nxs,osi92763_graphite002_red.nxs',
             OutputWorkspace='__ElWinMulti_InputWS')

        ElasticWindowMultiple(InputWorkspaces='__ElWinMulti_InputWS',
                              IntegrationRangeStart=-0.2,
                              IntegrationRangeEnd=0.2,
                              BackgroundRangeStart='-0.24',
                              BackgroundRangeEnd='-0.22',
                              OutputInQ='eq',
                              OutputInQSquared='eq2',
                              OutputELF='elf',
                              OutputELT='elt')

        GroupWorkspaces(InputWorkspaces=['elf', 'elt'],
                        OutputWorkspace='__ElWinMulti_OutputWS')

    def validate(self):
        self.tolerance = 1e-10
        return '__ElWinMulti_OutputWS', 'II.AnalysisElwinMulti.nxs'
