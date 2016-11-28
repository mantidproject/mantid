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

        return ["165944.nxs", "165945.nxs", "165946.nxs", "165947.nxs", "165948.nxs",
                "165949.nxs", "165950.nxs", "165951.nxs", "165952.nxs", "165953.nxs",
                "143720.nxs", "143721.nxs", "143722.nxs", "143723.nxs", "143724.nxs",
                "143725.nxs", "143726.nxs", "143727.nxs", "143728.nxs", "143729.nxs"]

    def runTest(self):

        self.tolerance = 1e-6

        self.disableChecking = ['Instrument']

        # test EFWS+IFWS mixed
        IndirectILLReductionFWS(Run="165944:165953", SortXAxis=True, OutputWorkspace="ifws")

        # test EFWS with sum/interpolate options with background and calibration
        IndirectILLReductionFWS(Run="143720:143728:2",
                                BackgroundRun="143721,143723,143725",
                                CalibrationRun="143727,143729",
                                BackgroundOption="Interpolate",
                                CalibrationOption="Sum",
                                SortXAxis=True,
                                OutputWorkspace="efws")

        self.tearDown()

    def validate(self):
        return [['ifws','ILLIN16B_FWS.nxs'],
                ['efws','ILLIN16B_EFWS.nxs']]
