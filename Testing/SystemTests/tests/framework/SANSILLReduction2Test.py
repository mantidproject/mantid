# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import *


class ILL_SANS_D11_MONO_TEST(systemtesting.MantidSystemTest):
    '''
    Tests a standard monochromatic reduction with the v2 of the algorithm and data from the old D11
    '''

    def __init__(self):
        super(D11_Mono_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D11'
        config.appendDataSearchSubDir('ILL/D11/')

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['out', 'ILL_SANS_D11_MONO.nxs']

    def runTest(self):
        # Load a pre-drawn mask
        LoadNexusProcessed(Filename='D11_mask.nxs',
                           OutputWorkspace='mask')
        # First reduce the water measured at 8m
        SANSILLReduction(Runs='010455',
                         ProcessAs='DarkCurrent',
                         OutputWorkspace='cad')
        # Process the empty beam for water
        SANSILLReduction(Runs='010414',
                         ProcessAs='EmptyBeam',
                         DarkCurrentWorkspace='cad',
                         OutputWorkspace='mt',
                         OutputFluxWorkspace='flux')
        # Water container transmission
        SANSILLReduction(Runs='010446',
                         ProcessAs='Transmission',
                         DarkCurrentWorkspace='cad',
                         EmptyBeamWorkspace='mt',
                         FluxWorkspace='flux',
                         OutputWorkspace='wc_tr')
        # Water container
        SANSILLReduction(Runs='010454',
                         ProcessAs='EmptyContainer',
                         DarkCurrentWorkspace='cad',
                         EmptyBeamWorkspace='mt',
                         TransmissionWorkspace='wc_tr',
                         OutputWorkspace='wc')
        # Water transmission
        SANSILLReduction(Runs='010445',
                         ProcessAs='Transmission',
                         DarkCurrentWorkspace='cad',
                         EmptyBeamWorkspace='mt',
                         FluxWorkspace='flux',
                         OutputWorkspace='w_tr')
        # Water
        SANSILLReduction(Runs='010453',
                         ProcessAs='Water',
                         DarkCurrentWorkspace='cad',
                         MaskWorkspace='mask',
                         EmptyBeamWorkspace='mt',
                         EmptyContainerWorkspace='wc',
                         TransmissionWorkspace='w_tr',
                         OutputSensitivityWorkspace='sens',
                         FluxWorkspace='flux',
                         OutputWorkspace='water')
        # The sample is measured at 20m, its transmission is measured at 8m
        # Measure the transmissions next
        # Sample container transmission
        SANSILLReduction(Runs='010444',
                         ProcessAs='Transmission',
                         DarkCurrentWorkspace='cad',
                         EmptyBeamWorkspace='mt',
                         FluxWorkspace='flux',
                         OutputWorkspace='sc_tr')
        # Sample transmission
        SANSILLReduction(Runs='010585',
                         ProcessAs='Transmission',
                         DarkCurrentWorkspace='cad',
                         EmptyBeamWorkspace='mt',
                         OutputWorkspace='s_tr',
                         FluxWorkspace='flux')
        # Reduce the sample
        # Process the dark current Cd/B4C for sample
        SANSILLReduction(Runs='010462',
                         ProcessAs='DarkCurrent',
                         OutputWorkspace='scad')
        # Process the empty beam for sample
        SANSILLReduction(Runs='010413',
                         ProcessAs='EmptyBeam',
                         DarkCurrentWorkspace='scad',
                         OutputWorkspace='smt',
                         OutputFluxWorkspace='sflux')
        # Sample container
        SANSILLReduction(Runs='010460',
                         ProcessAs='EmptyContainer',
                         DarkCurrentWorkspace='scad',
                         EmptyBeamWorkspace='smt',
                         TransmissionWorkspace='sc_tr',
                         OutputWorkspace='sc')
        # Sample with flux and sensitivity
        SANSILLReduction(Runs='010569',
                         ProcessAs='Sample',
                         DarkCurrentWorkspace='scad',
                         EmptyContainerWorkspace='sc',
                         EmptyBeamWorkspace='smt',
                         SensitivityWorkspace='sens',
                         MaskWorkspace='mask',
                         TransmissionWorkspace='s_tr',
                         OutputWorkspace='sample_sens',
                         FluxWorkspace='sflux')
        # Sample with flux and water normalisation
        SANSILLReduction(Runs='010569',
                         ProcessAs='Sample',
                         DarkCurrentWorkspace='scad',
                         EmptyContainerWorkspace='sc',
                         EmptyBeamWorkspace='smt',
                         FlatFieldWorkspace='water',
                         MaskWorkspace='mask',
                         TransmissionWorkspace='s_tr',
                         OutputWorkspace='sample_water',
                         FluxWorkspace='sflux')
        GroupWorkspaces(InputWorkspaces=['water', 'sens', 'sample_sens', 'sample_water'],
                        OutputWorkspace='out')
