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
                              CreateGroupingWorkspace, CreateEmptyTableWorkspace)
from numpy.testing import assert_allclose, assert_equal


def create_test_ws_and_group():
    myFunc = "name=Gaussian, PeakCentre=2, Height=100, Sigma=0.01;" + \
        "name=Gaussian, PeakCentre=1, Height=100, Sigma=0.01;" + \
        "name=Gaussian, PeakCentre=4, Height=100, Sigma=0.01"
    ws = CreateSampleWorkspace("Event","User Defined", myFunc, BankPixelWidth=1,
                               XUnit='dSpacing', XMax=5, BinWidth=0.001, NumEvents=100000, NumBanks=8)
    for n in range(1,5):
        MoveInstrumentComponent(ws, ComponentName=f'bank{n}', X=1+n/10, Y=0, Z=1+n/10, RelativePosition=False)
        MoveInstrumentComponent(ws, ComponentName=f'bank{n+4}', X=2+n/10, Y=0, Z=2+n/10, RelativePosition=False)

    MaskDetectors(ws, WorkspaceIndexList=[3,7])

    ws = ScaleX(ws, Factor=1.05, IndexMin=1, IndexMax=1)
    ws = ScaleX(ws, Factor=0.95, IndexMin=2, IndexMax=2)
    ws = ScaleX(ws, Factor=1.05, IndexMin=4, IndexMax=6)
    ws = ScaleX(ws, Factor=1.02, IndexMin=5, IndexMax=5)
    ws = ScaleX(ws, Factor=0.98, IndexMin=6, IndexMax=6)
    ws = Rebin(ws, '0,0.001,5')
    ws = ConvertUnits(ws, Target='TOF')

    groups, _, _ = CreateGroupingWorkspace(InputWorkspace=ws, ComponentName='basic_rect', CustomGroupingString='1-4,5-8')

    return ws, groups


