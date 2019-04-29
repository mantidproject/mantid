# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from mantid.simpleapi import (ElasticWindowMultiple, GroupWorkspaces, Load)


class ElasticWindowMultipleOSIRISTest(systemtesting.MantidSystemTest):

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


class ElasticWindowMultipleIRISTest(systemtesting.MantidSystemTest):

    def runTest(self):
        input_files = Load(Filename='irs26173_graphite002_red.nxs,irs26176_graphite002_red.nxs')

        ElasticWindowMultiple(InputWorkspaces=input_files,
                              IntegrationRangeStart=-0.2,
                              IntegrationRangeEnd=0.2,
                              BackgroundRangeStart='-0.24',
                              BackgroundRangeEnd='-0.22',
                              OutputInQ='eq',
                              OutputInQSquared='eq2',
                              OutputELF='elf')

        GroupWorkspaces(InputWorkspaces=['eq', 'eq2', 'elf'],
                        OutputWorkspace='__ElwinResultsGroup')

    def validate(self):
        self.tolerance = 1e-10
        return '__ElwinResultsGroup', 'II.IRISElasticWindowMultiple.nxs'
