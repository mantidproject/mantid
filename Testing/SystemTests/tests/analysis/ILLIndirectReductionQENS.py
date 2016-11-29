import stresstesting
from mantid.simpleapi import *
from mantid import config
from testhelpers import run_algorithm


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

    def test_unmirror_0_1_2_3(self):

        args = {'Run' : '136553.nxs',
                'UnmirrorOption' : 0,
                'OutputWorkspace' : 'both'}

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 0")

        args['UnmirrorOption'] = 1

        args['OutputWorkspace'] = 'red'

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 1")

        args['UnmirrorOption'] = 2

        args['OutputWorkspace'] = 'left'

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 2")

        args['UnmirrorOption'] = 3

        args['OutputWorkspace'] = 'right'

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 3")

        summed = Plus(mtd['left'].getItem(0),mtd['right'].getItem(0))

        Scale(InputWorkspace=summed,Factor=0.5,OutputWorkspace=summed)

        result = CompareWorkspaces(summed,mtd['red'].getItem(0))

        self.assertTrue(result[0],"Unmirror 1 should be the sum of 2 and 3")

        left_right = GroupWorkspaces([mtd['left'].getItem(0).getName(), mtd['right'].getItem(0).getName()])

        result = CompareWorkspaces(left_right,'both')

        self.assertTrue(result[0],"Unmirror 0 should be the group of 2 and 3")

    def test_unmirror_4_5(self):

        args = {'Run': '136553.nxs',
                'UnmirrorOption': 4,
                'OutputWorkspace': 'vana4'}

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 4")

        args['AlignmentRun'] = '136553.nxs'

        args['UnmirrorOption'] = 5

        args['OutputWorkspace'] = 'vana5'

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 5")

        result = CompareWorkspaces('vana4', 'vana5')

        self.assertTrue(result[0], "Unmirror 4 should be the same as 5 if "
                                   "the same run is also defined as alignment run")

    def test_unmirror_6_7(self):

        args = {'Run': '136553.nxs',
                'UnmirrorOption': 6,
                'OutputWorkspace': 'vana6'}

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 6")

        args['AlignmentRun'] = '136553.nxs'

        args['UnmirrorOption'] = 7

        args['OutputWorkspace'] = 'vana7'

        alg_test = run_algorithm('IndirectILLReductionQENS', **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionQENS not executed for unmirror 7")

        result = CompareWorkspaces('vana6','vana7')

        self.assertTrue(result[0], "Unmirror 6 should be the same as 7 if "
                                   "the same run is also defined as alignment run")

    def runTest(self):

        self.test_unmirror_0_1_2_3()

        self.test_unmirror_4_5()

        self.test_unmirror_6_7()

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
