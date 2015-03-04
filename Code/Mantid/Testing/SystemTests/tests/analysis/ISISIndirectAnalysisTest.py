import stresstesting
import os
from mantid.simpleapi import *
from IndirectImport import is_supported_f2py_platform


class ElasticWindowMultipleTest(stresstesting.MantidStressTest):

    def runTest(self):
        Load(Filename='osi92762_graphite002_red.nxs,osi92763_graphite002_red.nxs',
            OutputWorkspace='__ElWinMulti_InputWS')

        ElasticWindowMultiple(
            InputWorkspaces='__ElWinMulti_InputWS',
            Range1Start=-0.2,
            Range1End=0.2,
            Range2Start='-0.24',
            Range2End='-0.22',
            OutputInQ='eq',
            OutputInQSquared='eq2',
            OutputELF='elf',
            OutputELT='elt')

        GroupWorkspaces(InputWorkspaces=['elf', 'elt'],
            OutputWorkspace='__ElWinMulti_OutputWS')

        SaveNexus(Filename='__ElWinMulti_OutputWS', InputWorkspace='__ElWinMulti_OutputWS')

    def validate(self):
        self.tolerance = 1e-10
        return '__ElWinMulti_OutputWS', 'II.AnalysisElwinMulti.nxs'
