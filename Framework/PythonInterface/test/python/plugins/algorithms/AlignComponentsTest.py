import unittest
from mantid.simpleapi import AlignComponents, CreateSampleWorkspace, MoveInstrumentComponent, CreateEmptyTableWorkspace, mtd, RotateInstrumentComponent

class AlignComponentsTest(unittest.TestCase):

    def testAlignComponentsPositionXY(self):
        CreateSampleWorkspace(OutputWorkspace='testWS', NumBanks=1,BankPixelWidth=4)
        component='bank1'
        MoveInstrumentComponent(Workspace='testWS',ComponentName=component,X=0.06,Y=0.04,Z=4.98,RelativePosition=False)

        ### Detector should move to [0.05,0.03,4.98]
        # Calibration table generated with:
        # CreateSampleWorkspace(OutputWorkspace='sample', NumBanks=1,BankPixelWidth=4)
        # MoveInstrumentComponent(Workspace='sample',ComponentName='bank1',X=0.05,Y=0.03,Z=4.98,RelativePosition=False)
        # CalculateDIFC(InputWorkspace='sample', OutputWorkspace='sample')
        # d=mtd['sample'].extractY()
        # for i in range(len(d)):
        #        print "calTable.addRow(["+str(i+16)+", "+str(d[i][0])+"])"

        calTable = CreateEmptyTableWorkspace()
        calTable.addColumn("int", "detid")
        calTable.addColumn("double", "difc")

        calTable.addRow([16, 44.3352831346])
        calTable.addRow([17, 47.7503426493])
        calTable.addRow([18, 51.6581064544])
        calTable.addRow([19, 55.9553976608])
        calTable.addRow([20, 49.6495672525])
        calTable.addRow([21, 52.7214213944])
        calTable.addRow([22, 56.285004349])
        calTable.addRow([23, 60.2530897937])
        calTable.addRow([24, 55.1227558338])
        calTable.addRow([25, 57.9048914599])
        calTable.addRow([26, 61.1671229038])
        calTable.addRow([27, 64.8369848035])
        calTable.addRow([28, 60.7118272387])
        calTable.addRow([29, 63.2484968666])
        calTable.addRow([30, 66.2480051141])
        calTable.addRow([31, 69.650545037])

        ws = mtd["testWS"]
        startPos = ws.getInstrument().getComponentByName(component).getPos()
        startRot = ws.getInstrument().getComponentByName(component).getRotation().getEulerAngles()
        AlignComponents(CalibrationTable="calTable",
                        InputWorkspace="testWS",
                        ComponentList=component,
                        Xposition=True,
                        Yposition=True)
        ws = mtd["testWS"]
        endPos = ws.getInstrument().getComponentByName(component).getPos()
        endRot = ws.getInstrument().getComponentByName(component).getRotation().getEulerAngles()
        self.assertAlmostEqual(endPos.getX(), 0.05)
        self.assertAlmostEqual(endPos.getY(), 0.03)
        self.assertEqual(startPos.getZ(), endPos.getZ())
        self.assertEqual(startRot[0], endRot[0])
        self.assertEqual(startRot[1], endRot[1])
        self.assertEqual(startRot[2], endRot[2])

    def testAlignComponentsRotationY(self):
        CreateSampleWorkspace(OutputWorkspace='testWS', NumBanks=1,BankPixelWidth=4)
        component='bank1'
        MoveInstrumentComponent(Workspace='testWS',ComponentName=component,X=0.05,Y=0.03,Z=4.98,RelativePosition=False)
        RotateInstrumentComponent(Workspace='testWS',ComponentName='bank1',X=0,Y=1,Z=0,Angle=1,RelativeRotation=False)

        # Detector should rotate to +3deg around Y
        calTable = CreateEmptyTableWorkspace()
        calTable.addColumn("int", "detid")
        calTable.addColumn("double", "difc")

        calTable.addRow([16, 44.3352831346])
        calTable.addRow([17, 47.7503426493])
        calTable.addRow([18, 51.6581064544])
        calTable.addRow([19, 55.9553976608])
        calTable.addRow([20, 49.6449496013])
        calTable.addRow([21, 52.7174076764])
        calTable.addRow([22, 56.2816322009])
        calTable.addRow([23, 60.2503705523])
        calTable.addRow([24, 55.1137655776])
        calTable.addRow([25, 57.8969430684])
        calTable.addRow([26, 61.160311581])
        calTable.addRow([27, 64.8313598544])
        calTable.addRow([28, 60.6988733631])
        calTable.addRow([29, 63.236900201])
        calTable.addRow([30, 66.237921314])
        calTable.addRow([31, 69.6420722102])
        

        ws = mtd["testWS"]
        startPos = ws.getInstrument().getComponentByName(component).getPos()
        startRot = ws.getInstrument().getComponentByName(component).getRotation().getEulerAngles()
        AlignComponents(CalibrationTable="calTable",
                        InputWorkspace="testWS",
                        ComponentList=component,
                        Yrotation=True)
        ws = mtd["testWS"]
        endPos = ws.getInstrument().getComponentByName(component).getPos()
        endRot = ws.getInstrument().getComponentByName(component).getRotation().getEulerAngles()
        self.assertEqual(startPos, endPos)
        self.assertEqual(startRot[0], endRot[0])
        self.assertAlmostEqual(endRot[1],3.0)
        self.assertEqual(startRot[2], endRot[2])

if __name__ == "__main__":
    unittest.main()
