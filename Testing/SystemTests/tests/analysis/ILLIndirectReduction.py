import stresstesting
from mantid.simpleapi import *
from mantid import config

class ILLIndirectReductionTest(stresstesting.MantidStressTest):

    tolerance = 0.0001
    # cache default instrument and datadirs
    facility = config['default.facility']
    instrument = config['default.instrument']
    datadirs = config['datasearch.directories']

    def __init__(self):
        super(ILLIndirectReductionTest, self).__init__()

    def requiredFiles(self):

        # these must be set, so the required files without instrument name can be retrieved
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN16B'
        config.appendDataSearchSubDir('ILL/IN16B/')

        return ["090658.nxs","090659.nxs","090660.nxs","090661.nxs","090662.nxs","090663.nxs",
                "091497.nxs","091515.nxs","091516.nxs","IN16B_QENS_RESULT.nxs"]

    def runTest(self):

        calib = ILLIN16BCalibration("090662-090663")
        result = IndirectILLReduction(Run="091515-091516",
                                      UnmirrorOption=7,
                                      CalibrationWorkspace='calib',
                                      BackgroundRun="090658")
        # tear down
        config['default.facility'] = self.facility
        config['default.instrument'] = self.instrument
        config['datasearch.directories'] = self.datadirs

    def validate(self):
        return ['091515_result','IN16B_QENS_RESULT.nxs']
