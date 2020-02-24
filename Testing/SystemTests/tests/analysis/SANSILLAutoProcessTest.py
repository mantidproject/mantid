# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import systemtesting
from mantid.simpleapi import SANSILLAutoProcess, config, mtd, GroupWorkspaces, SaveNexusProcessed
import os
from tempfile import gettempdir


class D11_AutoProcess_Test(systemtesting.MantidSystemTest):
    """
    Tests auto process for D11 with 3 samples at 3 different distances
    """

    def __init__(self):
        super(D11_AutoProcess_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D11'
        config['logging.loggers.root.level'] = 'Warning'
        config.appendDataSearchSubDir('ILL/D11/')

    def cleanup(self):
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
                CalculateResolution='MildnerCarpenter',
                OutputWorkspace='iq_s' + str(i + 1)
            )

        GroupWorkspaces(InputWorkspaces=['iq_s1', 'iq_s2', 'iq_s3'], OutputWorkspace='out')


class D33_AutoProcess_Test(systemtesting.MantidSystemTest):
    """
    Tests auto process with D33 monochromatic data
    One sample at one angle, with separation of the panels
    Uses the measurement of Pluronic F127 D20 Anethol
    """

    def __init__(self):
        super(D33_AutoProcess_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D33'
        config['logging.loggers.root.level'] = 'Warning'
        config.appendDataSearchSubDir('ILL/D33/')

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        return ['out', 'D33_AutoProcess_Reference.nxs']

    def runTest(self):

        absorber = '002227'
        tr_beam = '002192'
        can_tr = '002193'
        empty_beam = '002219'
        can = '002228'
        mask = 'D33Mask2.nxs'

        SANSILLAutoProcess(
            SampleRuns='001464',
            SampleTransmissionRuns='002197',
            MaskFiles=mask,
            AbsorberRuns=absorber,
            BeamRuns=empty_beam,
            ContainerRuns=can,
            ContainerTransmissionRuns=can_tr,
            TransmissionBeamRuns=tr_beam,
            OutputWorkspace='iq',
            PanelOutputWorkspaces='panels'
        )

        GroupWorkspaces(InputWorkspaces=['iq', 'panels'], OutputWorkspace='out')


class D16_AutoProcess_Test(systemtesting.MantidSystemTest):
    """
    Tests autoprocess with D16 data, with a scan on 3 consecutives gamma values.
    """
    def __init__(self):
        super(D16_AutoProcess_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D16'
        config['logging.loggers.root.level'] = 'Warning'
        config.appendDataSearchSubDir('ILL/D16/')
        config['algorithms.retained'] = '0'

    def cleanup(self):
        mtd.clear()
        for i in range(3):
            os.remove(os.path.join(gettempdir(), 'water_reference_g' + str(i) + '.nxs'))

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking.append("Instrument")

        return ['iq', 'ILL_D16_scan.nxs']

    def runTest(self):
        water = '3659, 3663, 3667'
        sample = "3674, 3677, 3680"
        transmission_sample = '3671'
        beam = '3587'
        transmission_water = '3655'
        transmission_water_cell = '3592'
        transmission_beam = '3587'
        absorber = '3598, 3604, 3654'
        empty_cell_water = '3618, 3623, 3646'
        cell_background = '3676, 3679, 3682'
        transmission_empty_cell = '3673'

        # first process the water
        SANSILLAutoProcess(SampleRuns=water,
                           BeamRuns=beam,
                           DefaultMaskFile="side_mask.nxs",
                           MaskFiles="beam_mask.nxs, side_mask.nxs, side_mask.nxs",
                           TransmissionBeamRuns=transmission_beam,
                           SampleTransmissionRuns=transmission_water,
                           ContainerTransmissionRuns=transmission_water_cell,
                           OutputWorkspace='water',
                           BeamRadius=1,
                           ContainerRuns=empty_cell_water,
                           ThetaDependent=False,
                           WaterCrossSection=0.87,
                           SampleThickness=0.2,
                           AbsorberRuns=absorber)
        tmp_dir = gettempdir()
        water_dir = [os.path.join(tmp_dir, 'water_reference_g' + str(i) + '.nxs') for i in range(3)]
        SaveNexusProcessed('003659_Sample', water_dir[0])
        SaveNexusProcessed('003663_Sample', water_dir[1])
        SaveNexusProcessed('003667_Sample', water_dir[2])

        # then process the sample
        SANSILLAutoProcess(SampleRuns=sample,
                           BeamRuns=beam,
                           DefaultMaskFile="side_mask",
                           MaskFiles="beam_mask, side_mask, side_mask",
                           TransmissionBeamRuns=transmission_beam,
                           OutputWorkspace='iq',
                           ContainerTransmissionRuns=transmission_empty_cell,
                           SampleTransmissionRuns=transmission_sample,
                           ContainerRuns=cell_background,
                           AbsorberRuns=absorber,
                           ThetaDependent=False,
                           WaterCrossSection=0.87,
                           SampleThickness=0.2,
                           BeamRadius=1,
                           ReferenceFiles=",".join(water_dir))


