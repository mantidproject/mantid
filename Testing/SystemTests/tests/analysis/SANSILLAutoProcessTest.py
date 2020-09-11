# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import SANSILLAutoProcess, GroupWorkspaces, \
                             SaveNexusProcessed, LoadNexusProcessed, config, mtd
import systemtesting
from tempfile import gettempdir
import os


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
        # this also tests that already loaded workspace can be passed instead of a file
        LoadNexusProcessed(Filename='sens-lamp.nxs', OutputWorkspace='sens-lamp')
        for i in range(len(samples)):
            SANSILLAutoProcess(
                SampleRuns=samples[i],
                BeamRuns=beams,
                ContainerRuns=containers,
                MaskFiles='mask1.nxs,mask2.nxs,mask3.nxs',
                SensitivityMaps='sens-lamp',
                SampleTransmissionRuns=sample_tr[i],
                ContainerTransmissionRuns=container_tr,
                TransmissionBeamRuns=beam_tr,
                SampleThickness=thick[i],
                CalculateResolution='MildnerCarpenter',
                OutputWorkspace='iq_s' + str(i + 1)
            )

        GroupWorkspaces(InputWorkspaces=['iq_s1', 'iq_s2', 'iq_s3'], OutputWorkspace='out')


class D11_AutoProcess_Wedges_Test(systemtesting.MantidSystemTest):
    """
    AutoProcess test with wedges for d11 data.
    """

    def __init__(self):
        super(D11_AutoProcess_Wedges_Test, self).__init__()
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
        return ['out', 'D11_AutoProcess_Wedges_Reference.nxs']

    def runTest(self):

        beams = '2866,2867+2868,2878'
        containers = '2888+2971,2884+2960,2880+2949'
        container_tr = '2870+2954'
        beam_tr = '2867+2868'
        sample = '3187,3177,3167'
        sample_tr = '2869'
        thick = 0.2

        # reduce samples
        # this also tests that already loaded workspace can be passed instead of a file
        LoadNexusProcessed(Filename='sens-lamp.nxs', OutputWorkspace='sens-lamp')
        SANSILLAutoProcess(
            SampleRuns=sample,
            BeamRuns=beams,
            ContainerRuns=containers,
            MaskFiles='mask1.nxs,mask2.nxs,mask3.nxs',
            SensitivityMaps='sens-lamp',
            SampleTransmissionRuns=sample_tr,
            ContainerTransmissionRuns=container_tr,
            TransmissionBeamRuns=beam_tr,
            SampleThickness=thick,
            CalculateResolution='MildnerCarpenter',
            NumberOfWedges=4,
            OutputWorkspace='iq'
            )

        # here, 4 symmetric wedges with an offset of 0 will produce 2 times the
        # same outputs. NumberOfWedges=2 would have made more sense.

        GroupWorkspaces(
            InputWorkspaces=['iq_1', 'iq_2', 'iq_3',
                             'iq_wedge_1_1', 'iq_wedge_1_2', 'iq_wedge_1_3',
                             'iq_wedge_2_1', 'iq_wedge_2_2', 'iq_wedge_2_3',
                             'iq_wedge_3_1', 'iq_wedge_3_2', 'iq_wedge_3_3',
                             'iq_wedge_4_1', 'iq_wedge_4_2', 'iq_wedge_4_3'],
            OutputWorkspace='out'
            )


class D11_AutoProcess_IQxQy_Test(systemtesting.MantidSystemTest):
    """
    Tests auto process for D11 with output type as I(Qx, Qy).
    """

    def __init__(self):
        super(D11_AutoProcess_IQxQy_Test, self).__init__()
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
        return ['iqxy', 'D11_AutoProcess_IQxQy_Reference.nxs']

    def runTest(self):
        SANSILLAutoProcess(
            SampleRuns='3187,3177,3167',
            BeamRuns='2866,2867+2868,2878',
            ContainerRuns='2888+2971,2884+2960,2880+2949',
            MaskFiles='mask1.nxs,mask2.nxs,mask3.nxs',
            SensitivityMaps='sens-lamp.nxs',
            SampleTransmissionRuns='3172',
            ContainerTransmissionRuns='2870+2954',
            TransmissionBeamRuns='2867+2868',
            SampleThickness=0.2,
            CalculateResolution='MildnerCarpenter',
            OutputWorkspace='iqxy',
            OutputType='I(Qx,Qy)'
        )


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
            OutputPanels=True
        )

        GroupWorkspaces(InputWorkspaces=['iq', 'iq_panels'], OutputWorkspace='out')


class D33_AutoProcess_IPhiQ_Test(systemtesting.MantidSystemTest):
    """
    Tests auto process with D33 data.
    Separation of panels and I(Phi, Q) output.
    """

    def __init__(self):
        super(D33_AutoProcess_IPhiQ_Test, self).__init__()
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
        return ['out', 'D33_AutoProcess_IPhiQ_Reference.nxs']

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
            OutputWorkspace='iphiq',
            OutputPanels=True,
            NumberOfWedges=60,
            OutputType='I(Phi,Q)'
        )

        GroupWorkspaces(InputWorkspaces=['iphiq', 'iphiq_panels'],
                        OutputWorkspace='out')


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

        return ['iq', 'ILL_D16_Gamma_scan.nxs']

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
                           AbsorberRuns=absorber,
                           ClearCorrected2DWorkspace=False)
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
