from __future__ import absolute_import, division, print_function
from mantid.simpleapi import (WANDPowderReduction,
                              CreateSampleWorkspace, RotateInstrumentComponent,
                              MoveInstrumentComponent, CloneWorkspace, AddSampleLog)
from mantid.kernel import V3D
import unittest
import numpy as np


class WANDPowderReductionTest(unittest.TestCase):

    def _create_workspaces(self):
        cal=CreateSampleWorkspace(NumBanks=1,BinWidth=20000,PixelSpacing=0.1,BankPixelWidth=100)
        RotateInstrumentComponent(cal, ComponentName='bank1', X=1, Y=0.5, Z=2, Angle=35)
        MoveInstrumentComponent(cal, ComponentName='bank1', X=1, Y=1, Z=5)
        bkg=CloneWorkspace(cal)
        data=CloneWorkspace(cal)
        AddSampleLog(cal, LogName="gd_prtn_chrg", LogType='Number', NumberType='Double', LogText='200')
        AddSampleLog(bkg, LogName="gd_prtn_chrg", LogType='Number', NumberType='Double', LogText='50')
        AddSampleLog(data, LogName="gd_prtn_chrg", LogType='Number', NumberType='Double', LogText='100')
        AddSampleLog(cal, LogName="duration", LogType='Number', NumberType='Double', LogText='20')
        AddSampleLog(bkg, LogName="duration", LogType='Number', NumberType='Double', LogText='5')
        AddSampleLog(data, LogName="duration", LogType='Number', NumberType='Double', LogText='10')

        def get_cal_counts(n):
            if n < 5000:
                return 0.9
            else:
                return 1.0

        def get_bkg_counts(n):
            return 1.5*get_cal_counts(n)

        def get_data_counts(n,twoTheta):
            tt1=30
            tt2=45
            return get_bkg_counts(n)+10*np.exp(-(twoTheta-tt1)**2/1)+20*np.exp(-(twoTheta-tt2)**2/0.2)

        for i in range(cal.getNumberHistograms()):
            cal.setY(i, [get_cal_counts(i)*2.0])
            bkg.setY(i, [get_bkg_counts(i)/2.0])
            twoTheta=data.getInstrument().getDetector(i+10000).getTwoTheta(V3D(0,0,0),V3D(0,0,1))*180/np.pi
            data.setY(i, [get_data_counts(i,twoTheta)])

        return data, cal, bkg

    def test(self):
        data, cal, bkg = self._create_workspaces()

        # data normalised by monitor
        pd_out=WANDPowderReduction(InputWorkspace=data,
                                   Target='Theta',
                                   NumberBins=1000)

        x = pd_out.extractX()
        y = pd_out.extractY()

        self.assertAlmostEqual(x.min(),  8.07086781)
        self.assertAlmostEqual(x.max(), 50.82973519)
        self.assertAlmostEqual(y.min(),  0.00328244)
        self.assertAlmostEqual(y.max(),  4.88908824)
        self.assertAlmostEqual(x[0,y.argmax()], 45.094311535)

        # data and calibration, limited range
        pd_out2=WANDPowderReduction(InputWorkspace=data,
                                    CalibrationWorkspace=cal,
                                    Target='Theta',
                                    NumberBins=2000,
                                    XMin=10,
                                    XMax=40)

        x = pd_out2.extractX()
        y = pd_out2.extractY()

        self.assertAlmostEqual(x.min(), 10.0075)
        self.assertAlmostEqual(x.max(), 39.9925)
        self.assertAlmostEqual(y.min(),  1.5)
        self.assertAlmostEqual(y.max(), 12.6107234)
        self.assertAlmostEqual(x[0,y.argmax()], 30.0025)

        # data, cal and background, normalised by time
        pd_out3=WANDPowderReduction(InputWorkspace=data,
                                    CalibrationWorkspace=cal,
                                    BackgroundWorkspace=bkg,
                                    Target='Theta',
                                    NumberBins=1000,
                                    NormaliseBy='Time')

        x = pd_out3.extractX()
        y = pd_out3.extractY()

        self.assertAlmostEqual(x.min(), 8.07086781)
        self.assertAlmostEqual(x.max(), 50.82973519)
        self.assertAlmostEqual(y.min(),  0)
        self.assertAlmostEqual(y.max(), 19.97968357)
        self.assertAlmostEqual(x[0,y.argmax()], 45.008708196)

        # data, cal and background. To d spacing
        pd_out4=WANDPowderReduction(InputWorkspace=data,
                                    CalibrationWorkspace=cal,
                                    BackgroundWorkspace=bkg,
                                    Target='ElasticDSpacing',
                                    EFixed=30,
                                    NumberBins=1000)

        x = pd_out4.extractX()
        y = pd_out4.extractY()

        self.assertAlmostEqual(x.min(), 1.92800159)
        self.assertAlmostEqual(x.max(), 11.7586705)
        self.assertAlmostEqual(y.min(),  0)
        self.assertAlmostEqual(y.max(), 19.03642005)
        self.assertAlmostEqual(x[0,y.argmax()], 2.1543333)

        # data, cal and background with mask angle, to Q.
        pd_out4=WANDPowderReduction(InputWorkspace=data,
                                    CalibrationWorkspace=cal,
                                    BackgroundWorkspace=bkg,
                                    Target='ElasticQ',
                                    EFixed=30,
                                    NumberBins=2000,
                                    MaskAngle=60)

        x = pd_out4.extractX()
        y = pd_out4.extractY()

        self.assertAlmostEqual(x.min(), 0.53479223)
        self.assertAlmostEqual(x.max(), 3.21684994)
        self.assertAlmostEqual(y.min(),  0)
        self.assertAlmostEqual(y.max(), 19.9948756)
        self.assertAlmostEqual(x[0,y.argmax()], 2.9122841)

        # data, cal and background, scale background
        pd_out4=WANDPowderReduction(InputWorkspace=data,
                                    CalibrationWorkspace=cal,
                                    BackgroundWorkspace=bkg,
                                    BackgroundScale=0.5,
                                    Target='Theta',
                                    NumberBins=1000,
                                    NormaliseBy='Time')

        x = pd_out4.extractX()
        y = pd_out4.extractY()

        self.assertAlmostEqual(x.min(), 8.07086781)
        self.assertAlmostEqual(x.max(), 50.82973519)
        self.assertAlmostEqual(y.min(),  0.75)
        self.assertAlmostEqual(y.max(), 20.72968357)
        self.assertAlmostEqual(x[0,y.argmax()], 45.008708196)


if __name__ == '__main__':
    unittest.main()
