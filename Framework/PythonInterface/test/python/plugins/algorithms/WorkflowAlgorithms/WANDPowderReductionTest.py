# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import (
    WANDPowderReduction,
    CreateSampleWorkspace,
    RotateInstrumentComponent,
    MoveInstrumentComponent,
    CloneWorkspace,
    AddSampleLog,
    GroupWorkspaces,
    LoadInstrument,
    CreateWorkspace,
    mtd,
)
from mantid.api import (
    MatrixWorkspace,
    WorkspaceGroup,
)
from mantid.kernel import V3D
import unittest
import numpy as np


class WANDPowderReductionTest(unittest.TestCase):
    def _create_workspaces(self):
        cal = CreateSampleWorkspace(NumBanks=1, BinWidth=20000, PixelSpacing=0.1, BankPixelWidth=100)
        RotateInstrumentComponent(cal, ComponentName="bank1", X=1, Y=0.5, Z=2, Angle=35)
        MoveInstrumentComponent(cal, ComponentName="bank1", X=1, Y=1, Z=5)
        bkg = CloneWorkspace(cal)
        data = CloneWorkspace(cal)
        AddSampleLog(
            cal,
            LogName="gd_prtn_chrg",
            LogType="Number",
            NumberType="Double",
            LogText="200",
        )
        AddSampleLog(
            bkg,
            LogName="gd_prtn_chrg",
            LogType="Number",
            NumberType="Double",
            LogText="50",
        )
        AddSampleLog(
            data,
            LogName="gd_prtn_chrg",
            LogType="Number",
            NumberType="Double",
            LogText="100",
        )
        AddSampleLog(cal, LogName="duration", LogType="Number", NumberType="Double", LogText="20")
        AddSampleLog(bkg, LogName="duration", LogType="Number", NumberType="Double", LogText="5")
        AddSampleLog(
            data,
            LogName="duration",
            LogType="Number",
            NumberType="Double",
            LogText="10",
        )

        def get_cal_counts(n):
            if n < 5000:
                return 0.9
            else:
                return 1.0

        def get_bkg_counts(n):
            return 1.5 * get_cal_counts(n)

        def get_data_counts(n, twoTheta):
            tt1 = 30
            tt2 = 45
            return get_bkg_counts(n) + 10 * np.exp(-((twoTheta - tt1) ** 2) / 1) + 20 * np.exp(-((twoTheta - tt2) ** 2) / 0.2)

        for i in range(cal.getNumberHistograms()):
            cal.setY(i, [get_cal_counts(i) * 2.0])
            bkg.setY(i, [get_bkg_counts(i) / 2.0])
            twoTheta = data.getInstrument().getDetector(i + 10000).getTwoTheta(V3D(0, 0, 0), V3D(0, 0, 1)) * 180 / np.pi
            data.setY(i, [get_data_counts(i, twoTheta)])

        return data, cal, bkg

    def test(self):
        data, cal, bkg = self._create_workspaces()

        # data normalised by monitor
        pd_out = WANDPowderReduction(InputWorkspace=data, Target="Theta", NumberBins=1000)

        x = pd_out.extractX()
        y = pd_out.extractY()

        self.assertAlmostEqual(x.min(), 8.07086781)
        self.assertAlmostEqual(x.max(), 50.82973519)
        self.assertAlmostEqual(y.min(), 0.00328244)
        self.assertAlmostEqual(y.max(), 4.88908824)
        self.assertAlmostEqual(x[0, y.argmax()], 45.094311535)

        # data normalised by monitor <- duplicate input as two
        # NOTE:
        # still needs to check physics
        pd_out_multi = WANDPowderReduction(
            InputWorkspace=[data, data],
            Target="Theta",
            NumberBins=1000,
            Sum=True,
        )

        x = pd_out_multi.extractX()
        y = pd_out_multi.extractY()

        self.assertAlmostEqual(x.min(), 8.07086781)
        self.assertAlmostEqual(x.max(), 50.82973519)
        self.assertAlmostEqual(y.min(), 0.00328244 * 2)
        self.assertAlmostEqual(y.max(), 4.88908824 * 2)
        self.assertAlmostEqual(x[0, y.argmax()], 45.094311535)

        # data and calibration, limited range
        pd_out2 = WANDPowderReduction(
            InputWorkspace=data,
            CalibrationWorkspace=cal,
            Target="Theta",
            NumberBins=2000,
            XMin=10,
            XMax=40,
        )

        x = pd_out2.extractX()
        y = pd_out2.extractY()

        self.assertAlmostEqual(x.min(), 10.0075)
        self.assertAlmostEqual(x.max(), 39.9925)
        self.assertAlmostEqual(y.min(), 1.5)
        self.assertAlmostEqual(y.max(), 12.6107234)
        self.assertAlmostEqual(x[0, y.argmax()], 30.0025)

        # data and calibration, limited range
        # NOTE:
        # still needs to check physics
        pd_out2_multi = WANDPowderReduction(
            InputWorkspace=[data, data],
            CalibrationWorkspace=cal,
            Target="Theta",
            NumberBins=2000,
            XMin=10,
            XMax=40,
            Sum=True,
        )

        x = pd_out2_multi.extractX()
        y = pd_out2_multi.extractY()

        self.assertAlmostEqual(x.min(), 10.0075)
        self.assertAlmostEqual(x.max(), 39.9925)
        self.assertAlmostEqual(y.min(), 1.5)
        self.assertAlmostEqual(y.max(), 12.6107234)
        self.assertAlmostEqual(x[0, y.argmax()], 30.0025)

        # data, cal and background, normalised by time
        pd_out3 = WANDPowderReduction(
            InputWorkspace=data,
            CalibrationWorkspace=cal,
            BackgroundWorkspace=bkg,
            Target="Theta",
            NumberBins=1000,
            NormaliseBy="Time",
        )

        x = pd_out3.extractX()
        y = pd_out3.extractY()

        self.assertAlmostEqual(x.min(), 8.07086781)
        self.assertAlmostEqual(x.max(), 50.82973519)
        self.assertAlmostEqual(y.min(), 0)
        self.assertAlmostEqual(y.max(), 19.97968357)
        self.assertAlmostEqual(x[0, y.argmax()], 45.008708196)

        # data, cal and background, normalised by time
        # NOTE:
        # still needs to check physics
        pd_out3_multi = WANDPowderReduction(
            InputWorkspace=[data, data],
            CalibrationWorkspace=cal,
            BackgroundWorkspace=bkg,
            Target="Theta",
            NumberBins=1000,
            NormaliseBy="Time",
            Sum=True,
        )

        x = pd_out3_multi.extractX()
        y = pd_out3_multi.extractY()

        self.assertAlmostEqual(x.min(), 8.07086781)
        self.assertAlmostEqual(x.max(), 50.82973519)
        self.assertAlmostEqual(y.min(), 0)
        self.assertAlmostEqual(y.max(), 19.97968357)
        self.assertAlmostEqual(x[0, y.argmax()], 45.008708196)

        # data, cal and background. To d spacing
        pd_out4 = WANDPowderReduction(
            InputWorkspace=data,
            CalibrationWorkspace=cal,
            BackgroundWorkspace=bkg,
            Target="ElasticDSpacing",
            Wavelength=1.6513045600369298,
            NumberBins=1000,
        )

        x = pd_out4.extractX()
        y = pd_out4.extractY()

        self.assertAlmostEqual(x.min(), 1.92800159)
        self.assertAlmostEqual(x.max(), 11.7586705)
        self.assertAlmostEqual(y.min(), 0)
        self.assertAlmostEqual(y.max(), 19.03642005)
        self.assertAlmostEqual(x[0, y.argmax()], 2.1543333)

        pd_out4_multi = WANDPowderReduction(
            InputWorkspace=[data, data],
            CalibrationWorkspace=cal,
            BackgroundWorkspace=bkg,
            Target="ElasticDSpacing",
            Wavelength=1.6513045600369298,
            NumberBins=1000,
            Sum=True,
        )

        x = pd_out4_multi.extractX()
        y = pd_out4_multi.extractY()

        self.assertAlmostEqual(x.min(), 1.92800159)
        self.assertAlmostEqual(x.max(), 11.7586705)
        self.assertAlmostEqual(y.min(), 0)
        self.assertAlmostEqual(y.max(), 19.03642005)
        self.assertAlmostEqual(x[0, y.argmax()], 2.1543333)

        # data, cal and background with mask angle, to Q.
        pd_out4 = WANDPowderReduction(
            InputWorkspace=data,
            CalibrationWorkspace=cal,
            BackgroundWorkspace=bkg,
            Target="ElasticQ",
            Wavelength=1.6513045600369298,
            NumberBins=2000,
            MaskAngle=60,
        )

        x = pd_out4.extractX()
        y = pd_out4.extractY()

        self.assertAlmostEqual(x.min(), 0.53479223, places=4)
        self.assertAlmostEqual(x.max(), 3.21684994, places=4)
        self.assertAlmostEqual(y.min(), 0, places=4)
        self.assertAlmostEqual(y.max(), 19.9948756, places=4)
        self.assertAlmostEqual(x[0, y.argmax()], 2.9122841, places=4)

        # NOTE:
        # Need to check the physics
        pd_out4_multi = WANDPowderReduction(
            InputWorkspace=[data, data],
            CalibrationWorkspace=cal,
            BackgroundWorkspace=bkg,
            Target="ElasticQ",
            Wavelength=1.6513045600369298,
            NumberBins=2000,
            MaskAngle=60,
            Sum=True,
        )

        x = pd_out4_multi.extractX()
        y = pd_out4_multi.extractY()

        self.assertAlmostEqual(x.min(), 0.53479223, places=4)
        self.assertAlmostEqual(x.max(), 3.21684994, places=4)
        self.assertAlmostEqual(y.min(), 0, places=4)
        self.assertAlmostEqual(y.max(), 19.9948756, places=4)
        self.assertAlmostEqual(x[0, y.argmax()], 2.9122841, places=4)

        # data, cal and background, scale background
        pd_out4 = WANDPowderReduction(
            InputWorkspace=data,
            CalibrationWorkspace=cal,
            BackgroundWorkspace=bkg,
            BackgroundScale=0.5,
            Target="Theta",
            NumberBins=1000,
            NormaliseBy="Time",
        )

        x = pd_out4.extractX()
        y = pd_out4.extractY()

        self.assertAlmostEqual(x.min(), 8.07086781)
        self.assertAlmostEqual(x.max(), 50.82973519)
        self.assertAlmostEqual(y.min(), 0.75)
        self.assertAlmostEqual(y.max(), 20.72968357)
        self.assertAlmostEqual(x[0, y.argmax()], 45.008708196)

        pd_out4_multi = WANDPowderReduction(
            InputWorkspace=[data, data],
            CalibrationWorkspace=cal,
            BackgroundWorkspace=bkg,
            BackgroundScale=0.5,
            Target="Theta",
            NumberBins=1000,
            NormaliseBy="Time",
            Sum=True,
        )

        x = pd_out4_multi.extractX()
        y = pd_out4_multi.extractY()

        self.assertAlmostEqual(x.min(), 8.07086781)
        self.assertAlmostEqual(x.max(), 50.82973519)
        self.assertAlmostEqual(y.min(), 0.75)
        self.assertAlmostEqual(y.max(), 20.72968357)
        self.assertAlmostEqual(x[0, y.argmax()], 45.008708196)

    def test_event(self):
        # check that the workflow runs with event workspaces as input, junk data

        event_data = CreateSampleWorkspace(
            NumBanks=1,
            BinWidth=20000,
            PixelSpacing=0.1,
            BankPixelWidth=100,
            WorkspaceType="Event",
        )
        event_cal = CreateSampleWorkspace(
            NumBanks=1,
            BinWidth=20000,
            PixelSpacing=0.1,
            BankPixelWidth=100,
            WorkspaceType="Event",
            Function="Flat background",
        )
        event_bkg = CreateSampleWorkspace(
            NumBanks=1,
            BinWidth=20000,
            PixelSpacing=0.1,
            BankPixelWidth=100,
            WorkspaceType="Event",
            Function="Flat background",
        )

        # CASE 1
        # input single workspace, output single workspace
        pd_out = WANDPowderReduction(
            InputWorkspace=event_data,
            CalibrationWorkspace=event_cal,
            BackgroundWorkspace=event_bkg,
            Target="Theta",
            NumberBins=1000,
            NormaliseBy="None",
            Sum=False,
        )

        assert isinstance(pd_out, MatrixWorkspace)

        x = pd_out.extractX()
        y = pd_out.extractY()

        self.assertAlmostEqual(x.min(), 0.03517355)
        self.assertAlmostEqual(x.max(), 70.3119282)
        self.assertAlmostEqual(y[0, 0], 0.0)

        # CASE 2
        # input multiple single ws, output (single) summed ws
        pd_out = WANDPowderReduction(
            InputWorkspace=[event_data, event_data],
            CalibrationWorkspace=event_cal,
            BackgroundWorkspace=event_bkg,
            Target="Theta",
            NumberBins=1000,
            NormaliseBy="None",
            Sum=True,
        )

        x = pd_out.extractX()
        y = pd_out.extractY()

        self.assertAlmostEqual(x.min(), 0.03517355)
        self.assertAlmostEqual(x.max(), 70.3119282)
        self.assertAlmostEqual(y[0, 0], 0.0)
        assert isinstance(pd_out, MatrixWorkspace)

        # CASE 3
        # input group ws containing several ws, output group ws containing several ws
        pd_out = WANDPowderReduction(
            InputWorkspace=[event_data, event_data],
            CalibrationWorkspace=event_cal,
            BackgroundWorkspace=event_bkg,
            Target="Theta",
            NumberBins=1000,
            NormaliseBy="None",
            Sum=False,
        )

        for i in pd_out:
            x = i.extractX()
            y = i.extractY()

            self.assertAlmostEqual(x.min(), 0.03517355)
            self.assertAlmostEqual(x.max(), 70.3119282)
            self.assertAlmostEqual(y[0, 0], 0.0)

        assert isinstance(pd_out, WorkspaceGroup)
        assert len(pd_out) == 2

        event_data2 = CloneWorkspace(event_data)

        event_data_group = WorkspaceGroup()
        event_data_group.addWorkspace(event_data)
        event_data_group.addWorkspace(event_data2)

        # CASE 4 - input group ws, output group ws
        pd_out = WANDPowderReduction(
            InputWorkspace=event_data_group,
            CalibrationWorkspace=event_cal,
            BackgroundWorkspace=event_bkg,
            Target="Theta",
            NumberBins=1000,
            NormaliseBy="None",
            Sum=False,
        )

        for i in pd_out:
            x = i.extractX()
            y = i.extractY()

            self.assertAlmostEqual(x.min(), 0.03517355)
            self.assertAlmostEqual(x.max(), 70.3119282)
            self.assertAlmostEqual(y[0, 0], 0.0)

        assert isinstance(pd_out, WorkspaceGroup)
        assert len(pd_out) == 2

        event_data2 = CloneWorkspace(event_data)
        event_data_group = GroupWorkspaces([event_data, event_data2])

        pd_out = WANDPowderReduction(
            InputWorkspace=event_data_group,
            CalibrationWorkspace=event_cal,
            BackgroundWorkspace=event_bkg,
            Target="Theta",
            NumberBins=1000,
            NormaliseBy="None",
            Sum=False,
        )

        for i in pd_out:
            x = i.extractX()
            y = i.extractY()

            self.assertAlmostEqual(x.min(), 0.03517355)
            self.assertAlmostEqual(x.max(), 70.3119282)
            self.assertAlmostEqual(y[0, 0], 0.0)

        assert isinstance(pd_out, WorkspaceGroup)
        assert len(pd_out) == 2

    def test_edge_case(self):
        # This is to capture the case where there are identical theta
        # values for multiple spectra which cause problems when
        # ResampleX is run with particular Xmin and Xmax causing the
        # output to be all zeros.

        # Create more real test workspace, using same properties as
        # HB2C_558131
        data = np.ones((256, 1920))
        CreateWorkspace(
            DataX=[0, 1], DataY=data, DataE=np.sqrt(data), UnitX="Empty", YUnitLabel="Counts", NSpec=1966080 // 4, OutputWorkspace="tmp_ws"
        )
        AddSampleLog("tmp_ws", LogName="HB2C:Mot:s2.RBV", LogText="29.9774", LogType="Number Series", NumberType="Double")
        AddSampleLog("tmp_ws", LogName="HB2C:Mot:detz.RBV", LogText="0", LogType="Number Series", NumberType="Double")
        tmp_ws = mtd["tmp_ws"]

        for n in range(tmp_ws.getNumberHistograms()):
            s = tmp_ws.getSpectrum(n)
            for i in range(2):
                for j in range(2):
                    s.addDetectorID(int(n * 2 % 512 + n // (512 / 2) * 512 * 2 + j + i * 512))
        LoadInstrument("tmp_ws", InstrumentName="WAND", RewriteSpectraMap=False)

        out = WANDPowderReduction("tmp_ws", Target="Theta", XMin=29, XMax=31, NumberBins=10, NormaliseBy="None")
        np.testing.assert_allclose(
            out.readY(0), [0, 0, 0, 0, 269.068237, 486.311606, 618.125152, 720.37274, 811.141863, 821.032586], rtol=5e-4
        )

        tmp_ws.delete()
        out.delete()


if __name__ == "__main__":
    unittest.main()
