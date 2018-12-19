# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init

from __future__ import (absolute_import, division, print_function)
import systemtesting
import os.path
from mantid.kernel import config
from mantid.api import FileFinder
from mantid.simpleapi import (LoadNexus, Plus)
from sans.command_interface.ISISCommandInterface import (LOQ, Detector, Set1D, MaskFile, Gravity,
                                                         BatchReduce, UseCompatibilityMode)


class SANSLOQBatchTest_V2(systemtesting.MantidSystemTest):

    def runTest(self):
        UseCompatibilityMode()
        LOQ()
        Detector("main-detector-bank")
        csv_file = FileFinder.getFullPath('batch_input.csv')

        Set1D()
        MaskFile('MASK.094AA')
        Gravity(True)

        BatchReduce(csv_file, 'raw', plotresults=False, saveAlgs={'SaveCanSAS1D': 'xml', 'SaveNexus': 'nxs'})

        LoadNexus(Filename='54433sans.nxs', OutputWorkspace='result')
        Plus(LHSWorkspace='result', RHSWorkspace='99630sanotrans', OutputWorkspace= 'result')

        os.remove(os.path.join(config['defaultsave.directory'],'54433sans.nxs'))
        os.remove(os.path.join(config['defaultsave.directory'],'99630sanotrans.nxs'))
        os.remove(os.path.join(config['defaultsave.directory'],'54433sans.xml'))
        os.remove(os.path.join(config['defaultsave.directory'],'99630sanotrans.xml'))

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')

        return 'result','SANSLOQBatch.nxs'
