import stresstesting
from mantid.simpleapi import *
from mantid import config

class ILLIndirectReductionTest(stresstesting.MantidStressTest):

    # cache default instrument and datadirs
    facility = config['default.facility']
    instrument = config['default.instrument']
    datadirs = config['datasearch.directories']

    def __init__(self):
        super(ILLIndirectReductionTest, self).__init__()
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
                "136555.nxs","136556.nxs","136557.nxs",  # alignment vanadium files
                "136599.nxs","136600.nxs",  # background (empty can)
                "136558.nxs","136559.nxs","136560.nxs",  # sample
                "136645.nxs","136646.nxs","136647.nxs"]  # D20

    def runTest(self):

        self.tolerance = 1e-4
        self.disableChecking = ['Masking','Instrument']

        calib = ILLIN16BCalibration("136553-136554")
        result = IndirectILLReduction(Run="136558-136560",
                                      UnmirrorOption=7,
                                      CalibrationWorkspace='calib',
                                      BackgroundRun="136599",
                                      VanadiumRun="136555")

        self.tearDown()

    def validate(self):
        return ['136558_result','ILLIN16B_QENS.nxs']
