# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init

# package imports
from mantid.simpleapi import AlignComponents, DeleteWorkspaces, LoadAscii, LoadEmptyInstrument
from mantid.utils.logging import capture_logs

# standard imports
from systemtesting import MantidSystemTest


class DetectorIDTest(MantidSystemTest):
    r"""
    Component SNAP/Column1 has unsorted detectorID. This tests only checks that the first and last
    detector ID's are correctly identified.
    """
    def requiredFiles(self):
        r"""
        Runs for standard sample CsLaNb2O7
        """
        return ['SNAP_51984_peakcenters_tof.dat']

    def runTest(self):
        prefix = 'mU6g7vP92_'  # pseudo-random string to prefix workspace names
        LoadAscii(Filename='SNAP_51984_peakcenters_tof.dat',
                  Separator='Tab',
                  Unit='Dimensionless', OutputWorkspace=prefix + 'peak_centers')
        LoadEmptyInstrument(InstrumentName='SNAP', OutputWorkspace=prefix + 'snap')

        with capture_logs(level='debug') as logs:
            AlignComponents(PeakCentersTofTable=prefix + 'peak_centers',
                            PeakPositions='1.3143, 1.3854,1.6967, 1.8587, 2.0781',
                            AdjustmentsTable=prefix + 'adjustments',
                            DisplacementsTable=prefix + 'displacements',
                            InputWorkspace=prefix + 'snap',
                            OutputWorkspace=prefix + 'snap_modified',
                            FitSourcePosition=False, FitSamplePosition=False,
                            ComponentList='Column1',
                            XPosition=False, YPosition=False, ZPosition=True,
                            AlphaRotation=False, BetaRotation=False, GammaRotation=False)
            assert 'First and last detectorID for Column1 are 0, 196607' in logs.getvalue()

        # Clean up workspaces
        temporary = ['peak_centers', 'snap', 'adjustments', 'displacements', 'snap_modified']
        DeleteWorkspaces([prefix + suffix for suffix in temporary])
