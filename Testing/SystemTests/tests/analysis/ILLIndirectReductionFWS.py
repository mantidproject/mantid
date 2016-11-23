import stresstesting
from mantid.simpleapi import *
from mantid import config

class ILLIndirectReductionFWSTest(stresstesting.MantidStressTest):

    # cache default instrument and datadirs
    facility = config['default.facility']
    instrument = config['default.instrument']
    datadirs = config['datasearch.directories']

    def __init__(self):
        super(ILLIndirectReductionFWSTest, self).__init__()
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

        return ["083072.nxs","083073.nxs",
                "083074.nxs","083075.nxs",
                "083076.nxs","083077.nxs"]

    def runTest(self):

        self.tolerance = 1e-6

        IndirectILLReductionFWS(Run="083072:083073")

        self.tearDown()

    def validate(self):
        return ['red','ILLIN16B_FWS.nxs']