class TestGroupCalibration(unittest.TestCase):
    def test_from_eng(self):

        ws, groups = create_test_ws_and_group()

        output_workspace_basename = 'test_from_eng'

        starting_difc = [ws.spectrumInfo().difcUncalibrated(i) for i in range(ws.getNumberHistograms())]

        cc_diffcal = group_calibration.cc_calibrate_groups(ws,
                                                           groups,
                                                           output_workspace_basename,
                                                           DReference=2.0,
                                                           Xmin=1.75,
                                                           Xmax=2.25)

        assert_allclose(cc_diffcal.column('difc'),
                        [starting_difc[0],
                         starting_difc[1]/0.95,
                         starting_difc[2]/1.05,
                         starting_difc[4],
                         starting_difc[5]/0.98,
                         starting_difc[6]/1.02], rtol=0.005)

        diffcal, mask = group_calibration.pdcalibration_groups(ws,
                                                               groups,
                                                               cc_diffcal,
                                                               output_workspace_basename,
                                                               PeakPositions = [1.0, 2.0, 4.0],
                                                               PeakFunction='Gaussian',
                                                               PeakWindow=0.4)

        assert_allclose(diffcal.column('difc'),
                        [starting_difc[0],
                         starting_difc[1]/0.95,
                         starting_difc[2]/1.05,
                         0,
                         starting_difc[4]/0.95,
                         starting_difc[5]/(0.95*0.98),
                         starting_difc[6]/(0.95*1.02),
                         0], rtol=0.005)

        assert_equal(mask.extractY(), [[0],
                                       [0],
                                       [0],
                                       [1],
                                       [0],
                                       [0],
                                       [0],
                                       [1]])

    def test_from_prev_cal(self):

        ws, groups = create_test_ws_and_group()

        output_workspace_basename = 'test_from_eng_prev_cal'

        starting_difc = [ws.spectrumInfo().difcUncalibrated(i) for i in range(ws.getNumberHistograms())]

        previous_diffcal = CreateEmptyTableWorkspace()

        previous_diffcal.addColumn("int", "detid")
        previous_diffcal.addColumn("double", "difc")
        previous_diffcal.addColumn("double", "difa")
        previous_diffcal.addColumn("double", "tzero")

        previous_diffcal.addRow([1, starting_difc[0]*1.01, 0, 0])
        previous_diffcal.addRow([2, starting_difc[1]*1.01, 0, 0])
        previous_diffcal.addRow([3, starting_difc[2]*1.01, 0, 0])
        previous_diffcal.addRow([4, starting_difc[3]*1.01, 0, 0])
        previous_diffcal.addRow([5, starting_difc[4]*1.01, 0, 0])
        previous_diffcal.addRow([6, starting_difc[5]*1.01, 0, 0])
        previous_diffcal.addRow([7, starting_difc[6]*1.01, 0, 0])
        previous_diffcal.addRow([8, starting_difc[7]*1.01, 0, 0])

        cc_diffcal = group_calibration.cc_calibrate_groups(ws,
                                                           groups,
                                                           output_workspace_basename,
                                                           previous_calibration=previous_diffcal,
                                                           DReference=2.0,
                                                           Xmin=1.75,
                                                           Xmax=2.25)

        assert_allclose(cc_diffcal.column('difc'),
                        [starting_difc[0]*1.01,
                         starting_difc[1]*1.01/0.95,
                         starting_difc[2]*1.01/1.05,
                         starting_difc[3]*1.01,
                         starting_difc[4]*1.01,
                         starting_difc[5]*1.01/0.98,
                         starting_difc[6]*1.01/1.02,
                         starting_difc[7]*1.01], rtol=0.005)

        diffcal, mask = group_calibration.pdcalibration_groups(ws,
                                                               groups,
                                                               cc_diffcal,
                                                               output_workspace_basename,
                                                               previous_calibration=previous_diffcal,
                                                               PeakPositions = [1.0, 2.0, 4.0],
                                                               PeakFunction='Gaussian',
                                                               PeakWindow=0.4)

        assert_allclose(diffcal.column('difc'),
                        [starting_difc[0],
                         starting_difc[1]/0.95,
                         starting_difc[2]/1.05,
                         starting_difc[3]*1.01,
                         starting_difc[4]/0.95,
                         starting_difc[5]/(0.95*0.98),
                         starting_difc[6]/(0.95*1.02),
                         starting_difc[7]*1.01], rtol=0.005)

        assert_equal(mask.extractY(), [[0],
                                       [0],
                                       [0],
                                       [1],
                                       [0],
                                       [0],
                                       [0],
                                       [1]])

    def test_di_group_calibration(self):
        ws, groups = create_test_ws_and_group()

        starting_difc = [ws.spectrumInfo().difcUncalibrated(i) for i in range(ws.getNumberHistograms())]

        previous_diffcal = CreateEmptyTableWorkspace()

        previous_diffcal.addColumn("int", "detid")
        previous_diffcal.addColumn("double", "difc")
        previous_diffcal.addColumn("double", "difa")
        previous_diffcal.addColumn("double", "tzero")

        previous_diffcal.addRow([1, starting_difc[0]*1.01, 0, 0])
        previous_diffcal.addRow([2, starting_difc[1]*1.01, 0, 0])
        previous_diffcal.addRow([3, starting_difc[2]*1.01, 0, 0])
        previous_diffcal.addRow([4, starting_difc[3]*1.01, 0, 0])
        previous_diffcal.addRow([5, starting_difc[4]*1.01, 0, 0])
        previous_diffcal.addRow([6, starting_difc[5]*1.01, 0, 0])
        previous_diffcal.addRow([7, starting_difc[6]*1.01, 0, 0])
        previous_diffcal.addRow([8, starting_difc[7]*1.01, 0, 0])

        diffcal, mask  = group_calibration.do_group_calibration(ws,
                                                                groups,
                                                                previous_calibration=previous_diffcal,
                                                                output_basename = "group_calibration",
                                                                cc_kwargs={
                                                                    "Step": 0.001,
                                                                    "DReference": 2.0,
                                                                    "Xmin": 1.75,
                                                                    "Xmax": 2.25},
                                                                pdcal_kwargs={
                                                                    "PeakPositions": [1.0, 2.0, 4.0],
                                                                    "TofBinning": [300,-.001,16666.7],
                                                                    "PeakFunction": 'Gaussian',
                                                                    "PeakWindow": 0.4,
                                                                    "PeakWidthPercent": None})

        assert_allclose(diffcal.column('difc'),
                        [starting_difc[0],
                         starting_difc[1]/0.95,
                         starting_difc[2]/1.05,
                         starting_difc[3]*1.01,
                         starting_difc[4]/0.95,
                         starting_difc[5]/(0.95*0.98),
                         starting_difc[6]/(0.95*1.02),
                         starting_difc[7]*1.01], rtol=0.005)

        assert_equal(mask.extractY(), [[0],
                                       [0],
                                       [0],
                                       [1],
                                       [0],
                                       [0],
                                       [0],
                                       [1]])


if __name__ == '__main__':
    unittest.main()
