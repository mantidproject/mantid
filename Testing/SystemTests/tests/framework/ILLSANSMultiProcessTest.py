# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import *


class ILL_SANS_D11B_MONO_MULTI_TEST(systemtesting.MantidSystemTest):
    '''
    Tests a standard monochromatic reduction with the multiprocess algorithm for the new D11B data.
    Tests 2 samples (H2O and D2O) measured at 3 distances with the same wavelength.
    '''

    def __init__(self):
        super(ILL_SANS_D11B_MONO_MULTI_TEST, self).__init__()
        self.setUp()
        self.facility = config['default.facility']
        self.instrument = config['default.instrument']
        self.directories = config['datasearch.directories']

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D11'
        config.appendDataSearchSubDir('ILL/D11B/')

    def cleanup(self):
        mtd.clear()
        config['default.facility'] = self.facility
        config['default.instrument'] = self.instrument
        config['datasearch.directories'] = self.directories

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['water', 'ILL_SANS_D11B_MONO_MULTI.nxs']

    def runTest(self):

        cadmiums = ['8551', '8566', '8581']
        empty_beams = ['8552', '8567', '8582']
        tr_beam = '8538'
        can_tr = '8535'
        cans = ['8550', '8565', '8580']
        edge_mask = 'edge_mask_2p5m'
        masks = ['bs_mask_1p7m', 'bs_mask_2p0m', 'bs_mask_2p5m']

        samplesD1 = '8539,8549'
        samplesD2 = '8554,8564'
        samplesD3 = '8569,8579'
        sampleTrs = '8524,8534'

        SANSILLMultiProcess(SampleRunsD1=samplesD1,
                            SampleRunsD2=samplesD2,
                            SampleRunsD3=samplesD3,
                            SampleTrRunsW1=sampleTrs,
                            EmptyBeamRuns=','.join(empty_beams),
                            DarkCurrentRuns=','.join(cadmiums),
                            TrEmptyBeamRuns=tr_beam,
                            ContainerTrRuns=can_tr,
                            EmptyContainerRuns=','.join(cans),
                            DefaultMask=edge_mask,
                            BeamStopMasks=','.join(masks),
                            SampleNamesFrom='Nexus',
                            OutputWorkspace='water')
