import unittest
from mantid.simpleapi import AlignComponents, LoadCalFile, LoadEmptyInstrument

class AlignComponentsTest(unittest.TestCase):

    def testAlignComponentsPG3bank26Position(self):
        LoadCalFile(InstrumentName="PG3",
            CalFilename="PG3_golden.cal",
            MakeGroupingWorkspace=False,
            MakeOffsetsWorkspace=True,
            MakeMaskWorkspace=False,
            WorkspaceName="PG3")
        ws = LoadEmptyInstrument(Filename="POWGEN_Definition_2014-03-10.xml")
        startPos = ws.getInstrument().getComponentByName("bank26").getPos()
        startRot = ws.getInstrument().getComponentByName("bank26").getRotation()
        AlignComponents(CalibrationTable="PG3_cal",
                        InputWorkspace=ws,
                        ComponentList="bank26",
                        PosY=False, PosZ=False,
                        RotX=False, RotY=False, RotZ=False)
        endPos = ws.getInstrument().getComponentByName("bank26").getPos()
        endRot = ws.getInstrument().getComponentByName("bank26").getRotation()
        self.assertAlmostEqual(endPos.getX(), 1.51596056)
        self.assertEqual(startPos.getY(), endPos.getY())
        self.assertEqual(startPos.getZ(), endPos.getZ())
        self.assertEqual(startRot, endRot)

        startPos = ws.getInstrument().getComponentByName("bank64").getPos()
        startRot = ws.getInstrument().getComponentByName("bank64").getRotation().getEulerAngles()
        AlignComponents(CalibrationTable="PG3_cal",
                        InputWorkspace=ws,
                        ComponentList="bank64",
                        PosX=False, PosY=False, PosZ=False,
                        RotY=False, RotZ=False)
        endPos = ws.getInstrument().getComponentByName("bank64").getPos()
        endRot = ws.getInstrument().getComponentByName("bank64").getRotation().getEulerAngles()
        self.assertEqual(startPos, endPos)
        self.assertAlmostEqual(startRot[0], endRot[0])
        self.assertAlmostEqual(startRot[1], endRot[1])
        self.assertAlmostEqual(startRot[2], endRot[2])

if __name__ == "__main__":
    unittest.main()
