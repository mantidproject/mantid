#pylint: disable=no-init

from __future__ import (absolute_import, division, print_function)
import stresstesting
from mantid.simpleapi import *
from mantid import config
import SANSBatchMode as bm
import os.path
import ISISCommandInterface as ii
import sans.command_interface.ISISCommandInterface as ii2


class SANSLOQBatch(stresstesting.MantidStressTest):

    def runTest(self):
    #DataPath("../Data/LOQ/")
    #UserPath("../Data/LOQ/")

    #here we are testing the LOQ setup
        ii.LOQ()
    #rear detector
        ii.Detector("main-detector-bank")
    #test batch mode, although only the analysis from the last line is checked
    # Find the file , this should really be in the BatchReduce reduction step
        csv_file = FileFinder.getFullPath('batch_input.csv')

        ii.Set1D()
        ii.MaskFile('MASK.094AA')
        ii.Gravity(True)

        bm.BatchReduce(csv_file, 'raw', plotresults=False, saveAlgs={'SaveCanSAS1D':'xml','SaveNexus':'nxs'})

        LoadNexus(Filename='54433sans.nxs',OutputWorkspace= 'result')
        Plus(LHSWorkspace='result',RHSWorkspace= '99630sanotrans',OutputWorkspace= 'result')

        os.remove(os.path.join(config['defaultsave.directory'],'54433sans.nxs'))
        os.remove(os.path.join(config['defaultsave.directory'],'99630sanotrans.nxs'))
        os.remove(os.path.join(config['defaultsave.directory'],'54433sans.xml'))
        os.remove(os.path.join(config['defaultsave.directory'],'99630sanotrans.xml'))

    def validate(self):
    # Need to disable checking of the Spectra-Detector map because it isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')

        return 'result','SANSLOQBatch.nxs'


class SANSLOQBatchTest_V2(stresstesting.MantidStressTest):

    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.LOQ()
        ii2.Detector("main-detector-bank")
        csv_file = FileFinder.getFullPath('batch_input.csv')

        ii2.Set1D()
        ii2.MaskFile('MASK.094AA')
        ii2.Gravity(True)

        ii2.BatchReduce(csv_file, 'raw', plotresults=False, saveAlgs={'SaveCanSAS1D': 'xml', 'SaveNexus': 'nxs'})

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
