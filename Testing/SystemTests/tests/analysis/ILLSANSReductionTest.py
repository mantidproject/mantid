from __future__ import (absolute_import, division, print_function)

import stresstesting
from mantid.simpleapi import ILLSANSReduction, Q1DWeighted, config, mtd


class ILL_D11_Test(stresstesting.MantidStressTest):

    def __init__(self):
        super(ILL_D11_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D11'
        config.appendDataSearchSubDir('ILL/D11/')

    def requiredFiles(self):
        return ['010455.nxs', '010414.nxs', '010446.nxs', '010454.nxs', '010445.nxs', '010453.nxs',
                '010462.nxs', '010413.nxs', '010444.nxs', '010460.nxs', '010585.nxs', '010569.nxs',
                'ILL_SANS_D11_IQ.nxs']

    def tearDown(self):
        mtd.clear()

    def runTest(self):

        # Process the dark current Cd/B4C for water
        ILLSANSReduction(Run='010455.nxs', ProcessAs='Absorber', OutputWorkspace='Cdw')

        # Process the empty beam for water
        ILLSANSReduction(Run='010414.nxs', ProcessAs='Beam', AbsorberInputWorkspace='Cdw', OutputWorkspace='Dbw')

        # Water container transmission
        ILLSANSReduction(Run='010446.nxs', ProcessAs='Transmission', AbsorberInputWorkspace='Cdw',
                         BeamInputWorkspace='Dbw', OutputWorkspace='wc_tr')

        # Water container
        ILLSANSReduction(Run='010454.nxs', ProcessAs='Container', AbsorberInputWorkspace='Cdw',
                         BeamInputWorkspace='Dbw', TransmissionInputWorkspace='wc_tr', OutputWorkspace='wc')

        # Water transmission
        ILLSANSReduction(Run='010445.nxs', ProcessAs='Transmission', AbsorberInputWorkspace='Cdw',
                         BeamInputWorkspace='Dbw', OutputWorkspace='w_tr')

        # Water
        ILLSANSReduction(Run='010453.nxs', ProcessAs='Reference', AbsorberInputWorkspace='Cdw',
                         ContainerInputWorkspace='wc', BeamInputWorkspace='Dbw', TransmissionInputWorkspace='wc_tr',
                         SensitivityOutputWorkspace='sens', OutputWorkspace='water')

        # Process the dark current Cd/B4C for sample
        ILLSANSReduction(Run='010462.nxs', ProcessAs='Absorber', OutputWorkspace='Cd')

        # Process the empty beam for sample
        ILLSANSReduction(Run='010413.nxs', ProcessAs='Beam', AbsorberInputWorkspace='Cd', OutputWorkspace='Db')

        # Sample container transmission
        ILLSANSReduction(Run='010444.nxs', ProcessAs='Transmission', AbsorberInputWorkspace='Cd',
                         BeamInputWorkspace='Dbw', OutputWorkspace='sc_tr')

        # Sample container
        ILLSANSReduction(Run='010460.nxs', ProcessAs='Container', AbsorberInputWorkspace='Cd', BeamInputWorkspace='Db',
                         TransmissionInputWorkspace='sc_tr', OutputWorkspace='sc')

        # Sample transmission
        ILLSANSReduction(Run='010585.nxs', ProcessAs='Transmission', AbsorberInputWorkspace='Cd', BeamInputWorkspace='Dbw',
                         OutputWorkspace='s_tr')

        # Sample
        ILLSANSReduction(Run='010569.nxs', ProcessAs='Sample', AbsorberInputWorkspace='Cd', ContainerInputWorkspace='sc',
                         BeamInputWorkspace='Db', SensitivityInputWorkspace='sens',
                         TransmissionInputWorkspace='s_tr', OutputWorkspace='sample_flux')

        # Convert to I(Q)
        Q1DWeighted(InputWorkspace='sample_flux', NumberOfWedges=0, OutputBinning='0.0027,0.0004,0.033', OutputWorkspace='iq')

    def validate(self):
        self.tolerance = 1e-5
        return ['iq', 'ILL_SANS_D11_IQ.nxs']
