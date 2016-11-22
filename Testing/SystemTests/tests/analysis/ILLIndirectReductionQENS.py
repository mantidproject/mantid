import stresstesting
from mantid.simpleapi import *
from mantid import config


class ILLIndirectReductionQENSTest(stresstesting.MantidStressTest):

    # cache default instrument and datadirs
    facility = config['default.facility']
    instrument = config['default.instrument']
    datadirs = config['datasearch.directories']

    def __init__(self):
        super(ILLIndirectReductionQENSTest, self).__init__()
        self.setUp()

    def setUp(self):
        # these must be set, so the required files
        # without instrument name can be retrieved
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN16B'
        config.appendDataSearchSubDir('ILL/IN16B/')

    def tearDown(self):
        config['default.facility'] = self.facility
        config['default.instrument'] = self.instrument
        config['datasearch.directories'] = self.datadirs

    def requiredFiles(self):

        return ["136553.nxs","136554.nxs",  # calibration vanadium files
                "136555.nxs","136556.nxs",  # alignment vanadium files
                "136599.nxs","136600.nxs",  # background (empty can)
                "136558.nxs","136559.nxs"]  # sample

    def runTest(self):

        self.tolerance = 1e-6

        self.disableChecking = ['Instrument']

        IndirectILLReductionQENS(Run="136558-136559",
                                 CalibrationRun="136553-136554",
                                 BackgroundRun="136599-136600",
                                 AlignmentRun="136555-136556",
                                 BackgroundScalingFactor=0.1,
                                 UnmirrorOption=7)

        self.tearDown()

    def validate(self):
        return ['136558_red','ILLIN16B_QENS.nxs']
