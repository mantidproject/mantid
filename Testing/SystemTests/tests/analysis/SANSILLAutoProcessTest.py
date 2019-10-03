# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import systemtesting
from mantid.simpleapi import SANSILLAutoProcess, config, mtd, GroupWorkspaces


class D11_AutoProcess_Test(systemtesting.MantidSystemTest):

    def __init__(self):
        super(D11_AutoProcess_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D11'
        config['logging.loggers.root.level'] = 'Warning'
        config.appendDataSearchSubDir('ILL/D11/')

    def tearDown(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        return ['out', 'D11_AutoProcess_Reference.nxs']

    def runTest(self):

        beams = '2866,2867+2868,2878'
        containers = '2888+2971,2884+2960,2880+2949'
        container_tr = '2870+2954'
        beam_tr = '2867+2868'
        samples = ['2889,2885,2881',
                   '2887,2883,2879',
                   '3187,3177,3167']
        sample_tr = ['2871', '2869', '3172']
        thick = [0.1, 0.2, 0.2]

        # reduce samples
        for i in range(len(samples)):
            SANSILLAutoProcess(
                SampleRuns=samples[i],
                BeamRuns=beams,
                ContainerRuns=containers,
                MaskFiles='mask1.nxs,mask2.nxs,mask3.nxs',
                SensitivityMaps='sens-lamp.nxs',
                SampleTransmissionRuns=sample_tr[i],
                ContainerTransmissionRuns=container_tr,
                TransmissionBeamRuns=beam_tr,
                SampleThickness=thick[i],
                OutputWorkspace='iq_s' + str(i + 1)
            )

        GroupWorkspaces(InputWorkspaces=['iq_s1', 'iq_s2', 'iq_s3'], OutputWorkspace='out')
