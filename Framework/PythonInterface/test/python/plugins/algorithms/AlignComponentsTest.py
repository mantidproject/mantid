import unittest
from mantid.simpleapi import AlignComponents, LoadCalFile, LoadEmptyInstrument, mtd

class AlignComponentsTest(unittest.TestCase):

    def testAlignComponentsPG3bank26Position(self):
        LoadCalFile(InstrumentName="PG3",
            CalFilename="PG3_golden.cal",
            MakeGroupingWorkspace=False,
            MakeOffsetsWorkspace=True,
            MakeMaskWorkspace=False,
            WorkspaceName="PG3")
        LoadEmptyInstrument(Filename="POWGEN_Definition_2014-03-10.xml",OutputWorkspace="testWS")

        # Test X position with bank26
        ws = mtd['testWS']
        startPos = ws.getInstrument().getComponentByName("bank26").getPos()
        startRot = ws.getInstrument().getComponentByName("bank26").getRotation()
        AlignComponents(CalibrationTable="PG3_cal",
                        InputWorkspace="testWS",
                        ComponentList="bank26",
                        Yposition=False, Zposition=False,
                        Xrotation=False, Yrotation=False, Zrotation=False)
        ws = mtd["testWS"]
        endPos = ws.getInstrument().getComponentByName("bank26").getPos()
        endRot = ws.getInstrument().getComponentByName("bank26").getRotation()
        self.assertAlmostEqual(endPos.getX(), 1.51596)
        self.assertEqual(startPos.getY(), endPos.getY())
        self.assertEqual(startPos.getZ(), endPos.getZ())
        self.assertEqual(startRot, endRot)

        # Test X rotation with bank46
        startPos = ws.getInstrument().getComponentByName("bank46").getPos()
        startRot = ws.getInstrument().getComponentByName("bank46").getRotation().getEulerAngles()
        AlignComponents(CalibrationTable="PG3_cal",
                        InputWorkspace=ws,
                        ComponentList="bank46",
                        Xposition=False, Yposition=False, Zposition=False,
                        Yrotation=False, Zrotation=False)
        endPos = ws.getInstrument().getComponentByName("bank46").getPos()
        endRot = ws.getInstrument().getComponentByName("bank46").getRotation().getEulerAngles()
        self.assertEqual(startPos, endPos)
        self.assertAlmostEqual(startRot[0], -37.5517)
        self.assertAlmostEqual(startRot[1], endRot[1])
        self.assertAlmostEqual(startRot[2], endRot[2])

if __name__ == "__main__":
    unittest.main()
