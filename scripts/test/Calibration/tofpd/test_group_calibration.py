# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from Calibration.tofpd import group_calibration
from mantid.simpleapi import (CreateSampleWorkspace, MaskDetectors,
                              MoveInstrumentComponent, ScaleX, Rebin, ConvertUnits,
                              LoadDetectorsGroupingFile)
from numpy.testing import assert_allclose


class TestGroupCalibration(unittest.TestCase):

    def test_from_eng(self):
        myFunc = "name=Gaussian, PeakCentre=2, Height=100, Sigma=0.01;" + \
            "name=Gaussian, PeakCentre=1, Height=100, Sigma=0.01;" + \
            "name=Gaussian, PeakCentre=4, Height=100, Sigma=0.01"
        ws = CreateSampleWorkspace("Event","User Defined", myFunc, BankPixelWidth=1,
                                   XUnit='dSpacing', XMax=5, BinWidth=0.001, NumEvents=100000, NumBanks=8)
        for n in range(1,9):
            MoveInstrumentComponent(ws, ComponentName=f'bank{n}', X=1, Y=0, Z=1, RelativePosition=False)

        MaskDetectors(ws, WorkspaceIndexList=[3,7])

        ws = ScaleX(ws, Factor=1.05, IndexMin=1, IndexMax=1)
        ws = ScaleX(ws, Factor=0.95, IndexMin=2, IndexMax=2)
        ws = ScaleX(ws, Factor=1.05, IndexMin=4, IndexMax=6)
        ws = ScaleX(ws, Factor=1.02, IndexMin=5, IndexMax=5)
        ws = ScaleX(ws, Factor=0.98, IndexMin=6, IndexMax=6)
        ws = Rebin(ws, '0,0.001,5')
        ws_tof = ConvertUnits(ws, Target='TOF')
        ws_tof = Rebin(ws_tof, '1000,10,10000')

        # smae for all spectra
        starting_difc = ws_tof.spectrumInfo().difcUncalibrated(0)

        grouping_file = '/tmp/grouping.xml'

        groupingFileContent = \
            """<?xml version="1.0" encoding="UTF-8" ?>
            <detector-grouping>
            <group ID="1"> <detids>1,2,3,4</detids> </group>
            <group ID="2"> <detids>5,6,7,8</detids> </group>
            </detector-grouping>
            """

        with open(grouping_file, 'w') as f:
            f.write( groupingFileContent )

        output_workspace_basename = 'output'

        groups = LoadDetectorsGroupingFile(grouping_file, InputWorkspace=ws)

        cc_diffcal, cc_offsets = group_calibration.cc_calibrate_groups(ws_tof,
                                                                       groups,
                                                                       output_workspace_basename,
                                                                       Step=0.001,
                                                                       DReference=2.0,
                                                                       Xmin=1.75,
                                                                       Xmax=2.25)

        assert_allclose(cc_diffcal.column('difc'),
                        [starting_difc,
                         starting_difc/(0.95),
                         starting_difc/(1.05),
                         starting_difc,
                         starting_difc,
                         starting_difc/(0.98),
                         starting_difc/(1.02),
                         starting_difc], rtol=0.003)

        diffcal = group_calibration.pdcalibration_groups(ws_tof,
                                                         groups,
                                                         cc_diffcal, cc_offsets,
                                                         output_workspace_basename,
                                                         PeakPositions = [1.0, 2.0, 4.0],
                                                         PeakWindow=0.4)

        assert_allclose(diffcal.column('difc'),
                        [starting_difc,
                         starting_difc/(0.95),
                         starting_difc/(1.05),
                         0,
                         starting_difc/(0.95),
                         starting_difc/(0.95*0.98),
                         starting_difc/(0.95*1.02),
                         0], rtol=0.004)


if __name__ == '__main__':
    unittest.main()
