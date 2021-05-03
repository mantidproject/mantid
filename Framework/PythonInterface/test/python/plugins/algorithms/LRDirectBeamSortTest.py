# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import LoadEventNexus, LRDirectBeamSort, DeleteWorkspace, config, mtd
import unittest
import os
import tempfile


class LRDirectBeamSortTest(unittest.TestCase):
    medium = 'air'

    def __init__(self, *args):
        # set previous
        config.setFacility('SNS')

        unittest.TestCase.__init__(self, *args)

    def test_RunsSuccessfully(self):
        LoadEventNexus(Filename='REF_L_179926.nxs.h5', OutputWorkspace='REF_L_179926')
        LoadEventNexus(Filename='REF_L_179927.nxs.h5', OutputWorkspace='REF_L_179927')

        LRDirectBeamSort(WorkspaceList=['REF_L_179926', 'REF_L_179927'],
                         ComputeScalingFactors=True,
                         OrderDirectBeamsByRunNumber=False,
                         SlitTolerance=0.06,
                         IncidentMedium=self.medium,
                         UseLowResCut=False,
                         TOFSteps=200)

        # Clean
        DeleteWorkspace('REF_L_179926')
        DeleteWorkspace('REF_L_179927')

    def test_RunsCalculateScale(self):
        """Test that will find peak range and then calculate scale and export
        """
        direct_beam_runs = [183110, 183111, 183112]
        sf_tof_step = 200.0
        incident_medium = 'Air'
        slit_tolerance = 0.06
        order_by_runs = True

        out_scale_file = os.path.join(tempfile.gettempdir(), '183102_Air_test_partial.cfg')
        LRDirectBeamSort(RunList=direct_beam_runs,
                         UseLowResCut=True,
                         ComputeScalingFactors=True,
                         TOFSteps=sf_tof_step,
                         IncidentMedium=incident_medium,
                         SlitTolerance=slit_tolerance,
                         OrderDirectBeamsByRunNumber=order_by_runs,
                         ScalingFactorFile=out_scale_file)

        # Verify
        assert os.path.exists(out_scale_file)
        with open(out_scale_file, 'r') as result:
            lines = result.readlines()
            assert len(lines) == 6, f'{len(lines)} is not correct'

            # data line
            dataline = lines[5]
            # check: S1W=8.804999999999993 S2iW=8.804976096 a=6.431245845798633
            # default resolution 1E-7
            s1w = float(dataline.split('S1W=')[1].split()[0])
            self.assertAlmostEqual(s1w, 8.805000000)
            s2w = float(dataline.split('S2iW=')[1].split()[0])
            self.assertAlmostEqual(s2w, 8.804976096)
            a = float(dataline.split('a=')[1].split()[0])
            self.assertAlmostEqual(a, 6.431245846)

    def __del__(self):
        if len(mtd.getObjectNames()) > 0:
            raise RuntimeError('Workspaces are not cleaned')


if __name__ == '__main__':
    unittest.main()
